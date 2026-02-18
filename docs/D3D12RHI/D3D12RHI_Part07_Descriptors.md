# D3D12RHI Part 7: ディスクリプタ管理

## 7.1 概要

D3D12RHI のディスクリプタ管理は、D3D12 のディスクリプタヒープ制約を UE5 のレンダリングパイプラインに適合させるための多層システムである。オフライン (CPU-only) とオンライン (GPU-visible) の2種類のヒープを管理し、Bindless Rendering にも対応する。

```
+------------------------------------------------------------------+
|                    FD3D12DescriptorHeapManager                    |
|   (デバイスレベル。グローバルヒープ管理、プーリング)                    |
+------+-----------------------------------------------------------+
       |
       +--- FD3D12DescriptorManager (GlobalHeaps[2])
       |       グローバル Resource / Sampler ヒープ割当
       |
       +--- FD3D12OnlineDescriptorManager
       |       GPU-visible ヒープのブロック割当
       |
       +--- FD3D12OfflineDescriptorManager (ヒープタイプ別×4)
       |       CPU-only ディスクリプタのオンデマンド割当
       |
       +--- FD3D12BindlessDescriptorManager
               Bindless Rendering 用統合マネージャ
```


## 7.2 ディスクリプタヒープタイプマッピング

### 7.2.1 ERHIDescriptorHeapType → D3D12

| ERHIDescriptorHeapType | D3D12_DESCRIPTOR_HEAP_TYPE |
|------------------------|---------------------------|
| `Standard` | `CBV_SRV_UAV` |
| `RenderTarget` | `RTV` |
| `DepthStencil` | `DSV` |
| `Sampler` | `FD3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER` |

### 7.2.2 ERHIDescriptorType → ヒープタイプ

| ERHIDescriptorTypeMask | D3D12 ヒープタイプ |
|------------------------|--------------------|
| `CBV \| SRV \| UAV` | `CBV_SRV_UAV` |
| `Sampler` | Sampler |

### 7.2.3 ヒープフラグ

```cpp
enum class ED3D12DescriptorHeapFlags : uint8
{
    None       = 0,
    GpuVisible = 1 << 0,  // D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    Poolable   = 1 << 1,  // 破棄時にプールに返却
};
```


## 7.3 FD3D12DescriptorHeap — ヒープラッパー

`ID3D12DescriptorHeap` をラップし、CPU/GPU ハンドル計算とサブアロケーションを提供する。

```
FD3D12DescriptorHeap : FD3D12DeviceChild, FThreadSafeRefCountedObject
    |
    +--- TRefCountPtr<ID3D12DescriptorHeap> Heap
    +--- CpuBase / GpuBase   (ベースハンドル)
    +--- Offset              (サブアロケーション内オフセット)
    +--- NumDescriptors      (ヒープ内ディスクリプタ総数)
    +--- DescriptorSize      (ディスクリプタサイズ: デバイス依存)
    +--- Type / Flags
    +--- bIsGlobal / bIsSuballocation
```

**2種類のコンストラクタ:**
1. **独立ヒープ:** 自身の `ID3D12DescriptorHeap` を保持
2. **サブアロケーション:** 親ヒープの一部を参照 (`Offset` 付き、同一 D3D ヒープを共有)

**ハンドル計算:**
```cpp
GetCPUSlotHandle(Slot) → CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBase, Slot, DescriptorSize)
GetGPUSlotHandle(Slot) → CD3DX12_GPU_DESCRIPTOR_HANDLE(GpuBase, Slot, DescriptorSize)
```

**プーリング:** `CanBePooled()` (= `Poolable` フラグ) がtrueの場合、デストラクタで `FD3D12DescriptorHeapManager::AddHeapToPool()` に返却される。


## 7.4 FD3D12OfflineDescriptor と FD3D12OfflineDescriptorManager

### 7.4.1 FD3D12OfflineDescriptor

