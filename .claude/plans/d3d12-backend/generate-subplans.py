#!/usr/bin/env python3
"""D3D12 backend subplan generator - splits oversized plans into <=5 TODO each."""
import os, sys

sys.stdout = open(sys.stdout.fileno(), mode='w', encoding='utf-8', buffering=1)

BASE = os.path.dirname(os.path.abspath(__file__))

# Delete old files (00-16)
old_files = [
    "00-module-setup.md", "01-adapter-device.md", "02-queue-fence-sync.md",
    "03-command-context.md", "04-resources.md", "05-descriptors-views.md",
    "06-pipeline-state.md", "07-swapchain-present.md", "08-barriers.md",
    "09-memory-allocators.md", "10-bindless.md", "11-query-profiling.md",
    "12-residency-devicelost.md", "13-raytracing.md", "14-work-graphs.md",
    "15-mesh-shader-vrs.md", "16-dispatch-wiring.md",
]
for f in old_files:
    p = os.path.join(BASE, f)
    if os.path.exists(p):
        os.remove(p)
        print(f"Deleted: {f}")

# Subplan definitions: (filename, content)
plans = []

# ============================================================
# Phase 1: Core (01-28)
# ============================================================

plans.append(("01-premake-setup.md", """\
# 01: premake設定 + ディレクトリ構造

## 目的
premake5.luaにD3D12RHIプロジェクトを追加し、ディレクトリ構造を作成する。

## 参照
- premake5.lua（既存構成）
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md

## TODO
- [ ] premake5.lua: project "D3D12RHI" 定義（StaticLib）+ 依存設定
- [ ] インクルードパス: Public/, Internal/, Private/
- [ ] Windows SDK パス設定（d3d12.lib, dxgi.lib, dxguid.lib）
- [ ] Shipping構成: NS_RHI_STATIC_BACKEND=D3D12 define追加
- [ ] ディレクトリ構造作成: Public/ + Internal/ + Private/

## 完了条件
- premake再生成成功、空プロジェクト認識

## 見積もり
- 新規ファイル: 0（ディレクトリのみ）
- 変更ファイル: premake5.lua
"""))

plans.append(("02-module-skeleton.md", """\
# 02: モジュール骨格ファイル

## 目的
D3D12RHIモジュールの最小ヘッダー群を作成する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md §1

## TODO
- [ ] D3D12RHI/Internal/D3D12ThirdParty.h — d3d12.h/dxgi1_6.h集約、COM smartポインタ
- [ ] D3D12RHI/Public/D3D12RHI.h — 公開定数（MAX_SRVS等）、エントリポイント宣言
- [ ] D3D12RHI/Private/D3D12RHIPrivate.h — 内部共通ヘッダー、ユーティリティマクロ

## 完了条件
- #include <d3d12.h> と #include <dxgi1_6.h> がコンパイル通過

## 見積もり
- 新規ファイル: 3
"""))

plans.append(("03-module-bootstrap.md", """\
# 03: モジュールブートストラップ

## 目的
D3D12RHIModuleのIDynamicRHIModule実装とエンジン起動時の選択パス。

## 参照
- source/engine/RHI/Public/IDynamicRHI.h

## TODO
- [ ] D3D12RHI/Private/D3D12RHIModule.cpp — IDynamicRHIModule実装
- [ ] D3D12RHIModule::CreateRHI() → D3D12DynamicRHIインスタンス返却（スタブ）
- [ ] エンジン起動時のD3D12バックエンド選択パス
- [ ] 4構成ビルド通過確認（D3D12RHI.lib生成）

## 完了条件
- 4構成ビルド通過、D3D12RHI.lib生成、リンクエラーなし

## 見積もり
- 新規ファイル: 1
"""))

plans.append(("04-adapter-enumeration.md", """\
# 04: DXGI Factory + Adapter列挙

## 目的
IDXGIFactory6でGPUアダプタを列挙しIRHIAdapterを実装する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part02_AdapterDeviceQueue.md
- source/engine/RHI/Public/IRHIAdapter.h

## TODO
- [ ] D3D12Adapter.h/.cpp 作成
- [ ] IDXGIFactory6::EnumAdapterByGpuPreference
- [ ] DXGI_ADAPTER_DESC3 → RHIAdapterDesc変換
- [ ] D3D_FEATURE_LEVEL_12_0以上の判定
- [ ] IRHIAdapter実装: D3D12Adapter : public IRHIAdapter

## 完了条件
- Adapter情報がログに出力される

## 見積もり
- 新規ファイル: 2 (D3D12Adapter.h/.cpp)
"""))

plans.append(("05-device-creation.md", """\
# 05: Device作成 + デバッグ支援

## 目的
ID3D12Deviceを作成し、デバッグレイヤーとFeature検出を設定する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part02_AdapterDeviceQueue.md
- source/engine/RHI/Public/IRHIDevice.h

## TODO
- [ ] D3D12Device.h/.cpp — D3D12CreateDevice + IRHIDevice実装
- [ ] CheckFeatureSupport (Options, Options5, Options7等)
- [ ] Debug Layer: ID3D12Debug::EnableDebugLayer + GPU Based Validation
- [ ] DRED設定 + ID3D12InfoQueue メッセージフィルタ
- [ ] HRESULTエラーハンドリングヘルパーマクロ

## 完了条件
- D3D12デバイス作成成功（Debug Layer有効）

## 見積もり
- 新規ファイル: 2 (D3D12Device.h/.cpp)
"""))

