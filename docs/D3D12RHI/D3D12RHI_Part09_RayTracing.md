# D3D12RHI Part 9: レイトレーシング

## 1. アーキテクチャ概要

D3D12RHIのレイトレーシングサブシステムは、DXR (DirectX Raytracing) APIを抽象化し、
UE5のRHIレイトレーシングインターフェースを実装する。

```
┌─────────────────────────────────────────────────────────────────┐
│                    FD3D12DynamicRHI                              │
│  RHICreateRayTracingPipelineState()                             │
│  RHICreateRayTracingGeometry()                                  │
│  RHICreateRayTracingScene()                                     │
│  RHICreateShaderBindingTable()                                  │
│  RHICalcRayTracingSceneSize() / RHICalcRayTracingGeometrySize() │
└─────────┬───────────────────────────────────────────────────────┘
          │
    ┌─────▼──────────────────────────────────────────────────┐
    │              FD3D12CommandContext                       │
    │  RHIBuildAccelerationStructures() (BLAS/TLAS)          │
    │  RHISetBindingsOnShaderBindingTable()                  │
    │  RHICommitShaderBindingTable()                         │
    │  RHIRayTraceDispatch() / RHIRayTraceDispatchIndirect() │
    │  RHIBindAccelerationStructureMemory()                  │
    │  RHIExecuteMultiIndirectClusterOperation() (NVAPI)     │
    └─────┬──────────────────────────────────────────────────┘
          │
    ┌─────▼─────────────────────────────────────────────────────┐
    │              内部コンポーネント                             │
    │                                                           │
    │  FD3D12RayTracingPipelineCache   Collection-based キャッシュ │
    │  FD3D12RayTracingPipelineState   RTPSO (linked pipeline)   │
    │  FD3D12RayTracingGeometry        BLAS管理                  │
    │  FD3D12RayTracingScene           TLAS管理                  │
    │  FD3D12RayTracingShaderBindingTable[Internal]  SBT管理     │
    │  FD3D12RayTracingCompactionRequestHandler  非同期コンパクション│
    │  FD3D12ExplicitDescriptorCache   RT用ディスクリプタ          │
    └───────────────────────────────────────────────────────────┘
```

**主要ソースファイル:**

| ファイル | 行数 | 内容 |
|---------|------|------|
| D3D12RayTracing.h | 307 | コアクラス宣言 |
| D3D12RayTracing.cpp | 6419 | 全実装（パイプライン、AS、SBT、ディスパッチ、クラスタ） |
| D3D12RayTracingResources.h | 41 | FD3D12HitGroupSystemParameters定義 |
| D3D12RayTracingDebug.h/cpp | 378 | シーンシリアライズデバッグ |

## 2. コアデータ構造

### 2.1 FD3D12ShaderIdentifier

DXRシェーダー識別子（32バイト）のラッパー。

```
struct FD3D12ShaderIdentifier
{
    uint64 Data[4] = {~0ull, ~0ull, ~0ull, ~0ull};  // デフォルト: 全ビット1 (無効値)
    // D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES = 32

    bool IsValid() const;       // 全要素が~0ullでないかチェック (全1=無効)
    bool operator==(other);     // 全4要素の比較
    bool operator!=(other);

    void SetData(const void* InData);  // memcpy で32バイトコピー
};
```

**用途:** SBTレコードの先頭に書き込まれ、DispatchRays時にGPUが該当シェーダーを特定する。

### 2.2 FD3D12RayTracingShaderLibrary

シェーダー種別ごとのコレクション管理。

```
struct FD3D12RayTracingShaderLibrary
{
    TArray<TRefCountPtr<FD3D12RayTracingShader>> Shaders;  // シェーダー配列 (参照カウント)
    TArray<FD3D12ShaderIdentifier>  Identifiers;    // 対応する識別子

    int32 Find(const FSHAHash& Hash) const;         // ハッシュで検索
    void  Reserve(int32 Num);
};
```

**FD3D12RayTracingPipelineStateが以下4つのライブラリを保持:**
- `RayGenShaders` — レイ生成シェーダー
- `MissShaders` — ミスシェーダー
- `HitGroupShaders` — ヒットグループ (CHS + AHS + IS)
- `CallableShaders` — コーラブルシェーダー

### 2.3 FD3D12HitGroupSystemParameters

ヒットグループのローカルルートシグネチャ先頭に配置されるシステムパラメータ。

```
struct FD3D12HitGroupSystemParameters
{
    FHitGroupSystemRootConstants RootConstants;  // UserData, Strides, IndexOffset, FirstPrimitive

    union {
        // Bindless有効時
        struct {
            uint32 BindlessHitGroupSystemIndexBuffer;
            uint32 BindlessHitGroupSystemVertexBuffer;
        };
        // 従来モード
        struct {
            D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
            D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
        };
    };
};
```

- **RootConstants:** 頂点ストライド、インデックスストライド、インデックスバッファオフセット、FirstPrimitive、UserData
- **バインドレス時:** SRVのバインドレスハンドルインデックスを格納
- **従来モード:** GPU仮想アドレスを直接格納

