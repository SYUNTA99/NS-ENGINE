# D3D12RHI Part 10: Work Graph

## 1. アーキテクチャ概要

D3D12RHIのWork Graphサブシステムは、D3D12 Work Graphs APIを活用して
Shader Bundle（複数シェーダーの一括ディスパッチ）を実現する。
UE5.7では主にNaniteのコンピュート/ラスタライズマテリアルの一括実行に使用される。

```
┌─────────────────────────────────────────────────────────────────┐
│                    FD3D12DynamicRHI                              │
│  RHICreateWorkGraphPipelineState()                              │
└─────────┬───────────────────────────────────────────────────────┘
          │
    ┌─────▼──────────────────────────────────────────────────────┐
    │              FD3D12CommandContext                           │
    │  DispatchWorkGraphShaderBundle()  [Compute版]              │
    │  DispatchWorkGraphShaderBundle()  [Graphics版]             │
    └─────┬──────────────────────────────────────────────────────┘
          │
    ┌─────▼─────────────────────────────────────────────────────────┐
    │              内部コンポーネント                                 │
    │                                                               │
    │  FD3D12WorkGraphPipelineState    State Object + バッキングメモリ │
    │  FWorkGraphShaderBundleBinder    ノード単位リソースバインディング  │
    │  FShaderBundleBinderOps          リソース遷移の重複排除         │
    │  FD3D12BindlessConstantSetter    Bindless定数バッファ書き込み   │
    │  FD3D12ExplicitDescriptorCache   トランジェントディスクリプタ    │
    │  RecordBindings()                ローカルルート引数の構築        │
    └───────────────────────────────────────────────────────────────┘
```

**フィーチャーフラグ:**

| フラグ | 用途 |
|--------|------|
| `D3D12_RHI_WORKGRAPHS` | Work Graph基本機能（コンピュートノード） |
| `D3D12_RHI_WORKGRAPHS_GRAPHICS` | グラフィクスノード（メッシュ+ピクセルシェーダー） |

**主要ソースファイル:**

| ファイル | 行数 | 内容 |
|---------|------|------|
| D3D12WorkGraph.h | 43 | FD3D12WorkGraphPipelineState宣言 |
| D3D12WorkGraph.cpp | 1435 | パイプライン構築、バインディング、ディスパッチ全実装 |

**関連シェーダークラス:**

| クラス | 基底 | 説明 |
|--------|------|------|
| `FD3D12WorkGraphShader` | FRHIWorkGraphShader, FD3D12ShaderData | Work Graphノードシェーダー |
| `FD3D12ShaderBundle` | FRHIShaderBundle, FD3D12DeviceChild | Shader Bundleリソース |

---

## 2. FD3D12WorkGraphPipelineState

### 2.1 データ構造

```
struct FD3D12WorkGraphPipelineState : public FRHIWorkGraphPipelineState
{
    FD3D12Device* Device;

    // D3D12 State Object
    TRefCountPtr<ID3D12RootSignature> RootSignature;     // グローバルルートシグネチャ
    TRefCountPtr<ID3D12StateObject>   StateObject;       // EXECUTABLE型

    // プログラム識別
    D3D12_PROGRAM_IDENTIFIER ProgramIdentifier;          // SetProgram用

    // バッキングメモリ
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE BackingMemoryAddressRange;

    // ローカルルート引数テーブル
    uint32 RootArgStrideInBytes;      // 16バイトアライメント済みストライド
    uint32 MaxRootArgOffset;          // 最大テーブルインデックス
    TArray<uint32> RootArgOffsets;    // ノード→テーブルインデックスマッピング

    // ノード管理
    TMap<FString, uint32> NodeCountPerName;   // ノード名→配列インデックスカウンタ

    bool bInitialized;                // 初回SetProgramフラグ
    D3D12ResourceFrameCounter FrameCounter;   // フレームカウンタ（Graphics版）

    // NV_AFTERMATH: デバッグ用シェーダー参照保持
    TArray<TRefCountPtr<FRHIShader>> Shaders;
};
```

### 2.2 State Object構築フロー

コンストラクタ `FD3D12WorkGraphPipelineState(Device, Initializer)` が
D3D12 State Objectの全構築を実行する。