plans.append(("06-dynamic-rhi.md", """\
# 06: IDynamicRHI接続

## 目的
D3D12DynamicRHIを完成させ、Adapter列挙→Device作成の初期化フローを接続する。

## 参照
- source/engine/RHI/Public/IDynamicRHI.h
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md §2

## TODO
- [ ] D3D12DynamicRHI.h/.cpp — IDynamicRHI完全実装
- [ ] Init() → Factory作成 → Adapter列挙 → Device作成
- [ ] Shutdown() → デバイス・Factory破棄
- [ ] GetAdapterDesc() 実装

## 完了条件
- Init/Shutdownサイクル正常動作

## 見積もり
- 新規ファイル: 2 (D3D12DynamicRHI.h/.cpp)
"""))

plans.append(("07-command-queue.md", """\
# 07: CommandQueue

## 目的
3種のD3D12コマンドキューを作成しIRHIQueueを実装する。

## 参照
- docs/D3D12RHI/D3D12RHI_Part02_AdapterDeviceQueue.md §Queue
- source/engine/RHI/Public/IRHIQueue.h

## TODO
- [ ] D3D12Queue.h/.cpp 作成
- [ ] IRHIQueue実装: D3D12Queue : public IRHIQueue
- [ ] 3キュータイプ: Graphics(DIRECT), Compute, Copy
- [ ] ExecuteCommandLists呼び出し
- [ ] Signal/Wait: キュー間フェンス同期

## 完了条件
- 3キュー作成成功、ExecuteCommandLists動作

## 見積もり
- 新規ファイル: 2 (D3D12Queue.h/.cpp)
"""))

plans.append(("08-fence-syncpoint.md", """\
# 08: Fence + SyncPoint

## 目的
ID3D12Fenceラッパーと RHISyncPoint のD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §Fence
- source/engine/RHI/Public/IRHIFence.h
- source/engine/RHI/Public/RHISyncPoint.h

## TODO
- [ ] D3D12Fence.h/.cpp — IRHIFence実装 + 単調増加フェンス値管理
- [ ] CPU待機: SetEventOnCompletion + WaitForSingleObject
- [ ] ポーリング: GetCompletedValue + フェンスプール
- [ ] RHISyncPoint D3D12実装: Queue+FenceValueペア
- [ ] クロスキュー待機（Graphics → Copy完了待ち等）

## 完了条件
- GPU-CPU同期動作、キュー間同期動作

## 見積もり
- 新規ファイル: 2 (D3D12Fence.h/.cpp)
"""))

plans.append(("09-submission.md", """\
# 09: Submission

## 目的
コマンドリストのキューへの投入とフレーム完了同期。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §Submission

## TODO
- [ ] D3D12Submission.h/.cpp 作成
- [ ] コマンドリストのキューへの投入
- [ ] バッチ投入（複数CommandList一括実行）
- [ ] フレーム完了同期: Present後のフェンス待機

## 完了条件
- コマンドリスト投入→GPU完了待機の一連動作

## 見積もり
- 新規ファイル: 2 (D3D12Submission.h/.cpp)
"""))

plans.append(("10-command-allocator.md", """\
# 10: CommandAllocator

## 目的
D3D12のCommandAllocatorプール管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md
- source/engine/RHI/Public/IRHICommandAllocator.h

## TODO
- [ ] D3D12CommandAllocator.h/.cpp 作成
- [ ] IRHICommandAllocator実装
- [ ] タイプ別プール: DIRECT/COMPUTE/COPY
- [ ] フレーム遅延リセット（GPU完了後にReset）

## 完了条件
- Allocator取得→Reset→再利用サイクル動作

## 見積もり
- 新規ファイル: 2 (D3D12CommandAllocator.h/.cpp)
"""))

plans.append(("11-command-list.md", """\
# 11: CommandList

## 目的
ID3D12GraphicsCommandListラッパーとIRHICommandList実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md
- source/engine/RHI/Public/IRHICommandList.h

## TODO
- [ ] D3D12CommandList.h/.cpp 作成
- [ ] IRHICommandList実装
- [ ] ID3D12GraphicsCommandList7ラッパー
- [ ] Close/Resetライフサイクル管理
- [ ] デバッグマーカー: PIXBeginEvent/PIXEndEvent

## 完了条件
- CommandList生成→記録→Close動作

## 見積もり
- 新規ファイル: 2 (D3D12CommandList.h/.cpp)
"""))