## 3. FD3D12RayTracingPipelineCache

コレクション単位のState Objectキャッシュ。FD3D12AdapterChild を継承し、デバイスごとに1つ存在する（FD3D12Device のメンバ）。

### 3.1 キャッシュキーとエントリ

```
struct FKey {
    uint64             ShaderHash;              // シェーダーバイトコードのハッシュ
    uint32             MaxAttributeSizeInBytes;
    uint32             MaxPayloadSizeInBytes;
    ID3D12RootSignature* GlobalRootSignature;
    ID3D12RootSignature* LocalRootSignature;
};

struct FEntry {
    ECollectionType                 CollectionType;
    TRefCountPtr<FD3D12RayTracingShader> Shader;   // 参照カウント付き
    TRefCountPtr<ID3D12StateObject> StateObject;    // Collection型SO
    FD3D12RayTracingPipelineInfo    PipelineInfo;
    FGraphEventRef                  CompileEvent;   // 非同期コンパイル完了イベント
    bool                            bDeserialized = false; // プリコンパイルPSO
    static constexpr uint32 MaxExports = 4;
    TArray<FString, TFixedAllocator<MaxExports>> ExportNames;  // 固定アロケータ
    FD3D12ShaderIdentifier          Identifier;
    float                           CompileTimeMS = 0.0f;
};
```

### 3.2 コレクションタイプ

| ECollectionType | 用途 |
|----------------|------|
| `Unknown` | デフォルト（未設定） |
| `RayGen` | レイ生成シェーダー |
| `Miss` | ミスシェーダー |
| `HitGroup` | CHS+AHS+ISの組み合わせ |
| `Callable` | コーラブルシェーダー |

### 3.3 GetOrCompileShader フロー

```
GetOrCompileShader(Device, Shader, GlobalRS, MaxAttr, MaxPayload, bRequired, CollectionType)
│
├─ ShaderHash = GetShaderHash64(Shader)
├─ LocalRS = (RayGen ? DefaultEmptyRS : Shader->LocalRootSignature)
├─ CacheKey = {ShaderHash, MaxAttr, MaxPayload, GlobalRS, LocalRS}
│
├─ Cache.FindOrAdd(CacheKey)
│  ├─ [ヒット] → CompileEvent待ちリストに追加
│  └─ [ミス] →
│     ├─ Shader->bPrecompiledPSO ?
│     │  ├─ [Yes] → DeserializeRayTracingStateObject → GetIdentifier
│     │  └─ [No]  → FShaderCompileTask をタスクグラフにディスパッチ
│     │             (AnyHiPriThreadHiPriTask)
│     └─ CompletionList に CompileEvent を追加
│
└─ return FEntry*
```

**FShaderCompileTask:**
- FDXILLibraryでDXILバイトコードをラップ
- エントリポイントをハッシュベースの一意名にリネーム（衝突回避）
- HitGroupの場合：CHS/AHS/ISの最大3エントリポイントを1つのD3D12_HIT_GROUP_DESCにまとめる
- `CreateRayTracingStateObject()` で `D3D12_STATE_OBJECT_TYPE_COLLECTION` を作成
- 完了後に `GetShaderIdentifier()` でIdentifierを取得
- 1000ms超のコンパイルはログ出力

### 3.4 ローカルルートシグネチャ

| シェーダー種別 | ローカルRS |
|--------------|-----------|
| RayGen | DefaultLocalRootSignature（空のRS） |
| HitGroup | Shader->LocalRootSignature（カスタム） |
| Miss | Shader->LocalRootSignature（カスタム） |
| Callable | Shader->LocalRootSignature（カスタム） |

RayGenシェーダーはすべてのリソースをグローバルRSでバインドするため、空のローカルRSを使用。

## 4. FD3D12RayTracingPipelineState (RTPSO)

### 4.1 構築フロー

```
FD3D12RayTracingPipelineState(Device, Initializer)
│
├─ GlobalRootSignature = PipelineCache->GetGlobalRootSignature(ShaderBindingLayout)
│
├─ BasePipeline取得 (GRHISupportsRayTracingPSOAdditions時)
│  └─ PipelineShaderHashes を継承
│
├─ シェーダーコレクション収集 (AddShaderCollection lambda):
│  ├─ RayGen → ECollectionType::RayGen
│  ├─ Miss  → ECollectionType::Miss  (MaxHitGroupViewDescriptors/MaxLocalRootSignatureSize更新)
│  ├─ HitGroup → ECollectionType::HitGroup (同上更新)
│  └─ Callable → ECollectionType::Callable (同上更新)
│  各シェーダーで PipelineCache->GetOrCompileShader() を呼び出し
│  重複ハッシュは UniqueShaderCollections に追加しない
│
├─ FTaskGraphInterface::WaitUntilTasksComplete(CompileCompletionList)
│
├─ [Partial PSO] → コンパイル完了で早期リターン
│
├─ リンク:
│  ├─ [BasePipeline あり && !SpecializeStateObjects]
│  │  ├─ [新シェーダーなし] → BasePipeline->StateObject を再利用
│  │  └─ [新シェーダーあり] → Device7->AddToStateObject() (増分追加)
│  └─ [それ以外]
│     └─ CreateRayTracingStateObject(RAYTRACING_PIPELINE)
│        全コレクションを D3D12_EXISTING_COLLECTION_DESC として渡す
│
├─ 特殊化 (GRayTracingSpecializeStateObjects != 0):
│  └─ CreateSpecializedStateObjects() — パフォーマンスグループ別RTPSO
│
├─ PipelineProperties->QueryInterface → シェーダー識別子取得
│  各ライブラリ(RayGen/Miss/HitGroup/Callable)のIdentifiers[]を埋める
│
└─ PipelineStackSize = PipelineProperties->GetPipelineStackSize()
```

