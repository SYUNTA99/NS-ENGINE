# UE5 RHI 実装ガイド Part 10: 高度なリソース管理

## 目次
1. [Reserved/Sparse Resources](#1-reservedsparse-resources)
2. [Resource Collections](#2-resource-collections)
3. [Transient Resource Allocator 詳細](#3-transient-resource-allocator-詳細)
4. [高度なテクスチャ/バッファフラグ](#4-高度なテクスチャバッファフラグ)
5. [リソース遷移の高度な使用](#5-リソース遷移の高度な使用)

---

## 1. Reserved/Sparse Resources

### 1.1 概要

Reserved Resources（別名: Tiled, Virtual, Sparse Resources）は、物理メモリを事前に割り当てずにリソースを作成する実験的機能です。

```cpp
// RHIGlobals.h
struct FReservedResources
{
    // Reserved Resources がサポートされているか
    // バッファと2Dテクスチャ（ミップなし）で使用可能
    bool Supported = false;

    // ボリュームテクスチャでの Reserved Resource サポート
    bool SupportsVolumeTextures = false;

    // Reserved Resource のタイルサイズ（固定）
    static constexpr int32 TileSizeInBytes = 65536;  // 64KB
};
```

### 1.2 サポート検出

```cpp
// Reserved Resources が使用可能か確認
if (GRHIGlobals.ReservedResources.Supported)
{
    // バッファまたは2Dテクスチャで Reserved Resource を使用可能
}

if (GRHIGlobals.ReservedResources.SupportsVolumeTextures)
{
    // 3Dテクスチャでも Reserved Resource を使用可能
}
```

### 1.3 作成フラグ

```cpp
// バッファ作成時
FRHIBufferCreateInfo BufferInfo;
BufferInfo.Size = MaxExpectedSize;  // 最大予想サイズで作成
BufferInfo.Usage = BUF_ReservedResource | BUF_ShaderResource;

// テクスチャ作成時
FRHITextureCreateInfo TextureInfo;
TextureInfo.Flags = TexCreate_ReservedResource | TexCreate_ShaderResource;

// 即時コミット（作成時にメモリを割り当て）
TextureInfo.Flags |= TexCreate_ImmediateCommit;
```

### 1.4 メモリコミット操作

```cpp
// RHITransition.h
struct FRHICommitResourceInfo
{
    uint64 SizeInBytes = 0;
    explicit FRHICommitResourceInfo(uint64 InSizeInBytes)
        : SizeInBytes(InSizeInBytes) {}
};
```

**コミット操作の特徴:**
- 物理メモリはタイル（65536バイト）単位で割り当て
- リソースの末尾にマップ
- 拡大・縮小が可能（データ移動なし）
- SRV/UAV の再作成不要
- 新しくコミットされた領域の内容は未定義

### 1.5 遷移時のコミット

```cpp
// バッファのコミットサイズを変更しながら遷移
FRHITransitionInfo TransitionInfo(
    Buffer,
    ERHIAccess::Unknown,
    ERHIAccess::SRVCompute,
    FRHICommitResourceInfo(NewCommitSize)  // コミットサイズ指定
);

RHICmdList.Transition(MakeArrayView(&TransitionInfo, 1));
```

### 1.6 ユースケース

| ユースケース | 説明 |
|-------------|------|
| メガテクスチャ | 巨大テクスチャの部分ロード |
| 仮想テクスチャ | オンデマンドのミップマップロード |
| ストリーミング | 大規模シーンの段階的ロード |
| スパースボクセル | 3Dデータの効率的な格納 |

### 1.7 プラットフォームサポート

| プラットフォーム | バッファ | 2Dテクスチャ | 3Dテクスチャ |
|-----------------|---------|-------------|-------------|
| D3D12 | ✅ | ✅ | ✅ (Tier 3) |
| Vulkan | ✅ | ✅ | 部分的 |
| Metal | ❌ | ❌ | ❌ |

---

## 2. Resource Collections

### 2.1 概要

Resource Collections は、テクスチャ、サンプラー、SRV をグループ化してバインドレスレンダリングを効率化する機能です。

```cpp
// RHIResourceCollection.h

// メンバータイプ
struct FRHIResourceCollectionMember
{
    enum class EType : uint8
    {
        Texture,            // FRHITexture*
        TextureReference,   // FRHITextureReference*
        ShaderResourceView, // FRHIShaderResourceView*
        Sampler,            // FRHISamplerState*
    };

    FRHIResource* Resource = nullptr;
    EType Type = EType::Texture;
};
```

### 2.2 FRHIResourceCollection クラス

```cpp
class FRHIResourceCollection : public FRHIResource
{
public:
    // コンストラクタ - メンバー配列で初期化
    FRHIResourceCollection(TConstArrayView<FRHIResourceCollectionMember> InMembers);

    // 単一メンバーの更新
    void UpdateMember(int32 Index, FRHIResourceCollectionMember InMember);

    // 複数メンバーの一括更新
    void UpdateMembers(int32 StartIndex,
                       TConstArrayView<FRHIResourceCollectionMember> NewMembers);

    // バインドレスディスクリプタハンドル取得
    virtual FRHIDescriptorHandle GetBindlessHandle() const;

    // メンバー配列取得
    TConstArrayView<FRHIResourceCollectionMember> GetMembers() const;

private:
    TArray<FRHIResourceCollectionMember> Members;
};
```

### 2.3 使用例

```cpp
// 1. コレクションメンバーを準備
TArray<FRHIResourceCollectionMember> Members;
Members.Add(FRHIResourceCollectionMember(Texture1));       // テクスチャ
Members.Add(FRHIResourceCollectionMember(SRV1));           // SRV
Members.Add(FRHIResourceCollectionMember(SamplerState1));  // サンプラー

// 2. Resource Collection 作成
FRHIResourceCollection* Collection =
    new FRHIResourceCollection(Members);

// 3. バインドレスハンドル取得
FRHIDescriptorHandle Handle = Collection->GetBindlessHandle();

// 4. シェーダーでハンドルを使用
// HLSL: Texture2D Textures[] = ResourceDescriptorHeap[Handle];

// 5. 動的更新
Collection->UpdateMember(0, FRHIResourceCollectionMember(NewTexture));
```

### 2.4 リソースタイプ

```cpp
enum class ERHIResourceType
{
    // ... 他のタイプ ...
    RRT_ResourceCollection,  // Resource Collection タイプ
};
```

### 2.5 ユースケース

| ユースケース | 説明 |
|-------------|------|
| マテリアルシステム | マテリアルパラメータのグループ化 |
| GPU駆動レンダリング | インスタンス毎のリソースバインディング |
| メガテクスチャ | タイルテクスチャの動的管理 |
| デカールシステム | デカールテクスチャのバッチ処理 |

---

## 3. Transient Resource Allocator 詳細

### 3.1 FRHITransientAllocationFences

マルチパイプラインでの一時リソースエイリアシング同期を管理します。

```cpp
// RHITransientResourceAllocator.h

class FRHITransientAllocationFences
{
public:
    // Discard → Acquire 遷移時の取得フェンスを計算
    static uint32 GetAcquireFence(
        const FRHITransientAllocationFences& Discard,
        const FRHITransientAllocationFences& Acquire);

    // 二つの領域が重複するか判定
    static bool Contains(
        const FRHITransientAllocationFences& Discard,
        const FRHITransientAllocationFences& Acquire);

    // パイプライン指定でコンストラクト
    FRHITransientAllocationFences(ERHIPipeline InPipelines);

    // グラフィックスフェンス設定
    void SetGraphics(uint32 InGraphics);

    // Async Compute フェンス設定（Fork/Join 範囲含む）
    void SetAsyncCompute(uint32 InAsyncCompute,
                         TInterval<uint32> InGraphicsForkJoin);

    // パイプライン取得
    ERHIPipeline GetPipelines() const;
    uint32 GetSinglePipeline() const;
    bool IsSinglePipeline() const;

private:
    static const uint32 Invalid = std::numeric_limits<uint32>::max();
    uint32 Graphics = Invalid;
    uint32 AsyncCompute = Invalid;
    TInterval<uint32> GraphicsForkJoin;
    ERHIPipeline Pipelines = ERHIPipeline::None;
};
```

### 3.2 フェンス遷移パターン

```
パイプライン遷移マトリクス:

  Discard → Acquire    | 使用フェンス
-----------------------|------------------
  Graphics → Graphics  | Discard.Graphics
  Graphics → AsyncComp | Acquire.GraphicsForkJoin.Min
  AsyncComp → AsyncComp| Discard.AsyncCompute
  AsyncComp → Graphics | Discard.GraphicsForkJoin.Max
  All → Graphics       | Discard.GraphicsForkJoin.Max
  All → AsyncCompute   | Acquire.GraphicsForkJoin.Min
```

### 3.3 Transient Heap Allocation

```cpp
struct FRHITransientHeapAllocation
{
    bool IsValid() const { return Size != 0; }

    FRHITransientHeap* Heap = nullptr;   // 割り当て元ヒープ
    uint64 Size = 0;                      // 割り当てサイズ（アライン済み）
    uint64 Offset = 0;                    // ヒープ内オフセット
    uint32 AlignmentPad = 0;              // パディングバイト数
};
```

### 3.4 Page Pool Allocation

```cpp
struct FRHITransientPageSpan
{
    uint16 Offset = 0;  // ページプール内オフセット（ページ単位）
    uint16 Count = 0;   // ページ数
};

struct FRHITransientPagePoolAllocation
{
    bool IsValid() const { return Pool != nullptr; }

    FRHITransientPagePool* Pool = nullptr;  // ページプール
    uint64 Hash = 0;                         // 一意識別ハッシュ
    uint16 SpanIndex = 0;                    // スパンインデックス
    uint16 SpanOffsetMin = 0;                // スパン開始オフセット
    uint16 SpanOffsetMax = 0;                // スパン終了オフセット
};

struct FRHITransientPageAllocation
{
    TArray<FRHITransientPagePoolAllocation> PoolAllocations;
    TArray<FRHITransientPageSpan> Spans;
};
```

### 3.5 Allocation Types

```cpp
enum class ERHITransientAllocationType : uint8
{
    Heap,  // ヒープベース割り当て
    Page   // ページプールベース割り当て
};

enum class ERHITransientResourceType : uint8
{
    Texture,
    Buffer
};
```

### 3.6 IRHITransientResourceAllocator インターフェース

```cpp
class IRHITransientResourceAllocator
{
public:
    // リソースタイプのサポート確認
    virtual bool SupportsResourceType(ERHITransientResourceType Type) const = 0;

    // 作成モード設定
    virtual void SetCreateMode(ERHITransientResourceCreateMode CreateMode) {};

    // リソース作成
    virtual FRHITransientTexture* CreateTexture(
        const FRHITextureCreateInfo& CreateInfo,
        const TCHAR* DebugName,
        const FRHITransientAllocationFences& Fences) = 0;

    virtual FRHITransientBuffer* CreateBuffer(
        const FRHIBufferCreateInfo& CreateInfo,
        const TCHAR* DebugName,
        const FRHITransientAllocationFences& Fences) = 0;

    // メモリ解放（将来の再利用用）
    virtual void DeallocateMemory(FRHITransientTexture* Texture,
                                  const FRHITransientAllocationFences& Fences) = 0;
    virtual void DeallocateMemory(FRHITransientBuffer* Buffer,
                                  const FRHITransientAllocationFences& Fences) = 0;

    // フラッシュ（レンダリング前）
    virtual void Flush(FRHICommandListImmediate& RHICmdList,
                       FRHITransientAllocationStats* OutStats = nullptr) = 0;

    // リリース
    virtual void Release(FRHICommandListImmediate& RHICmdList);
};
```

### 3.7 Transient Allocation Stats

```cpp
class FRHITransientAllocationStats
{
public:
    struct FAllocation
    {
        uint64 OffsetMin = 0;
        uint64 OffsetMax = 0;
        uint32 MemoryRangeIndex = 0;
    };

    enum class EMemoryRangeFlags
    {
        None = 0,
        FastVRAM = 1 << 0  // 高速VRAMを使用
    };

    struct FMemoryRange
    {
        uint64 Capacity = 0;    // 利用可能容量
        uint64 CommitSize = 0;  // コミット済みサイズ
        EMemoryRangeFlags Flags = EMemoryRangeFlags::None;
    };

    TArray<FMemoryRange> MemoryRanges;
    TMap<const FRHITransientResource*, FAllocationArray> Resources;
};
```

### 3.8 Resource Create Mode

```cpp
enum class ERHITransientResourceCreateMode
{
    // 常にインラインで作成
    Inline,

    // タスクにオフロード可能（プラットフォーム依存）
    // FRHITransientResource::Finish() を呼び出す必要あり
    Task
};
```

---

## 4. 高度なテクスチャ/バッファフラグ

### 4.1 テクスチャフラグ（高度）

| フラグ | 説明 | 用途 |
|--------|------|------|
| `TexCreate_Memoryless` | タイルメモリのみ使用（TBDR GPU） | モバイル最適化、一時レンダーターゲット |
| `TexCreate_Virtual` | 仮想テクスチャ | ストリーミングテクスチャ |
| `TexCreate_External` | 外部リソース（Vulkan） | プロセス間共有、ビデオデコーダー出力 |
| `TexCreate_InputAttachmentRead` | レンダーパス入力 | Vulkan サブパス入力 |
| `TexCreate_Foveation` | フォビエーション対応 | VRレンダリング最適化 |
| `TexCreate_AtomicCompatible` | アトミック操作対応 | 64bitアトミックテクスチャ |
| `TexCreate_DisableDCC` | Delta Color Compression 無効 | AMD GPU圧縮無効化 |
| `TexCreate_ReservedResource` | Reserved Resource | 仮想アドレス予約のみ |
| `TexCreate_ImmediateCommit` | 即時メモリコミット | Reserved Resource 用 |
| `TexCreate_Shared` | プロセス間共有 | 外部プロセス共有 |
| `TexCreate_Transient` | 一時リソース | Transient Allocator 管理 |

### 4.2 バッファフラグ（高度）

| フラグ | 説明 | 用途 |
|--------|------|------|
| `BUF_ReservedResource` | Reserved Resource | 仮想アドレス予約のみ |
| `BUF_RayTracingScratch` | RT AS ビルド用スクラッチ | Acceleration Structure ビルド |
| `BUF_NullResource` | Null リソース | ストリーミングプレースホルダー |
| `BUF_MultiGPUAllocate` | GPU毎に割り当て | マルチGPU分散 |
| `BUF_Shared` | プロセス間共有 | 外部プロセスアクセス |
| `BUF_UniformBuffer` | Uniform Buffer | 定数バッファ |

### 4.3 Virtual Texture 操作

```cpp
// 仮想テクスチャのメモリ割り当て/解放
// TexCreate_Virtual フラグで作成されたテクスチャ用
void RHIVirtualTextureSetFirstMipInMemory(
    FRHITexture* Texture,
    uint32 FirstMip  // メモリに存在する最初のミップ
);

// GPU から見える最初のミップを設定
void RHIVirtualTextureSetFirstMipVisible(
    FRHITexture* Texture,
    uint32 FirstMip  // GPUから見える最初のミップ
);
```

---

## 5. リソース遷移の高度な使用

### 5.1 ERHITransitionCreateFlags

```cpp
enum class ERHITransitionCreateFlags
{
    None = 0,

    // パイプライン間フェンスを無効化
    NoFence = 1 << 0,

    // Begin/End 間に有用な作業がない場合
    // フェンスではなくパーシャルフラッシュを使用
    NoSplit = 1 << 1,

    // RenderPass 中の遷移を許可
    AllowDuringRenderPass = 1 << 2,

    // 高度なモード: 単一パイプラインを前のパイプラインとして使用
    // ERHIPipeline::All に遷移されたリソース用
    // 手動フェンスが発行されていることを前提
    // （続きの定義はコメント参照）
};
```

### 5.2 FRHITransitionInfo 構造体

```cpp
struct FRHITransitionInfo : public FRHISubresourceRange
{
    union
    {
        FRHIResource* Resource = nullptr;
        FRHIViewableResource* ViewableResource;
        FRHITexture* Texture;
        FRHIBuffer* Buffer;
        FRHIUnorderedAccessView* UAV;
        FRHIRayTracingAccelerationStructure* BVH;
    };

    enum class EType : uint8
    {
        Unknown,
        Texture,
        Buffer,
        UAV,
        BVH,
    } Type = EType::Unknown;

    ERHIAccess AccessBefore = ERHIAccess::Unknown;
    ERHIAccess AccessAfter = ERHIAccess::Unknown;
    EResourceTransitionFlags Flags = EResourceTransitionFlags::None;

    // Reserved Resource 用コミット情報（オプション）
    TOptional<FRHICommitResourceInfo> CommitInfo;
};
```

### 5.3 Subresource Range

```cpp
struct FRHISubresourceRange
{
    static const uint16 kDepthPlaneSlice = 0;
    static const uint16 kStencilPlaneSlice = 1;
    static const uint16 kAllSubresources = TNumericLimits<uint16>::Max();

    uint16 MipIndex = kAllSubresources;
    uint16 ArraySlice = kAllSubresources;
    uint16 PlaneSlice = kAllSubresources;

    bool IsAllMips() const;
    bool IsAllArraySlices() const;
    bool IsAllPlaneSlices() const;
    bool IsWholeResource() const;
    bool IgnoreDepthPlane() const;   // Stencil のみ
    bool IgnoreStencilPlane() const; // Depth のみ
};
```

### 5.4 Transient Aliasing

```cpp
struct FRHITransientAliasingOverlap
{
    union
    {
        FRHIResource* Resource = nullptr;
        FRHITexture* Texture;
        FRHIBuffer* Buffer;
    };

    enum class EType : uint8 { Texture, Buffer } Type;
};

struct FRHITransientAliasingInfo
{
    union
    {
        FRHIResource* Resource = nullptr;
        FRHITexture* Texture;
        FRHIBuffer* Buffer;
    };

    // Acquire 時の前リソースオーバーラップ
    TArrayView<const FRHITransientAliasingOverlap> Overlaps;

    enum class EType : uint8 { Texture, Buffer } Type;
    enum class EAction : uint8 { Acquire, Discard } Action;

    // ファクトリ関数
    static FRHITransientAliasingInfo Acquire(
        FRHITexture* Texture,
        TArrayView<const FRHITransientAliasingOverlap> InOverlaps);
    static FRHITransientAliasingInfo Acquire(
        FRHIBuffer* Buffer,
        TArrayView<const FRHITransientAliasingOverlap> InOverlaps);
};
```

**注意:** `Discard` 操作は UE 5.5 で非推奨化されました。

### 5.5 Split Barrier パターン

```cpp
// 1. Transition 作成
FRHITransitionCreateInfo CreateInfo(
    ERHIPipeline::Graphics,    // ソースパイプライン
    ERHIPipeline::AsyncCompute, // デスティネーションパイプライン
    ERHITransitionCreateFlags::None,
    TransitionInfos
);
FRHITransition* Transition = RHICreateTransition(CreateInfo);

// 2. Begin Transition（ソースパイプライン上）
GraphicsCmdList.BeginTransitions(MakeArrayView(&Transition, 1));

// 3. 他の作業を実行...

// 4. End Transition（デスティネーションパイプライン上）
AsyncComputeCmdList.EndTransitions(MakeArrayView(&Transition, 1));
```

---

## 6. ベストプラクティス

### 6.1 Reserved Resources

1. **サポート確認必須** - 使用前に `GRHIGlobals.ReservedResources.Supported` を確認
2. **最大サイズで作成** - 将来の拡張を考慮したサイズで作成
3. **タイルアライメント** - 65536バイト境界でコミット
4. **内容未定義** - 新しくコミットした領域は上書き必須

### 6.2 Resource Collections

1. **バインドレス前提** - バインドレスが有効な場合のみ使用
2. **動的更新最小化** - 可能な限りバッチ更新を使用
3. **ライフタイム管理** - メンバーリソースより長く生存させる

### 6.3 Transient Resources

1. **フェンス正確性** - パイプライン間の同期を正確に設定
2. **Discard 非推奨** - UE 5.5 以降は Discard 操作を使用しない
3. **Stats 活用** - デバッグ時に `FRHITransientAllocationStats` を確認

---

## 7. 関連ドキュメント

- [Part 6: スレッディングとリソース管理](RHI_Implementation_Guide_Part6.md) - 基本的なリソース管理
- [Part 8: ベンダー固有機能](RHI_Implementation_Guide_Part8.md) - Enhanced Barriers
- [Part 9: ケイパビリティフラグ](RHI_Implementation_Guide_Part9.md) - サポートフラグ一覧