plans.append(("12-command-context.md", """\
# 12: CommandContext

## 目的
IRHICommandContextのD3D12実装とRHICommandBufferからの変換。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md
- source/engine/RHI/Public/IRHICommandContext.h
- source/engine/RHI/Internal/RHICommandBuffer.h

## TODO
- [ ] D3D12CommandContext.h/.cpp 作成
- [ ] IRHICommandContext / IRHIComputeContext 実装
- [ ] RHICommandBuffer → ID3D12GraphicsCommandList変換（switchディスパッチ）
- [ ] ステートトラッキング: 現在のPSO、RootSignature、頂点バッファ等

## 完了条件
- RHICommandBufferのswitchディスパッチがD3D12 APIを呼び出す

## 見積もり
- 新規ファイル: 2 (D3D12CommandContext.h/.cpp)
"""))

plans.append(("13-dispatch-table-core.md", """\
# 13: DispatchTable（コアエントリ）

## 目的
Phase 1で必要な主要DispatchTableエントリをD3D12に接続する。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h
- source/engine/RHI/Private/RHIDispatchTable.cpp

## TODO
- [ ] 主要30-40エントリのD3D12_Xxx関数実装
- [ ] GRHIDispatchTable初期化時登録
- [ ] Draw/DrawIndexed/Dispatch系
- [ ] SetPipelineState/SetRootSignature/SetVertexBuffer系
- [ ] SetRenderTargets/ClearRTV/ClearDSV系

## 完了条件
- DispatchTableの主要エントリがD3D12に接続、IsValid()通過

## 見積もり
- 新規ファイル: 2 (D3D12Dispatch.h/.cpp)
"""))

plans.append(("14-cmdlist-lifecycle.md", """\
# 14: CommandList管理

## 目的
CommandAllocatorとCommandListのライフサイクル管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §リサイクル

## TODO
- [ ] Allocator + CommandListのペアリング管理
- [ ] フレーム単位のリサイクルシステム
- [ ] 非同期コンピュートコマンドリスト対応

## 完了条件
- CommandList生成→記録→Close→Execute→フェンス待ち→リサイクル一連動作

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12CommandAllocator/List/Context
"""))

plans.append(("15-resource-base.md", """\
# 15: Resource基底

## 目的
D3D12リソースの基底クラスとIRHIResource実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md
- source/engine/RHI/Public/IRHIResource.h

## TODO
- [ ] D3D12Resource.h/.cpp — ID3D12Resourceラッパー基底
- [ ] IRHIResource実装
- [ ] リソース状態追跡（初期状態管理）
- [ ] GPUVirtualAddressキャッシュ
- [ ] Map/Unmapサポート

## 完了条件
- Resource基底が他リソースクラスの基底として使用可能

## 見積もり
- 新規ファイル: 2 (D3D12Resource.h/.cpp)
"""))

plans.append(("16-buffer.md", """\
# 16: Buffer

## 目的
IRHIBufferのD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Buffer
- source/engine/RHI/Public/IRHIBuffer.h

## TODO
- [ ] D3D12Buffer.h/.cpp + IRHIBuffer実装
- [ ] CreateCommittedResourceでバッファ作成
- [ ] ヒープタイプ判定: DEFAULT / UPLOAD / READBACK
- [ ] 頂点/インデックス/定数/構造化/間接引数バッファ対応

## 完了条件
- 各種バッファ作成成功、GPUVirtualAddress取得可能

## 見積もり
- 新規ファイル: 2 (D3D12Buffer.h/.cpp)
"""))

plans.append(("17-texture.md", """\
# 17: Texture

## 目的
IRHITextureのD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Texture
- source/engine/RHI/Public/IRHITexture.h

## TODO
- [ ] D3D12Texture.h/.cpp + IRHITexture実装
- [ ] 2D/3D/Cubeテクスチャ作成
- [ ] ミップレベル、配列テクスチャ対応
- [ ] テクスチャアップロード: UpdateSubresources (upload heap経由)

## 完了条件
- テクスチャ作成+ミップデータアップロード成功

## 見積もり
- 新規ファイル: 2 (D3D12Texture.h/.cpp)
"""))

plans.append(("18-upload-format.md", """\
# 18: Upload Heap + フォーマット変換

## 目的
初期データ転送用Upload Heapとピクセルフォーマット変換テーブル。

## 参照
- docs/D3D12RHI/D3D12RHI_Part04_Resources.md §Upload
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md §Upload

## TODO
- [ ] 一時アップロードバッファ作成
- [ ] CopyBufferRegion / CopyTextureRegion
- [ ] フレーム完了後の一時バッファ解放
- [ ] ERHIPixelFormat ↔ DXGI_FORMAT変換テーブル
- [ ] Typeless/SRV/UAV/RTV/DSVフォーマット派生

## 完了条件
- バッファ/テクスチャへの初期データ転送成功

## 見積もり
- 新規ファイル: 2 (D3D12Upload.h/.cpp)
"""))

plans.append(("19-descriptor-heap.md", """\
# 19: ディスクリプタヒープ

## 目的
D3D12の4タイプディスクリプタヒープとアロケータ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md
- source/engine/RHI/Public/RHIDescriptorHeap.h

## TODO
- [ ] D3D12Descriptors.h/.cpp 作成
- [ ] 4タイプヒープ: CBV_SRV_UAV, SAMPLER, RTV, DSV
- [ ] フリーリスト方式CPUディスクリプタアロケータ
- [ ] シェーダー可視ヒープ線形アロケータ（フレーム単位リセット）
- [ ] オフライン→オンラインCopyDescriptors

## 完了条件
- ディスクリプタ確保/解放/コピー動作

## 見積もり
- 新規ファイル: 2 (D3D12Descriptors.h/.cpp)
"""))