```
FWorkGraphPipelineStateInitializer
    │
    ▼
┌─────────────────────────────────────────────────────┐
│ 1. グローバルルートシグネチャ決定                       │
│    RootShaderIndex指定あり → そのシェーダーのRS使用     │
│    (SF_WorkGraphRoot周波数)                           │
│    指定なし → Adapter->GetGlobalWorkGraphRootSignature │
└─────────────┬───────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────┐
│ 2. CD3DX12_STATE_OBJECT_DESC構築                     │
│    Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE          │
│                                                       │
│    [Global Root Signature Subobject]                  │
│    [Work Graph Subobject] ← ProgramName設定           │
│         AddEntrypoint(EntryShader)                    │
└─────────────┬───────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────┐
│ 3. コンピュートノード登録 (ShaderTable)                │
│    各ノードごと:                                      │
│      CD3DX12_DXIL_LIBRARY_SUBOBJECT (バイトコード)    │
│      CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT           │
│      CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION         │
│      CD3DX12_COMMON_COMPUTE_NODE_OVERRIDES            │
│        NewName → D3D12_NODE_ID{Name, ArrayIndex}     │
│        LocalRootArgumentsTableIndex → ノードインデックス│
│    RootArgStrideInBytes = Max(各ノードRS合計サイズ)     │
└─────────────┬───────────────────────────────────────┘
              │
              ▼ (D3D12_RHI_WORKGRAPHS_GRAPHICS時)
┌─────────────────────────────────────────────────────┐
│ 4. グラフィクスノード登録 (GraphicsPSOTable)           │
│    D3D12_STATE_OBJECT_FLAG_WORK_GRAPHS_USE_GRAPHICS   │
│    _STATE_FOR_GLOBAL_ROOT_SIGNATURE                   │
│                                                       │
│    各ノード: MeshShader + PixelShader                 │
│      DXIL Library x2 (MS, PS各1)                     │
│      Local Root Signature (GetWorkGraphGraphicsRootSignature)│
│      CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT             │
│      CD3DX12_RASTERIZER_SUBOBJECT                     │
│      CD3DX12_DEPTH_STENCIL_SUBOBJECT                  │
│      CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT (任意)    │
│      CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT          │
│      CD3DX12_GENERIC_PROGRAM_SUBOBJECT (MS+PS統合)    │
│      CD3DX12_MESH_LAUNCH_NODE_OVERRIDES               │
│        MaxInputRecordsPerGraphEntryRecord(1, false)   │
└─────────────┬───────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────┐
│ 5. State Object生成 + プロパティ取得                   │
│    RootArgStrideInBytes = Align16(MaxStride)          │
│    ID3D12Device9::CreateStateObject()                 │
│    ID3D12StateObjectProperties1::GetProgramIdentifier │
│    ID3D12WorkGraphProperties::GetWorkGraphIndex       │
│    (Graphics: SetMaximumInputRecords(1, 1))           │
│    GetWorkGraphMemoryRequirements()                   │
└─────────────┬───────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────┐
│ 6. バッキングメモリ確保                               │
│    CreateCommittedResource(                           │
│      MaxSizeInBytes, UAV, alignment=65536)            │
│    D3D12_HEAP_TYPE_DEFAULT, UNORDERED_ACCESS          │
│    BackingMemoryAddressRange ← GPU仮想アドレス         │
└─────────────────────────────────────────────────────┘
```

**ノード命名規則:**

```
ExportName (シェーダーエントリポイント)
    → NameTableで NodeName に変換
    → NodePathName = "{NodeName}_{ArrayIndex}"
    → State Object内のDXIL Export名として使用

例: EntryPoint="MainCS" → NodeName="ShaderBundleNode" → "ShaderBundleNode_0"
```

**ルートシグネチャ種別** (`ERootSignatureType`):

| 種別 | 用途 |
|------|------|
| `RS_WorkGraphGlobal` | Work Graphのグローバルルートシグネチャ |
| `RS_WorkGraphLocalCompute` | コンピュートノードのローカルルートシグネチャ |
| `RS_WorkGraphLocalRaster` | グラフィクスノードのローカルルートシグネチャ |

### 2.3 RHI作成API

```cpp
// FD3D12DynamicRHI::RHICreateWorkGraphPipelineState
//   常にDevice(0)で作成（LDA環境でも全ノードで共用可能）
FWorkGraphPipelineStateRHIRef RHICreateWorkGraphPipelineState(
    const FWorkGraphPipelineStateInitializer& Initializer);
```

---

## 3. リソースバインディングシステム

### 3.1 FShaderBundleBinderOps

並列RecordBindings実行時のリソース遷移を重複排除する構造体。

```
struct FShaderBundleBinderOps
{
    // Sherwood Setで重複検出（O(1)ルックアップ）
    Experimental::TSherwoodSet<FD3D12View*> TransitionViewSet;
    Experimental::TSherwoodSet<FD3D12View*> TransitionClearSet;

    // 遷移対象リスト
    TArray<FD3D12ShaderResourceView*>  TransitionSRVs;
    TArray<FD3D12UnorderedAccessView*> TransitionUAVs;
    TArray<FD3D12UnorderedAccessView*> ClearUAVs;

    void AddResourceTransition(SRV/UAV);   // RequiresResourceStateTracking()チェック付き
    void AddResourceClear(UAV);            // UAVクリア対象登録
};
```

