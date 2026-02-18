# D3D12RHI Part 5: メモリアロケーションとプーリング

## 目次
1. [アロケーションアーキテクチャ概要](#1-アロケーションアーキテクチャ概要)
2. [Buddy Allocator](#2-buddy-allocator)
3. [MultiBuddy Allocator](#3-multibuddy-allocator)
4. [Bucket Allocator](#4-bucket-allocator)
5. [Pool Allocator (FD3D12PoolAllocator)](#5-pool-allocator-fd3d12poolallocator)
6. [Segregated Free-List Allocator (FD3D12SegListAllocator)](#6-segregated-free-list-allocator-fd3d12seglistallocator)
7. [Upload Heap Allocator](#7-upload-heap-allocator)
8. [Default Buffer Allocator](#8-default-buffer-allocator)
9. [Texture Allocator Pool](#9-texture-allocator-pool)
10. [Fast Allocator (一時アロケーション)](#10-fast-allocator-一時アロケーション)
11. [Transient Resource Allocator](#11-transient-resource-allocator)
12. [コンパイル時選択とプラットフォーム分岐](#12-コンパイル時選択とプラットフォーム分岐)

---

## 1. アロケーションアーキテクチャ概要

D3D12RHIはGPUメモリを効率的に管理するため、用途別に複数のアロケータを使い分ける。D3D12の制約(Placed Resourceの64KB最小サイズ等)に対応しつつ、Committed Resource作成コスト(PC上で平均~420μs)を削減するため、プール方式を積極採用。

```
┌─────────────────────────────────────────────────────────────────────┐
│                    FD3D12DynamicRHI / FD3D12Adapter                  │
└────────────────────────────┬────────────────────────────────────────┘
                             │
         ┌───────────────────┼───────────────────────┐
         ▼                   ▼                       ▼
┌────────────────┐  ┌────────────────┐  ┌─────────────────────────┐
│FD3D12Default   │  │FD3D12Upload    │  │FD3D12TextureAllocator   │
│BufferAllocator │  │HeapAllocator   │  │Pool                     │
│(per Device)    │  │(per GPU)       │  │(per Device)             │
└───────┬────────┘  └───────┬────────┘  └──────────┬──────────────┘
        │                   │                      │
   ┌────┴────┐      ┌──────┴──────┐         ┌─────┴─────┐
   ▼         ▼      ▼      ▼     ▼         ▼           ▼
┌──────┐ ┌──────┐ ┌────┐┌────┐┌────┐ ┌──────────┐ ┌──────────┐
│Pool  │ │Buddy │ │Bud.││Pool││Bud.│ │Pool      │ │SegList   │
│Alloc.│ │Alloc.│ │(sm)││(lg)││(fc)│ │Allocator │ │Allocator │
└──────┘ └──────┘ └────┘└────┘└────┘ └──────────┘ └──────────┘
  (Win)   (Xbox)                       (Win Pool)   (Win SegList)

           ┌──────────────────────────────────┐
           │  FD3D12FastAllocator             │
           │  (per Context, Upload Heap)       │
           │  → 一時的なUploadメモリ             │
           └──────────────────────────────────┘

           ┌──────────────────────────────────┐
           │  FD3D12TransientResource         │
           │  HeapAllocator                   │
           │  → フレーム内一時リソース            │
           └──────────────────────────────────┘
```

### EResourceAllocationStrategy — アロケーション戦略

| 戦略                   | 説明                                                    |
|-----------------------|--------------------------------------------------------|
| `kPlacedResource`     | ID3D12Heap上にPlaced Resource作成。リソース個別ステート管理可能。最小64KB。 |
| `kManualSubAllocation`| 単一大リソース内の手動サブアロケーション。1バイト粒度。読み取り専用バッファ向き。 |

### FD3D12ResourceInitConfig — アロケータ設定

| フィールド              | 型                       | 説明                        |
|-----------------------|-------------------------|----------------------------|
| `HeapType`            | `D3D12_HEAP_TYPE`       | UPLOAD / DEFAULT / READBACK |
| `HeapFlags`           | `D3D12_HEAP_FLAGS`      | ALLOW_ONLY_BUFFERS等        |
| `ResourceFlags`       | `D3D12_RESOURCE_FLAGS`  | UAV / RT / DS等             |
| `InitialD3D12Access`  | `ED3D12Access`          | リソース初期ステート           |

### 定数

```
MIN_PLACED_RESOURCE_SIZE = 64 * 1024   (64KB)
D3D_BUFFER_ALIGNMENT     = 64 * 1024   (64KB)
kD3D12ManualSubAllocationAlignment = 256
```

---

## 2. Buddy Allocator

**クラス:** `FD3D12BuddyAllocator` (継承: `FD3D12ResourceAllocator`)

MSFTリファレンス実装に基づくバディアロケーション。固定範囲からの任意サイズブロック割り当てを効率的に行う。解放時に隣接バディとのマージを試行し、フラグメンテーションを抑制。

### アルゴリズム

```
MaxBlockSize (例: 2MB)
├── Order = ceil(log2(UnitSize))
├── FreeBlocks[Order]: TArray<TSet<uint32>>  // オーダーごとの空きブロック
└── MaxOrder = log2(MaxBlockSize / MinBlockSize)

TryAllocate(Size, Alignment)
  │
  ├── SizeToUnitSize(Size) → MinBlockSize単位に切り上げ
  ├── UnitSizeToOrder() → ceil(log2(UnitSize))
  ├── AllocateBlock(Order)
  │   ├── FreeBlocks[Order]に空きあり → そのブロックを返す
  │   └── 無い → 上位Orderを分割 (再帰的)
  │       └── 親ブロック2分割 → 片方返却、片方FreeBlocksに追加
  └── ResourceLocation設定
      ├── PlacedResource: CreatePlacedResource on BackingHeap
      └── ManualSubAllocation: BackingResource内のオフセット設定

Deallocate(ResourceLocation)
  │
  ├── DeferredDeletionQueueにRetiredBlock追加
  └── CleanUpAllocations() → FrameFence経過後
      ├── DeallocateBlock(offset, order)
      │   ├── BuddyOffset = offset ^ size
      │   ├── バディが空き → マージ → 上位Orderへ昇格 (再帰)
      │   └── バディ使用中 → FreeBlocks[order]に追加
      └── PlacedResource: Release()
```

### メンバ変数

| メンバ                   | 型                              | 説明                              |
|------------------------|---------------------------------|----------------------------------|
| `MaxBlockSize`         | `uint32` (const)                | アロケータ管理最大サイズ              |
| `MinBlockSize`         | `uint32` (const)                | 最小ブロックサイズ (アライメント単位)   |
| `AllocationStrategy`   | `EResourceAllocationStrategy`   | Placed or ManualSubAllocation     |
| `BackingResource`      | `TRefCountPtr<FD3D12Resource>`  | ManualSubAllocation時のバッキング   |
| `BackingHeap`          | `TRefCountPtr<FD3D12Heap>`      | PlacedResource時のバッキング        |
| `FreeBlocks`           | `TArray<TSet<uint32>>`          | オーダーごとの空きブロックセット       |
| `DeferredDeletionQueue`| `TArray<RetiredBlock>`          | 遅延解放キュー                      |
| `MaxOrder`             | `uint32`                        | 最大オーダー                        |
| `TotalSizeUsed`        | `uint32`                        | 現在使用サイズ                      |

---

## 3. MultiBuddy Allocator

**クラス:** `FD3D12MultiBuddyAllocator` (継承: `FD3D12ResourceAllocator`)

単一Buddy Allocatorの欠点(固定サイズプール)を補うため、複数のBuddy Allocatorインスタンスを管理する上位アロケータ。

```
FD3D12MultiBuddyAllocator
├── Allocators: TArray<FD3D12BuddyAllocator*>
├── AllocationStrategy: EResourceAllocationStrategy
├── MinBlockSize: uint32
└── DefaultPoolSize: uint32

TryAllocate(Size, Alignment)
  │
  ├── 既存Allocatorsを順に試行
  │   └── BuddyAllocator.TryAllocate() 成功 → 返却
  │
  └── 全て失敗 → CreateNewAllocator(Size)
      ├── PoolSize = max(DefaultPoolSize, Size)
      └── new FD3D12BuddyAllocator(..., PoolSize, MinBlockSize)
```

### CleanUpAllocations

FrameLag経過後、空になったBuddy Allocatorを解放:
- `IsEmpty()`: 最大Orderに単一空きブロック = 全領域空き
- `GetLastUsedFrameFence()` + `FrameLag` ≤ 現在Fence → 安全に解放

---

## 4. Bucket Allocator

**クラス:** `FD3D12BucketAllocator` (継承: `FD3D12ResourceAllocator`)

サイズバケットベースのアロケータ。リソースを2の累乗サイズのバケットに分類し、プール再利用。

### パラメータ

| 定数              | 値          | 説明                        |
|------------------|-------------|----------------------------|
| `BucketShift`    | 6           | 最小バケット = 2^6 = 64バイト  |
| `NumBuckets`     | 22          | 64B ~ 2^28 (256MB)          |
| `MIN_HEAP_SIZE`  | 256KB (サブアロケーション有効時) | サブアロケーション最小ヒープサイズ |
| `BlockRetentionFrameCount` | コンストラクタ指定 | ブロック保持フレーム数 |

### アルゴリズム

```
BucketFromSize(size) = ceil(log2(size)) - BucketShift

TryAllocate(Size, Alignment)
  ├── BucketIndex = BucketFromSize(Size)
  ├── BlockSize = RoundUpToPowerOfTwo(Size)
  ├── AvailableBlocks[BucketIndex]から取得
  │   └── 空き有り → 再利用
  └── 空き無し → 新規サブアロケーションリソース作成

Deallocate
  └── ExpiredBlocks キューに投入
      → CleanUpAllocations(FrameLag) → AvailableBlocksに戻す or 解放
```

---

## 5. Pool Allocator (FD3D12PoolAllocator)

**クラス:** `FD3D12PoolAllocator` (継承: `FRHIPoolAllocator`, `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`, `ID3D12ResourceAllocator`)

RHICore層の`FRHIPoolAllocator`をD3D12向けに実装。Free-Listベースのプールメモリ管理、デフラグメンテーション対応。Windows環境でのバッファ・テクスチャプーリングに使用。

### 主要メンバ

| メンバ                      | 型                                 | 説明                          |
|----------------------------|------------------------------------|------------------------------|
| `InitConfig`               | `FD3D12ResourceInitConfig`         | プール設定                     |
| `AllocationStrategy`       | `EResourceAllocationStrategy`      | Placed or SubAllocation       |
| `FrameFencedOperations`    | `TArray<FrameFencedAllocationData>`| フレーム同期操作キュー           |
| `PendingDeleteRequestSize` | `uint64`                           | 保留中解放要求サイズ             |
| `PendingCopyOps`           | `TArray<FD3D12VRAMCopyOperation>`  | 保留中VRAMコピー操作            |
| `AllocationDataPool`       | `TArray<FRHIPoolAllocationData*>`  | アロケーションデータプール        |

### FD3D12MemoryPool — プール単体

`FRHIMemoryPool`をD3D12向けに実装。各プールは一つの`FD3D12Heap`(Placed)または`FD3D12Resource`(SubAllocation)をバッキングとして持つ。

| メンバ               | 説明                              |
|---------------------|----------------------------------|
| `BackingResource`   | ManualSubAllocation時のバッキング   |
| `BackingHeap`       | PlacedResource時のバッキング        |
| `LastUsedFrameFence`| 最終使用フレームフェンス              |

### FrameFencedAllocationData — 遅延操作

| EOperation     | 説明                              |
|---------------|----------------------------------|
| `Invalid`     | 無効値 (デフォルト)                  |
| `Deallocate`  | フレームフェンス到達後に解放           |
| `Unlock`      | フレームフェンス到達後にロック解除       |
| `Nop`         | 何もしない (キャンセル済み)           |

### AllocDefaultResource フロー

```
AllocDefaultResource(HeapType, Desc, Usage, StateMode, CreateAccess, ...)
  │
  ├── サイズ > MaxAllocationSize → 直接 Committed Resource 作成
  │   └── Adapter->CreateCommittedResource() → AsStandAlone
  │
  └── サイズ ≤ MaxAllocationSize → プールからアロケート
      ├── FRHIPoolAllocator::Allocate() → Free-Listから空き検索
      │   ├── 適切なプール内に空きあり → 返却
      │   └── 無い → CreateNewPool() → FD3D12MemoryPool作成
      │       ├── PlacedResource: ID3D12Heap作成
      │       └── SubAllocation: ID3D12Resource作成
      │
      ├── PlacedResource: CreatePlacedResource on Pool.BackingHeap
      └── SubAllocation: Offset/GPUVirtualAddress設定
```

### デフラグメンテーション

`HandleDefragRequest()`でVRAMコピー操作を生成:
1. ソースブロックの現在リソースを取得
2. ターゲットブロック位置に新規Placed Resource作成 or Offset更新
3. `FD3D12VRAMCopyOperation`をPendingCopyOpsに追加
4. `FlushPendingCopyOps()`でCommandContextに発行

---

## 6. Segregated Free-List Allocator (FD3D12SegListAllocator)

**クラス:** `FD3D12SegListAllocator` (継承: `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`)

サイズごとにヒープを分離管理するセグメントリストアロケータ。主に読み取り専用テクスチャのPlaced Resource作成に使用。Committed Resource作成コストを~420μsから~72μsに削減。

### 3層構造

```
FD3D12SegListAllocator
├── SegLists: TMap<uint32, FD3D12SegList*>  // ブロックサイズ → SegList
├── DeferredDeletionQueue: TArray<TArray<FRetiredBlock>>
├── FenceValues: TArray<uint64>
└── パラメータ: MinPoolSize, MinNumToPool, MaxPoolSize

FD3D12SegList (ブロックサイズ固定)
├── BlockSize: uint32     // 全ブロック同一サイズ
├── HeapSize: uint32      // = NumPooled × BlockSize
└── FreeHeaps: TArray<TRefCountPtr<FD3D12SegHeap>>

FD3D12SegHeap (FD3D12Heap派生)
├── FreeBlockOffsets: TArray<uint32>  // 空きブロックオフセット
├── FirstFreeOffset: uint32           // 未使用領域先頭
├── OwnerList: FD3D12SegList*
└── ArrayIdx: int32                   // FreeHeaps内インデックス
```

### アロケーションフロー

```
Allocate(SizeInBytes, Alignment)
  │
  ├── BlockSize = Align(SizeInBytes, Alignment)
  ├── ShouldPool(BlockSize) → BlockSize * 2 ≤ MaxPoolSize
  │   └── false → InvalidOffset 返却 (呼び出し元でCommitted作成)
  │
  ├── SegList検索 (RWLock: ReadOnly)
  │   └── 無い → 新規作成 (Write Lock)
  │       └── HeapSize = max(MinPoolSize, MinNumToPool × BlockSize)
  │           constrained by MaxPoolSize
  │
  └── SegList->AllocateBlock(Device, VisibleNodeMask, HeapType, HeapFlags)
      ├── FreeHeaps非空 → 最後のSegHeapから割当
      │   ├── FreeBlockOffsets.Pop() or FirstFreeOffset += BlockSize
      │   └── ヒープ満杯 → FreeHeapsから除去
      └── FreeHeaps空 → 新規SegHeap作成 (ID3D12Heap)
          └── BeginTrackingResidency
```

### 解放フロー

```
Deallocate(PlacedResource, Offset, SizeInBytes)
  │
  └── DeferredDeletionQueue + FenceValue追加

CleanUpAllocations()
  │
  ├── FenceValue経過チェック
  └── FreeRetiredBlocks()
      └── SegHeap->FreeBlock(Offset)
          ├── ヒープ満杯だった → FreeHeapsに復帰
          └── ヒープ空になった → FreeHeapsから除去 (自動解放)
```

### サイズクラス

`sizeof(FD3D12SegList) ≤ 64` (1キャッシュライン制約、TRACK_WASTAGE無効時)

---

## 7. Upload Heap Allocator

**クラス:** `FD3D12UploadHeapAllocator` (継承: `FD3D12AdapterChild`, `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`)

スクラッチメモリ(一時ステージングバッファ、動的リソースシャドウバッファ)のアロケーション。GPU単位で1インスタンス。

### 内部構成

```
FD3D12UploadHeapAllocator
├── SmallBlockAllocator: FD3D12MultiBuddyAllocator
│     小サイズアロケーション用。2の累乗アライメント。高速。
│
├── BigBlockAllocator: FD3D12PoolAllocator (+ BigBlockCS)
│     大サイズアロケーション用。アライメント浪費が少ない。
│
└── FastConstantPageAllocator: FD3D12MultiBuddyAllocator
      定数バッファページ専用。同一サイズアロケーションの繰り返し。
      フラグメンテーション回避のため独立。
```

### API

| メソッド                         | 説明                                        |
|---------------------------------|---------------------------------------------|
| `AllocUploadResource()`        | 汎用アップロードメモリ割当 (Small/Big分岐)       |
| `AllocFastConstantAllocationPage()` | 定数バッファページ割当 (FastConstantPageAllocator) |
| `CleanUpAllocations(FrameLag)` | 期限切れアロケーション解放                       |
| `UpdateMemoryStats()`          | 統計更新                                      |

---

## 8. Default Buffer Allocator

**クラス:** `FD3D12DefaultBufferAllocator` (継承: `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`)

Defaultヒープ(GPU専用)バッファのアロケーションを管理。内部で複数の`FD3D12BufferPool`を保持し、HeapType/ResourceFlags/BufferUsage/StateModeの組み合わせごとにプールを作成。

### FD3D12BufferPool — プラットフォーム別typedef

```cpp
#if USE_BUFFER_POOL_ALLOCATOR  // Windows
    typedef FD3D12PoolAllocator FD3D12BufferPool;
#else                          // Xbox等
    typedef FD3D12DefaultBufferPool FD3D12BufferPool;
#endif
```

### FD3D12DefaultBufferPool (非Pool版)

`FD3D12MultiBuddyAllocator`をラップし、`SupportsAllocation()`でHeapType/Flags/Usageの適合判定。

### AllocDefaultResource フロー

```
FD3D12DefaultBufferAllocator::AllocDefaultResource(HeapType, Desc, Usage, ...)
  │
  ├── 既存DefaultBufferPoolsを走査
  │   └── SupportsAllocation() 一致 → Pool.AllocDefaultResource()
  │
  └── 一致なし → CreateBufferPool()
      └── new FD3D12BufferPool(Device, InitConfig, Strategy, ...)
```

### IsPlacedResource 判定

`FD3D12BufferPool::GetResourceAllocationStrategy()` に委譲する:

```
static bool IsPlacedResource(ResourceFlags, ResourceStateMode, Alignment)
  └── GetResourceAllocationStrategy(Flags, StateMode, Alignment) == kPlacedResource

GetResourceAllocationStrategy (FD3D12PoolAllocator版):
  │
  ├── Alignment > kD3D12ManualSubAllocationAlignment (256) → kPlacedResource
  │
  ├── ResourceStateMode == Default の場合:
  │   └── UAV フラグ → MultiState 扱い、それ以外 → SingleState 扱い
  │
  ├── ResourceStateMode == MultiState → kPlacedResource (個別ステート必要)
  └── その他 → kManualSubAllocation
```

### GetDefaultInitialD3D12Access

| HeapType     | 条件                     | 初期Access               |
|-------------|--------------------------|--------------------------|
| READBACK    | -                        | `CopyDest`               |
| UPLOAD      | -                        | `GenericRead`            |
| DEFAULT     | BUF_UnorderedAccess && SingleState | `UAVMask`        |
| DEFAULT     | AccelerationStructure (`#if D3D12_RHI_RAYTRACING`) | `BVHRead \| BVHWrite` |
| DEFAULT     | その他                    | `GenericRead`            |

---

## 9. Texture Allocator Pool

**クラス:** `FD3D12TextureAllocatorPool` (継承: `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`)

テクスチャアロケーションの上位管理。コンパイル設定により3つの実装から選択:

### USE_TEXTURE_POOL_ALLOCATOR (Windows デフォルト)

```
FD3D12TextureAllocatorPool
└── PoolAllocators[4]: FD3D12PoolAllocator*
    ├── [ReadOnly4K]   // 4KBアライメント対応テクスチャ
    ├── [ReadOnly]     // 64KB標準テクスチャ
    ├── [RenderTarget] // RT/DS テクスチャ
    └── [UAV]          // UAV テクスチャ
```

### D3D12RHI_SEGREGATED_TEXTURE_ALLOC (レガシーWindows)

```
FD3D12TextureAllocatorPool
└── ReadOnlyTexturePool: FD3D12SegListAllocator
    // 読み取り専用テクスチャのみセグメントリストプール
    // RT/DS/UAV はCommitted Resource直接作成
```

### その他プラットフォーム (MultiBuddy)

```
FD3D12TextureAllocatorPool
└── ReadOnlyTexturePool: FD3D12TextureAllocator (FD3D12MultiBuddyAllocator派生)
    // 読み取り専用テクスチャのみバディプール
```

### AllocateTexture フロー

```
AllocateTexture(Desc, ClearValue, UEFormat, TextureLocation, ...)
  │
  ├── CanBe4KAligned() → ReadOnly4K プール選択
  ├── RT/DS → RenderTarget プール
  ├── UAV → UAV プール
  └── その他 → ReadOnly プール
      │
      └── PoolAllocator->AllocateResource()
          ├── サイズ判定
          │   ├── > MaxAllocationSize → Committed Resource 直接作成
          │   └── ≤ MaxAllocationSize → プールからアロケート
          └── TextureLocation設定
```

---

## 10. Fast Allocator (一時アロケーション)

### FD3D12FastAllocator

Uploadヒープからのページベース一時アロケータ。コマンドリスト実行時の一時データ(定数バッファ、テクスチャアップロード等)に使用。

```
FD3D12FastAllocator
├── PagePool: FD3D12FastAllocatorPagePool
│   ├── Pool: TArray<FD3D12FastAllocatorPage*>  // 再利用ページ
│   └── PageSize: uint32
├── CurrentAllocatorPage: FD3D12FastAllocatorPage*
└── CS: FCriticalSection

Allocate(Size, Alignment)
  │
  ├── CurrentAllocatorPage内に収まる → バンプアロケーション
  │   ├── NextFastAllocOffset += AlignedSize
  │   └── ResourceLocation.AsFastAllocation(Resource, Size, GPUBase, CPUBase, Offset)
  │
  └── 収まらない → 現在ページ返却 + 新規ページ要求
      ├── PagePool.ReturnFastAllocatorPage(CurrentAllocatorPage)
      └── CurrentAllocatorPage = PagePool.RequestFastAllocatorPage()
```

### FD3D12FastAllocatorPage

| メンバ                  | 説明                                |
|------------------------|-------------------------------------|
| `PageSize`             | ページサイズ (const)                  |
| `FastAllocBuffer`      | バッキングD3D12リソース (Upload Heap)  |
| `NextFastAllocOffset`  | 次のアロケーションオフセット             |
| `FastAllocData`        | Map済みCPUアドレス                    |
| `FrameFence`           | 最終使用フレームフェンス                |

### FD3D12FastConstantAllocator

コマンドコンテキスト専用の超軽量定数バッファアロケータ。UploadHeapAllocatorからページを取得し、256バイトアライメントでバンプアロケーション。

```
FD3D12FastConstantAllocator
├── UnderlyingResource: FD3D12ResourceLocation  // 現在ページ
├── Offset: uint32                               // 現在オフセット
└── PageSize: uint32

Allocate(Bytes, OutLocation, OutCBView)
  │
  ├── Offset + Bytes > PageSize → 新規ページ取得
  │   └── UploadHeapAllocator.AllocFastConstantAllocationPage()
  │
  ├── OutLocation.AsFastAllocation(...)
  ├── OutCBView.CreateView(GPUAddress + Offset, Size)
  └── Offset += Align(Bytes, 256)
```

---

## 11. Transient Resource Allocator

**クラス:** `FD3D12TransientResourceHeapAllocator` (継承: `FRHITransientResourceHeapAllocator`, `FD3D12AdapterChild`)

RDG (Render Dependency Graph) のトランジェントリソースアロケータ。フレーム内でのみ生存するリソースをヒープベースでエイリアシング管理。

### 構成

```
FD3D12TransientHeapCache (FRHITransientHeapCache, FD3D12AdapterChild)
├── Create(Adapter) → TUniquePtr<FD3D12TransientHeapCache>
└── CreateHeap(Initializer) → FD3D12TransientHeap*

FD3D12TransientHeap (FRHITransientHeap, FRefCountBase, FD3D12LinkedAdapterObject)
├── Heap: TRefCountPtr<FD3D12Heap>  // バッキングD3D12ヒープ
└── マルチGPU: LinkedAdapterObjectとして複数GPU対応

FD3D12TransientResourceHeapAllocator
├── SupportsResourceType: Buffer/Texture両対応
├── CreateTexture(CreateInfo, Name, Fences) → FRHITransientTexture*
├── CreateBuffer(CreateInfo, Name, Fences)  → FRHITransientBuffer*
└── FResourceAllocatorAdapter (内部クラス, ID3D12ResourceAllocator実装)
    ├── Heap: FD3D12TransientHeap&
    ├── Allocation: FRHITransientHeapAllocation&
    └── AllocateResource() → Placed Resource on TransientHeap
```

### リソース作成フロー

```
CreateTexture(CreateInfo, Name, Fences)
  │
  ├── FRHITransientHeapAllocator::AllocateTransientResource()
  │   └── ヒープからアロケーション取得 (エイリアシング対応)
  │
  ├── FResourceAllocatorAdapter 構築
  │   └── TransientHeap + Allocation + ResourceDesc
  │
  └── FD3D12DynamicRHI::CreateTextureInternal(CreateDesc, &ResourceAllocatorAdapter)
      └── ResourceAllocatorAdapter.AllocateResource()
          └── Adapter->CreatePlacedResource(TransientHeap->Heap, Allocation.Offset, ...)
              → ResourceLocation.AsHeapAliased(Resource)
```

---

## 12. コンパイル時選択とプラットフォーム分岐

| マクロ                                | デフォルト        | 説明                                |
|--------------------------------------|-----------------|-------------------------------------|
| `SUB_ALLOCATED_DEFAULT_ALLOCATIONS`  | 1               | デフォルトバッファのサブアロケーション有効  |
| `USE_BUFFER_POOL_ALLOCATOR`          | Windows: 1      | バッファにPoolAllocator使用            |
| `USE_TEXTURE_POOL_ALLOCATOR`         | Windows: 1      | テクスチャにPoolAllocator使用           |
| `D3D12RHI_SEGREGATED_TEXTURE_ALLOC`  | Windows: 1      | テクスチャSegList有効 (Pool無効時)       |
| `D3D12RHI_SEGLIST_ALLOC_TRACK_WASTAGE` | `!(UE_BUILD_TEST \|\| UE_BUILD_SHIPPING)` | SegListメモリ浪費トラッキング |
| `D3D12RHI_TRACK_DETAILED_STATS`      | `PLATFORM_WINDOWS && !(UE_BUILD_TEST \|\| UE_BUILD_SHIPPING)` | 詳細統計トラッキング |
| `MIN_PLACED_RESOURCE_SIZE`           | 64KB            | Placed Resource最小サイズ              |

### アロケータ選択チャート

```
バッファアロケーション:
  ├── Dynamic → UploadHeapAllocator.AllocUploadResource()
  ├── Static (Windows) → DefaultBufferAllocator → FD3D12PoolAllocator
  └── Static (Xbox等)  → DefaultBufferAllocator → FD3D12DefaultBufferPool (MultiBuddy)

テクスチャアロケーション:
  ├── Windows (USE_TEXTURE_POOL_ALLOCATOR)
  │   └── FD3D12TextureAllocatorPool → PoolAllocator[4種]
  ├── Windows (D3D12RHI_SEGREGATED_TEXTURE_ALLOC)
  │   └── FD3D12TextureAllocatorPool → SegListAllocator (ReadOnly)
  └── その他
      └── FD3D12TextureAllocatorPool → MultiBuddyAllocator

一時アロケーション:
  ├── コマンドコンテキスト → FD3D12FastAllocator (ページバンプ)
  ├── 定数バッファ → FD3D12FastConstantAllocator
  └── RDGトランジェント → FD3D12TransientResourceHeapAllocator (Placed on TransientHeap)
```

---

## 関連ファイル

| ファイル                                | 内容                                          |
|---------------------------------------|----------------------------------------------|
| `D3D12Allocation.h`                   | 全アロケータクラス定義                           |
| `D3D12Allocation.cpp`                 | Buddy/MultiBuddy/Bucket/SegList/FastAllocator実装 |
| `D3D12PoolAllocator.h`               | FD3D12PoolAllocator, FD3D12MemoryPool          |
| `D3D12PoolAllocator.cpp`             | Pool Allocator実装、デフラグ                     |
| `D3D12TransientResourceAllocator.h`   | TransientHeap/Cache/HeapAllocator               |
| `D3D12TransientResourceAllocator.cpp` | TransientResource作成実装                        |