plans.append(("20-views.md", """\
# 20: ビュー作成

## 目的
SRV/UAV/CBV/RTV/DSVビューの作成。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Views
- source/engine/RHI/Public/IRHIViews.h

## TODO
- [ ] D3D12View.h/.cpp 作成
- [ ] SRV: CreateShaderResourceView（Buffer/Texture）
- [ ] UAV: CreateUnorderedAccessView + CBV: CreateConstantBufferView
- [ ] RTV: CreateRenderTargetView + DSV: CreateDepthStencilView
- [ ] IRHIShaderResourceView等のD3D12実装

## 完了条件
- 全ビュータイプ作成成功

## 見積もり
- 新規ファイル: 2 (D3D12View.h/.cpp)
"""))

plans.append(("21-sampler.md", """\
# 21: Sampler管理

## 目的
サンプラー作成とキャッシュ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Sampler

## TODO
- [ ] CreateSampler + サンプラーヒープ管理
- [ ] 静的サンプラー（Root Signature組み込み）
- [ ] 動的サンプラーヒープ
- [ ] サンプラーキャッシュ（重複排除）

## 完了条件
- シェーダーからテクスチャサンプリング可能

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Descriptors, D3D12View
"""))

plans.append(("22-root-signature.md", """\
# 22: RootSignature

## 目的
D3D12のRoot Signature作成とIRHIRootSignature実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §RootSignature
- source/engine/RHI/Public/IRHIRootSignature.h

## TODO
- [ ] D3D12RootSignature.h/.cpp + IRHIRootSignature実装
- [ ] シリアライズ → CreateRootSignature
- [ ] テーブルレイアウト: CBV/SRV/UAV/Samplerスロット配置
- [ ] Root Descriptor（インラインCBV）対応
- [ ] 静的サンプラー定義

## 完了条件
- Root Signature作成成功、テーブルレイアウト設定可能

## 見積もり
- 新規ファイル: 2 (D3D12RootSignature.h/.cpp)
"""))

plans.append(("23-pso-create.md", """\
# 23: PSO作成

## 目的
Graphics/Compute Pipeline State Objectの作成。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §PSO
- source/engine/RHI/Public/IRHIPipelineState.h

## TODO
- [ ] D3D12PipelineState.h/.cpp 作成
- [ ] Graphics PSO: D3D12_GRAPHICS_PIPELINE_STATE_DESC構築 + Create
- [ ] InputLayout → D3D12_INPUT_ELEMENT_DESC変換
- [ ] Compute PSO: D3D12_COMPUTE_PIPELINE_STATE_DESC構築 + Create

## 完了条件
- Graphics/Compute PSO作成成功

## 見積もり
- 新規ファイル: 2 (D3D12PipelineState.h/.cpp)
"""))

plans.append(("24-shader-compile.md", """\
# 24: シェーダーコンパイル

## 目的
HLSL→DXILコンパイルとIRHIShader実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §Shader
- source/engine/RHI/Public/IRHIShader.h

## TODO
- [ ] D3D12Shader.h/.cpp + IRHIShader実装（VS/PS/CS/GS/HS/DS）
- [ ] HLSL→DXILコンパイル（IDxcCompiler3）
- [ ] シェーダーリフレクション: 入力レイアウト自動抽出
- [ ] プリコンパイル済みシェーダー(.cso)読み込み

## 完了条件
- HLSLシェーダーコンパイル→バイトコード取得成功

## 見積もり
- 新規ファイル: 2 (D3D12Shader.h/.cpp)
"""))

plans.append(("25-pso-cache.md", """\
# 25: PSOキャッシュ + ステート変換

## 目的
PSOキャッシュとRHI→D3D12ステート変換ヘルパー。

## 参照
- docs/D3D12RHI/D3D12RHI_Part08_PipelineState.md §Cache

## TODO
- [ ] ハッシュベースPSOキャッシュ（重複作成防止）
- [ ] PSOディスクキャッシュ（シリアライズ/デシリアライズ）
- [ ] ERHICullMode → D3D12_CULL_MODE等ステート変換ヘルパー
- [ ] ERHIBlendFactor/CompareFunc/Topology等の変換

## 完了条件
- PSO重複作成なし、ステート変換の網羅性確認

## 見積もり
- 新規ファイル: 2 (D3D12StateConvert.h/.cpp)
"""))

plans.append(("26-swapchain.md", """\
# 26: SwapChain + バックバッファ

## 目的
DXGI SwapChain作成とバックバッファ管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part12_ViewportShaders.md
- source/engine/RHI/Public/IRHISwapChain.h

## TODO
- [ ] D3D12SwapChain.h/.cpp + IRHISwapChain実装
- [ ] IDXGISwapChain4作成（CreateSwapChainForHwnd）
- [ ] バックバッファ取得 → D3D12Texture化 + RTV作成
- [ ] フレームインデックス管理 + Resize対応

## 完了条件
- SwapChain作成成功、バックバッファRTV取得可能

## 見積もり
- 新規ファイル: 2 (D3D12SwapChain.h/.cpp)
"""))