**並列実行の安全性:** 各ワーカースレッドに独立な`FShaderBundleBinderOps`インスタンスを割り当て、
ディスパッチ後にワーカー0のインスタンスへマージする。

### 3.2 FWorkGraphShaderBundleBinder

ノード単位のリソースバインド状態を追跡する構造体。

```
struct FWorkGraphShaderBundleBinder
{
    FD3D12CommandContext& Context;
    FShaderBundleBinderOps& BinderOps;
    const uint32 GPUIndex;
    const EShaderFrequency Frequency;

    // Bindlessモード判定
    bool bBindlessResources;    // ShaderData->UsesBindlessResources()
    bool bBindlessSamplers;     // ShaderData->UsesBindlessSamplers()

    // CPU-sideディスクリプタ + バージョン追跡
    D3D12_CPU_DESCRIPTOR_HANDLE LocalCBVs/SRVs/UAVs/Samplers[MAX_*];
    uint32 CBV/SRV/UAV/SamplerVersions[MAX_*];
    uint64 BoundCBV/SRV/UAV/SamplerMask;

    void SetUAV(InUAV, Index, bClearResources);
    void SetSRV(InSRV, Index);
    void SetTexture(InTexture, Index);
    void SetSampler(InSampler, Index);
    void SetResourceCollection(InCollection, Index);  // Bindless専用
};
```

**バインディングフロー:**

1. Bindless**無効**時: `FD3D12OfflineDescriptor`をローカル配列に記録、バージョンを保存、ビットマスク更新
2. Bindless**有効**時（SetSRV/SetUAV/SetTexture）: ディスクリプタコピーをスキップし、`BinderOps.AddResourceTransition()`のみ実行
3. **例外**: `SetResourceCollection()`はBindless時に`StateCache.QueueBindlessSRV()`を呼び出し、`BinderOps.AddResourceTransition()`は呼ばない
4. 共通（SetSRV/SetUAV/SetTexture）: `BinderOps.AddResourceTransition()`で遷移リストに追加（`RequiresResourceStateTracking()`チェック付き）

### 3.3 FD3D12BindlessConstantSetter

Bindlessリソースの定数バッファへのハンドル書き込みを管理する。

```
struct FD3D12BindlessConstantSetter
{
    FD3D12CommandContext& Context;
    FD3D12ConstantBuffer& ConstantBuffer;   // StageConstantBuffers[SF_Compute]
    const uint32 GpuIndex;
    const EShaderFrequency Frequency;

    void SetBindlessHandle(Handle, Offset);     // Handle.GetIndex()を4バイト書き込み
    void SetUAV(UAV, Offset);                   // BindlessHandle書き込み + QueueBindlessUAV
    void SetSRV(SRV, Offset);                   // BindlessHandle書き込み + QueueBindlessSRV
    void SetTexture(Texture, Offset);           // DefaultBindlessHandle使用 + QueueBindlessSRV
    void SetSampler(Sampler, Offset);           // サンプラーBindlessHandle
    void SetResourceCollection(RC, Offset);     // QueueBindlessSRV(自身) + QueueBindlessSRVs(AllSrvs) + AllTextureReferences個別登録（ハンドル書き込みなし）

    void Finalize(OutConstantBuffer);           // ConstantBuffer.Version()で確定
};
```

### 3.4 FAllocatedConstantBuffer

定数バッファとそのリソース割り当てをペアにするラッパー。

```
struct FAllocatedConstantBuffer
{
    FD3D12ConstantBuffer* ConstantBuffer;    // null = 共有定数なし
    FD3D12ResourceLocation ResourceLocation; // GPU仮想アドレス
};
```

---

## 4. RecordBindings

`RecordBindings()`はWork Graphの各ノードに対するローカルルート引数を構築する
中核関数である（D3D12WorkGraph.cpp:495-670）。

### 4.1 シグネチャ

```cpp
static void RecordBindings(
    FD3D12CommandContext& Context,
    EShaderFrequency Frequency,          // SF_Compute / SF_Mesh / SF_Pixel
    FD3D12ExplicitDescriptorCache& TransientDescriptorCache,
    FShaderBundleBinderOps& BinderOps,
    uint32 WorkerIndex,                  // 並列実行ワーカーID
    FRHIShader* ShaderRHI,
    FD3D12ShaderData const* D3D12ShaderData,
    FRHIBatchedShaderParameters const& Parameters,
    FD3D12RootSignature const* LocalRootSignature,
    FAllocatedConstantBuffer const& SharedConstantBuffer,
    FUint32Vector4 const& Constants,     // ルート定数 (4 DWORD)
    TArrayView<uint32> RootArgs          // 出力: ルート引数配列
);
```

