# 発見事項: D3D12 Backend実装

## ビルドシステム
- LOG_INFO/LOG_ERROR/LOG_WARN: 単一`std::string`引数のみ（printf-style不可）
- LOG_HRESULT(hr, msg): HRESULT用マクロあり
- C4251: std::string in RHI_API struct → disablewarnings必要（StaticLibでは無害）
- LNK4006: d3d12.lib/dxgi.lib間の重複定義 → `/ignore:4006`

## RHIフロントエンド設計
- ERHIFeature: 元はIDynamicRHI.hに定義、RHIEnums.hに移動済み
- IRHIAdapter: RHIEnums.h経由でERHIFeature取得可能
- IDynamicRHI.h: 多くの型を要求するため、D3D12RHI側で直接includeすると依存爆発
  - 回避策: 必要なenumはRHIEnums.h、型はRHIFwd.h経由で取得
- RHIAdapterDesc.deviceName: std::string型（RHI_API付きstruct内）

## RHIヘッダー依存関係パターン
- IRHIDevice.hは軽量ヘッダー(RHIEnums/RHIFwd/RHITypes等)のみinclude
- IRHIDevice.hが使う型で重いヘッダーにある場合 → **enumはRHIEnums.hに移動**（ERHIFeature, ERHIQueryFlags, ERHIHeapType）
- inline methodで完全型が必要 → 最小限のinclude追加（例: RHIValidation.h for ERHIValidationLevel）
- 戻り値がforwardDecl可能 → RHIFwd.hに追加
- RHIQuery.hはDECLARE_RHI_RESOURCE_TYPEマクロ使用 → IRHIDevice.hからincludeすると cascade error
- RHIBarrier.hはIRHIBuffer.h/IRHITexture.hのfull includeが必要（static_cast + RHISubresourceRange by-value）

## COM/D3D12
- ComPtr: `Microsoft::WRL::ComPtr<>` via `<wrl/client.h>`
- IDXGIFactory6: Windows 10 1803+ 必須
- EnumAdapterByGpuPreference: GPU性能順で列挙
- D3D12CreateDevice(nullptr引数): Feature Level対応チェック用（デバイス作成なし）
- D3D12CreateDevice: Feature Level 12_0→12_2降順で試行
- ID3D12Device5: QI取得（DXR等の拡張API用）
- CheckFeatureSupport: Options/1/5/7/12/16/21で機能検出
- DRED: ID3D12DeviceRemovedExtendedDataSettings1で有効化
- CommandSignature: DrawInstanced/DrawIndexedInstanced/DispatchはD3D12_INDIRECT_ARGUMENT_DESC 1つ

## TRefCountPtr完全型要件
- TRefCountPtr<T>のデストラクタが`m_ptr->Release()`を呼ぶため、Tの完全型定義が必要
- IDynamicRHIの戻り値型（IRHIBuffer, IRHIFence等）全てのヘッダーをD3D12DynamicRHI.hでinclude
- 前方宣言だけではコンパイルエラー（C2027: undefined type）

## NS_DISALLOW_COPYマクロの罠
- `NS_DISALLOW_COPY(ClassName)` は `private:` アクセス指定子を含む
- マクロ後に宣言される全メンバーがprivateになる
- 対策: マクロ直後に `public:` を追記

## RHI enum命名規則
- ERHIPixelFormat: 全大文字＋アンダースコア（D32_FLOAT, R8G8B8A8_UNORM）
- 一部ヘッダーでは小文字混在バグあり（D32_Float等）— 発見次第修正

## NS_DISALLOW_COPYバグ: 追加発見
- RHIDeferredDelete.h: NS_DISALLOW_COPY(RHIDeferredDeleteQueue)後のメソッドが全てprivate化
- 既知パターン: IRHIResource.h (sub-plan 06) と同一
- 修正方法: マクロ直後に `public:` 追記

## D3D12GpuResource設計
- IRHIResource派生ではない（内部ユーティリティクラス）
- D3D12Buffer/D3D12Textureがメンバーとして保持
- InitFromExisting: SwapChainバックバッファ等の外部リソース用
- InitCommitted: 新規リソース作成用
- Upload/Readbackヒープは状態遷移不要（requiresStateTracking_=false）
- Map/Unmapは参照カウント方式（ネストMap対応）
- ERHIResourceState→D3D12_RESOURCE_STATES変換: VertexBuffer/ConstantBufferは同一D3D12フラグ

## RHIDeferredDeleteQueue
- Device所有（deferredDeleteQueue_メンバー）
- BeginFrame: SetCurrentFrame + ProcessCompletedDeletions
- Shutdown: FlushAll（GPU待機後）
- フェンスベース or フレームベースの2方式