`D3D12_CPU_DESCRIPTOR_HANDLE` を継承し、バージョニングとヒープインデックスを追加。

```cpp
struct FD3D12OfflineDescriptor : public D3D12_CPU_DESCRIPTOR_HANDLE
{
    uint32 HeapIndex = 0;   // 所属ヒープの識別子
    uint32 Version = 0;     // バージョン (リネーム検出用)

    void IncrementVersion();  // View 更新時にインクリメント
    uint32 GetVersion() const;
};
```

**用途:** View (`FD3D12View`) が保持するオフラインディスクリプタ。バージョンを使ってキャッシュ無効化を行う。

### 7.4.2 FD3D12OfflineDescriptorManager

CPU-only ヒープからのディスクリプタスロット割当を管理する。ヒープタイプ (Standard, RTV, DSV, Sampler) ごとに1インスタンス。

```
FD3D12OfflineDescriptorManager : FD3D12DeviceChild
    |
    +--- TArray<FD3D12OfflineHeapEntry> Heaps  (オンデマンド拡張)
    |        |
    |        +--- TRefCountPtr<FD3D12DescriptorHeap> Heap
    |        +--- TDoubleLinkedList<FD3D12OfflineHeapFreeRange> FreeList
    |
    +--- TDoubleLinkedList<uint32> FreeHeaps  (空きスロットがあるヒープ一覧)
    +--- NumDescriptorsPerHeap / DescriptorSize
```

**割当フロー:**
1. `AllocateHeapSlot()` → `FreeHeaps` から空きスロットのあるヒープを検索
2. 見つからなければ `AllocateHeap()` で新規ヒープを作成
3. ヒープの `FreeList` からスロットを取得

**解放:** `FreeHeapSlot()` で FreeList にレンジを返却。


## 7.5 FD3D12DescriptorHeapManager — デバイスレベル統合管理

```
FD3D12DescriptorHeapManager : FD3D12DeviceChild
    |
    +--- GlobalHeaps[2]   (FD3D12DescriptorManager)
    |       [0] = Resource (CBV/SRV/UAV) グローバルヒープ
    |       [1] = Sampler グローバルヒープ
    |
    +--- TArray<FPooledHeap> PooledHeaps  (D3D ヒープのプール)
```

**ヒープ割当方法:**

| メソッド | 動作 |
|----------|------|
| `AllocateIndependentHeap()` | 独立した D3D ヒープを新規作成 |
| `AllocateHeap()` | グローバルヒープからサブアロケート可能ならサブアロケート、不可なら独立ヒープ |
| `DeferredFreeHeap()` | 遅延解放 (GPU 使用完了待ち) |
| `ImmediateFreeHeap()` | 即時解放 |
| `AddHeapToPool()` | D3D ヒープをプールに返却 |

**プーリング:** `AcquirePooledHeap()` がプールから同一タイプ・サイズ・フラグのヒープを再利用する。


## 7.6 FD3D12OnlineDescriptorManager — GPU-visible ブロック割当

GPU-visible ヒープをブロック単位で管理する。パイプライン (Graphics / AsyncCompute) ごとにヒープ参照を保持。

```
FD3D12OnlineDescriptorManager : FD3D12DeviceChild
    |
    +--- TRHIPipelineArray<FD3D12DescriptorHeapPtr> Heaps
    |       (パイプライン別ヒープ参照)
    |
    +--- TQueue<FD3D12OnlineDescriptorBlock*> FreeBlocks
            (利用可能ブロックキュー)
```

**ブロック割当:** `AllocateHeapBlock()` → `FreeBlocks` からデキュー。

**ブロック構造:**
```cpp
struct FD3D12OnlineDescriptorBlock
{
    uint32 BaseSlot;   // グローバルヒープ内の開始スロット
    uint32 Size;       // ブロック内スロット数
    uint32 SizeUsed;   // 使用済みスロット数
};
```