### 4.2 処理フロー

```
┌─────────────────────────────────────────────────┐
│ 1. FWorkGraphShaderBundleBinder作成               │
│    Bindlessモード・シェーダーリソースカウント初期化   │
└─────────┬───────────────────────────────────────┘
          ▼
┌─────────────────────────────────────────────────┐
│ 2. ResourceParameters処理                         │
│    Texture → SetTexture()                        │
│    ResourceView → SetSRV()                       │
│    UnorderedAccessView → SetUAV()                │
│    Sampler → SetSampler()                        │
│    UniformBuffer → BundleUniformBuffers[]に格納   │
│    ResourceCollection → SetResourceCollection()  │
└─────────┬───────────────────────────────────────┘
          ▼
┌─────────────────────────────────────────────────┐
│ 3. Static Uniform Bufferの適用                    │
│    ApplyStaticUniformBuffers() →                 │
│    BundleUniformBuffers[]にマージ                 │
└─────────┬───────────────────────────────────────┘
          ▼
┌─────────────────────────────────────────────────┐
│ 4. UBテーブルからのリソース設定                    │
│    SetUniformBufferResourcesFromTables()          │
│    → BundleBinderのSet*メソッドを経由              │
└─────────┬───────────────────────────────────────┘
          ▼
┌─────────────────────────────────────────────────┐
│ 5. バインディング完全性検証                        │
│    IsCompleteBinding(ExpectedCount, BoundMask)    │
│    → SRV/UAV/CBV/Sampler全てチェック              │
│    不足があればcheck()でアサーション               │
└─────────┬───────────────────────────────────────┘
          ▼
┌─────────────────────────────────────────────────┐
│ 6. ディスクリプタテーブル → RootArgs書き込み        │
│                                                   │
│    SRV: TransientDescriptorCache.Allocate()       │
│         → GPU handle → RootArgs[SRVRDTBindSlot]   │
│                                                   │
│    Sampler: AllocateDeduplicated() (重複排除)      │
│         → GPU handle → RootArgs[SamplerRDTBindSlot]│
│                                                   │
│    UAV: AllocateDeduplicated()                    │
│         → GPU handle → RootArgs[UAVRDTBindSlot]    │
│                                                   │
│    CBV (Shared): GPU VA → RootArgs[CBVRDBindSlot]  │
│    CBV (UB): GPU VA → RootArgs[CBVRDBindSlot(i)]   │
│                                                   │
│    Root Constants: Constants.XYZW →                │
│         RootArgs[RootConstantsSlot] (4 DWORD)      │
└─────────────────────────────────────────────────┘
```

**Memcpy書き込み:** ディスクリプタテーブルハンドル（`D3D12_GPU_DESCRIPTOR_HANDLE`）と
CBVアドレス（`D3D12_GPU_VIRTUAL_ADDRESS`）はバイト単位オフセットを4で割ったDWORDオフセットで
`RootArgs`配列に`FMemory::Memcpy`で書き込まれる。

---

## 5. SharedBindlessConstants

`SetShaderBundleSharedBindlessConstants()`は、全ノード間で共有される
Bindless定数をセットアップする。

```cpp
void SetShaderBundleSharedBindlessConstants(
    FD3D12CommandContext& Context,
    TConstArrayView<FRHIShaderParameterResource> SharedBindlessParameters,
    FAllocatedConstantBuffer& OutConstantBuffer);
```

**処理内容:**
1. `FD3D12BindlessConstantSetter`を`SF_Compute`周波数で作成
2. SharedBindlessParameters内の各リソースのBindlessハンドルを定数バッファに書き込み
   - Texture → `SetTexture()`
   - ResourceView → `SetSRV()`
   - UnorderedAccessView → `SetUAV()`
   - ResourceCollection → `SetResourceCollection()`
   - Sampler → スキップ（定数バッファ経由不要）
3. `Setter.Finalize(OutConstantBuffer)` → 定数バッファのバージョン確定

**出力:** `OutConstantBuffer.ConstantBuffer != nullptr`かつ`ResourceLocation`に
GPU仮想アドレスが設定された状態となり、各ノードのCBVスロット0にバインドされる。

---

## 6. Computeディスパッチ