plans.append(("27-present-frame.md", """\
# 27: Present + フレームループ

## 目的
Presentとフレームループの統合。

## 参照
- docs/D3D12RHI/D3D12RHI_Part12_ViewportShaders.md §Present

## TODO
- [ ] Present(SyncInterval, Flags) + VSync/Tearing対応
- [ ] フレーム完了フェンス: Present後にSignal
- [ ] BeginFrame → 描画 → EndFrame → Present ループ
- [ ] バックバッファ状態遷移: PRESENT → RENDER_TARGET → PRESENT
- [ ] フレームリソースリング（N-buffering）

## 完了条件
- ウィンドウにクリアカラー表示

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12SwapChain, D3D12CommandContext
"""))

plans.append(("28-triangle-test.md", """\
# 28: 三角形描画テスト

## 目的
Phase 1の集大成: 画面に三角形を描画する。

## 参照
- Phase 1 全サブ計画の統合

## TODO
- [ ] 最小頂点シェーダー（position passthrough）+ ピクセルシェーダー（solid color）
- [ ] 頂点バッファ: 3頂点作成 + データアップロード
- [ ] PSO作成: VS + PS + InputLayout
- [ ] DrawInstanced(3, 1, 0, 0) 実行
- [ ] 画面にカラー三角形表示確認

## 完了条件
- カラー三角形が画面に描画される、4構成ビルド通過

## 見積もり
- 新規ファイル: 2 (テストシェーダー .hlsl × 2)
- 変更ファイル: フレームループ統合
"""))

# ============================================================
# Phase 2: Features (29-39)
# ============================================================

plans.append(("29-legacy-barrier.md", """\
# 29: Legacy Barrier

## 目的
D3D12_RESOURCE_BARRIERベースのLegacyバリアシステム。

## 参照
- docs/D3D12RHI/D3D12RHI_Part06_Barriers.md §Legacy
- source/engine/RHI/Public/RHIBarrier.h

## TODO
- [ ] D3D12Barriers.h/.cpp 作成
- [ ] D3D12_RESOURCE_BARRIER (Transition/UAV/Aliasing)
- [ ] ResourceBarrierバッチ投入
- [ ] Split Barriers (BEGIN_ONLY / END_ONLY)
- [ ] 状態プロモーション/ディケイ規則

## 完了条件
- Legacy Barrier動作（全プラットフォーム）

## 見積もり
- 新規ファイル: 2 (D3D12Barriers.h/.cpp)
"""))

plans.append(("30-enhanced-barrier.md", """\
# 30: Enhanced Barrier

## 目的
D3D12_BARRIER_xxx ベースのEnhanced Barrierシステム（FL 12.2+）。

## 参照
- docs/D3D12RHI/D3D12RHI_Part06_Barriers.md §Enhanced

## TODO
- [ ] D3D12_BARRIER_GROUP構築
- [ ] D3D12_TEXTURE_BARRIER / BUFFER_BARRIER / GLOBAL_BARRIER
- [ ] D3D12_BARRIER_SYNC / D3D12_BARRIER_ACCESS
- [ ] Feature判定: CheckFeatureSupport → Enhanced有効時切替

## 完了条件
- Enhanced Barrier動作（対応GPU）

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Barriers
"""))

plans.append(("31-auto-barrier.md", """\
# 31: 自動バリア挿入 + StateTracker

## 目的
RHIResourceStateTrackerとD3D12バリアの接続、自動バリア挿入。

## 参照
- source/engine/RHI/Public/RHIStateTracking.h
- source/engine/RHI/Private/RHIStateTracking.cpp

## TODO
- [ ] RHIResourceStateTracker → D3D12バリア変換
- [ ] RequireState() → 実D3D12バリア発行
- [ ] 描画/ディスパッチ/コピー前の暗黙的バリア挿入
- [ ] レンダーパス開始/終了時の遷移 + グローバル状態コミット
- [ ] バリアバッチ最適化（冗長バリア排除）

## 完了条件
- バリアバッチ最適化動作

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Barriers, D3D12CommandContext
"""))

plans.append(("32-buddy-pool-allocator.md", """\
# 32: Buddy/Pool アロケータ

## 目的
Placed Resourceを活用したBuddy/Poolアロケータ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md

## TODO
- [ ] Buddyアロケータ: 2のべき乗分割メモリアロケータ
- [ ] ID3D12Heap上のPlaced Resource配置
- [ ] Poolアロケータ: 固定サイズブロックプール
- [ ] スレッドセーフ対応

## 完了条件
- Placed Resource経由のバッファ/テクスチャ作成

## 見積もり
- 新規ファイル: 2 (D3D12Allocation.h/.cpp)
"""))