**初期化 (`Init`):**
- `InTotalSize` を `InBlockSize` で分割
- 各ブロックを `FreeBlocks` キューに追加
- Bindless Resources 有効時は `BindlessDescriptorManager` からヒープを取得


## 7.7 オンラインヒープ階層 — FD3D12OnlineHeap

### 7.7.1 基底クラス

```
FD3D12OnlineHeap : FD3D12DeviceChild
    |
    +--- FD3D12DescriptorHeapPtr Heap
    +--- NextSlotIndex    (次の割当位置)
    +--- FirstUsedSlot    (最初の使用中スロット)
    +--- bCanLoopAround   (循環割当サポート)
    |
    +--- CanReserveSlots() / ReserveSlots()  (スロット予約)
    +--- RollOver() = 0   (ヒープ枯渇時の処理: 純粋仮想)
```

### 7.7.2 3つの派生クラス

```
FD3D12OnlineHeap
    |
    +---> FD3D12GlobalOnlineSamplerHeap
    |       (グローバルサンプラーヒープ。ユニークサンプラーテーブル管理)
    |
    +---> FD3D12SubAllocatedOnlineHeap
    |       (FD3D12OnlineDescriptorManager からブロックをサブアロケート)
    |
    +---> FD3D12LocalOnlineHeap
            (コンテキストローカルオーバーフローヒープ。SyncPoint ベース循環)
```

**FD3D12GlobalOnlineSamplerHeap:**
- デバイス全体で共有
- `TSharedPtr<FD3D12SamplerSet>` でユニークサンプラーテーブルを管理
- `ConsolidateUniqueSamplerTables()` で重複排除

**FD3D12SubAllocatedOnlineHeap:**
- `FD3D12OnlineDescriptorManager` からブロックをリース
- コマンドリスト開始時にブロック割当 (`OpenCommandList`)
- `RollOver()` 時に新しいブロックを取得

**FD3D12LocalOnlineHeap:**
- オーバーフロー用のコンテキストローカルヒープ
- `TQueue<FSyncPointEntry>` で GPU 完了追跡
- `RollOver()` で SyncPoint を追加、`HeapLoopedAround()` でスロットリサイクル
- `TQueue<FPoolEntry> ReclaimPool` でヒープ再利用


## 7.8 FD3D12DescriptorCache — コンテキスト単位ディスクリプタキャッシュ

コマンドリスト記録中のディスクリプタバインディングを管理する中核クラス。

```
FD3D12DescriptorCache : FD3D12DeviceChild, FD3D12SingleNodeGPUObject
    |
    +--- FD3D12SubAllocatedOnlineHeap SubAllocatedViewHeap
    |       (通常時のビューディスクリプタ)
    |
    +--- FD3D12LocalOnlineHeap* LocalViewHeap
    |       (オーバーフロー時のフォールバック)
    |
    +--- FD3D12LocalOnlineHeap LocalSamplerHeap
    |       (ローカルサンプラーヒープ)
    |
    +--- FD3D12OnlineHeap* CurrentViewHeap     ← 現在使用中のビューヒープ
    +--- FD3D12OnlineHeap* CurrentSamplerHeap  ← 現在使用中のサンプラーヒープ
    |
    +--- ID3D12DescriptorHeap* LastSetViewHeap    ← 最後に SetDescriptorHeaps したヒープ
    +--- ID3D12DescriptorHeap* LastSetSamplerHeap
    |
    +--- FD3D12SamplerMap SamplerMap  (Conservative Map: サンプラーテーブルキャッシュ)
    +--- TArray<FD3D12UniqueSamplerTable> UniqueTables
    +--- TSharedPtr<FD3D12SamplerSet> LocalSamplerSet
    |
    +--- Bindless ヒープ (PLATFORM_SUPPORTS_BINDLESS_RENDERING)
         +--- BindlessResourcesHeap / BindlessSamplersHeap
         +--- bCouldUseBindless / bFullyBindless
```