### 4.2 RTPSO特殊化 (Specialization)

RayGenシェーダーが複数ある場合、パフォーマンスグループに基づいて特殊化RTPSOを作成する。

```
CreateSpecializedStateObjects():
│
├─ 各コレクションのPipelineInfo.PerformanceGroupを取得
├─ RayGen以外のMin/Maxパフォーマンスグループを算出
├─ 全RayGenが同一グループ → 特殊化不要で早期リターン
│
├─ RayGenをパフォーマンスグループ別バケットに分類
│  (Group0=最悪性能はデフォルトRTPSOを使用)
│
└─ 各バケットで:
   ├─ 非RayGenコレクション + バケット内RayGenコレクションを結合
   └─ CreateRayTracingStateObject(RAYTRACING_PIPELINE)で専用PSOを作成
```

**DispatchRays時:** `SpecializationIndices[RayGenShaderIndex]` で専用PSOを選択。
`INDEX_NONE`の場合はデフォルトRTPSOを使用。

### 4.3 State Object作成 (CreateRayTracingStateObject)

```
CreateRayTracingStateObject(Device5, Libraries, Exports, MaxAttr, MaxPayload,
                            HitGroups, GlobalRS, LocalRSs, Associations,
                            ExistingCollections, Type)
│
├─ サブオブジェクト構築:
│  ├─ D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS (増分追加用)
│  ├─ DXILライブラリ (リネーム済みエクスポート付き)
│  ├─ D3D12_HIT_GROUP_DESC (CHS/AHS/IS)
│  ├─ D3D12_RAYTRACING_SHADER_CONFIG (MaxPayload, MaxAttribute)
│  ├─ D3D12_RAYTRACING_PIPELINE_CONFIG (MaxTraceRecursionDepth=1)
│  ├─ グローバルルートシグネチャ
│  ├─ ローカルルートシグネチャ + 関連付け (Associations)
│  ├─ D3D12_EXISTING_COLLECTION_DESC (事前コンパイル済みコレクション)
│  └─ NVAPI: Shader Execution Reordering有効時の設定
│
└─ Device5->CreateStateObject(&Desc, IID_PPV_ARGS(&StateObject))
```

## 5. アクセラレーション構造 — BLAS (FD3D12RayTracingGeometry)

### 5.1 クラス構造

```
class FD3D12RayTracingGeometry : public FRHIRayTracingGeometry, public FD3D12AdapterChild,
    public FD3D12ShaderResourceRenameListener, public FNoncopyable
{
    // GPU毎のASバッファ
    TRefCountPtr<FD3D12Buffer> AccelerationStructureBuffers[MAX_NUM_GPUS];

    // ジオメトリ記述子テンプレート
    TArray<D3D12_RAYTRACING_GEOMETRY_DESC> GeometryDescs;

    // ヒットグループシステムパラメータ (GPU × セグメント)
    TArray<FD3D12HitGroupSystemParameters> HitGroupSystemParameters[MAX_NUM_GPUS];

    // SRV (バインドレス用)
    TSharedPtr<FD3D12ShaderResourceView> HitGroupSystemIndexBufferSRV[MAX_NUM_GPUS];
    TArray<TSharedPtr<FD3D12ShaderResourceView>> HitGroupSystemSegmentVertexBufferSRVs[MAX_NUM_GPUS];

    // コンパクション状態
    bool bHasPendingCompactionRequests[MAX_NUM_GPUS];
    uint64 AccelerationStructureCompactedSize = 0;

    // サイズ情報
    FRayTracingAccelerationStructureSize SizeInfo;

    // リネームリスナー登録状態
    bool bRegisteredAsRenameListener[MAX_NUM_GPUS];

    // SBT通知用リスナー (FCriticalSection保護)
    mutable FCriticalSection UpdateListenersCS;
    mutable TArray<ID3D12RayTracingGeometryUpdateListener*> UpdateListeners;

    // NullTransformBuffer (BLAS変換用、全ゼロ)
    static FBufferRHIRef NullTransformBuffer;
};
```

### 5.2 構築フロー