plans.append(("33-transient-allocator.md", """\
# 33: Transientアロケータ + RHI接続

## 目的
フレーム単位の一時リソースアロケータとRHIフロントエンド接続。

## 参照
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md §Transient
- source/engine/RHI/Public/RHITransientAllocator.h

## TODO
- [ ] フレーム単位リニアアロケータ
- [ ] Upload Heapリングバッファ
- [ ] 1フレーム寿命の一時リソース
- [ ] RHIBufferAllocator → D3D12 Buddy/Pool接続
- [ ] RHITransientBufferPool → D3D12 Transient接続

## 完了条件
- 一時バッファのフレーム間再利用、メモリ使用量統計取得

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Allocation
"""))

plans.append(("34-bindless-manager.md", """\
# 34: Bindless Manager

## 目的
Bindless描画モデルのディスクリプタ管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Bindless
- source/engine/RHI/Public/RHIBindless.h

## TODO
- [ ] グローバルSRV/UAVヒープの永続割り当て
- [ ] リソース作成時にBindlessハンドル発行
- [ ] ハンドル解放 → フリーリスト
- [ ] Unbounded descriptor range対応

## 完了条件
- Bindlessインデックスでリソースアクセス可能

## 見積もり
- 新規ファイル: 2 (D3D12Bindless.h/.cpp)
"""))

plans.append(("35-resource-table.md", """\
# 35: ResourceTable + Bindless RootSig

## 目的
リソーステーブルのD3D12実装とBindless版Root Signature。

## 参照
- source/engine/RHI/Public/RHIResourceTable.h

## TODO
- [ ] リソーステーブルD3D12実装
- [ ] 動的追加/削除
- [ ] テーブルインデックスの定数バッファ経由渡し
- [ ] SM6.6 ResourceDescriptorHeap / SamplerDescriptorHeap

## 完了条件
- 動的リソース追加/削除、Bindless vs Table-based切替

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Bindless, D3D12RootSignature
"""))

plans.append(("36-query-timestamp.md", """\
# 36: QueryHeap + Timestamp

## 目的
GPUクエリヒープとタイムスタンプ計測。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §Profiling
- source/engine/RHI/Public/RHIQuery.h

## TODO
- [ ] 3タイプクエリヒープ作成 (TIMESTAMP/OCCLUSION/PIPELINE_STATISTICS)
- [ ] ResolveQueryData → Readbackバッファ
- [ ] BeginTimestamp / EndTimestamp
- [ ] GPU周波数取得: GetTimestampFrequency + ミリ秒変換

## 完了条件
- GPUタイムスタンプ取得・表示

## 見積もり
- 新規ファイル: 2 (D3D12Query.h/.cpp)
"""))

plans.append(("37-occlusion-profiler.md", """\
# 37: Occlusion + GPU Profiler

## 目的
オクルージョンクエリとGPUプロファイラー。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md
- source/engine/RHI/Public/RHIOcclusion.h
- source/engine/RHI/Public/RHIGPUProfiler.h

## TODO
- [ ] Predicated Rendering対応
- [ ] Binary / Precise occlusion
- [ ] フレーム全体のGPU時間計測
- [ ] ドローコール単位計測
- [ ] PIX連携（オプション）

## 完了条件
- オクルージョンクエリ動作、フレームGPU時間計測

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Query, D3D12CommandContext
"""))

plans.append(("38-residency.md", """\
# 38: Residency Manager

## 目的
GPUメモリレジデンシー管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §Residency
- source/engine/RHI/Public/RHIResidency.h

## TODO
- [ ] IDXGIAdapter3::QueryVideoMemoryInfo
- [ ] MakeResident / Evict
- [ ] LRUベースのページング判定
- [ ] メモリ予算モニタリング

## 完了条件
- メモリ予算超過時の自動Evict

## 見積もり
- 新規ファイル: 2 (D3D12Residency.h/.cpp)
"""))

plans.append(("39-devicelost-stats.md", """\
# 39: DeviceLost + MemoryStats

## 目的
デバイスロスト復旧とメモリ使用量統計。

## 参照
- docs/D3D12RHI/D3D12RHI_Part11_ResidencyProfiling.md §DeviceLost
- source/engine/RHI/Public/RHIDeviceLost.h
- source/engine/RHI/Public/RHIMemoryStats.h

## TODO
- [ ] DXGI_ERROR_DEVICE_REMOVED検出 + GetDeviceRemovedReason
- [ ] DRED: ID3D12DeviceRemovedExtendedDataSettings
- [ ] 全リソース解放 → デバイス再作成 → リソース再構築
- [ ] ヒープ使用量追跡 + アロケータ別統計
- [ ] HUD表示用メモリ統計データ提供

## 完了条件
- デバイスロスト検出→エラー報告、メモリ使用量統計表示

## 見積もり
- 新規ファイル: 2 (D3D12DeviceLost.h/.cpp)
"""))

# ============================================================
# Phase 3: Advanced (40-52)
# ============================================================