### 7.8.1 ディスクリプタテーブル構築

| メソッド | 説明 |
|----------|------|
| `BuildSRVTable()` | SRV テーブル構築 → `D3D12_GPU_DESCRIPTOR_HANDLE` |
| `BuildUAVTable()` | UAV テーブル構築 |
| `BuildSamplerTable()` | Sampler テーブル構築 |
| `SetSRVTable()` / `SetUAVTable()` / `SetSamplerTable()` | テーブルをバインド |
| `SetConstantBufferViews()` | CBV テーブル設定 |
| `SetRootConstantBuffers()` | ルート CBV 設定 |

各 Build メソッドは:
1. `CurrentViewHeap->ReserveSlots()` でスロットを予約
2. オフラインディスクリプタを GPU-visible ヒープにコピー
3. `D3D12_GPU_DESCRIPTOR_HANDLE` を返却

### 7.8.2 ヒープ切り替え

| メソッド | 条件 |
|----------|------|
| `SwitchToContextLocalViewHeap()` | SubAllocated ヒープ枯渇時 |
| `SwitchToContextLocalSamplerHeap()` | グローバルサンプラーヒープ枯渇時 |
| `SwitchToGlobalSamplerHeap()` | ローカル→グローバルへ復帰 |

### 7.8.3 SetDescriptorHeaps

```cpp
bool SetDescriptorHeaps(ED3D12SetDescriptorHeapsFlags Flags);
```

ヒープが変更された場合のみ `ID3D12GraphicsCommandList::SetDescriptorHeaps()` を呼出す。
フラグ:
- `ForceChanged` — 強制更新
- `Bindless` — Bindless ヒープを使用

### 7.8.4 サンプラーマップ

`FD3D12ConservativeMap<FD3D12SamplerArrayDesc, D3D12_GPU_DESCRIPTOR_HANDLE>`:
ハッシュベースの高速ルックアップ (偽陰性あり)。同一サンプラー組合せの再利用を実現する。

`FD3D12SamplerArrayDesc`:
```cpp
struct FD3D12SamplerArrayDesc
{
    uint32 Count;
    uint16 SamplerID[MAX_SAMPLERS];  // サンプラー ID 配列
};
```

### 7.8.5 Explicit Descriptor Cache 連携

`SetExplicitDescriptorCache()` / `UnsetExplicitDescriptorCache()` でレイトレーシング用の Explicit Cache を接続/切断する。


## 7.9 サンプラー重複排除

### 7.9.1 FD3D12UniqueSamplerTable

```cpp
struct FD3D12UniqueSamplerTable
{
    FD3D12SamplerArrayDesc Key;                        // サンプラー ID 配列
    D3D12_CPU_DESCRIPTOR_HANDLE CPUTable[MAX_SAMPLERS]; // CPU ハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;              // グローバルヒープ内 GPU ハンドル
};
```

### 7.9.2 重複排除フロー

1. `FD3D12DescriptorCache` が `UniqueTables` にユニークサンプラーテーブルを蓄積
2. コマンドリスト終了時に `FD3D12GlobalOnlineSamplerHeap::ConsolidateUniqueSamplerTables()` で統合
3. `FD3D12SamplerSet` (`TSet<FD3D12UniqueSamplerTable>`) でグローバルに重複排除


## 7.10 UE::D3D12Descriptors ユーティリティ

| 関数 | 説明 |
|------|------|
| `CreateDescriptorHeap()` | D3D ヒープ作成ラッパー |
| `CopyDescriptor()` | 単一ディスクリプタコピー |
| `CopyDescriptors()` (4オーバーロード) | ヒープ間 / 散在 / オフライン→ヒープ コピー |
| `CreateOfflineCopy()` | ディスクリプタのオフラインコピー作成 |
| `FreeOfflineCopy()` | オフラインコピー解放 |