## IRHIResourceサブクラスのprotectedコンストラクタパターン
- IRHIResource(ERHIResourceType type)は唯一のコンストラクタ（default削除済み）
- 各IRHIXxx中間クラスにprotectedデフォルトコンストラクタが必要:
  - IRHIFence() : IRHIResource(ERHIResourceType::Fence) {} — sub-plan 08で追加
  - IRHICommandAllocator() : IRHIResource(ERHIResourceType::CommandAllocator) {} — sub-plan 10で追加
  - IRHICommandList() : IRHIResource(ERHIResourceType::CommandList) {} — sub-plan 11で追加
  - IRHIBuffer() : IRHIResource(ERHIResourceType::Buffer) {} — sub-plan 16で追加
  - IRHITexture() : IRHIResource(ERHIResourceType::Texture) {} — sub-plan 17で追加
  - IRHIDescriptorHeap() : IRHIResource(ERHIResourceType::DescriptorHeap) {} — sub-plan 19で追加
  - IRHIShaderResourceView/IRHIUnorderedAccessView/IRHIRenderTargetView/IRHIDepthStencilView/IRHIConstantBufferView — sub-plan 20で追加
  - IRHISampler() : IRHIResource(ERHIResourceType::Sampler) {} — sub-plan 21で追加
- 今後のIRHIXxxサブクラスでも同じパターンが必要

## DECLARE_RHI_RESOURCE_TYPEマクロ依存
- DECLARE_RHI_RESOURCE_TYPE(X)はRHIResourceType.hで定義
- IRHIResource.hはRHIResourceType.hをincludeしない場合がある
- 各IRHIXxx.hが自分でRHIResourceType.hをincludeする必要がある
- 修正済み: RHIDescriptorHeap.h(sub-plan 19), IRHIRootSignature.h(sub-plan 21)

## RHIShaderBytecodeメンバー名
- `data` (const void*) — NOT `pData`
- `size` (MemorySize)
- 定義場所: IRHIShader.h

## RHIRenderTargetFormatsメンバー名
- `count` (uint32) — NOT `numRenderTargets`
- `formats[kMaxRenderTargets]` (ERHIPixelFormat[]) — NOT `renderTargetFormats`
- `depthStencilFormat` (ERHIPixelFormat)
- `sampleCount` (ERHISampleCount) — static_cast<UINT>で変換
- `sampleQuality` (uint32)
- 定義場所: IRHIPipelineState.h

## DECLARE_RHI_RESOURCE_TYPEマクロ依存（追加）
- IRHIShader.h: RHIResourceType.h include追加（sub-plan 23）
- RHIPipelineState.h: RHIResourceType.h include追加（sub-plan 23）

## D3D12バックエンドnamespace
- 全D3D12クラス: `NS::D3D12RHI` namespace（NOT `NS::RHI`）
- RHI型参照時: `NS::RHI::` 完全修飾が必要
- `MemorySize` は `NS::RHI::MemorySize`
- ログマクロ: D3D12RHIPrivate.h経由（Common/Logging/Logging.h）

## インクルードパスパターン
- D3D12RHI: `source`, `source/engine`, `source/engine/hal/Public`, `source/engine/D3D12RHI/Private`
- リンク: d3d12.lib, dxgi.lib, dxguid.lib

## DECLARE_RHI_RESOURCE_TYPEマクロ依存（さらに追加）
- IRHISwapChain.h: RHIResourceType.h include追加（sub-plan 26）

## SwapChain設計パターン
- D3D12Device::dxgiFactory_: 非所有ポインタ（D3D12DynamicRHI::Init()でSetDXGIFactory呼び出し）
- CreateSwapChainはIRHIDevice仮想関数 → シグネチャ変更不可 → factory_をDevice内に保存
- バックバッファ: D3D12Texture::InitFromExisting + D3D12GpuResource::InitFromExisting
- DXGI_PRESENT_PARAMETERS: pDirtyRects/pScrollRect/pScrollOffsetは非const → const_cast必要
- IDXGISwapChain4: IDXGISwapChain1→QueryInterfaceで取得

## D3D12GpuResource::ConvertToD3D12Stateアクセスレベル
- 元はprivate（D3D12Resource.h）→ D3D12Dispatch.cppで使用するためpublicに変更（sub-plan 28）
- static関数なのでpublicでも安全

## D3D12RHI_CHECK_HRマクロ修正
- 元のLOG_ERROR呼び出しがprintf-style format args使用 → C4002（引数が多すぎる）
- LOG_ERRORは単一string引数のみ → format args除去で修正