```
FD3D12RayTracingGeometry(RHICmdList, Adapter, Initializer)
│
├─ GeometryDescs作成 (TranslateRayTracingGeometryDescs)
│  ├─ Triangle: VertexFormat, IndexFormat, VertexCount
│  └─ Procedural: AABBCount
│
├─ [StreamingDestination] → SizeInfoのみ計算して早期リターン
│
├─ SizeInfo計算:
│  ├─ [OfflineData] → OfflineBvhMetadata.Sizeを使用
│  └─ [通常] → RHICalcRayTracingGeometrySize() (全GPU/インデックスフォーマットの最大値)
│
├─ AccelerationStructureBuffers[] 確保 (各GPU)
│  └─ CreateRayTracingBuffer(AccelerationStructure)
│
├─ [OfflineData あり]:
│  ├─ アップロードヒープにコピー
│  └─ CopyRaytracingAccelerationStructure(DESERIALIZE)
│
└─ RegisterAsRenameListener (頂点/インデックスバッファのリネーム監視)
```

### 5.3 ビルドフラグ決定

```
GetRayTracingAccelerationStructureBuildFlags(Initializer):
│
├─ bFastBuild ? FastBuild : FastTrace
├─ bAllowUpdate → AllowUpdate追加
├─ !FastBuild && !AllowUpdate && bAllowCompaction
│  && GD3D12RayTracingAllowCompaction
│  && PrimitiveCount > CompactionMinPrimitiveCount
│  → AllowCompaction追加
│
└─ GRayTracingDebugForceBuildMode によるオーバーライド:
   ├─ 1 → FastBuild強制
   └─ 2 → FastTrace強制
```

### 5.4 BLAS構築 (RHIBuildAccelerationStructures)

```
RHIBuildAccelerationStructures(Params, ScratchBufferRange)
│
├─ 各ジオメトリ: セグメント更新 + UnregisterAsRenameListener
│
├─ FlushResourceBarriers()
│
├─ 各ジオメトリで:
│  ├─ RegisterAsRenameListener
│  ├─ SetupHitGroupSystemParameters (IB/VBアドレス更新)
│  ├─ CreateAccelerationStructureBuildDesc:
│  │  ├─ GeometryDescsにGPUアドレスを設定
│  │  ├─ PrebuildInfo検証 (サイズがコンストラクタ計算値以下)
│  │  └─ D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC作成
│  └─ UpdateResidency (IB/VB/AS)
│
├─ [GPUバリデーション] → FRayTracingValidateGeometryBuildParamsCS::Dispatch
│
├─ BuildAccelerationStructuresInternal
│  └─ GraphicsCommandList4()->BuildRaytracingAccelerationStructure()
│
├─ 各ジオメトリ: ShouldCompactAfterBuild → RequestCompact
│
└─ AddGlobalBarrier(BVHWrite → BVHRead)
```

### 5.5 SetupHitGroupSystemParameters

頂点/インデックスバッファのGPUアドレスまたはバインドレスハンドルをキャッシュする。

```
SetupHitGroupSystemParameters(GPUIndex):
│
├─ [Bindless] → AllocateBufferSRVs (IB/VB用SRV作成)
│
├─ 各セグメント:
│  ├─ RootConstants.SetVertexAndIndexStride(VBStride, IBStride)
│  ├─ [Bindless]
│  │  ├─ BindlessHitGroupSystemVertexBuffer = SRV.GetBindlessHandle().GetIndex()
│  │  └─ BindlessHitGroupSystemIndexBuffer = SRV.GetBindlessHandle().GetIndex()
│  └─ [従来モード]
│     ├─ VertexBuffer = VB.GetGPUVirtualAddress() + Offset
│     └─ IndexBuffer = IB.GetGPUVirtualAddress()
│  ├─ IndexBufferOffsetInBytes計算
│  └─ FirstPrimitive設定
│
└─ HitGroupParamatersUpdated() → 登録済みSBTリスナーに通知
```

### 5.6 リネームリスナーとコンパクション

**リネームリスナー:** 頂点/インデックスバッファがデフラグ等でリネームされた場合、
`ResourceRenamed()` コールバックで `SetupHitGroupSystemParameters()` を再実行。
バインドレスモードではリスナー不要（SRVハンドルは不変）。

**コンパクション:**
```
CompactAccelerationStructure(CommandContext, GPUIndex, SizeAfterCompaction):
│
├─ bHasPendingCompactionRequests[GPUIndex] = false
├─ 旧ASバッファを退避
├─ 新バッファをSizeAfterCompactionで確保
├─ CopyRaytracingAccelerationStructure(COMPACT)
└─ AccelerationStructureCompactedSize = SizeAfterCompaction
```

## 6. FD3D12RayTracingCompactionRequestHandler

非同期BLASコンパクションの管理。