## 7.11 Bindless Rendering

### 7.11.1 アーキテクチャ

```
FD3D12BindlessDescriptorAllocator (Adapter レベル)
    |  共有ディスクリプタハンドル割当
    |  ERHIBindlessConfiguration 管理
    |
    +--- FRHIHeapDescriptorAllocator* ResourceAllocator
    +--- FRHIHeapDescriptorAllocator* SamplerAllocator
    |
    v
FD3D12BindlessDescriptorManager (Device レベル)
    |
    +--- FD3D12BindlessResourceManager
    |       CPU ヒープ + GPU ヒープ管理
    |       リネーム追跡、ヒープリサイズ
    |
    +--- FD3D12BindlessSamplerManager
            GPU ヒープ管理
```

### 7.11.2 FD3D12BindlessDescriptorAllocator (Adapter レベル)

Adapter 単位でハンドルを割り当て、マルチ GPU オブジェクト間で共有する。

```cpp
class FD3D12BindlessDescriptorAllocator : FD3D12AdapterChild
{
    ERHIBindlessConfiguration BindlessConfiguration;
    FRHIHeapDescriptorAllocator* ResourceAllocator;  // CBV/SRV/UAV 用
    FRHIHeapDescriptorAllocator* SamplerAllocator;   // Sampler 用

    FRHIDescriptorHandle AllocateDescriptor(ERHIDescriptorType);
    TOptional<FRHIDescriptorAllocation> AllocateDescriptors(ERHIDescriptorType, uint32 Count);
    void FreeDescriptor(FRHIDescriptorHandle);
};
```

`D3D12RHI_BINDLESS_RESOURCE_MANAGER_SUPPORTS_RESIZING` が定義されている場合、リソースアロケータの動的リサイズ (`ResizeGrowAndAllocate`) をサポートする。

### 7.11.3 FD3D12BindlessResourceManager (Device レベル)

リソースディスクリプタの CPU ヒープ→GPU ヒープ転送を管理する。

```
FD3D12BindlessResourceManager : FD3D12DeviceChild
    |
    +--- CpuHeap                     ← CPU-only ヒープ (マスターコピー)
    +--- ActiveGpuHeaps[]            ← GPU-visible ヒープ配列
    |       +--- GpuHeap
    |       +--- UpdatedHandles[]    ← 更新されたディスクリプタ一覧
    |       +--- bInUse / LastUsedGarbageCollectCycle
    +--- PooledGpuHeaps[]            ← プール済み GPU ヒープ
    +--- ActiveGpuHeapIndex          ← 現在のアクティブ GPU ヒープ
    +--- FMovingWindowMax<uint32,100> MovingWindowMaxInUseGPUHeaps
```

**ディスクリプタ更新フロー:**

1. `InitializeDescriptor()` — 初期化時に CPU ヒープにディスクリプタをコピー
2. `UpdateDescriptor()` — リネーム時に CPU ヒープ更新 + `UpdatedHandles[]` に追加
3. `FlushPendingDescriptorUpdates()` — コマンドリスト記録前に、更新されたディスクリプタをアクティブ GPU ヒープにコピー
4. `OpenCommandList()` — GPU ヒープをコンテキストに設定
5. `CloseCommandList()` — ヒープ使用カウント更新

**GPU ヒープローテーション:**
- `CheckRequestNewActiveGPUHeap()` — 更新が発生すると新しい GPU ヒープをリクエスト
- `AddActiveGPUHeap()` — CPU ヒープ全体を新しい GPU ヒープにコピー
- `GarbageCollect()` — 未使用 GPU ヒープをプールに返却

**CPU ヒープリサイズ:** `GrowCPUHeap()` で既存 CPU ヒープの内容を新しい大きなヒープにコピーし、`bCPUHeapResized = true` をマーク。

### 7.11.4 FD3D12BindlessSamplerManager (Device レベル)