`FD3D12CommandContext::DispatchWorkGraphShaderBundle()` (Compute版)
はShader Bundleをコンピュートノードとしてディスパッチする（D3D12WorkGraph.cpp:798-1083）。

### 6.1 入力パラメータ

```cpp
void DispatchWorkGraphShaderBundle(
    FRHIShaderBundle* ShaderBundle,                          // バンドル定義
    FRHIBuffer* RecordArgBuffer,                             // GPUレコード引数バッファ
    TConstArrayView<FRHIShaderParameterResource> SharedBindlessParameters,  // 共有Bindless
    TConstArrayView<FRHIShaderBundleComputeDispatch> Dispatches);  // ノード単位パラメータ
```

### 6.2 実行フロー

```
┌─────────────────────────────────────────────────────┐
│ Phase 1: パイプライン準備                             │
│                                                       │
│  EntryShader ← GetGlobalShaderMap                     │
│                  ->GetShader<FDispatchShaderBundleWorkGraph>()│
│                                                       │
│  ValidRecords収集:                                    │
│    Dispatch.IsValid() かつ WorkGraphShader != null     │
│    → ディスクリプタカウント集計                         │
│                                                       │
│  NameTable構築:                                       │
│    "WorkGraphMainCS" → エントリノード                  │
│    "" → "ShaderBundleNode" (空スロット)                │
│    "MainCS" → "ShaderBundleNode" (Naniteコンピュート)  │
│    "MicropolyRasterize" → "ShaderBundleNode" (SW Raster)│
│                                                       │
│  checkf(NumRecords <= GetMaxShaderBundleSize())        │
│  r.ShaderBundle.MaxSize CVarで上限制御                 │
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 2: リソースセットアップ                          │
│                                                       │
│  Pipeline = PipelineStateCache::                      │
│    GetAndOrCreateWorkGraphPipelineState()              │
│                                                       │
│  FD3D12ExplicitDescriptorCache 作成 (MaxTasks)         │
│    Init(0, NumViewDescriptors, NumSamplerDescriptors, │
│         ERHIBindlessConfiguration::Minimal)            │
│                                                       │
│  BinderOps[MaxTasks] 初期化                           │
│  LocalRootArgs = zero-filled uint32配列               │
│    サイズ = (RootArgStrideInBytes/4) * (MaxOffset+1)  │
│                                                       │
│  SetShaderBundleSharedBindlessConstants() → 共有CB     │
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 3: 並列 RecordBindings                          │
│                                                       │
│  ParallelForWithExistingTaskContext(                   │
│    "DispatchShaderBundle",                            │
│    MaxTasks=4, ItemsPerTask=1024)                     │
│                                                       │
│  各有効レコード:                                      │
│    ShaderTableIndex = RecordIndex + 1                 │
│    RootArgOffset = Pipeline->RootArgOffsets[idx]       │
│    RecordBindings(SF_Compute, ...)                    │
│      → LocalRootArgs[offset..offset+stride] に書き込み│
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 4: RecordArgBuffer SRV作成                      │
│                                                       │
│  Bindless有効:                                        │
│    RecordArgBufferBindlessHandle = SRV.GetIndex()     │
│  Bindless無効:                                        │
│    TransientDescriptorCache.Allocate() →              │
│    RootArgs[EntryNodeSlot] にGPUハンドル書き込み       │
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 5: ルート引数テーブルアップロード                 │
│                                                       │
│  "BundleRecordBuffer" 作成 (Static, CopyDest)         │
│  UploadResourceDataViaCopyQueue()                     │
│  AddBarrier(CopyDest → Common)                        │
│                                                       │
│  NodeLocalRootArgumentsTable:                         │
│    StartAddress = RootArgBuffer GPU VA                 │
│    SizeInBytes  = バッファサイズ                       │
│    StrideInBytes = Pipeline->RootArgStrideInBytes      │
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 6: BinderOpsマージ + バリアフラッシュ            │
│                                                       │
│  Worker 1..N → Worker 0 にSRV/UAV遷移リストをマージ   │
│  ClearShaderResources(ClearUAVs, SRVMask)             │
│  FlushResourceBarriers()                              │
└─────────────┬───────────────────────────────────────┘
              ▼
┌─────────────────────────────────────────────────────┐
│ Phase 7: ディスパッチ実行                              │
│                                                       │
│  SetExplicitDescriptorCache(TransientDescriptorCache) │
│  (Bindless: ApplyBindlessResources(SF_Compute))       │
│                                                       │
│  SetComputeRootSignature(Pipeline->RootSignature)     │
│                                                       │
│  D3D12_SET_PROGRAM_DESC:                              │
│    Type = D3D12_PROGRAM_TYPE_WORK_GRAPH               │
│    ProgramIdentifier                                  │
│    Flags = INITIALIZE (初回) / NONE (以降)             │
│    BackingMemory = Pipeline->BackingMemoryAddressRange │
│    NodeLocalRootArgumentsTable                        │
│  GraphicsCommandList10()->SetProgram(&desc)            │
│                                                       │
│  InputRecord = MakeInputRecord(                       │
│    NumRecords, ArgOffset, ArgStride, BindlessHandle)   │
│                                                       │
│  D3D12_DISPATCH_GRAPH_DESC:                           │
│    Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT           │
│    EntrypointIndex = 0                                │
│    NumRecords = 1                                     │
│    pRecords = &InputRecord                            │
│  GraphicsCommandList10()->DispatchGraph(&desc)         │
│                                                       │
│  Pipeline->bInitialized = true                        │
│  UnsetExplicitDescriptorCache()                       │
│  StateCache.DirtyState()  // ステートキャッシュ無効化  │
│  ConditionalSplitCommandList()                        │
└─────────────────────────────────────────────────────┘
```