```
class FD3D12RayTracingCompactionRequestHandler : FD3D12DeviceChild  // private継承
{
    FCriticalSection CS;                                        // スレッドセーフ
    TArray<FD3D12RayTracingGeometry*> PendingRequests;          // 未処理リクエスト
    TArray<FD3D12RayTracingGeometry*> ActiveRequests;           // GPU実行中
    TArray<D3D12_GPU_VIRTUAL_ADDRESS> ActiveBLASGPUAddresses;   // BLASアドレス配列

    TRefCountPtr<FD3D12Buffer>  PostBuildInfoBuffer;                // PostBuildInfo結果バッファ
    FStagingBufferRHIRef        PostBuildInfoStagingBuffer;         // CPU読み取り用ステージング
    FD3D12SyncPointRef          PostBuildInfoBufferReadbackSyncPoint; // 完了待ちSyncPoint
};
```

### 6.1 Update フロー

```
Update(CommandContext):
│
├─ [ActiveRequests完了 (PostBuildInfoBufferReadbackSyncPoint.IsComplete)]
│  ├─ PostBuildInfoStagingBufferからコンパクションサイズ読み取り
│  ├─ 各ジオメトリに CompactAccelerationStructure 呼び出し
│  └─ ActiveRequests/PostBuildInfoBuffer クリア
│
└─ [PendingRequests > 0]
   ├─ バッチサイズ = min(PendingRequests, GD3D12RayTracingMaxBatchedCompaction)
   ├─ PostBuildInfoBuffer確保
   ├─ 各ジオメトリで:
   │  EmitRaytracingAccelerationStructurePostbuildInfo(
   │      POSTBUILD_INFO_COMPACTED_SIZE)
   ├─ PostBuildInfoStagingBufferにコピー
   ├─ PostBuildInfoBufferReadbackSyncPoint作成
   └─ PendingRequests → ActiveRequestsに移動
```

## 7. アクセラレーション構造 — TLAS (FD3D12RayTracingScene)

### 7.1 クラス構造

```
class FD3D12RayTracingScene : public FRHIRayTracingScene, public FD3D12AdapterChild, public FNoncopyable
{
    FRayTracingSceneInitializer Initializer;
    FRayTracingAccelerationStructureSize SizeInfo;

    TRefCountPtr<FD3D12Buffer> AccelerationStructureBuffers[MAX_NUM_GPUS];
    uint32 BufferOffset = 0;

    // TLAS構築時にジオメトリから収集
    TArray<FRHIRayTracingGeometry*> ReferencedGeometries;
    TArray<const FD3D12Resource*> ResourcesToMakeResident[MAX_NUM_GPUS];

    uint32 NumInstances = 0;
    bool bBuilt = false;
};
```

### 7.2 TLAS構築フロー

```
RHIBuildAccelerationStructures(TConstArrayView<FRayTracingSceneBuildParams>):
│
├─ 各シーン:
│  ├─ ReferencedGeometries収集
│  ├─ PrepareAccelerationStructureBuild:
│  │  ├─ ScratchBuffer自動割当 (未指定時)
│  │  ├─ PrebuildInfo検証
│  │  ├─ 各ReferencedGeometry:
│  │  │  ├─ IsDirty == false の検証
│  │  │  ├─ UpdateResidency
│  │  │  └─ ResourcesToMakeResident に追加 (固有ResidencyHandle)
│  │  ├─ [GPU Validation] → FRayTracingValidateSceneBuildParamsCS::Dispatch
│  │  └─ D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC構築
│  │     (TOP_LEVEL, InstanceDescs, BuildFlags)
│  │
│  └─ MemoryBinding: BindBuffer → 外部バッファでAS格納先指定
│
├─ AddGlobalBarrier(BVHWrite → BVHRead) [前バリア]
├─ FlushResourceBarriers
├─ BuildAccelerationStructuresInternal
├─ AddGlobalBarrier(BVHWrite → BVHRead) [後バリア]
│
└─ Scene->bBuilt = true
   [D3D12_RHI_SUPPORT_RAYTRACING_SCENE_DEBUGGING] → DebugSerializeScene
```

## 8. Shader Binding Table (SBT)

### 8.1 アーキテクチャ

