# D3D12 Backend 実装計画

## 目的

NS-ENGINEのRHIフロントエンド（87ヘッダー + 50+ Private/*.cppスタブ）に対するD3D12バックエンド実装。
`docs/D3D12RHI/`（12パート、110ソースファイル分析）を参照設計とする。

## 前提

- RHIフロントエンド: Public/87ヘッダー、Internal/2ヘッダー、Private/50+スタブ
- NS_RHI_DISPATCH基盤: 107エントリ関数ポインタテーブル完成済み
- Shipping構成: NS_BUILD_SHIPPING + LTO対応マクロ準備済み
- ビルド構成: Debug/Release/Burst/Shipping 4構成
- RHICommandBuffer: コマンド記録/switch実行基盤完成済み

## 参照ドキュメント

| パート | 内容 | 優先度 |
|--------|------|--------|
| Part01 | アーキテクチャ概要・クラス階層 | 全フェーズ |
| Part02 | Adapter/Device/Queue | Phase 1 |
| Part03 | コマンド実行 | Phase 1 |
| Part04 | リソース（Buffer/Texture/View） | Phase 1 |
| Part05 | メモリアロケーション | Phase 1-2 |
| Part06 | バリア/状態追跡 | Phase 2 |
| Part07 | ディスクリプタ管理 | Phase 1-2 |
| Part08 | パイプラインステート | Phase 1 |
| Part09 | レイトレーシング | Phase 3 |
| Part10 | Work Graphs / Mesh Shader / VRS | Phase 3 |
| Part11 | レジデンシー/プロファイリング | Phase 2 |
| Part12 | Viewport/シェーダー | Phase 1 |

## フェーズ構成

### Phase 1: コア基盤（三角形描画まで）— 28サブ計画

| # | 計画 | 状態 | 依存 | 概要 |
|---|------|------|------|------|
| 01 | [premake設定 + ディレクトリ](01-premake-setup.md) | complete | — | premake5.lua、ディレクトリ構造 |
| 02 | [モジュール骨格](02-module-skeleton.md) | complete | 01 | ThirdParty.h、D3D12RHI.h、Private.h |
| 03 | [モジュールブートストラップ](03-module-bootstrap.md) | complete | 02 | IDynamicRHIModule、CreateRHI、ビルド通過 |
| 04 | [Adapter列挙](04-adapter-enumeration.md) | complete | 03 | IDXGIFactory6、IRHIAdapter |
| 05 | [Device作成](05-device-creation.md) | complete | 04 | D3D12CreateDevice、Feature検出、Debug Layer |
| 06 | [IDynamicRHI接続](06-dynamic-rhi.md) | complete | 05 | Init/Shutdown、Adapter→Device初期化フロー |
| 07 | [CommandQueue](07-command-queue.md) | complete | 06 | 3キュー、ExecuteCommandLists |
| 08 | [Fence + SyncPoint](08-fence-syncpoint.md) | complete | 07 | ID3D12Fence、CPU待機、クロスキュー同期 |
| 09 | [Submission](09-submission.md) | complete | 08 | バッチ投入、フレーム完了同期 |
| 10 | [CommandAllocator](10-command-allocator.md) | complete | 06 | タイプ別プール、フレーム遅延リセット |
| 11 | [CommandList](11-command-list.md) | complete | 10 | ID3D12GraphicsCommandList7、PIXマーカー |
| 12 | [CommandContext](12-command-context.md) | complete | 11 | IRHICommandContext、switchディスパッチ |
| 13 | [DispatchTable（コア）](13-dispatch-table-core.md) | complete | 12 | 主要30-40エントリ登録 |
| 14 | [CommandList管理](14-cmdlist-lifecycle.md) | complete | 12 | ペアリング、リサイクル |
| 15 | [Resource基底](15-resource-base.md) | complete | 06 | ID3D12Resourceラッパー、状態追跡 |
| 16 | [Buffer](16-buffer.md) | complete | 15 | IRHIBuffer、CommittedResource |
| 17 | [Texture](17-texture.md) | complete | 15 | IRHITexture、2D/3D/Cube |
| 18 | [Upload + フォーマット変換](18-upload-format.md) | complete | 16,17 | Upload Heap、DXGI_FORMAT変換 |
| 19 | [ディスクリプタヒープ](19-descriptor-heap.md) | complete | 06 | 4タイプヒープ、アロケータ |
| 20 | [ビュー作成](20-views.md) | complete | 19 | SRV/UAV/CBV/RTV/DSV |
| 21 | [Sampler管理](21-sampler.md) | complete | 19 | サンプラー作成、キャッシュ |
| 22 | [RootSignature](22-root-signature.md) | complete | 19 | テーブルレイアウト、静的サンプラー |
| 23 | [PSO作成](23-pso-create.md) | complete | 22 | Graphics/Compute PSO |
| 24 | [シェーダーコンパイル](24-shader-compile.md) | complete | 06 | HLSL→DXIL、IDxcCompiler3 |
| 25 | [PSOキャッシュ + ステート変換](25-pso-cache.md) | complete | 23,24 | ハッシュキャッシュ、ステート変換 |
| 26 | [SwapChain + バックバッファ](26-swapchain.md) | complete | 07,20 | IDXGISwapChain4、RTV |
| 27 | [Present + フレームループ](27-present-frame.md) | complete | 26 | VSync、状態遷移、N-buffering |
| 28 | [三角形描画テスト](28-triangle-test.md) | complete | 27,25 | VS+PS、頂点バッファ、DrawInstanced |

### Phase 2: 機能拡張 — 11サブ計画

| # | 計画 | 状態 | 依存 | 概要 |
|---|------|------|------|------|
| 29 | [Legacy Barrier](29-legacy-barrier.md) | complete | 28 | Transition/UAV/Aliasing、Split Barriers |
| 30 | [Enhanced Barrier](30-enhanced-barrier.md) | complete | 29 | BARRIER_GROUP、Sync/Access |
| 31 | [自動バリア挿入](31-auto-barrier.md) | complete | 30 | StateTracker接続、バッチ最適化 |
| 32 | [Buddy/Poolアロケータ](32-buddy-pool-allocator.md) | complete | 28 | Placed Resource、2のべき乗分割 |
| 33 | [Transientアロケータ](33-transient-allocator.md) | complete | 32 | リニアアロケータ、RHI接続 |
| 34 | [Bindless Manager](34-bindless-manager.md) | complete | 28 | 永続SRV/UAVヒープ、ハンドル発行 |
| 35 | [ResourceTable + Bindless RootSig](35-resource-table.md) | complete | 34 | 動的追加/削除、SM6.6 |
| 36 | [QueryHeap + Timestamp](36-query-timestamp.md) | complete | 28 | クエリヒープ、GPU周波数 |
| 37 | [Occlusion + GPU Profiler](37-occlusion-profiler.md) | complete | 36 | Predicated Rendering、PIX |
| 38 | [Residency Manager](38-residency.md) | complete | 28 | MakeResident/Evict、LRU |
| 39 | [DeviceLost + MemoryStats](39-devicelost-stats.md) | complete | 38 | DRED、メモリ統計 |

### Phase 3: 先進機能 — 14サブ計画

| # | 計画 | 状態 | 依存 | 概要 |
|---|------|------|------|------|
| 40 | [BLAS](40-blas.md) | complete | 28 | BLAS構築、コンパクション |
| 41 | [TLAS + Feature検出](41-tlas.md) | complete | 40 | TLAS構築、RaytracingTier |
| 42 | [RT PSO + シェーダー識別子](42-rtpso.md) | complete | 41 | RTPSO、Collection、HitGroup |
| 43 | [SBT + DispatchRays](43-dispatch-rays.md) | complete | 42 | シェーダーバインディングテーブル |
| 44 | [WG State Object + バッキングメモリ](44-wg-state-object.md) | complete | 28 | EXECUTABLE、ノード登録 |
| 45 | [WG Dispatch](45-wg-dispatch.md) | complete | 44 | SetProgram、DispatchGraph |
| 46 | [WG Binder](46-wg-binder.md) | complete | 45 | 並列RecordBindings |
| 47 | [Mesh Shader PSO](47-mesh-pso.md) | complete | 28 | AS+MS+PS、SM6.5 |
| 48 | [DispatchMesh](48-dispatch-mesh.md) | complete | 47 | メッシュレットカリング |
| 49 | [VRS](49-vrs.md) | complete | 28 | Tier 1/2、シェーディングレートイメージ |
| 50 | [DT関数シグネチャ](50-dt-signatures.md) | complete | 13 | 全107+エントリ定義 |
| 51 | [DT登録 + STATIC_BACKEND](51-dt-registration.md) | complete | 50 | GRHIDispatchTable登録、LTO |
| 52 | [DT検証](52-dt-verification.md) | complete | 51 | 全構成動作確認 |
| 53 | [統合テスト](53-integration-test.md) | complete | 52 | 全フェーズE2E検証 |

## 実装方針

### ディレクトリ構造
```
source/engine/D3D12RHI/
  Public/
    D3D12RHI.h                    ← 公開定数・型
  Internal/
    D3D12ThirdParty.h             ← D3D12/DXGIヘッダー集約
  Private/
    D3D12Adapter.h/.cpp           ← Adapter列挙
    D3D12Device.h/.cpp            ← Device作成
    D3D12DynamicRHI.h/.cpp        ← IDynamicRHI実装
    D3D12Queue.h/.cpp             ← Queue管理
    D3D12Fence.h/.cpp             ← フェンス
    D3D12Submission.h/.cpp        ← サブミッション
    D3D12CommandAllocator.h/.cpp  ← アロケータ
    D3D12CommandList.h/.cpp       ← コマンドリスト
    D3D12CommandContext.h/.cpp    ← コマンドコンテキスト
    D3D12Dispatch.h/.cpp          ← DispatchTable関数群
    D3D12Resource.h/.cpp          ← リソース基底
    D3D12Buffer.h/.cpp            ← バッファ
    D3D12Texture.h/.cpp           ← テクスチャ
    D3D12Upload.h/.cpp            ← Upload Heap
    D3D12Descriptors.h/.cpp       ← ディスクリプタヒープ
    D3D12View.h/.cpp              ← ビュー（SRV/UAV/RTV/DSV）
    D3D12RootSignature.h/.cpp     ← ルートシグネチャ
    D3D12PipelineState.h/.cpp     ← PSO
    D3D12Shader.h/.cpp            ← シェーダー
    D3D12StateConvert.h/.cpp      ← ステート変換
    D3D12SwapChain.h/.cpp         ← スワップチェーン
    D3D12Barriers.h/.cpp          ← バリア管理
    D3D12Allocation.h/.cpp        ← メモリアロケータ
    D3D12Bindless.h/.cpp          ← Bindless管理
    D3D12Query.h/.cpp             ← クエリ
    D3D12Residency.h/.cpp         ← レジデンシー
    D3D12DeviceLost.h/.cpp        ← デバイスロスト
    D3D12RayTracingGeometry.h/.cpp ← BLAS
    D3D12RayTracingScene.h/.cpp   ← TLAS
    D3D12RayTracingPSO.h/.cpp     ← RT PSO
    D3D12RayTracingSBT.h/.cpp     ← SBT
    D3D12WorkGraph.h/.cpp         ← Work Graph
    D3D12WorkGraphBinder.h/.cpp   ← WG Binder
    D3D12MeshShader.h/.cpp        ← Mesh Shader
    D3D12VRS.h/.cpp               ← VRS
    D3D12RHIModule.cpp            ← モジュールエントリ
    D3D12RHIPrivate.h             ← 内部共通ヘッダー
```

### 命名規則
- クラス: `D3D12Xxx` (NS::D3D12RHI namespace)
- RHIインターフェース実装: `D3D12Buffer : public IRHIBuffer`
- ディスパッチ関数: `D3D12_Draw()`, `D3D12_Dispatch()` 等（NS_RHI_STATIC_BACKEND用）

### NS_RHI_DISPATCH接続
- 各DispatchTable関数エントリに対応するD3D12実装を提供
- `GRHIDispatchTable.Draw = &D3D12_Draw;` の形でモジュール初期化時に登録
- Shipping: `#define NS_RHI_STATIC_BACKEND D3D12` で直接呼出し

## 成功基準

### Phase 1 完了条件
- D3D12デバイス初期化成功
- ウィンドウにクリアカラー描画
- 三角形（頂点バッファ + シェーダー）描画
- 4構成（Debug/Release/Burst/Shipping）ビルド通過

### Phase 2 完了条件
- Enhanced Barriers自動挿入
- Bindlessリソースバインディング
- GPUタイムスタンプ取得
- デバイスロスト復旧

### Phase 3 完了条件
- DXR 1.1 レイトレーシング動作
- Mesh Shader パイプライン動作
- NS_RHI_STATIC_BACKEND によるゼロオーバーヘッド確認
- 全フェーズ統合テスト通過

## 拡張予定（本計画スコープ外）

以下は初期実装完了後に追加サブ計画として検討:

| 機能 | 概要 | 依存 |
|------|------|------|
| Multi-GPU / LDA | FD3D12LinkedAdapterObject、マルチノードリソース共有 | Phase 1 |
| HDR / 色空間 | IDXGISwapChain::SetColorSpace1、scRGB/HDR10 | 26-27 |
| MSAA リゾルブ | ResolveSubresource、サンプルレベルReadback | 17,20 |
| PSO Library | ID3D12PipelineLibrary、起動時プリロード | 25 |
