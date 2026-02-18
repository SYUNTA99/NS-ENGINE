# D3D12RHI Part 4: リソース管理 (Buffer / Texture / View)

## 目次
1. [リソースアーキテクチャ概要](#1-リソースアーキテクチャ概要)
2. [FD3D12Resource — D3D12リソースラッパー](#2-fd3d12resource--d3d12リソースラッパー)
3. [FD3D12Heap — ヒープ管理](#3-fd3d12heap--ヒープ管理)
4. [FD3D12ResourceLocation — サブアロケーション位置追跡](#4-fd3d12resourcelocation--サブアロケーション位置追跡)
5. [FD3D12Buffer — バッファリソース](#5-fd3d12buffer--バッファリソース)
6. [FD3D12Texture — テクスチャリソース](#6-fd3d12texture--テクスチャリソース)
7. [View階層 (SRV/UAV/RTV/DSV/CBV)](#7-view階層-srvuavrtvdsvcbv)
8. [リソース作成API (Committed/Placed/Reserved)](#8-リソース作成api-committedplacedreserved)
9. [CVar一覧](#9-cvar一覧)

---

## 1. リソースアーキテクチャ概要

D3D12RHIのリソース管理は、D3D12ネイティブリソースを薄くラップし、サブアロケーションとステートトラッキングを統合した3層構造で構成される。

```
┌──────────────────────────────────────────────────────────────────┐
│                    RHI パブリックインターフェース                    │
│  FRHIBuffer    FRHITexture    FRHIShaderResourceView    etc.     │
└──────────────┬───────────────┬────────────────────┬──────────────┘
               │               │                    │
┌──────────────▼───────────────▼────────────────────▼──────────────┐
│                    D3D12 RHI リソース層                            │
│  FD3D12Buffer   FD3D12Texture   FD3D12ShaderResourceView_RHI    │
│       │               │                    │                     │
│       ▼               ▼                    ▼                     │
│  FD3D12BaseShaderResource          FD3D12View                    │
│       │                                │                         │
│       ▼                                ▼                         │
│  FD3D12ResourceLocation      FD3D12OfflineDescriptor             │
│       │                                                          │
│       ▼                                                          │
│  FD3D12Resource  ←→  FD3D12Heap                                 │
│       │                    │                                     │
│       ▼                    ▼                                     │
│  ID3D12Resource      ID3D12Heap                                  │
└──────────────────────────────────────────────────────────────────┘
```

### リソースステートトラッキングモデル

```
┌─────────────────────────────────────────────────────────────────┐
│              ED3D12ResourceStateMode                             │
├─────────────┬───────────────────────────────────────────────────┤
│ Default     │ リソースフラグに基づき自動判定                       │
│             │ - !bWritable (DSV/RTV/UAV全無し): トラッキング無効  │
│             │ - bWritable: トラッキング有効                       │
├─────────────┼───────────────────────────────────────────────────┤
│ SingleState │ 常にInitialState固定、トラッキング強制無効           │
│             │ - AccelerationStructure等で使用                     │
├─────────────┼───────────────────────────────────────────────────┤
│ MultiState  │ トラッキング強制有効 (!bWritableでも無効化されない)  │
│             │ - Reserved Resource テクスチャ等で使用               │
└─────────────┴───────────────────────────────────────────────────┘
```

### TD3D12ResourceTraits — RHI型→D3D12具象型マッピング

| RHI型                          | D3D12具象型                              |
|-------------------------------|----------------------------------------|
| `FRHIBuffer`                  | `FD3D12Buffer`                         |
| `FRHITexture`                 | `FD3D12Texture`                        |
| `FRHIUniformBuffer`           | `FD3D12UniformBuffer`                  |
| `FRHIShaderResourceView`      | `FD3D12ShaderResourceView_RHI`         |
| `FRHIUnorderedAccessView`     | `FD3D12UnorderedAccessView_RHI`        |
| `FRHISamplerState`            | `FD3D12SamplerState`                   |
| `FRHIRasterizerState`         | `FD3D12RasterizerState`                |
| `FRHIDepthStencilState`       | `FD3D12DepthStencilState`              |
| `FRHIBlendState`              | `FD3D12BlendState`                     |
| `FRHIGraphicsPipelineState`   | `FD3D12GraphicsPipelineState`          |
| `FRHIComputePipelineState`    | `FD3D12ComputePipelineState`           |
| `FRHIWorkGraphPipelineState`  | `FD3D12WorkGraphPipelineState`         |
| `FRHIGPUFence`                | `FD3D12GPUFence`                       |
| `FRHIStagingBuffer`           | `FD3D12StagingBuffer`                  |
| `FRHIShaderBundle`            | `FD3D12ShaderBundle`                   |
| `FRHIRayTracingScene`         | `FD3D12RayTracingScene`                |
| `FRHIRayTracingGeometry`      | `FD3D12RayTracingGeometry`             |
| `FRHIRayTracingPipelineState` | `FD3D12RayTracingPipelineState`        |
| `FRHIShaderBindingTable`      | `FD3D12RayTracingShaderBindingTable`   |
| `FRHIRayTracingShader`        | `FD3D12RayTracingShader`               |

---

## 2. FD3D12Resource — D3D12リソースラッパー

`FD3D12Resource`はD3D12ネイティブ`ID3D12Resource`を包むコアクラスで、ステートトラッキング、レジデンシー管理、遅延削除を統合する。

**継承:** `FThreadSafeRefCountedObject`, `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`

### 主要メンバ変数

| メンバ                           | 型                            | 説明                                    |
|--------------------------------|-------------------------------|----------------------------------------|
| `Resource`                     | `TRefCountPtr<ID3D12Resource>`| ネイティブD3D12リソース                    |
| `Heap`                         | `TRefCountPtr<FD3D12Heap>`   | Placed Resource時のバッキングヒープ         |
| `Desc`                         | `FD3D12ResourceDesc` (const) | リソース記述子 (拡張版)                     |
| `InitialD3D12Access`           | `ED3D12Access`               | 初期アクセスステート                        |
| `DefaultD3D12Access`           | `ED3D12Access`               | デフォルトアクセスステート                    |
| `HeapType`                     | `D3D12_HEAP_TYPE`            | ヒープタイプ (DEFAULT/UPLOAD/READBACK)     |
| `GPUVirtualAddress`            | `D3D12_GPU_VIRTUAL_ADDRESS`  | GPUバーチャルアドレス                       |
| `ResourceBaseAddress`          | `void*`                      | Map時のCPUアドレス                         |
| `SubresourceCount`             | `uint16`                     | MipLevels × ArraySize × PlaneCount      |
| `PlaneCount`                   | `uint8`                      | プレーン数 (Depth/Stencilは2)              |
| `bRequiresResourceStateTracking` | `bool`                     | ステートトラッキング要否                    |
| `bRequiresResidencyTracking`   | `bool`                       | レジデンシートラッキング要否                 |
| `bDepthStencil`                | `bool`                       | Depth/Stencilリソースか                   |
| `bDeferDelete`                 | `bool`                       | 遅延削除対象か (デフォルトtrue)              |
| `NumMapCalls`                  | `int32`                      | Map参照カウント (ネストMap対応)              |
| `ReservedResourceData`         | `TUniquePtr<...>`            | Reserved Resource固有データ               |

### FD3D12ResourceDesc — 拡張リソース記述子

`D3D12_RESOURCE_DESC`を継承し、UE5固有のメタデータを追加。`TZeroedStruct<T>`テンプレートにより全パディングゼロ初期化(ハッシュ互換)。

| 追加フィールド                  | 型               | 説明                                      |
|------------------------------|-----------------|------------------------------------------|
| `PixelFormat`                | `EPixelFormat`  | UEピクセルフォーマット                       |
| `UAVPixelFormat`             | `EPixelFormat`  | UAVアクセス用代替フォーマット                 |
| `bRequires64BitAtomicSupport`| `bool`          | Intel 64bitアトミック拡張要否 (`#if D3D12RHI_NEEDS_VENDOR_EXTENSIONS`) |
| `bReservedResource`          | `bool`          | Reserved (Tiled) Resource フラグ           |
| `bBackBuffer`                | `bool`          | バックバッファか                            |
| `bExternal`                  | `bool`          | 外部所有リソースか                          |

### FD3D12ResourceTypeHelper — リソースタイプ自動判定

コンストラクタで`FD3D12ResourceDesc`と`D3D12_HEAP_TYPE`を受け取り、以下のフラグを自動設定:

```cpp
struct FD3D12ResourceTypeHelper {
    uint32 bSRV : 1;             // DENY_SHADER_RESOURCE が無い
    uint32 bDSV : 1;             // ALLOW_DEPTH_STENCIL
    uint32 bRTV : 1;             // ALLOW_RENDER_TARGET
    uint32 bUAV : 1;             // ALLOW_UNORDERED_ACCESS
    uint32 bWritable : 1;        // bDSV || bRTV || bUAV
    uint32 bSRVOnly : 1;         // bSRV && !bWritable
    uint32 bBuffer : 1;          // DIMENSION_BUFFER
    uint32 bReadBackResource : 1; // HEAP_TYPE_READBACK
};
```

`GetOptimalInitialD3D12Access(InD3D12Access)`は上記フラグに基づき最適な初期ステートを返す:

| 条件                                      | 返すステート                  |
|------------------------------------------|------------------------------|
| !SRVOnly && InD3D12Access != Unknown     | `InD3D12Access` (そのまま返す) |
| SRVOnly                                  | `ED3D12Access::SRVGraphics`  |
| Buffer && !UAV (READBACK)                | `ED3D12Access::CopyDest`     |
| Buffer && !UAV (DEFAULT)                 | `ED3D12Access::GenericRead`  |
| DSV (AccurateStates)                     | `ED3D12Access::DSVWrite`     |
| RTV (AccurateStates)                     | `ED3D12Access::RTV`          |
| UAV (AccurateStates)                     | `ED3D12Access::UAVMask`      |
| その他                                    | `ED3D12Access::Common`       |

※ 最初の分岐が最重要: 書き込み可能リソースに明示的な初期アクセスが指定されている場合、他の条件をスキップしてそのまま返す。

### Reserved Resource データ構造

```
FD3D12ReservedResourceData
├── BackingHeaps: TArray<TRefCountPtr<FD3D12Heap>>
│     バッキングヒープ配列 (最大HeapSize = d3d12.ReservedResourceHeapSizeMB)
├── ResidencyHandles: TArray<FD3D12ResidencyHandle*>
│     全ヒープのレジデンシーハンドル (フラット化)
├── NumResidencyHandlesPerHeap: TArray<int32>
│     ヒープごとのハンドル数
├── NumCommittedTiles: uint32
│     現在コミット済みタイル数
└── NumSlackTiles: uint32
      最後のヒープの未使用タイル数
```

### コンストラクタの初期化フロー

```
FD3D12Resource(Device, VisibleNodes, InResource, InitialAccess,
               ResourceStateMode, DefaultAccess, Desc, Heap, HeapType)
  │
  ├── PlaneCount = GetPlaneCount(Desc.Format)
  ├── bDeferDelete = true
  │
  ├── レジデンシートラッキング判定
  │   └── GPU専用 && !External && !BackBuffer → 有効
  │
  ├── GPUVirtualAddress取得
  │   └── Windowsではバッファのみ直接取得可能 (テクスチャは不可)
  │
  ├── InitializeResourceState()
  │   ├── SingleState: トラッキング無効、固定ステート
  │   ├── Default: DetermineResourceStates()で判定
  │   │   └── 非書き込み可能: トラッキング無効
  │   └── MultiState: トラッキング有効
  │
  ├── NV Aftermath ハンドル登録
  │
  └── bReservedResource → ReservedResourceData作成
```

### Map/Unmapメカニズム

`NumMapCalls`による参照カウント方式を採用。最初のMapで`ID3D12Resource::Map(0, ReadRange, &ResourceBaseAddress)`を呼び、参照カウントが0に戻った時のみ`Unmap(0, nullptr)`を発行。ネストされたMapを安全にサポート。

### CommitReservedResource — タイルマッピング

Reserved Resourceの物理メモリコミット/デコミットを行う。65536バイト(64KB)タイル単位で操作。

```
CommitReservedResource(D3DCommandQueue, RequiredCommitSizeInBytes)
  │
  ├── GetResourceTiling() で全タイル情報取得
  │   ├── Standard Mip: 3Dタイル座標 (WidthInTiles × HeightInTiles × DepthInTiles)
  │   └── Packed Mip: パックされた末尾ミップチェーン
  │
  ├── NumCommittedTiles > RequiredTiles → デコミット (縮小)
  │   ├── 逆順でヒープ走査
  │   ├── UpdateTileMappings(D3D12_TILE_RANGE_FLAG_NULL)
  │   └── 完全未使用ヒープはDeferDelete
  │
  ├── NumCommittedTiles < RequiredTiles → コミット (拡大)
  │   ├── NumSlackTiles > 0: 既存ヒープの空きタイル消費
  │   ├── 空き無し: 新規ヒープ作成 (サイズ = d3d12.ReservedResourceHeapSizeMB)
  │   │   ├── ALLOW_ONLY_BUFFERS / ALLOW_ONLY_NON_RT_DS_TEXTURES / ALLOW_ONLY_RT_DS_TEXTURES
  │   │   ├── BeginTrackingResidency
  │   │   └── High Priority (RT/DS/UAV時)
  │   └── UpdateTileMappings(D3D12_TILE_RANGE_FLAG_NONE, Heap, HeapOffset)
  │
  └── Residency MakeResident + SignalFence
```

---

## 3. FD3D12Heap — ヒープ管理

`FD3D12Heap`は`ID3D12Heap`のライフサイクルとレジデンシートラッキングを管理するラッパー。

**継承:** `FThreadSafeRefCountedObject`, `FD3D12DeviceChild`, `FD3D12MultiNodeGPUObject`

### 主要メンバ変数

| メンバ                       | 型                             | 説明                           |
|-----------------------------|-------------------------------|-------------------------------|
| `Heap`                      | `TRefCountPtr<ID3D12Heap>`    | ネイティブD3D12ヒープ            |
| `HeapName`                  | `FName`                       | デバッグ名                      |
| `HeapDesc`                  | `D3D12_HEAP_DESC`             | ヒープ記述子                     |
| `GPUVirtualAddress`         | `D3D12_GPU_VIRTUAL_ADDRESS`   | ヒープのGPUベースアドレス          |
| `ResidencyHandle`           | `FD3D12ResidencyHandle*`      | レジデンシーハンドル              |
| `TraceHeapId`               | `HeapId`                      | メモリトレース用ヒープID          |
| `TraceParentHeapId`         | `HeapId`                      | 親ヒープのトレースID              |
| `bTrack`                    | `bool`                        | アロケーション追跡有効フラグ (デフォルトtrue) |
| `bIsTransient`              | `bool`                        | トランジェントヒープフラグ         |
| `bRequiresResidencyTracking`| `bool`                        | レジデンシートラッキング要否       |

### SetHeap — GPUVirtualAddress取得テクニック

`ID3D12Heap`は直接GPUバーチャルアドレスを公開しないため、`SetHeap()`ではヒープ上にダミーバッファPlaced Resourceを一時的に作成し、そのGPUバーチャルアドレスを取得する:

```cpp
void FD3D12Heap::SetHeap(ID3D12Heap* HeapIn, ...)
{
    // アロケーショントラッキング時のみ実行
    if (Adapter->IsTrackingAllAllocations()
        && !(HeapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS)
        && HeapDesc.Properties.Type == D3D12_HEAP_TYPE_DEFAULT)
    {
        TRefCountPtr<ID3D12Resource> TempResource;
        D3D12_RESOURCE_DESC BufDesc = CD3DX12_RESOURCE_DESC::Buffer(HeapSize);
        D3DDevice->CreatePlacedResource(Heap, 0, &BufDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&TempResource));
        GPUVirtualAddress = TempResource->GetGPUVirtualAddress();
        // TempResource は即破棄 (アドレスのみ取得)
    }
}
```

### DeferDelete

`AddRef()`で参照カウント増加後、`FD3D12DynamicRHI::DeferredDelete(this)`で遅延削除キューに投入。遅延削除キューでの最終`Release()`でデストラクタが発火。

---

## 4. FD3D12ResourceLocation — サブアロケーション位置追跡

`FD3D12ResourceLocation`は「どのD3D12リソースのどのオフセットにどのサイズのアロケーションがあるか」を軽量に追跡するクラス。キャッシュフレンドリーな設計。

**継承:** `FRHIPoolResource`, `FD3D12DeviceChild`, `FNoncopyable`

### ResourceLocationType — アロケーション種別

| 列挙値                     | 説明                                             |
|--------------------------|--------------------------------------------------|
| `eUndefined`             | 未初期化                                          |
| `eStandAlone`            | 専用Committed Resource (1リソース = 1アロケーション)  |
| `eSubAllocation`         | 大きなリソース内のサブアロケーション                    |
| `eFastAllocation`        | Upload用一時アロケーション (揮発性、参照カウント無し)    |
| `eMultiFrameFastAllocation` | 複数フレーム持続するFastAllocation (参照カウント有り)  |
| `eAliased`               | XR HMD用エイリアス (参照カウント有り)                 |
| `eNodeReference`         | マルチGPU ノード参照 (参照カウント有り)                |
| `eHeapAliased`           | ヒープエイリアス                                    |

### EAllocatorType — アロケータ種別

| 列挙値       | アロケータクラス              | 用途                     |
|------------|--------------------------|--------------------------|
| `AT_Default` | `FD3D12BuddyAllocator`  | バディアロケーション        |
| `AT_SegList` | `FD3D12SegListAllocator` | セグメントリスト           |
| `AT_Pool`    | `FD3D12PoolAllocator`    | プールアロケーション        |
| `AT_Unknown` | (なし)                   | 未設定                    |

### メモリレイアウト

```
FD3D12ResourceLocation (キャッシュフレンドリー)
├── Owner: FD3D12BaseShaderResource*     // 所有者 (Buffer/Texture)
├── UnderlyingResource: FD3D12Resource*  // バッキングリソース
├── union Allocator                      // アロケータポインタ (3種共用体)
│   ├── FD3D12BaseAllocatorType*    (AT_Default)
│   ├── FD3D12SegListAllocator*     (AT_SegList)
│   └── FD3D12PoolAllocator*        (AT_Pool)
├── union AllocatorData                  // アロケータ固有データ (共用体)
│   ├── BuddyAllocatorPrivateData   {Offset, Order}
│   ├── BlockAllocatorPrivateData   {FrameFence, BucketIndex, Offset, ResourceHeap}
│   ├── SegListAllocatorPrivateData {Offset}
│   └── PoolAllocatorPrivateData    {FRHIPoolAllocationData}
├── MappedBaseAddress: void*             // CPU可視アドレス
├── GPUVirtualAddress: D3D12_GPU_VIRTUAL_ADDRESS
├── OffsetFromBaseOfResource: uint64     // リソース先頭からのオフセット
├── Size: uint64                         // アプリケーション要求サイズ
├── Type: ResourceLocationType           // 種別
├── AllocatorType: EAllocatorType        // アロケータ種別
└── bTransient: bool                     // トランジェントフラグ
```

### 所有権転送: TransferOwnership

```
TransferOwnership(Destination, Source)
  │
  ├── Owner保存 (Dst/Src)
  ├── Destination.Clear()
  ├── Memmove(&Destination, &Source, sizeof(FD3D12ResourceLocation))
  ├── AT_Pool: PoolAllocator->TransferOwnership(Source, Destination)
  ├── Source.InternalClear<false>()  // リソース解放無し
  └── Owner復元
```

### ReleaseResource — 種別ごとの解放処理

| 種別                  | 解放処理                                           |
|----------------------|---------------------------------------------------|
| `eStandAlone`        | 参照カウント確認 → DeferDelete or Release            |
| `eSubAllocation`     | AT_SegList: `SegListAllocator->Deallocate()`       |
|                      | AT_Pool: エイリアス解除 → `PoolAllocator->DeallocateResource()` |
|                      | AT_Default: `Allocator->Deallocate()`              |
| `eNodeReference`     | エイリアス除去 → DeferDelete or Release              |
| `eAliased`           | エイリアス除去 → DeferDelete or Release              |
| `eHeapAliased`       | DeferDelete or Release                             |
| `eFastAllocation`    | 何もしない (揮発性アロケーション)                      |
| `eUndefined`         | 何もしない                                          |

### AsStandAlone — スタンドアロン初期化

```cpp
void AsStandAlone(FD3D12Resource* Resource, uint64 InSize,
                  bool bInIsTransient, const D3D12_HEAP_PROPERTIES* CustomHeapProperties)
{
    SetType(eStandAlone);
    SetResource(Resource);
    SetSize(InSize);
    if (IsCPUAccessible(HeapType, CustomHeapProperties))
        SetMappedBaseAddress(Resource->Map(&range));
    SetGPUVirtualAddress(Resource->GetGPUVirtualAddress());
    SetTransient(bInIsTransient);
    UpdateStandAloneStats(true);
}
```

### OnAllocationMoved — デフラグ対応

Pool Allocatorのデフラグレーション時にコールバックされる。アロケーション戦略に応じて処理が分岐:

- **ManualSubAllocation:** GPUVirtualAddress/Offset/UnderlyingResourceの更新のみ
- **PlacedResource:** 新規Placed Resource作成 → CopyDest or DefaultAccess初期化

エイリアスチェーン全体の更新と、`Owner->ResourceRenamed()`によるView再構築を実行。

---

## 5. FD3D12Buffer — バッファリソース

**継承:** `FRHIBuffer`, `FD3D12BaseShaderResource`, `FD3D12LinkedAdapterObject<FD3D12Buffer>`

### FD3D12BaseShaderResource — シェーダーリソース基底

```
FD3D12BaseShaderResource (FD3D12DeviceChild, IRefCountedObject)
├── ResourceLocation: FD3D12ResourceLocation    // リソース位置
├── RenameListeners: TArray<FD3D12ShaderResourceRenameListener*>
│     └── View等がリスン → リソースリネーム時にView再構築
└── RenameListenersCS: FCriticalSection          // スレッドセーフ
```

`ResourceRenamed(Contexts)`を呼ぶと、全リスナーの`ResourceRenamed()`がコールバックされ、SRV/UAV/CBVが自動更新される。

### バッファ作成フロー

```
RHICreateBufferInitializer(RHICmdList, CreateDesc)
  │
  ├── Null Buffer → 空の FD3D12Buffer 作成のみ
  │
  ├── CreateBufferInternal(CreateDesc, bHasInitialData, ResourceAllocator)
  │   │
  │   ├── GetResourceDescAndAlignment()
  │   │   ├── Width = Align(Size, RHI_RAW_VIEW_ALIGNMENT=16)
  │   │   ├── D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS (UAV)
  │   │   ├── D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE (!SRV && !AccStruct)
  │   │   ├── D3D12RHI_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER (DrawIndirect)
  │   │   ├── D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS (Shared)
  │   │   ├── D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE (RT)
  │   │   └── Alignment:
  │   │       ├── ReservedResource: TileSizeInBytes (64KB)
  │   │       └── Structured: LCM(Stride, 16), otherwise: 16
  │   │
  │   ├── StateMode判定
  │   │   ├── AccelerationStructure → SingleState
  │   │   └── その他 → Default
  │   │
  │   ├── DesiredD3D12Access / CreatED3D12Access 計算
  │   │
  │   └── Adapter.CreateRHIBuffer()
  │       │
  │       ├── CreateLinkedObject<FD3D12Buffer>(GPUMask, Lambda)
  │       │   ├── new FD3D12Buffer(Device, CreateDesc)
  │       │   ├── Dynamic: AllocUploadResource (Upload Heap)
  │       │   ├── Static + ResourceAllocator: AllocateResource
  │       │   ├── Static + Default: AllocDefaultResource
  │       │   └── MultiGPU: 先頭GPU以外は ReferenceNode
  │       │
  │       └── UpdateBufferStats
  │
  └── 初期データ処理
      ├── Dynamic: MappedBaseAddress に直接コピー
      ├── ResourceArray: Upload Heap → CopyBufferRegion
      ├── Zeroed: Upload Heap → FillWithValue(0)
      └── Initializer: Upload Heap → カスタム初期化
```

### AllocateBuffer — アロケーション分岐

```
FD3D12Adapter::AllocateBuffer(Device, InDesc, Size, Usage, ...)
  │
  ├── Dynamic (AnyDynamic)
  │   └── GetUploadHeapAllocator(GPU).AllocUploadResource(Size, Alignment)
  │       → Upload Heap サブアロケーション
  │
  ├── Custom ResourceAllocator
  │   └── ResourceAllocator->AllocateResource(GPU, DEFAULT, ...)
  │
  └── Default
      └── Device->GetDefaultBufferAllocator().AllocDefaultResource(DEFAULT, ...)
          → Buddy/Pool Allocator or Committed Resource
```

### Lock/Unlock メカニズム

| バッファ種別     | LockMode         | 処理                                        |
|---------------|------------------|---------------------------------------------|
| Dynamic       | WriteOnly        | 初回: 直接 MappedBaseAddress 返却             |
|               |                  | 2回目以降: 新規Upload割当 → RenameLDAChain    |
|               | WriteOnly_NoOverwrite | 常に直接 MappedBaseAddress 返却          |
| Static        | ReadOnly         | Staging Buffer(READBACK) → GPU→CPU コピー     |
|               |                  | → SubmitAndBlockUntilGPUIdle (同期)           |
| Static        | WriteOnly        | Upload Heap割当 → Unlock時にCopyBufferRegion  |

### UploadResourceDataViaCopyQueue

コピーキュー経由の非同期アップロード:

```
UploadResourceDataViaCopyQueue(OwningContext, InResourceArray)
  │
  ├── FastAllocator でUploadメモリ確保
  ├── Memcpy(InResourceArray → UploadMemory)
  ├── FD3D12CopyScope 作成 (Copy Queue)
  │   ├── CopyBufferRegion(Dest=ResourceLocation, Src=UploadMemory)
  │   └── UpdateResidency
  ├── SyncPoint 返却 (呼び出し元で待機可能)
  └── InResourceArray->Discard()
```

### Rename / RenameLDAChain

Dynamic BufferのLock時、新しいUpload Heapアロケーションに切り替える:

- `Rename()`: `TransferOwnership(ResourceLocation, NewLocation)` + `ResourceRenamed()`
- `RenameLDAChain()`: Head GPUをRename後、残りのGPUは`ReferenceNode`で参照更新

---

## 6. FD3D12Texture — テクスチャリソース

**継承:** `FRHITexture`, `FD3D12BaseShaderResource`, `FD3D12LinkedAdapterObject<FD3D12Texture>`

### 主要メンバ変数

| メンバ                          | 型                                              | 説明                         |
|-------------------------------|------------------------------------------------|------------------------------|
| `ShaderResourceView`          | `TSharedPtr<FD3D12ShaderResourceView>`          | テクスチャSRV                  |
| `RenderTargetViews`           | `TArray<TSharedPtr<FD3D12RenderTargetView>, TInlineAllocator<1>>` | RTV配列 (Mip/Slice単位) |
| `DepthStencilViews`           | `TSharedPtr<FD3D12DepthStencilView>[MaxIndex]` | DSV配列 (Depth排他アクセス別)   |
| `LockedMap`                   | `TMap<uint32, TUniquePtr<FD3D12LockedResource>>`| サブリソースごとのロックデータ   |
| `FirstSubresourceFootprint`   | `TUniquePtr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>`| キャッシュ済みフットプリント     |
| `AliasingSourceTexture`       | `FTextureRHIRef`                                | エイリアス元テクスチャ          |
| `bCreatedRTVsPerSlice`        | `bool`                                          | スライス独立RTV作成フラグ       |
| `RTVArraySizePerMip`          | `int32`                                         | Mipあたりのスライス数           |

### テクスチャ作成フロー

```
CreateTextureInternal(CreateDesc, ResourceAllocator)
  │
  ├── GetResourceDesc(CreateDesc) → FD3D12ResourceDesc 構築
  │   ├── PlatformResourceFormat (DXGI_FORMAT変換)
  │   ├── 2D: CD3DX12_RESOURCE_DESC::Tex2D(Format, Width, Height, ArraySize, Mips, MSAA)
  │   ├── 3D: CD3DX12_RESOURCE_DESC::Tex3D(Format, Width, Height, Depth, Mips)
  │   ├── フラグ設定: RT/DS/UAV/Shared/DenyShaderResource
  │   ├── ReservedResource → Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
  │   ├── Block Compressed + UAV → UAVPixelFormat 設定
  │   └── 4KAlignment 判定 (小テクスチャ最適化)
  │
  ├── FillClearValue() → D3D12_CLEAR_VALUE (RT/DS用)
  │
  ├── InitialD3D12Access 計算
  │   ├── UncompressedUAV + FormatAlias → Common (Enhanced Barriers互換)
  │   ├── 初期データあり → CopyDest
  │   └── その他 → GetOptimalInitialD3D12Access()
  │
  ├── CreateLinkedObject<FD3D12Texture>(GPUMask, Lambda)
  │   ├── ResourceAllocator指定: AllocateTexture()
  │   ├── CPUReadback: CreateBuffer(READBACK) → AsStandAlone
  │   ├── 3D ReservedResource: CreateReservedResource → AsStandAlone
  │   ├── 3D Default: TextureAllocator.AllocateTexture()
  │   ├── 2D: SafeCreateTexture2D()
  │   │   ├── READBACK → CreateBuffer → AsStandAlone
  │   │   ├── ReservedResource → CreateReservedResource → AsStandAlone
  │   │   │   └── ImmediateCommit → CommitReservedResource(UINT64_MAX)
  │   │   └── Default → TextureAllocator.AllocateTexture()
  │   ├── UAV Alias Workaround (非UncompressedUAV環境)
  │   └── CreateViews(FirstLinkedObject)
  │
  └── D3D12TextureAllocated() → 統計更新
```

### 4KAlignment最適化

`CanBe4KAligned()`は小テクスチャに対し`D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT`(4KB)を使用可能か判定:

- Reserved Resourceは不可
- NV12/P010 ビデオフォーマットは不可
- RT/DSフラグ付きは不可
- MSAA > 1 は不可
- 16個の4Kブロック (= 64KBページ) に収まること

### FD3D12BackBufferReferenceTexture2D

`D3D12RHI_USE_DUMMY_BACKBUFFER`有効時に使用されるバックバッファプロキシクラス。`GetTextureBaseRHI()`で実際のバックバッファテクスチャを動的に返す。`FD3D12Viewport`への参照を保持。

---

## 7. View階層 (SRV/UAV/RTV/DSV/CBV)

### View基底クラス階層

```
FD3D12View (FD3D12DeviceChild, FD3D12ShaderResourceRenameListener)
│   ├── ResourceInfo: FResourceInfo  // リソース参照
│   ├── ViewSubset: FD3D12ViewSubset // サブリソース範囲
│   ├── OfflineCpuHandle: FD3D12OfflineDescriptor
│   ├── BindlessHandle: FRHIDescriptorHandle  (PLATFORM_SUPPORTS_BINDLESS_RENDERING)
│   └── HeapType: ERHIDescriptorHeapType
│
├── TD3D12View<TParent, TDesc>  // CRTP テンプレート
│       ├── D3DViewDesc: TDesc  // D3D12ネイティブView記述子
│       └── CreateView() / UpdateView() / ResourceRenamed()
│
├── FD3D12ConstantBufferView    (TD3D12View<..., D3D12_CONSTANT_BUFFER_VIEW_DESC>)
├── FD3D12ShaderResourceView    (TD3D12View<..., D3D12_SHADER_RESOURCE_VIEW_DESC>)
├── FD3D12UnorderedAccessView   (TD3D12View<..., D3D12_UNORDERED_ACCESS_VIEW_DESC>)
├── FD3D12RenderTargetView      (TD3D12View<..., D3D12_RENDER_TARGET_VIEW_DESC>)
└── FD3D12DepthStencilView      (TD3D12View<..., D3D12_DEPTH_STENCIL_VIEW_DESC>)
```

### ED3D12ViewType

| 値                | ヒープタイプ                           |
|-------------------|---------------------------------------|
| `ShaderResource`  | `ERHIDescriptorHeapType::Standard`    |
| `ConstantBuffer`  | `ERHIDescriptorHeapType::Standard`    |
| `UnorderedAccess` | `ERHIDescriptorHeapType::Standard`    |
| `RenderTarget`    | `ERHIDescriptorHeapType::RenderTarget`|
| `DepthStencil`    | `ERHIDescriptorHeapType::DepthStencil`|

### FD3D12ViewRange — サブリソース範囲

ミップ・配列スライス・プレーンの3次元範囲を表現:

```
FD3D12ViewRange
├── Array: FRHIRange16  {First, Num}
├── Plane: FRHIRange8   {First, Num}
└── Mip:   FRHIRange8   {First, Num}
```

各D3D12 View記述子(`D3D12_SHADER_RESOURCE_VIEW_DESC`等)から自動構築。`D3D12_SRV_DIMENSION_TEXTURECUBEARRAY`の場合、`Array = {First2DArrayFace, NumCubes * 6}`に展開される。

### FD3D12ViewSubset — サブリソースイテレータ

`FD3D12ResourceLayout`(リソース全体の配列数・プレーン数・ミップ数)と`FD3D12ViewRange`のペア。`D3D12CalcSubresource()`に基づくサブリソースインデックスを range-based for で列挙可能:

```cpp
for (uint32 SubresourceIndex : ViewSubset)
{
    // 各サブリソースに対する処理
}
```

### FD3D12DefaultViews — Nullディスクリプタ

| メンバ          | 説明                    |
|----------------|------------------------|
| `NullSRV`      | 未バインドSRV用          |
| `NullRTV`      | 未バインドRTV用          |
| `NullUAV`      | 未バインドUAV用          |
| `NullCBV`      | 未バインドCBV用          |
| `NullDSV`      | 未バインドDSV用          |
| `DefaultSampler`| デフォルトサンプラー      |

各Viewクラスは`static constexpr Null`メンバでNullディスクリプタポインタを保持。リソースが`nullptr`の場合、Nullディスクリプタが自動選択される。

### FD3D12View::FResourceInfo

Viewが参照するリソースを3つの方法で指定可能:

| コンストラクタ                          | 用途                                    |
|--------------------------------------|----------------------------------------|
| `FResourceInfo(FD3D12BaseShaderResource*)` | リネーム対応SRV/UAV (自動登録)            |
| `FResourceInfo(FD3D12ResourceLocation*)`  | 手動View (リネーム通知無し)               |
| デフォルト                              | 全nullptr                               |

### Viewライフサイクル

```
CreateView(ResourceInfo, D3DViewDesc)
  │
  ├── UpdateResourceInfo()
  │   ├── 旧リソースからRemoveRenameListener
  │   ├── 新リソースにAddRenameListener
  │   └── UpdateDescriptor() (仮想関数 → View種別固有のD3D12 API呼出)
  │
  └── InitializeBindlessSlot() (Bindless有効時)
      └── BindlessDescriptorManager.InitializeDescriptor(Handle, this)

ResourceRenamed(Contexts, Resource, Location)  // リソースリネーム通知
  │
  ├── UpdateView(Contexts, ResourceInfo, D3DViewDesc)
  │   ├── UpdateResourceInfo()
  │   └── UpdateBindlessSlot() (Bindless有効時)
  │       └── BindlessDescriptorManager.UpdateDescriptor(Contexts, Handle, this)
  │
  └── [SRV_RHI/UAV_RHI] FRHIViewDescから再構築 (ストリーミングによるリソース差替え対応)
```

### FD3D12ShaderResourceView (SRV)

| メンバ             | 説明                                              |
|-------------------|--------------------------------------------------|
| `OffsetInBytes`   | バッファSRVのバイトオフセット (リネーム時再計算)       |
| `StrideInBytes`   | バッファSRVの要素ストライド                          |
| `Flags`           | `EFlags::SkipFastClearFinalize` (FastClear最適化)  |
| `RayTracingScene` | RT SRV時のシーンポインタ                            |

`ERHIDescriptorType`は`RHIDescriptorTypeFromD3D12ViewDesc()`で判定:
- `D3D12_SRV_DIMENSION_BUFFER` → `BufferSRV`
- `D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE` → `AccelerationStructure`
- その他 → `TextureSRV`

### FD3D12UnorderedAccessView (UAV)

| メンバ            | 説明                                          |
|------------------|----------------------------------------------|
| `CounterResource`| Append/Consume Buffer用カウンターリソース        |
| `OffsetInBytes`  | バッファUAVのバイトオフセット                     |
| `StrideInBytes`  | バッファUAVの要素ストライド                       |

`EFlags::NeedsCounter`が設定された場合、追加のカウンターリソースが管理される。

### FD3D12DepthStencilView (DSV)

`FExclusiveDepthStencil`のインデックスで4つのDSVを管理:
- `HasDepth()`: Plane 0 (Depth) を含むか
- `HasStencil()`: Plane 1 (Stencil) を含むか
- `GetDepthOnlySubset()` / `GetStencilOnlySubset()`: プレーン選択

### FD3D12ShaderResourceView_RHI / FD3D12UnorderedAccessView_RHI

RHIインターフェース(`FRHIShaderResourceView`/`FRHIUnorderedAccessView`)と内部View(`FD3D12ShaderResourceView`/`FD3D12UnorderedAccessView`)を統合するラッパー。`FD3D12DeferredInitView<T>`テンプレートを使い、動的リソースのView初期化をRHIスレッドに遅延:

```
FD3D12DeferredInitView<T>::CreateViews(RHICmdList)
  │
  ├── TopOfPipe → EnqueueLambda + RHIThreadFence (遅延初期化)
  └── BottomOfPipe → 即時実行
```

`ResourceRenamed()`オーバーライドでは、`FRHIViewDesc`から完全に再構築される。ストリーミングシステムがリソースを異なるレイアウトのものに差し替える可能性があるため。

---

## 8. リソース作成API (Committed/Placed/Reserved)

`FD3D12Adapter`が提供する3つの低レベルリソース作成API:

### CreateCommittedResource

```
CreateCommittedResource(Desc, CreationNode, HeapProps, InitialAccess,
                        ResourceStateMode, DefaultAccess, ClearValue, ...)
  │
  ├── HeapFlags 判定
  │   ├── FD3D12_HEAP_FLAG_CREATE_NOT_ZEROED (RT/DS以外、ドライバ対応時)
  │   └── D3D12_HEAP_FLAG_SHARED (SimultaneousAccess時)
  │
  ├── GetRayTracingResourceFlags() (BVHRead/Write → RT + UAV フラグ)
  ├── ApplyCustomTextureLayout() (D3D12_WITH_CUSTOM_TEXTURE_LAYOUT有効時)
  │
  ├── Barriers->CreateCommittedResource(...) // Enhanced/Legacy バリア抽象化
  │
  ├── new FD3D12Resource(...) + AddRef
  ├── SetD3D12ObjectName
  ├── StartTrackingForResidency
  └── TraceMemoryAllocation
```

### CreatePlacedResource

```
CreatePlacedResource(Desc, BackingHeap, HeapOffset, InitialAccess, ...)
  │
  ├── Barriers->CreatePlacedResource(Heap, HeapOffset, Desc, ...)
  │
  ├── new FD3D12Resource(Device, ..., BackingHeap, HeapProps.Type)
  │
  ├── Windows GPUVirtualAddress設定
  │   ├── テクスチャ: Heap->GetGPUVirtualAddress() + HeapOffset
  │   └── バッファ: Resource直接取得 (verify一致)
  │
  ├── TraceMemoryAllocation (!Transient時のみ)
  ├── SetD3D12ObjectName
  └── AddRef
```

### CreateReservedResource

```
CreateReservedResource(Desc, CreationNode, InitialAccess, ...)
  │
  ├── Validation
  │   ├── bReservedResource == true
  │   ├── テクスチャ: Layout == D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
  │   └── Alignment == 0 or 65536
  │
  ├── Barriers->CreateReservedResource(Desc, InitialAccess, ClearValue, ...)
  │
  ├── new FD3D12Resource(Device, ..., nullptr /*Heap*/, DEFAULT)
  ├── SetD3D12ObjectName + AddRef
  └── ※ レジデンシートラッキングはエンジン管理外
```

### CreateBuffer — ヘルパー関数

3つのオーバーロード:

| パラメータ                              | 処理                                          |
|---------------------------------------|----------------------------------------------|
| `(HeapType, CreationNode, VisibleNodes, HeapSize, ...)` | HeapPropsから初期Access自動推定 → CreateCommittedResource |
| `(HeapType, CreationNode, VisibleNodes, InitialAccess, StateMode, DefaultAccess, ...)` | 明示的Access指定 → CreateCommittedResource |
| `(HeapProps, CreationNode, InitialAccess, ...)` | 直接HeapProps指定 → CreateCommittedResource |

### ID3D12ResourceAllocator — 汎用アロケータインターフェース

```cpp
struct ID3D12ResourceAllocator {
    // テクスチャ用: 4KB/64KB アライメント判定 → AllocateResource
    void AllocateTexture(GPUIndex, HeapType, Desc, UEFormat,
                         InitialAccess, StateMode, DefaultAccess,
                         ClearValue, Name, ResourceLocation);

    // 純粋仮想: 実際のアロケーション
    virtual void AllocateResource(GPUIndex, HeapType, Desc, Size, Alignment,
                                  InitialAccess, StateMode, DefaultAccess,
                                  ClearValue, Name, ResourceLocation) = 0;
};
```

---

## 9. CVar一覧

| CVar名                                     | デフォルト | 説明                                            |
|--------------------------------------------|---------|------------------------------------------------|
| `d3d12.ReservedResourceHeapSizeMB`         | 16      | Reserved Resourceバッキングヒープサイズ (MB)       |
| `D3D12.AdjustTexturePoolSizeBasedOnBudget` | 0       | OS予算に基づくテクスチャプールサイズ自動調整          |
| `D3D12.UseUpdateTexture3DComputeShader`    | 0       | 3Dテクスチャ更新にComputeShader使用               |
| `D3D12.TexturePoolOnlyAccountStreamableTexture` | false | プールサイズ計算をStreamableテクスチャのみに限定     |

---

## 関連ファイル

| ファイル                    | 内容                                          |
|---------------------------|----------------------------------------------|
| `D3D12Resources.h`        | FD3D12Resource, FD3D12Heap, FD3D12ResourceLocation, FD3D12Buffer, FD3D12UniformBuffer |
| `D3D12Resources.cpp`      | リソース作成 (Committed/Placed/Reserved), Location管理 |
| `D3D12Buffer.cpp`         | バッファ作成・Lock/Unlock・アップロード             |
| `D3D12Texture.h`          | FD3D12Texture, FD3D12BackBufferReferenceTexture2D |
| `D3D12Texture.cpp`        | テクスチャ作成・統計・フォーマット変換               |
| `D3D12View.h`             | FD3D12View階層, FD3D12ViewRange/Subset           |
| `D3D12View.cpp`           | View実装, ViewRange構築, Bindless管理            |