### 6.3 ディスクリプタキャッシュ

`FD3D12ExplicitDescriptorCache`をトランジェントモードで使用し、
Work Graphディスパッチ専用のディスクリプタヒープを構築する。

- **View Heap**: SRV + CBV + UAV（全ノードの合計数で初期化）
- **Sampler Heap**: サンプラー
- **MaxWorkers**: 最大4ワーカースレッド対応
- **Deduplication**: `AllocateDeduplicated()`でサンプラーとUAVのバージョンベース重複排除
- **Minimal Bindless**: `ERHIBindlessConfiguration::Minimal`で初期化

---

## 7. Graphicsディスパッチ

`FD3D12CommandContext::DispatchWorkGraphShaderBundle()` (Graphics版)
はメッシュシェーダー+ピクセルシェーダーノードをディスパッチする（D3D12WorkGraph.cpp:1085-1434）。

### 7.1 入力パラメータ

```cpp
void DispatchWorkGraphShaderBundle(
    FRHIShaderBundle* ShaderBundle,
    FRHIBuffer* RecordArgBuffer,
    const FRHIShaderBundleGraphicsState& BundleState,           // グラフィクスステート
    TConstArrayView<FRHIShaderParameterResource> SharedBindlessParameters,
    TConstArrayView<FRHIShaderBundleGraphicsDispatch> Dispatches);
```

### 7.2 Compute版との差分

| 項目 | Compute版 | Graphics版 |
|------|-----------|------------|
| Dispatch型 | `FRHIShaderBundleComputeDispatch` | `FRHIShaderBundleGraphicsDispatch` |
| シェーダー | WorkGraphShader のみ | MeshShader + PixelShader |
| RecordBindings | SF_Compute × 1回 | SF_Pixel + SF_Mesh × 2回 |
| NameTable | "MainCS", "MicropolyRasterize" | "HWRasterizeMS" |
| PSO初期化 | なし | GraphicsPSOTable → State Object |
| RS選択 | シェーダー直接RS | GetWorkGraphGraphicsRootSignature() |
| ルートRS | SetComputeRootSignature | SetGraphicsRootSignature |
| SetProgram Flags | INITIALIZE(初回)/NONE | 常にINITIALIZE |
| bInitialized設定 | `Pipeline->bInitialized = true` | 設定しない（常に再初期化） |
| ApplyBindless位置 | Phase 7（SetExplicitDescriptorCache後） | RecordArgBuffer SRV作成内（bBindless分岐内） |
| FrameCounter | なし | Pipeline->FrameCounter.Set() |

### 7.3 グラフィクスステート設定

Graphics版では`BundleState`から以下を直接D3D12コマンドリストに設定する:

```
// 頂点/インデックスバッファ（未使用をクリア）
IASetVertexBuffers(0, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, nullptr)
IASetIndexBuffer(nullptr)
OMSetRenderTargets(0, nullptr, 0, nullptr)

// ビューポート
RSSetViewports(1, &Viewport)     // BundleState.ViewRect + DepthMin/Max

// シザー矩形
RSSetScissorRects(1, &Rect)      // BundleState.ViewRect

// プリミティブトポロジー
IASetPrimitiveTopology(TranslatePrimitiveType(BundleState.PrimitiveType))

// ステンシル参照値 / ブレンドファクタ
OMSetStencilRef(BundleState.StencilRef)
OMSetBlendFactor(BundleState.BlendFactor)
```

### 7.4 RecordBindingsの二段階実行