plans.append(("40-blas.md", """\
# 40: BLAS（Bottom-Level Acceleration Structure）

## 目的
DXR 1.1 BLAS構築。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §BLAS
- source/engine/RHI/Public/RHIRaytracingAS.h

## TODO
- [ ] D3D12RayTracingGeometry.h/.cpp — BLAS管理
- [ ] D3D12_RAYTRACING_GEOMETRY_DESC作成
- [ ] BuildRaytracingAccelerationStructure (BLAS)
- [ ] コンパクション: PostBuildInfo → CopyRaytracingAccelerationStructure (COMPACT)
- [ ] SRV作成（BLAS参照用）

## 完了条件
- BLAS構築成功、コンパクション動作

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingGeometry.h/.cpp)
"""))

plans.append(("41-tlas.md", """\
# 41: TLAS + Feature検出

## 目的
DXR 1.1 TLAS構築とRT Feature検出。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §TLAS
- source/engine/RHI/Public/RHIRaytracingAS.h

## TODO
- [ ] D3D12RayTracingScene.h/.cpp — TLAS管理
- [ ] D3D12_RAYTRACING_INSTANCE_DESCバッファ構築
- [ ] BuildRaytracingAccelerationStructure (TLAS) + 更新
- [ ] D3D12_FEATURE_D3D12_OPTIONS5 (RaytracingTier) 判定

## 完了条件
- TLAS構築成功、RHIフロントエンド接続

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingScene.h/.cpp)
"""))

plans.append(("42-rtpso.md", """\
# 42: RT PSO + シェーダー識別子

## 目的
レイトレーシングPipeline State Objectとシェーダー識別子管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §RTPSO

## TODO
- [ ] D3D12RayTracingPSO.h/.cpp — RTPSO管理
- [ ] ID3D12StateObject (RAYTRACING) + Collection-basedキャッシュ
- [ ] シェーダーライブラリ登録 + HitGroup定義
- [ ] GetShaderIdentifier → 32バイトID取得
- [ ] グローバル/ローカルRoot Signature

## 完了条件
- RT PSO作成、シェーダー識別子取得成功

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingPSO.h/.cpp)
"""))

plans.append(("43-dispatch-rays.md", """\
# 43: SBT + DispatchRays

## 目的
シェーダーバインディングテーブルとDispatchRays実行。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §SBT, §Dispatch
- source/engine/RHI/Public/RHIRaytracingShader.h

## TODO
- [ ] SBTレイアウト構築: ShaderID(32B) + ローカルRSパラメータ
- [ ] D3D12_DISPATCH_RAYS_DESC構築
- [ ] リソースバインディング（グローバル + ローカル）
- [ ] ID3D12GraphicsCommandList4::DispatchRays()
- [ ] DispatchTable登録 + HasRayTracingSupport() = true

## 完了条件
- DispatchRaysで簡単なレイトレーシング描画

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingSBT.h/.cpp)
"""))

plans.append(("44-wg-state-object.md", """\
# 44: Work Graph State Object + バッキングメモリ

## 目的
D3D12 Work GraphsのState Object構築とバッキングメモリ管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §WorkGraphs
- source/engine/RHI/Public/IRHIWorkGraph.h

## TODO
- [ ] D3D12WorkGraph.h/.cpp — D3D12WorkGraphPipelineState
- [ ] ID3D12StateObject (EXECUTABLE) + ノード登録
- [ ] プロパティ取得: ProgramIdentifier + メモリ要件
- [ ] バッキングメモリ割り当て（65536アライメント、UAV）
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS21

## 完了条件
- Work Graph State Object作成、バッキングメモリ確保

## 見積もり
- 新規ファイル: 2 (D3D12WorkGraph.h/.cpp)
"""))

plans.append(("45-wg-dispatch.md", """\
# 45: Work Graph Dispatch

## 目的
Work GraphのCompute/Graphics Dispatch実行。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §Dispatch

## TODO
- [ ] Compute: SetProgram + DispatchGraph
- [ ] エントリシェーダー特定 + ルート引数テーブル構築
- [ ] ルート引数アップロード（コピーキュー経由）
- [ ] Graphics: ルートシグネチャ + ビューポート/シザー設定
- [ ] デュアルRecordBindings (SF_Pixel, SF_Mesh)

## 完了条件
- Compute DispatchGraph実行

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12WorkGraph, D3D12CommandContext
"""))

plans.append(("46-wg-binder.md", """\
# 46: Work Graph Binder

## 目的
Work Graphのシェーダーバンドルバインダーと並列バインディング。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §Binder

## TODO
- [ ] FWorkGraphShaderBundleBinder: ノード毎リソースバインド状態
- [ ] 並列RecordBindings（最大4ワーカー）
- [ ] FShaderBundleBinderOps: 遷移重複排除 + ワーカーマージ
- [ ] FD3D12BindlessConstantSetter

## 完了条件
- 並列RecordBindings動作

## 見積もり
- 新規ファイル: 2 (D3D12WorkGraphBinder.h/.cpp)
"""))

plans.append(("47-mesh-pso.md", """\
# 47: Mesh Shader PSO

## 目的
Mesh/Amplification ShaderパイプラインのPSO作成。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §MeshShader
- source/engine/RHI/Public/RHIMeshPipelineState.h

## TODO
- [ ] D3D12_MESH_SHADER_PIPELINE_STATE_DESC構築
- [ ] Amplification Shader (AS) + Mesh Shader (MS) + Pixel Shader (PS)
- [ ] DXCコンパイル: -T as_6_5 / -T ms_6_5
- [ ] RHIMeshPipelineState/RHIMeshShader D3D12実装
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS7 (MeshShaderTier)

## 完了条件
- Mesh Shader PSO作成成功

## 見積もり
- 新規ファイル: 2 (D3D12MeshShader.h/.cpp)
"""))