```
┌── FD3D12RayTracingShaderBindingTable (FRHIShaderBindingTable) ──┐
│                                                                 │
│  ShaderTablesPerGPU[MAX_NUM_GPUS] ──→ PerGPUの内部テーブル       │
│                                                                 │
│  ┌── FD3D12RayTracingShaderBindingTableInternal ──────────────┐ │
│  │                                                            │ │
│  │  Data[] (TResourceArray)                                   │ │
│  │  ┌──────────────────────────────────────────────────┐      │ │
│  │  │ HitGroup Table  [HitGroupShaderTableOffset]      │      │ │
│  │  │  Record 0: [ShaderID 32B][LocalRS params...]     │      │ │
│  │  │  Record 1: [ShaderID 32B][LocalRS params...]     │      │ │
│  │  │  ...                                             │      │ │
│  │  ├──────────────────────────────────────────────────┤      │ │
│  │  │ Callable Table [CallableShaderTableOffset]       │      │ │
│  │  │  Record 0: [ShaderID 32B][LocalRS params...]     │      │ │
│  │  │  ...                                             │      │ │
│  │  ├──────────────────────────────────────────────────┤      │ │
│  │  │ Miss Table [MissShaderTableOffset]               │      │ │
│  │  │  Record 0: [ShaderID 32B][LocalRS params...]     │      │ │
│  │  │  ...                                             │      │ │
│  │  └──────────────────────────────────────────────────┘      │ │
│  │                                                            │ │
│  │  InlineGeometryParameterData[] (インラインSBT用)           │ │
│  │  DescriptorCache (FD3D12ExplicitDescriptorCache*)          │ │
│  │  Buffer (GPU側SBTバッファ)                                 │ │
│  │  WorkerData[MaxBindingWorkers=5]                           │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 8.2 レコードレイアウト

```
各SBTレコード:
┌──────────────────┬─────────────────────────────────┐
│  ShaderIdentifier │  Local Root Signature Params    │
│  (32 bytes)       │  (FD3D12HitGroupSystemParams +  │
│                   │   UB addresses, descriptor      │
│                   │   tables, loose parameters)     │
└──────────────────┴─────────────────────────────────┘
│←── LocalRecordStride (D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT aligned) ──→│
```

**アライメント要件:**
- レコードストライド: `D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT` アライン
- テーブル間: `D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT` アライン
- レコード内パラメータ: DWORD (4バイト) アライン
- ローカルルートシグネチャ最大サイズ: 4KB

### 8.3 SBTバインディングモード

| ERayTracingShaderBindingMode | 説明 |
|------------------------------|------|
| `RTPSO` | 従来のDXR SBT (ShaderID + パラメータ) |
| `Inline` | インラインレイトレーシング用ジオメトリパラメータ |
| `RTPSO \| Inline` | 両方 (ハイブリッド) |

### 8.4 SBTライフタイムとバインディング種別

| ERayTracingShaderBindingTableLifetime | 説明 |
|--------------------------------------|------|
| `Transient` | フレームごとに完全再構築 |
| `Persistent` | レコード単位の更新リスナー付き永続化 |

| ERayTracingLocalShaderBindingType | 説明 |
|----------------------------------|------|
| `Transient` | 一時的バインディング（毎フレーム設定） |
| `Persistent` | 永続バインディング（リスナーでデフラグ等に追従） |
| `Validation` | DO_CHECKビルドのみ：既存データとの整合性検証 |
| `Clear` | ヒットレコードデータのクリア |

### 8.5 永続SBTの更新リスナー

永続SBTは以下のリスナーでリソース変更を自動追跡:

| リスナー | トリガー | 動作 |
|---------|---------|------|
| `FRecordUpdateUniformBufferListener` | UniformBuffer更新 | GPUアドレス再書込み |
| `FRecordUpdateRayTracingGeometryListener` | IB/VBリネーム/ストリームイン | HitGroupSystemParameters再設定 |
| `FRecordUpdateShaderResourceRenameListener` | ShaderResourceリネーム | レジデンシ参照更新 |
| `FRecordUpdateTextureReferenceReplaceListener` | テクスチャ参照差替え | レジデンシ参照更新 |

### 8.6 ShaderRecordCache

SBTレコードの重複排除キャッシュ。

```
struct FShaderRecordCacheKey {
    FRHIUniformBuffer* const* UniformBuffers[MaxUniformBuffers=6];
    uint64 Hash;         // XxHash64 of UB pointers
    uint32 ShaderIndex;
};
```

**有効条件:**
- `GRayTracingCacheShaderRecords == 1`
- LooseParameterDataSize == 0
- NumUniformBuffers > 0 && <= 6
- Transient SBTまたはTransientバインディング

ヒット時: `CopyHitGroupParameters()` で既存レコードのパラメータ部分をコピー。

### 8.7 Commit (GPUアップロード)

```
Commit(Context, InlineBindingDataBuffer):
│
├─ [ENABLE_RESIDENCY_MANAGEMENT]
│  ├─ ワーカースレッドデータをマージ (Dynamic/Persistent ReferencedResources)
│  └─ ReferencedResources再構築
│
├─ MaxUsedHitRecordIndex集約
├─ ShaderRecordCacheクリア
│
├─ [RTPSO mode]
│  ├─ SBTバッファ作成 (BUF_Static, CopyDest初期状態)
│  ├─ UploadResourceDataViaCopyQueue → BatchedSyncPoints
│  └─ バリア: CopyDest → SRVCompute
│
└─ [Inline mode]
   ├─ InlineGeometryParameterData → アップロードヒープにコピー
   └─ CopyBufferRegion → InlineBindingDataBuffer
```

## 9. DispatchRays

### 9.1 リソースバインダー

2種類のリソースバインダーがテンプレートパラメータとして使用される:

| バインダー | 用途 | バインド先 |
|-----------|------|----------|
| `FD3D12RayTracingGlobalResourceBinder` | RayGenのグローバルリソース | コマンドリストのComputeRootパラメータ |
| `FD3D12RayTracingLocalResourceBinder` | HitGroup/Miss/Callableのローカルリソース | SBTレコード内のバイナリデータ |

### 9.2 SetRayTracingShaderResources テンプレート

```
SetRayTracingShaderResources<ResourceBinderType>(
    Shader, RootSignature,
    BindlessParameters, Textures, SRVs, UniformBuffers, Samplers, UAVs,
    LooseParameterData, Binder)