Graphics版では各ノードに対して`RecordBindings()`を2回呼び出す:

```
RecordBindings(SF_Pixel, ..., *Dispatch.Parameters_PS, ...)
RecordBindings(SF_Mesh,  ..., *Dispatch.Parameters_MSVS, ...)
```

同じ`RootArgs`スライスに対して異なるシェーダー周波数のバインディングを書き込む。
`LocalRootSignature`は`GetWorkGraphGraphicsRootSignature(BoundShaderState)`で取得され、
メッシュシェーダーとピクセルシェーダーの両方のバインディングスロットを含む。

---

## 8. 並列実行モデル

### 8.1 ParallelFor構成

```
┌────────────────────────────────────────────┐
│  ParallelForWithExistingTaskContext         │
│                                            │
│  MaxWorkers = 4                            │
│  NumWorkerThreads = FTaskGraphInterface::Get().GetNumWorkerThreads()│
│  MaxTasks = Min(NumWorkerThreads, 4)       │
│  ItemsPerTask = 1024                       │
│                                            │
│  Threading条件:                            │
│    FApp::ShouldUseThreadingForPerformance()│
│    false → MaxTasks = 1 (シングルスレッド) │
└────────────────────────────────────────────┘
```

### 8.2 スレッドセーフティ

| リソース | 安全性 | 手法 |
|---------|--------|------|
| LocalRootArgs配列 | 安全 | 各ノードが異なるオフセット領域に書き込み |
| FD3D12ExplicitDescriptorCache | 安全 | WorkerIndex単位のアロケーション |
| FShaderBundleBinderOps | 安全 | ワーカーごとに独立インスタンス |
| FD3D12CommandContext | 非安全 | RecordBindingsはRetrieveObjectのみ使用（読み取り） |

### 8.3 BinderOpsマージ

並列実行完了後、ワーカー1..NのTransition情報をワーカー0にマージする:

```cpp
for (WorkerIndex = 1; WorkerIndex < BinderOps.Num(); ++WorkerIndex)
{
    // SRV/UAV遷移をマージ
    for (SRV : BinderOps[WorkerIndex].TransitionSRVs)
        BinderOps[0].AddResourceTransition(SRV);  // TSherwoodSetで重複排除
    for (UAV : BinderOps[WorkerIndex].TransitionUAVs)
        BinderOps[0].AddResourceTransition(UAV);

    // ワーカーのリストをクリア
    BinderOps[WorkerIndex].TransitionSRVs.Empty();
    BinderOps[WorkerIndex].TransitionUAVs.Empty();
    BinderOps[WorkerIndex].TransitionViewSet.Empty();
    BinderOps[WorkerIndex].ClearUAVs.Empty();
    BinderOps[WorkerIndex].TransitionClearSet.Empty();
}

// マージ後にクリア/遷移実行
for (UAV : BinderOps[0].ClearUAVs)
    ClearShaderResources(UAV, EShaderParameterTypeMask::SRVMask);
```

---

## 9. ルート引数テーブル

### 9.1 メモリレイアウト

```
LocalRootArgs配列 (uint32[]):

 Stride = RootArgStrideInBytes / 4 (DWORDs)
 ┌─────────────────────────────────────────┐
 │ Offset[0]: エントリノードのルート引数      │ ← RecordArgBuffer SRV
 │  [SRVRDTBindSlot]: GPU Descriptor Handle  │
 ├─────────────────────────────────────────┤
 │ Offset[1]: ノード0のルート引数             │
 │  [SRVRDTBindSlot]: GPU Descriptor Handle  │
 │  [SamplerRDTBindSlot]: GPU Desc Handle    │
 │  [UAVRDTBindSlot]: GPU Descriptor Handle  │
 │  [CBVRDBindSlot(0)]: GPU Virtual Address  │
 │  [CBVRDBindSlot(i)]: GPU Virtual Address  │
 │  [RootConstantsSlot]: XYZW (4 DWORD)     │
 ├─────────────────────────────────────────┤
 │ Offset[2]: ノード1のルート引数             │
 │  ...                                      │
 ├─────────────────────────────────────────┤
 │ ...                                       │
 └─────────────────────────────────────────┘

 アライメント: RootArgStrideInBytes = Align16(MaxNodeRSSize)
```

### 9.2 アップロードパス

```
LocalRootArgs (CPU配列)
    │
    ▼
CreateRHIBuffer("BundleRecordBuffer",
    Static, CopyDest, MultiState)
    │
    ▼ UploadResourceDataViaCopyQueue()
    │
    ▼ AddBarrier(CopyDest → Common)
    │
    ▼
D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE
    .StartAddress = Buffer GPU VA
    .SizeInBytes  = Total size
    .StrideInBytes = RootArgStrideInBytes
    │
    ▼
SetProgram()の NodeLocalRootArgumentsTable
```