plans.append(("48-dispatch-mesh.md", """\
# 48: DispatchMesh

## 目的
DispatchMesh実行とAS→MSペイロード渡し。

## 参照
- source/engine/RHI/Public/RHIMeshDispatch.h

## TODO
- [ ] ID3D12GraphicsCommandList6::DispatchMesh(X, Y, Z)
- [ ] ExecuteIndirect + D3D12_DISPATCH_MESH_ARGUMENTS
- [ ] ASからMSへのペイロード渡し
- [ ] GPU-drivenメッシュレットカリング対応
- [ ] DispatchTable登録 + HasMeshShaderSupport() = true

## 完了条件
- DispatchMesh動作、AS+MS二段パイプライン動作

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12MeshShader, D3D12CommandContext
"""))

plans.append(("49-vrs.md", """\
# 49: Variable Rate Shading

## 目的
VRS Tier 1/2の実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §VRS
- source/engine/RHI/Public/RHIVariableRateShading.h

## TODO
- [ ] Tier 1: RSSetShadingRate + D3D12_SHADING_RATE_COMBINER
- [ ] Tier 2: SV_ShadingRateセマンティクス + シェーディングレートイメージ
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS6 + タイルサイズ取得
- [ ] DispatchTable登録 + HasVariableRateShadingSupport() = true

## 完了条件
- VRS Tier 1/2動作

## 見積もり
- 新規ファイル: 2 (D3D12VRS.h/.cpp)
"""))

plans.append(("50-dt-signatures.md", """\
# 50: DispatchTable関数シグネチャ定義

## 目的
全107+エントリのD3D12_Xxx関数を定義する。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h (L849-866)

## TODO
- [ ] 全DispatchTableエントリ（107+）のD3D12_Xxx関数定義
- [ ] Base系: D3D12_GetDevice, D3D12_Begin, D3D12_Finish等
- [ ] Compute/Graphics/Upload系の全エントリ

## 完了条件
- 全エントリの関数シグネチャがRHIDispatchTable型と一致

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Dispatch.h/.cpp
"""))

plans.append(("51-dt-registration.md", """\
# 51: DispatchTable登録 + STATIC_BACKEND

## 目的
GRHIDispatchTableへの全関数ポインタ登録とNS_RHI_STATIC_BACKEND設定。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h
- source/engine/RHI/Private/RHIDispatchTable.cpp

## TODO
- [ ] GRHIDispatchTable全関数ポインタ登録 + IsValid()検証
- [ ] オプション機能の条件付き登録（RT/Mesh/WG/VRS）
- [ ] NS_RHI_STATIC_BACKEND=D3D12 定義（D3D12RHI.h）
- [ ] マクロ展開検証: NS_RHI_DISPATCH(Draw,...) → D3D12_Draw(...)
- [ ] premake5.lua Shipping構成 + LTO有効化確認

## 完了条件
- Dev: 全ポインタ登録、Shipping: 直接呼出しコンパイル成功

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12RHIModule, D3D12RHI.h, premake5.lua
"""))

plans.append(("52-dt-verification.md", """\
# 52: DispatchTable検証

## 目的
全構成でのDispatchTable動作検証。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h

## TODO
- [ ] Dev構成: GRHIDispatchTable経由の関数ポインタ呼出し検証
- [ ] Shipping構成: NS_RHI_EXPAND経由の直接呼出し検証
- [ ] CommandBuffer Executeパスの確認
- [ ] 4構成ビルド通過（Debug/Release/Burst/Shipping）
- [ ] パフォーマンス比較: Dev(間接) vs Shipping(直接)

## 完了条件
- 全構成動作、ゼロオーバーヘッド確認

## 見積もり
- 新規ファイル: 0
- 変更ファイル: テスト/検証コード
"""))

plans.append(("53-integration-test.md", """\
# 53: 統合テスト

## 目的
全フェーズの統合検証。

## 参照
- 全サブ計画

## TODO
- [ ] Phase 1テスト: 三角形描画E2E（デバイス初期化→Present）
- [ ] Phase 2テスト: バリア自動挿入 + Bindlessバインディング + GPU Timestamp
- [ ] Phase 3テスト: DXR DispatchRays + Mesh DispatchMesh
- [ ] Shipping構成: NS_RHI_STATIC_BACKEND=D3D12 ゼロオーバーヘッド
- [ ] 全4構成最終ビルド確認

## 完了条件
- 全フェーズ機能の統合動作、全構成ビルド通過

## 見積もり
- 新規ファイル: テストコード
"""))

# ============================================================
# Write all files
# ============================================================
for filename, content in plans:
    path = os.path.join(BASE, filename)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Created: {filename}")

print(f"\nTotal: {len(plans)} subplans created")