│
├─ FBindings構造体にリソース収集:
│  ├─ SRV/UAV/Sampler → CPU OfflineDescriptor + バウンドマスク
│  ├─ CBV → RemoteCBVs[] (GPU仮想アドレス)
│  ├─ Texture → SRVとして処理
│  └─ RayTracingScene参照を収集
│
├─ SetUniformBufferResourcesFromTables (テーブル経由リソース展開)
│
├─ LooseParameterData → ConstantBufferアップロード
│
├─ IsCompleteBinding検証 (全スロットがバウンド)
│
├─ ExplicitDescriptorCacheでディスクリプタ割当:
│  ├─ SRVs → AllocateDeduplicated → SetRootDescriptorTable
│  ├─ UAVs → AllocateDeduplicated → SetRootDescriptorTable
│  ├─ CBVs → CBVRDBaseBindSlot → SetRootCBV (個別)
│  └─ Samplers → AllocateDeduplicated → SetRootDescriptorTable
│
└─ RayTracingScene参照のレジデンシ更新
```

### 9.3 DispatchRays フロー

```
DispatchRays(CommandContext, GlobalBindings, Pipeline, RayGenShaderIndex,
             OptShaderTable, DispatchDesc, QueueType, [ArgumentBuffer])
│
├─ [Indirect Dispatch]
│  ├─ DispatchRaysDescBuffer取得 (キュー固有の固定バッファ)
│  ├─ SBTデータ部分をアップロードメモリにコピー
│  ├─ ArgumentBufferからDispatch次元をコピー
│  └─ バリア: CopyDest → IndirectArgs
│
├─ StateCache.TransitionComputeState(ED3D12PipelineType::RayTracing)
│
├─ ShaderBindingLayout検証 (RayGenShader.Hash == ShaderBindingLayout.Hash)
│
├─ [SBT DescriptorCache あり]
│  ├─ DispatchMutexロック
│  ├─ SetExplicitDescriptorCache
│  ├─ SetComputeRootSignature(GlobalRS)
│  ├─ FD3D12RayTracingGlobalResourceBinder でリソースバインド
│  └─ SBT UpdateResidency
│
├─ [SBT DescriptorCache なし]
│  └─ TransientDescriptorCache作成 → 同上のバインドフロー
│
├─ BindDiagnosticBuffer (アサート用)
├─ 静的UniformBuffersバインド (ShaderBindingLayout)
│
├─ [特殊化RTPSO選択]
│  ├─ SpecializationIndices[RayGenShaderIndex] != INDEX_NONE → 特殊化PSO
│  └─ それ以外 → デフォルトStateObject
│
├─ SetPipelineState1(RayTracingStateObject)
│
└─ [Indirect] → ExecuteIndirect(CommandSignature, 1, DescBuffer)
   [Direct]   → DispatchRays(&DispatchDesc)
```

### 9.4 GetDispatchRaysDesc

```
D3D12_DISPATCH_RAYS_DESC構築:
│
├─ RayGenShaderRecord:
│  ├─ FastAllocatorからRayGenRecordStride(64B)確保
│  ├─ ShaderIdentifier(32B)をコピー
│  └─ StartAddress = アップロードGPUアドレス
│
├─ MissShaderTable:
│  ├─ StartAddress = SBT GPUアドレス + MissShaderTableOffset
│  ├─ StrideInBytes = LocalRecordStride
│  └─ SizeInBytes = LocalRecordStride × NumMissRecords
│
├─ CallableShaderTable (NumCallableRecords > 0):
│  └─ 同様のレイアウト
│
└─ HitGroupTable:
   ├─ [Allow indexing]
   │  ├─ StrideInBytes = LocalRecordStride
   │  └─ SizeInBytes = NumHitRecords × LocalRecordStride
   └─ [Disallow indexing]
      ├─ StrideInBytes = 0 (インデクシング無効化)
      └─ SizeInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT (最小)