---

## 10. D3D12 APIインターフェース

### 10.1 使用API一覧

| API | 用途 | 要件 |
|-----|------|------|
| `ID3D12Device9::CreateStateObject` | State Object生成 | D3D12_STATE_OBJECT_TYPE_EXECUTABLE |
| `ID3D12StateObjectProperties1::GetProgramIdentifier` | プログラムID取得 | |
| `ID3D12WorkGraphProperties::GetWorkGraphIndex` | Work Graphインデックス | |
| `ID3D12WorkGraphProperties::GetWorkGraphMemoryRequirements` | メモリ要件照会 | |
| `ID3D12WorkGraphProperties1::SetMaximumInputRecords` | 入力レコード最大数（Graphics用） | D3D12_RHI_WORKGRAPHS_GRAPHICS |
| `ID3D12GraphicsCommandList10::SetProgram` | Work Graphプログラム設定 | |
| `ID3D12GraphicsCommandList10::DispatchGraph` | Work Graphディスパッチ | |

### 10.2 State Objectサブオブジェクト一覧

**コンピュートノード用:**

| サブオブジェクト | 用途 |
|-----------------|------|
| `CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT` | グローバルRS |
| `CD3DX12_WORK_GRAPH_SUBOBJECT` | Work Graphプログラム定義 |
| `CD3DX12_DXIL_LIBRARY_SUBOBJECT` | シェーダーバイトコード |
| `CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT` | ノードローカルRS |
| `CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT` | RS-Export関連付け |
| `CD3DX12_COMMON_COMPUTE_NODE_OVERRIDES` | ノード名/テーブルインデックスオーバーライド |

**グラフィクスノード用（追加）:**

| サブオブジェクト | 用途 |
|-----------------|------|
| `CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT` | Graphics用フラグ設定 |
| `CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT` | プリミティブトポロジー |
| `CD3DX12_RASTERIZER_SUBOBJECT` | ラスタライザ状態 |
| `CD3DX12_DEPTH_STENCIL_SUBOBJECT` | デプスステンシル状態 |
| `CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT` | DSVフォーマット |
| `CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT` | RTVフォーマット |
| `CD3DX12_GENERIC_PROGRAM_SUBOBJECT` | MS+PS統合プログラム |
| `CD3DX12_MESH_LAUNCH_NODE_OVERRIDES` | メッシュノードオーバーライド |

---

## 11. CVar一覧

| CVar | 型 | デフォルト | 説明 |
|------|-----|-----------|------|
| `wg.ShaderBundle.SkipDispatch` | bool | false | DispatchGraphをスキップ（デバッグ用） |

**関連CVar（他ファイル）:**

| CVar | 説明 |
|------|------|
| `r.ShaderBundle.MaxSize` | Shader Bundleの最大エントリ数上限 |

---

## 12. 実装上の注意事項

### 12.1 TODO/既知の制限

ソースコード中に以下のTODOコメントが存在する:

```
// TODO: don't use raw device?          (バッキングメモリ確保でDevice->GetDevice()直接使用)
// TODO: check resource view             (リソース遷移が一部コメントアウト)
// todo: Check if copy queue is the optimal way to upload the root args.
// todo: Use a single buffer owned by the shader bundle RHI object
```

### 12.2 バッキングメモリ

- `CreateCommittedResource`で直接確保（プールアロケータ未使用）
- アライメント: 65536バイト（64KB）
- `D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS`必須
- サイズ: `GetWorkGraphMemoryRequirements().MaxSizeInBytes`
- ヒープ: `D3D12_HEAP_TYPE_DEFAULT`

### 12.3 初期化フラグ

- Compute版: `bInitialized`フラグで初回`D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE`を設定、
  2回目以降は`D3D12_SET_WORK_GRAPH_FLAG_NONE`
- Graphics版: 常に`D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE`（フレームごとに再初期化）

### 12.4 ステートキャッシュ無効化

Work Graphディスパッチはステートキャッシュを経由せずにルートシグネチャや
ディスクリプタを設定するため、実行後に`StateCache.DirtyState()`で全ステートを
無効化する。後続のDraw/Dispatchで正しく再バインドされることを保証する。

### 12.5 SDK バージョン分岐

```cpp
#if D3D12_SDK_VERSION < 616
    WorkGraphSubobject->Finalize();   // 旧SDK: 明示的Finalize必要
#endif
```

SDK 616以降では`Finalize()`呼び出しが不要。