サンプラーは静的なので、単一 GPU ヒープを使用する。

```cpp
class FD3D12BindlessSamplerManager : FD3D12DeviceChild
{
    FD3D12DescriptorHeapPtr GpuHeap;
    ERHIBindlessConfiguration Configuration;

    void InitializeDescriptor(FRHIDescriptorHandle, FD3D12SamplerState*);
    FD3D12DescriptorHeap* GetHeap() const;
};
```

### 7.11.5 FD3D12BindlessDescriptorManager (Device レベル統合)

Resource/Sampler マネージャを統合し、コンテキスト操作のエントリポイントを提供する。

| メソッド | 説明 |
|----------|------|
| `InitializeDescriptor(Handle, View*)` | ビューの初期バインドレスディスクリプタ初期化 |
| `InitializeDescriptor(Handle, Sampler*)` | サンプラーの初期化 |
| `UpdateDescriptor(Contexts, Handle, View*)` | リネーム時更新 |
| `ImmediateFree(Handle)` | 即時解放 |
| `DeferredFreeFromDestructor(Handle)` | デストラクタからの遅延解放 |
| `GarbageCollect()` | 未使用ヒープ回収 |
| `FlushPendingDescriptorUpdates(Context)` | 保留更新のフラッシュ |
| `OpenCommandList(Context)` | コマンドリスト開始時処理 |
| `CloseCommandList(Context)` | コマンドリスト終了時処理 |
| `SetHeapsForRayTracing(Context)` | レイトレーシング用ヒープ設定 |
| `GetExplicitHeapsForContext(Context, Config)` | Explicit Cache 用ヒープペア取得 |

### 7.11.6 FD3D12ContextBindlessState

コンテキストごとの Bindless 状態:
```cpp
struct FD3D12ContextBindlessState
{
    FD3D12DescriptorHeapPtr CurrentGpuHeap;
    bool bRefreshHeap = false;  // GPU ヒープ更新が必要
};
```


## 7.12 Explicit Descriptor Cache (レイトレーシング用)

### 7.12.1 FD3D12ExplicitDescriptorHeapCache

D3D ヒープのキャッシュプール。レイトレーシング用ディスクリプタヒープの再利用を実現する。

```cpp
class FD3D12ExplicitDescriptorHeapCache : FD3D12DeviceChild
{
    struct FEntry {
        ID3D12DescriptorHeap* Heap;
        uint32 NumDescriptors;
        D3D12_DESCRIPTOR_HEAP_TYPE Type;
        uint64 LastUsedFrame;
        double LastUsedTime;
    };

    FEntry AllocateHeap(Type, NumDescriptors);
    void DeferredReleaseHeap(FEntry&&);
    void FlushFreeList();
private:
    void ReleaseStaleEntries(uint32 MaxAgeInFrames, float MaxAgeInSeconds);
};
```

### 7.12.2 FD3D12ExplicitDescriptorHeap

アトミック線形割当方式のヒープ。ワーカースレッドから安全に割当できる。

```cpp
struct FD3D12ExplicitDescriptorHeap : FD3D12DeviceChild
{
    int32 Allocate(uint32 InNumDescriptors);  // アトミック線形割当

    void CopyDescriptors(int32 BaseIndex, const D3D12_CPU_DESCRIPTOR_HANDLE*, uint32 Num);
    bool CompareDescriptors(int32 BaseIndex, const D3D12_CPU_DESCRIPTOR_HANDLE*, uint32 Num);

    ID3D12Device* D3DDevice;    // ホットパス用キャッシュ
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    ID3D12DescriptorHeap* D3D12Heap;
    uint32 MaxNumDescriptors;
    int32  NumAllocatedDescriptors;  // アトミック

    // サンプラー重複排除
    int32 NumWrittenSamplerDescriptors;
    bool bExhaustiveSamplerDeduplication;
};
```

### 7.12.3 FD3D12ExplicitDescriptorCache