```

## 10. RHISetBindingsOnShaderBindingTable

SBTレコードへのバインディング設定。並列ワーカーで実行可能。

```
RHISetBindingsOnShaderBindingTable(SBT, Pipeline, NumBindings, Bindings, BindingType):
│
├─ ワーカーコンテキスト生成 (最大MaxBindingWorkers=5)
│
├─ ParallelForWithExistingTaskContext (1024 items/task):
│  ├─ [HitGroup]:
│  │  ├─ [Clear] → ClearHitRecordData
│  │  └─ [その他] → SetRayTracingHitGroup:
│  │     ├─ SetHitGroupGeometrySystemParameters
│  │     ├─ [Record Cache ヒット] → CopyHitGroupParameters
│  │     └─ [キャッシュミス] →
│  │        ├─ FD3D12RayTracingLocalResourceBinder構築
│  │        ├─ SetRayTracingShaderResources (UB/LooseParams)
│  │        ├─ SetHitGroupIdentifier
│  │        └─ キャッシュ登録
│  │
│  ├─ [CallableShader] → SetRayTracingCallableShader
│  │  └─ SetCallableIdentifier + ローカルリソースバインド
│  │
│  └─ [MissShader] → SetRayTracingMissShader
│     └─ SetMissIdentifier + ローカルリソースバインド
│
└─ ShaderTable->bIsDirty = true
```

## 11. NVAPIクラスタ操作

NVAPI拡張によるクラスタレベルアクセラレーション構造 (CLAS) 操作。

### 11.1 操作タイプ

| ERayTracingClusterOperationType | 説明 |
|-------------------------------|------|
| `MOVE` | BLAS/CLAS/テンプレートの移動 |
| `CLAS_BUILD` | 三角形からCLAS構築 |
| `CLAS_BUILD_TEMPLATES` | 三角形からCLASテンプレート構築 |
| `CLAS_INSTANTIATE_TEMPLATES` | テンプレートからCLASインスタンス化 |
| `BLAS_BUILD` | CLASからBLAS構築 |

### 11.2 操作モード

| ERayTracingClusterOperationMode | 説明 |
|-------------------------------|------|
| `IMPLICIT_DESTINATIONS` | 出力先を自動決定 |
| `EXPLICIT_DESTINATIONS` | 出力先を明示指定 |
| `GET_SIZES` | サイズ取得のみ |

### 11.3 実行フロー

```
RHIExecuteMultiIndirectClusterOperation(Params):
│
├─ 入力バリデーション (ResultCount, Descriptors, Scratch, Addresses)
├─ NVAPI入力構造体構築:
│  ├─ maxArgCount, mode, flags
│  └─ 操作種別に応じたディスクリプタ変換
├─ レジデンシ更新 (全入出力バッファ + AdditionalResources)
├─ FlushResourceBarriers
├─ NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation
└─ AddGlobalBarrier(UAVCompute → UAVCompute)
```

### 11.4 サイズ計算

```
RHICalcRayTracingClusterOperationSize(Initializer):
├─ 操作種別に応じた NVAPI_..._INPUTS 構築
├─ NvAPI_D3D12_GetRaytracingMultiIndirectClusterOperationRequirementsInfo
└─ FRayTracingClusterOperationSize {ResultMaxSizeInBytes, ScratchSizeInBytes}
```

## 12. デバッグユーティリティ

### 12.1 シーンシリアライズ

`D3D12.RayTracing.SerializeScene` コンソールコマンドでTLASシーンをディスクに保存。

```
DebugSerializeScene():
├─ InstanceBufferをStagingBufferにコピー
│  (AccelerationStructureアドレスをジオメトリインデックスに変換)
├─ 各ReferencedGeometry: セグメント情報シリアライズ
├─ 参照バッファ(IB/VB)のGPUデータをStagingBuffer経由でコピー
├─ 文字列テーブル (DebugName, OwnerName)
└─ .uehwrtscene ファイルとして保存
```

**ファイルフォーマット:**
- 16バイトアライメント保証
- SceneHeader: マジック + バージョン + セクションオフセット
- Instances: D3D12_RAYTRACING_INSTANCE_DESC配列
- Geometries: ジオメトリ記述子 + セグメント情報
- Buffers: GPU バッファデータ
- Strings: ANSI文字列テーブル

### 12.2 ジオメトリトラッキング

```
RegisterD3D12RayTracingGeometry(Geometry)   // グローバルリストに追加
UnregisterD3D12RayTracingGeometry(Geometry) // グローバルリストから除去

コンソールコマンド:
D3D12.DumpRayTracingGeometries  // 全ジオメトリのダンプ (名前、サイズ、プリミティブ数)
```

## 13. CVar一覧

| CVar | 型 | デフォルト | 説明 |
|------|-----|---------|------|
| `r.D3D12.RayTracing.DebugForceBuildMode` | int32 | 0 | 0=デフォルト, 1=FastBuild強制, 2=FastTrace強制 |
| `r.D3D12.RayTracing.CacheShaderRecords` | int32 | 1 | SBTレコードキャッシュの有効化 |
| `r.D3D12.RayTracing.AllowCompaction` | int32 | 1 | BLASコンパクションの有効化 |
| `r.D3D12.RayTracing.MaxBatchedCompaction` | int32 | 64 | 1バッチのコンパクション最大数 |
| `r.D3D12.RayTracing.CompactionMinPrimitiveCount` | int32 | 128 | コンパクション対象の最小プリミティブ数 |
| `r.D3D12.RayTracing.SpecializeStateObjects` | int32 | 0 | RTPSO特殊化の有効化 |
| `r.D3D12.RayTracing.AllowSpecializedStateObjects` | int32 | 1 | 特殊化PSOのDispatch使用許可 |
| `r.D3D12.RayTracing.GPUValidation` | int32 | 0 | GPUバリデーション有効化 |
| `r.D3D12.ExplicitDescriptorHeap.ViewDescriptorHeapSize` | int32 | 250000 | RT用Viewディスクリプタヒープサイズ (~8MB/ヒープ) |
