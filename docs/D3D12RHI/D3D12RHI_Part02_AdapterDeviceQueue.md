# D3D12RHI 実装ガイド Part 2: Adapter / Device / Queue アーキテクチャ

## 目次

1. [階層構造概要](#1-階層構造概要)
2. [FD3D12Adapter](#2-fd3d12adapter)
3. [FD3D12Device](#3-fd3d12device)
4. [FD3D12Queue](#4-fd3d12queue)
5. [マルチGPU (LDA) セットアップ](#5-マルチgpu-lda-セットアップ)
6. [キュータイプとマッピング](#6-キュータイプとマッピング)

---

## 1. 階層構造概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│              D3D12RHI 階層構造 (D3D12Adapter.h より)                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  [Engine]--                                                            │
│          |                                                              │
│          |-[RHI (FD3D12DynamicRHI)]--                                  │
│                                     |                                   │
│                                     |-[Adapter (FD3D12Adapter)]-- (LDA)│
│                                     |                            |     │
│                                     |                            |- [Device (FD3D12Device)]
│                                     |                            |     |                  │
│                                     |                            |     |-[Queue (Graphics)]
│                                     |                            |     |-[Queue (Copy)]   │
│                                     |                            |     |-[Queue (Async)]  │
│                                     |                            |                        │
│                                     |                            |- [Device]              │
│                                     |                                                     │
│                                     |-[Adapter]--                                         │
│                                                  |                                        │
│                                                  |- [Device]--                            │
│                                                               |                           │
│                                                               |-[Queue]                   │
│                                                               |-[Queue]                   │
│                                                               |-[Queue]                   │
│                                                                                           │
│  FD3D12Device = 1物理GPU内の1ノード                                     │
│  FD3D12Adapter = 1物理アダプタ（LDA時は複数ノード=複数Device）           │
│                                                                         │
│  構成例:                                                                │
│  ・シングルGPU: 1 Adapter, 1 Device, 3 Queues                          │
│  ・SLI/Crossfire (LDA): 1 Adapter, 2 Devices, 6 Queues                │
│  ・Discrete+Integrated: 2 Adapters, 2 Devices, 6 Queues               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. FD3D12Adapter

### 2.1 クラス概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12Adapter : public FNoncopyable                                   │
│  (D3D12RHI/Private/D3D12Adapter.h)                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  LDA (Linked Display Adapter) 環境における物理アダプタ単位の管理クラス。│
│  1つの ID3D12Device を所有し、複数の FD3D12Device (ノード) を管理。    │
│  GPU 間で共有可能なリソース（PSO、Root Signature 等）をここで管理する。 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 FD3D12AdapterDesc

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `Desc` | `DXGI_ADAPTER_DESC` | DXGI アダプタ記述子 |
| `AdapterIndex` | `int32` | アダプタインデックス (-1: 未検出) |
| `MaxSupportedFeatureLevel` | `D3D_FEATURE_LEVEL` | 最大 Feature Level |
| `MaxSupportedShaderModel` | `D3D_SHADER_MODEL` | 最大 Shader Model |
| `ResourceBindingTier` | `D3D12_RESOURCE_BINDING_TIER` | リソースバインディングティア |
| `ResourceHeapTier` | `D3D12_RESOURCE_HEAP_TIER` | リソースヒープティア |
| `MaxRHIFeatureLevel` | `ERHIFeatureLevel::Type` | 最大 RHI Feature Level |
| `NumDeviceNodes` | `uint32` | デバイスノード数 (GPU 数) |
| `bUMA` | `bool` | Unified Memory Architecture (統合GPU) |
| `bSupportsWaveOps` | `bool` | SM6.0 Wave Operations 対応 |
| `bSupportsAtomic64` | `bool` | SM6.6 Atomic64 対応 |
| `GpuPreference` | `DXGI_GPU_PREFERENCE` | GPU 選好 (DXGI Factory6+) |

### 2.3 FD3D12DeviceBasicInfo

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12DeviceBasicInfo                                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FindAdapter() フェーズで GPU の基本能力を格納する構造体:               │
│  ・MaxFeatureLevel       (D3D_FEATURE_LEVEL)                           │
│  ・MaxShaderModel        (D3D_SHADER_MODEL)                            │
│  ・ResourceBindingTier   (D3D12_RESOURCE_BINDING_TIER)                 │
│  ・ResourceHeapTier      (D3D12_RESOURCE_HEAP_TIER)                    │
│  ・NumDeviceNodes        (uint32)                                      │
│  ・bSupportsWaveOps      (bool)                                        │
│  ・bSupportsAtomic64     (bool)                                        │
│  ・bUMA                  (bool)                                        │
│  ・MaxRHIFeatureLevel    (ERHIFeatureLevel::Type)                      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.4 主要メンバ変数

| メンバ | 型 | 説明 |
|--------|-----|------|
| **D3D12 デバイス** | | |
| `RootDevice` | `TRefCountPtr<ID3D12Device>` | 基本デバイス |
| `RootDevice1〜12` | `TRefCountPtr<ID3D12DeviceN>` | 各バージョンデバイスインターフェース |
| **DXGI Factory** | | |
| `DxgiFactory2〜7` | `TRefCountPtr<IDXGIFactoryN>` | DXGI Factory 各バージョン |
| `DxgiAdapter` | `TRefCountPtr<IDXGIAdapter>` | DXGI アダプタ |
| **デバイスノード** | | |
| `Devices` | `TStaticArray<FD3D12Device*, MAX_NUM_GPUS>` | GPU ノード配列 |
| **共有リソース** | | |
| `RootSignatureManager` | `FD3D12RootSignatureManager` | Root Signature 管理 |
| `PipelineStateCache` | `FD3D12PipelineStateCache` | PSO キャッシュ |
| `FrameFence` | `TUniquePtr<FD3D12ManualFence>` | フレーム単位フェンス |
| `DefaultContextRedirector` | `FD3D12CommandContextRedirector` | MGPU コンテキストルーティング |
| **Indirect コマンドシグネチャ** | | |
| `DrawIndirectCommandSignature` | `TRefCountPtr<ID3D12CommandSignature>` | Draw Indirect |
| `DrawIndexedIndirectCommandSignature` | 同上 | DrawIndexed Indirect |
| `DispatchIndirectGraphicsCommandSignature` | 同上 | Dispatch Indirect (Graphics) |
| `DispatchIndirectComputeCommandSignature` | 同上 | Dispatch Indirect (Compute) |
| `DispatchIndirectMeshCommandSignature` | 同上 | Mesh Shader Dispatch Indirect |
| `DispatchRaysIndirectCommandSignature` | 同上 | DispatchRays Indirect |
| **アロケータ** | | |
| `UploadHeapAllocator` | `FD3D12UploadHeapAllocator*[MAX_NUM_GPUS]` | GPU ごとのアップロードヒープ |
| **ケーパビリティ** | | |
| `RootSignatureVersion` | `D3D_ROOT_SIGNATURE_VERSION` | Root Signature バージョン |
| `bDepthBoundsTestSupported` | `bool` | Depth Bounds Test 対応 |
| `bCopyQueueTimestampQueriesSupported` | `bool` | コピーキューでのタイムスタンプ |
| `bHeapNotZeroedSupported` | `bool` | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED 対応 |
| `VRSTileSize` | `uint32` | Variable Rate Shading タイルサイズ |
| **バリア** | | |
| `PreferredBarrierImplType` | `static ED3D12BarrierImplementationType` | Enhanced/Legacy バリア選択 |
| `Barriers` | `static TUniquePtr<BarriersForAdapterType>` | バリアファクトリ実装 |
| **メモリ統計** | | |
| `MemoryStats` | `FD3DMemoryStats` | メモリ統計情報 |
| `TrackedAllocationData` | `TMap<...>` | アロケーション追跡マップ |
| `TrackedHeaps` | `TArray<FD3D12Heap*>` | 追跡ヒープ |

### 2.5 FD3D12MemoryInfo

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12MemoryInfo                                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  DXGI_QUERY_VIDEO_MEMORY_INFO ベースのメモリ情報:                       │
│  ・LocalMemoryInfo     — ローカル (VRAM) メモリ情報                    │
│  ・NonLocalMemoryInfo  — 非ローカル (システムメモリ) 情報              │
│  ・AvailableLocalMemory — 利用可能なローカルメモリ                     │
│  ・DemotedLocalMemory   — デモート (eviction) されたメモリ            │
│  ・IsOverBudget()       — DemotedLocalMemory > 0 で true              │
│  ・UpdateFrameNumber    — 最終更新フレーム番号                         │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.6 リソース作成メソッド

| メソッド | 説明 |
|---------|------|
| `CreateCommittedResource(Desc, Node, HeapProps, ...)` | Committed Resource 作成 (専用ヒープ) |
| `CreateCommittedResourceRaw(HeapProps, Flags, Desc, ...)` | 低レベル Committed Resource (バリアファクトリ経由) |
| `CreatePlacedResource(Desc, BackingHeap, Offset, ...)` | Placed Resource 作成 (既存ヒープ上) |
| `CreateReservedResource(Desc, Node, ...)` | Reserved Resource 作成 (仮想テクスチャ用) |
| `CreateBuffer(HeapType, Node, VisibleNodes, Size, ...)` | バッファ作成 (3つのオーバーロード) |
| `CreateRHIBuffer(Desc, Alignment, CreateDesc, ...)` | RHI バッファ作成 (高レベル) |
| `CreateLinkedObject<T>(GPUMask, pfnCreation)` | マルチGPU リンクオブジェクト作成ヘルパー |

### 2.7 初期化フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│        FD3D12Adapter 初期化シーケンス                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. FD3D12Adapter(DescIn)                                              │
│     └→ Desc を格納、Devices 配列を nullptr 初期化                     │
│                                                                         │
│  2. InitializeDevices()  [FD3D12DynamicRHI::Init() から呼出]            │
│     ├→ CreateDXGIFactory(bWithDebug)                                   │
│     │   └→ IDXGIFactory2〜7 を QueryInterface で取得                  │
│     ├→ CreateRootDevice(bWithDebug)                                    │
│     │   ├→ D3D12CreateDevice() (RootDevice 作成)                      │
│     │   └→ ID3D12Device1〜12 を QueryInterface で取得                 │
│     ├→ CreateD3DInfoQueue() (デバッグ時)                               │
│     ├→ Feature 検出:                                                   │
│     │   ・Depth Bounds Test                                            │
│     │   ・Heap Not Zeroed                                              │
│     │   ・Copy Queue Timestamp                                         │
│     │   ・Root Signature Version                                       │
│     │   ・VRS Tile Size                                                │
│     ├→ CreateCommandSignatures()                                       │
│     │   └→ DrawIndirect, DispatchIndirect, DispatchRays 等            │
│     ├→ FD3D12Device 生成 (ノードごと)                                  │
│     │   └→ SetupAfterDeviceCreation()                                 │
│     └→ FrameFence 生成                                                 │
│                                                                         │
│  3. InitializeExplicitDescriptorHeap()  [PostInit]                     │
│     └→ 各デバイスの ExplicitDescriptorHeapCache 初期化                │
│                                                                         │
│  4. InitializeRayTracing()  [PostInit, RT サポート時]                   │
│     └→ 各デバイスの RT パイプラインキャッシュ等を初期化                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. FD3D12Device

### 3.1 クラス概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12Device final : public FD3D12SingleNodeGPUObject,                │
│                       public FNoncopyable,                             │
│                       public FD3D12AdapterChild                        │
│  (D3D12RHI/Private/D3D12Device.h)                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  単一 GPU ノードを表す。物理 GPU の1ノードに対応し、                    │
│  コマンドキュー、ディスクリプタヒープ、メモリアロケータ等を管理する。   │
│                                                                         │
│  FD3D12SingleNodeGPUObject を継承し GPUIndex / GPUMask を持つ。        │
│  FD3D12AdapterChild を継承し ParentAdapter にアクセス可能。             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 主要サブシステム

```
┌─────────────────────────────────────────────────────────────────────────┐
│        FD3D12Device サブシステム一覧                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【コマンドキュー (3本)】                                                │
│  Queues : TArray<FD3D12Queue, TFixedAllocator<Count>>                  │
│    ・Direct (Graphics)  - 3D レンダリング                              │
│    ・Copy               - データ転送                                   │
│    ・Async (Compute)    - 非同期コンピュート                           │
│                                                                         │
│  【コマンドコンテキスト】                                                │
│  ImmediateCommandContext : FD3D12CommandContext*                        │
│    ・デフォルト (Graphics) コマンドコンテキスト                         │
│    ・ObtainContextGraphics/Compute/Copy() で追加コンテキスト取得       │
│    ・ReleaseContext() でプールに返却                                   │
│                                                                         │
│  【ディスクリプタ管理】                                                  │
│  DescriptorHeapManager : FD3D12DescriptorHeapManager                   │
│    ・GPU visible ヒープ管理                                            │
│  OnlineDescriptorManager : FD3D12OnlineDescriptorManager               │
│    ・オンラインディスクリプタ割り当て                                    │
│  OfflineDescriptorManagers : TArray<FD3D12OfflineDescriptorManager>    │
│    ・CPU only ヒープ (ヒープタイプごと)                                 │
│  GlobalSamplerHeap : FD3D12GlobalOnlineSamplerHeap                     │
│    ・グローバルサンプラーヒープ                                         │
│                                                                         │
│  【Bindless (PLATFORM_SUPPORTS_BINDLESS_RENDERING 時)】                │
│  BindlessDescriptorAllocator : FD3D12BindlessDescriptorAllocator&      │
│  BindlessDescriptorManager : FD3D12BindlessDescriptorManager           │
│                                                                         │
│  【メモリアロケータ】                                                    │
│  DefaultBufferAllocator : FD3D12DefaultBufferAllocator                 │
│    ・バッファのサブアロケーション                                       │
│  DefaultFastAllocator : FD3D12FastAllocator                            │
│    ・高速小規模アロケーション                                           │
│  TextureAllocator : FD3D12TextureAllocatorPool                         │
│    ・テクスチャ専用プール                                               │
│                                                                         │
│  【レジデンシー管理】                                                    │
│  ResidencyManager : FD3D12ResidencyManager                             │
│    ・GPU メモリレジデンシー管理                                         │
│                                                                         │
│  【クエリ管理】                                                          │
│  QueryHeapPool : TStaticArray<TD3D12ObjectPool<FD3D12QueryHeap>, 4>    │
│    ・クエリタイプ別のヒーププール                                       │
│                                                                         │
│  【サンプラーキャッシュ】                                                │
│  SamplerCache : TLruCache<D3D12_SAMPLER_DESC, TRefCountPtr<...>>       │
│    ・LRU キャッシュによるサンプラー重複排除                             │
│                                                                         │
│  【レイトレーシング (D3D12_RHI_RAYTRACING 時)】                         │
│  RayTracingPipelineCache : FD3D12RayTracingPipelineCache*              │
│  RayTracingCompactionRequestHandler : FD3D12RayTracing...Handler*      │
│  ExplicitDescriptorHeapCache : FD3D12ExplicitDescriptorHeapCache*      │
│                                                                         │
│  【タイルマッピング】                                                    │
│  TileMappingQueue : TRefCountPtr<ID3D12CommandQueue>                   │
│  TileMappingFence : FD3D12Fence                                        │
│    ・仮想テクスチャ用の専用キュー（必要時のみ）                         │
│                                                                         │
│  【その他】                                                              │
│  DefaultViews : FD3D12DefaultViews                                     │
│    ・デフォルト SRV/UAV/RTV/DSV                                        │
│  ConstantBufferPageProperties : D3D12_HEAP_PROPERTIES                  │
│    ・定数バッファ用ヒーププロパティ                                     │
│  AvailableMSAAQualities[9] : uint32                                    │
│    ・MSAA サンプル数ごとの品質レベル                                    │
│  ResourceAllocationInfoMap : TMap<uint64, D3D12_RESOURCE_ALLOCATION_INFO>│
│    ・リソースアロケーション情報キャッシュ (RWLock 保護)                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.3 オブジェクトプール操作

| メソッド | 説明 |
|---------|------|
| `ObtainCommandAllocator(QueueType)` | コマンドアロケータ取得 (プールから) |
| `ReleaseCommandAllocator(Allocator)` | コマンドアロケータ返却 |
| `ObtainContext(QueueType)` | コマンドコンテキスト取得 |
| `ObtainContextCopy()` | Copy コンテキスト取得 (static_cast) |
| `ObtainContextCompute()` | Compute コンテキスト取得 |
| `ObtainContextGraphics()` | Graphics コンテキスト取得 |
| `ReleaseContext(Context)` | コンテキスト返却 |
| `ObtainQueryHeap(QueueType, QueryType)` | クエリヒープ取得 |
| `ReleaseQueryHeap(QueryHeap)` | クエリヒープ返却 |
| `ObtainCommandList(Allocator, Timestamps, PipelineStats)` | コマンドリスト取得 |
| `ReleaseCommandList(CommandList)` | コマンドリスト返却 |

### 3.4 初期化フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│        FD3D12Device 初期化シーケンス                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12Device(GPUMask, Adapter)                                        │
│    ├→ FD3D12SingleNodeGPUObject(GPUMask) — GPUIndex 設定              │
│    ├→ FD3D12AdapterChild(Adapter) — 親 Adapter 設定                   │
│    ├→ ResidencyManager 初期化                                          │
│    ├→ DescriptorHeapManager 初期化                                     │
│    ├→ OfflineDescriptorManagers 初期化 (ヒープタイプごと)              │
│    ├→ GlobalSamplerHeap 初期化                                         │
│    ├→ OnlineDescriptorManager 初期化                                   │
│    ├→ DefaultViews 初期化                                              │
│    ├→ FD3D12Queue 3本生成:                                             │
│    │   ・Queue[0] = Direct (Graphics)                                  │
│    │   ・Queue[1] = Copy                                               │
│    │   ・Queue[2] = Async (Compute)                                    │
│    ├→ DefaultBufferAllocator 初期化                                    │
│    ├→ DefaultFastAllocator 初期化                                      │
│    └→ TextureAllocator 初期化                                          │
│                                                                         │
│  SetupAfterDeviceCreation()                                            │
│    ├→ CreateDefaultViews() — デフォルト SRV/UAV 等                    │
│    ├→ UpdateMSAASettings() — MSAA 品質レベル検出                      │
│    ├→ UpdateConstantBufferPageProperties()                             │
│    └→ ImmediateCommandContext 生成                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. FD3D12Queue

### 4.1 クラス概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12Queue final                                                     │
│  (D3D12RHI/Private/D3D12Device.h, enum は D3D12Queue.h)                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  単一の D3D12 コマンドキューをカプセル化する。                           │
│  サブミッションスレッドがキュー管理に必要な状態を保持する。             │
│                                                                         │
│  各 FD3D12Device は3本のキューを持つ:                                   │
│  ・Direct  (Graphics) — 3D レンダリング                                │
│  ・Copy               — データ転送                                    │
│  ・Async  (Compute)   — 非同期コンピュート                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 メンバ変数

| メンバ | 型 | 説明 |
|--------|-----|------|
| `Device` | `FD3D12Device* const` | 所有デバイス |
| `QueueType` | `ED3D12QueueType const` | キュータイプ |
| `QueueIndex` | `int32 const` | グローバルキューインデックス |
| `D3DCommandQueue` | `TRefCountPtr<ID3D12CommandQueue>` | ネイティブ D3D12 コマンドキュー |
| `Fence` | `FD3D12Fence` | キュー完了管理用フェンス |
| `PendingSubmission` | `TMpscQueue<FD3D12Payload*>` | サブミッション待ちペイロード (MPSC) |
| `PendingInterrupt` | `TSpscQueue<FD3D12Payload*>` | 割り込み待ちペイロード (SPSC) |
| `PayloadToSubmit` | `FD3D12Payload*` | 現在サブミット中のペイロード |
| `BarrierAllocator` | `FD3D12CommandAllocator*` | バリア用コマンドアロケータ |
| `BarrierTimestamps` | `FD3D12QueryAllocator` | バリアタイムスタンプ |
| `NumCommandListsInBatch` | `uint32` | バッチ内コマンドリスト数 |
| `BatchedObjects` | `FD3D12BatchedPayloadObjects` | バッチ化ペイロードオブジェクト |
| `ObjectPool` | `struct { Contexts, Allocators, Lists }` | 再利用可能オブジェクトプール |
| `Timing` | `FD3D12Timing*` | GPU タイミング情報 (割り込みスレッド管理) |
| `DiagnosticBuffer` | `TUniquePtr<FD3D12DiagnosticBuffer>` | 診断バッファ (ブレッドクラム) |
| `bSupportsTileMapping` | `bool` | タイルマッピング対応 |
| `ExecuteCommandListsFence` | `FD3D12Fence` | ECL 前の内部フェンス |

### 4.3 オブジェクトプール構造

```
┌─────────────────────────────────────────────────────────────────────────┐
│        FD3D12Queue::ObjectPool                                         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  struct {                                                              │
│    TD3D12ObjectPool<FD3D12ContextCommon>    Contexts;                  │
│    TD3D12ObjectPool<FD3D12CommandAllocator> Allocators;                │
│    TD3D12ObjectPool<FD3D12CommandList>      Lists;                     │
│  };                                                                    │
│                                                                         │
│  各プールは TLockFreePointerListUnordered ベースのロックフリーリスト。  │
│  デストラクタで残存オブジェクトを自動削除。                              │
│  サブミッションスレッドがコマンドリスト実行後に返却する。               │
│                                                                         │
│  MaxBatchedPayloads = 128                                              │
│  FPayloadArray = TArray<FD3D12Payload*, TInlineAllocator<128>>         │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.4 主要メソッド

| メソッド | 説明 |
|---------|------|
| `FinalizePayload(bRequiresSignal, PayloadsToHandDown)` | ペイロードのコマンドリストをバッチ化し、最新フェンス値を返す |
| `ExecuteCommandLists(D3DCommandLists, ResidencySets)` | `ID3D12Queue::ExecuteCommandLists` のラッパー (レジデンシー管理付き) |
| `CleanupResources()` | キューリソースのクリーンアップ |

### 4.5 FD3D12Fence

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12Fence (D3D12Submission.h)                                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  デバイスキューが GPU 完了を管理するフェンス型:                          │
│                                                                         │
│  ・OwnerQueue          (FD3D12Queue* const)  — 所有キュー             │
│  ・D3DFence            (TRefCountPtr<ID3D12Fence>)  — D3D12 フェンス  │
│  ・NextCompletionValue (uint64 = 1)          — 次のシグナル値         │
│  ・LastSignaledValue   (atomic<uint64> = 0)  — 最後にシグナルされた値 │
│  ・bInterruptAwaited   (bool = false)        — 割り込み待機中        │
│                                                                         │
│  フェンス値は単調増加。各 Payload の完了が FinalizePayload() で        │
│  NextCompletionValue に紐付けられ、GPU 側で Signal される。             │
│  割り込みスレッドが LastSignaledValue を更新して完了を検出する。        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.6 FD3D12DiagnosticBuffer

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12DiagnosticBuffer : public FRHIDiagnosticBuffer                  │
│  (D3D12Device.h)                                                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  仮想ヒープに支えられた診断バッファ。GPU クラッシュ後もアクセス可能。  │
│  ブレッドクラムマーカーによる GPU 進行状況追跡に使用。                  │
│                                                                         │
│  ・Heap      — FD3D12Heap (仮想ヒープ)                                │
│  ・Resource  — FD3D12Resource                                         │
│  ・GpuAddress — GPU 仮想アドレス                                       │
│                                                                         │
│  WITH_RHI_BREADCRUMBS 時:                                              │
│  ・GetGPUQueueMarkerIn()  — 入口マーカーの GPU アドレス               │
│  ・GetGPUQueueMarkerOut() — 出口マーカーの GPU アドレス               │
│  ・ReadMarkerIn/Out()     — マーカー値の読み取り                      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 5. マルチGPU (LDA) セットアップ

### 5.1 LDA フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│        マルチ GPU (LDA) セットアップフロー                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. FindAdapter() で GPU を列挙                                        │
│     ・FD3D12AdapterDesc.NumDeviceNodes > 1 → LDA 検出                 │
│       (NumDeviceNodes は ID3D12Device::GetNodeCount() から取得)       │
│     ・GNumExplicitGPUsForRendering に反映                               │
│                                                                         │
│  2. FD3D12Adapter::InitializeDevices()                                 │
│     ・1つの ID3D12Device (RootDevice) を作成                           │
│     ・ノードごとに FD3D12Device を生成                                  │
│     ・各 FD3D12Device に GPUMask (ノードマスク) を設定                 │
│                                                                         │
│  3. リソース作成                                                        │
│     ・CreateLinkedObjects<T>(GPUMask, ...) で各ノードに                │
│       オブジェクトを生成し FD3D12LinkedAdapterObject で連結             │
│     ・GPUMask.GetFirstIndex() の結果が代表オブジェクト                  │
│                                                                         │
│  4. コンテキストルーティング                                             │
│     ・FD3D12CommandContextRedirector が MGPU 環境で                     │
│       GPU マスクに基づきコマンドを適切なデバイスにルーティング          │
│     ・GNumExplicitGPUsForRendering == 1 時は直接                       │
│       Device->GetDefaultCommandContext() を使用                         │
│                                                                         │
│  5. クロスノード可視性                                                   │
│     ・FD3D12MultiNodeGPUObject で複数ノードからの                       │
│       可視性を持つリソースを管理                                        │
│     ・D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER 等で共有                   │
│                                                                         │
│  【Windows での WITH_MGPU 定義条件】                                    │
│  ・RHI.Build.cs: WITH_MGPU = Windows Desktop のみ true                │
│  ・D3D12RHICommon.h の各クラスは #if WITH_MGPU で分岐                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 FTransientUniformBufferAllocator

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FTransientUniformBufferAllocator                                      │
│  : FD3D12FastConstantAllocator, TThreadSingleton<...>                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  スレッドローカルな一時的 Uniform Buffer アロケータ。                   │
│  TThreadSingleton を継承しスレッドごとに1インスタンス。                 │
│                                                                         │
│  ・Adapter が管理する TransientUniformBufferAllocators に登録           │
│  ・GetTransientUniformBufferAllocator() で取得                         │
│  ・ReleaseTransientUniformBufferAllocator() で返却                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 6. キュータイプとマッピング

### 6.1 ED3D12QueueType (D3D12Queue.h)

| 値 | 名前 | D3D12 コマンドリスト型 | 表示名 |
|-----|------|----------------------|--------|
| `0` | `Direct` | `D3D12_COMMAND_LIST_TYPE_DIRECT` | "3D" |
| `1` | `Copy` | `D3D12RHI_PLATFORM_COPY_COMMAND_LIST_TYPE` | "Copy" |
| `2` | `Async` | `D3D12_COMMAND_LIST_TYPE_COMPUTE` | "Compute" |
| `3` | `Count` | — | — |

### 6.2 RHI パイプラインとのマッピング

```
┌─────────────────────────────────────────────────────────────────────────┐
│        ERHIPipeline → ED3D12QueueType マッピング                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  GetD3DCommandQueueType(ERHIPipeline Pipeline):                        │
│  ・ERHIPipeline::Graphics     → ED3D12QueueType::Direct               │
│  ・ERHIPipeline::AsyncCompute → ED3D12QueueType::Async                │
│                                                                         │
│  ※ Copy キューには直接マッピングされる ERHIPipeline がない             │
│    内部的にデータ転送時に使用される                                     │
│                                                                         │
│  GetD3DCommandListType(ED3D12QueueType):                               │
│  ・Direct → D3D12_COMMAND_LIST_TYPE_DIRECT                            │
│  ・Copy   → D3D12RHI_PLATFORM_COPY_COMMAND_LIST_TYPE                  │
│             (Windows: D3D12_COMMAND_LIST_TYPE_COPY)                     │
│  ・Async  → D3D12_COMMAND_LIST_TYPE_COMPUTE                           │
│                                                                         │
│  GD3D12MaxNumQueues = MAX_NUM_GPUS * 3 (Count)                        │
│  ※ シングル GPU: 3, デュアル GPU (LDA): 6                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.3 FD3D12Timing

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12Timing (D3D12Device.h)                                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  フレームあたりの GPU キューパフォーマンス追跡:                          │
│                                                                         │
│  ・Queue (FD3D12Queue&) — 対象キュー                                  │
│  ・PipelineStats (D3D12_QUERY_DATA_PIPELINE_STATISTICS)                │
│                                                                         │
│  【RHI_NEW_GPU_PROFILER 時】                                            │
│  ・GPUFrequency / GPUTimestamp — GPU 側タイマー校正                   │
│  ・CPUFrequency / CPUTimestamp — CPU 側タイマー校正                   │
│  ・EventStream (UE::RHI::GPUProfiler::FEventStream)                   │
│                                                                         │
│  【旧プロファイラー時】                                                  │
│  ・Timestamps (TArray<uint64>) — タイムスタンプ配列                   │
│  ・TimestampIndex — 現在位置                                           │
│  ・BusyCycles — GPU ビジーサイクル累計                                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```