レイトレーシングパイプラインで使用されるディスクリプタキャッシュ。ワーカースレッド並列実行に対応する。

```cpp
class FD3D12ExplicitDescriptorCache : FD3D12DeviceChild
{
    FD3D12ExplicitDescriptorHeap ViewHeap;
    FD3D12ExplicitDescriptorHeap SamplerHeap;

    int32 Allocate(Descriptors[], NumDescriptors, Type, WorkerIndex);
    int32 AllocateDeduplicated(Versions[], Descriptors[], Num, Type, WorkerIndex);

    void ReserveViewDescriptors(uint32 Count, uint32 WorkerIndex);

    struct FWorkerThreadData {
        TDescriptorHashMap ViewDescriptorTableCache;    // Sherwood Hash Map
        TDescriptorHashMap SamplerDescriptorTableCache;
        FDescriptorSlotRange ReservedViewDescriptors;
    };

    TArray<FWorkerThreadData> WorkerData;  // ワーカー数分、キャッシュライン整列
};
```

**重複排除:** `AllocateDeduplicated()` はディスクリプタバージョン配列のハッシュを使い、`TSherwoodMap<uint64, int32>` で既存テーブルを再利用する。

**ワーカースレッドデータ:** `alignas(PLATFORM_CACHE_LINE_SIZE)` でキャッシュライン境界に整列し、偽共有を回避する。


## 7.13 クラス関係図

```
FD3D12Device
    |
    +--- FD3D12DescriptorHeapManager
    |       +--- GlobalHeaps[2] (FD3D12DescriptorManager)
    |       +--- PooledHeaps[]
    |
    +--- FD3D12OnlineDescriptorManager
    |       +--- Heaps[Pipeline]    ← GPU-visible
    |       +--- FreeBlocks (ブロックキュー)
    |
    +--- FD3D12OfflineDescriptorManager [×4 ヒープタイプ]
    |       +--- Heaps[] (CPU-only、オンデマンド拡張)
    |
    +--- FD3D12GlobalOnlineSamplerHeap
    |       +--- UniqueDescriptorTables (FD3D12SamplerSet)
    |
    +--- FD3D12BindlessDescriptorManager
            +--- FD3D12BindlessResourceManager
            |       +--- CpuHeap (マスター)
            |       +--- ActiveGpuHeaps[] (ローテーション)
            |       +--- PooledGpuHeaps[]
            +--- FD3D12BindlessSamplerManager
                    +--- GpuHeap (静的)

FD3D12Adapter
    +--- FD3D12BindlessDescriptorAllocator
            +--- ResourceAllocator (FRHIHeapDescriptorAllocator)
            +--- SamplerAllocator

FD3D12CommandContext
    +--- FD3D12DescriptorCache
    |       +--- SubAllocatedViewHeap  ← FD3D12OnlineDescriptorManager のブロック
    |       +--- LocalViewHeap         ← オーバーフロー用
    |       +--- LocalSamplerHeap
    |       +--- BindlessResourcesHeap / BindlessSamplersHeap
    |
    +--- FD3D12ExplicitDescriptorCache (レイトレーシング時のみ)
            +--- ViewHeap / SamplerHeap
            +--- WorkerData[] (ワーカー並列)
```

**ソースファイル構成:**

| ファイル | 行数 | 内容 |
|----------|------|------|
| `D3D12Descriptors.h` | 337 | ヒープラッパー、マネージャ、オフラインディスクリプタ |
| `D3D12Descriptors.cpp` | ~630 | ヒープ管理実装、コピー関数 |
| `D3D12BindlessDescriptors.h` | 327 | Bindless Allocator/ResourceManager/SamplerManager |
| `D3D12DescriptorCache.h` | 449 | オンラインヒープ、DescriptorCache、サンプラーマップ |
| `D3D12ExplicitDescriptorCache.h` | 198 | Explicit Cache (レイトレーシング用) |
