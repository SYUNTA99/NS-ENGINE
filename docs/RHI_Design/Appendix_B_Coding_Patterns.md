# Appendix B: グラフィックスAPI抽象層 — コーディングパターン集

D3D12/Vulkan/Metal等を抽象化するレイヤーで使われる汎用的なC++パターン。
特定エンジンに依存しない、再利用可能な実装パターンのリファレンス。

---

## 0. 共通ユーティリティ

```cpp
// トークン連結マクロ
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

// 数学型（DirectXMath, glm, 自前定義のいずれか）
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct float4x4 { float m[4][4]; };
```

---

## 1. コマンドパターン

### 1.1 コマンドヘッダ + POD構造体 + 静的Execute

コマンドはvtableを持たない。`ERHICommandType` enum でコマンドを識別し、
`RHICommandHeader` をコマンド構造体の先頭に配置する。
実行は `static void Execute(IRHICommandContext* ctx, const CmdType& cmd)` パターン。

```cpp
/// コマンドタイプ識別子
enum class ERHICommandType : uint16
{
    Draw,
    DrawIndexed,
    DrawIndirect,
    Dispatch,
    DispatchIndirect,
    SetGraphicsPipelineState,
    TransitionResource,
    // ...
    Count
};

/// コマンドヘッダ（全コマンド構造体の先頭に配置）
struct RHICommandHeader
{
    ERHICommandType type;  ///< コマンドタイプ
    uint16 size;           ///< コマンド全体サイズ（ヘッダ含む）
    uint32 nextOffset;     ///< 次コマンドへのオフセット（0=終端）
};

/// コマンド構造体: POD + static Execute
struct RHI_API CmdDraw
{
    static constexpr ERHICommandType kType = ERHICommandType::Draw;
    RHICommandHeader header;
    uint32 vertexCount;
    uint32 instanceCount;
    uint32 startVertex;
    uint32 startInstance;

    static void Execute(IRHICommandContext* ctx, const CmdDraw& cmd)
    {
        NS_RHI_DISPATCH(Draw, ctx, cmd.vertexCount, cmd.instanceCount,
                         cmd.startVertex, cmd.startInstance);
    }
};
```

### 1.2 コマンド定義例

コマンド構造体は全て同じパターンで定義される。
コンストラクタは不要（POD構造体）。`static Execute` がディスパッチテーブル経由で呼び出す。

```cpp
struct RHI_API CmdSetViewports
{
    static constexpr ERHICommandType kType = ERHICommandType::SetViewports;
    RHICommandHeader header;
    uint32 count;
    const RHIViewport* viewports;

    static void Execute(IRHICommandContext* ctx, const CmdSetViewports& cmd)
    {
        NS_RHI_DISPATCH(SetViewports, ctx, cmd.count, cmd.viewports);
    }
};

struct RHI_API CmdTransitionResource
{
    static constexpr ERHICommandType kType = ERHICommandType::TransitionResource;
    RHICommandHeader header;
    IRHIResource* resource;
    ERHIAccess stateBefore;
    ERHIAccess stateAfter;

    static void Execute(IRHICommandContextBase* ctx, const CmdTransitionResource& cmd)
    {
        NS_RHI_DISPATCH(TransitionResource, ctx, cmd.resource,
                         cmd.stateBefore, cmd.stateAfter);
    }
};
```

### 1.3 コマンドアロケータ（IRHICommandAllocator + オフセットチェーン）

コマンドメモリは `IRHICommandAllocator` インターフェースが管理する。
コマンド間は `RHICommandHeader::nextOffset` でリニアにチェーンされる
（リンクドリストのポインタではなくオフセットベース）。

```cpp
/// コマンドアロケーターインターフェース
class RHI_API IRHICommandAllocator : public IRHIResource
{
public:
    /// リセット（GPU完了後にのみ呼び出し可能）
    virtual void Reset() = 0;

    /// GPU使用中か
    virtual bool IsInUse() const = 0;

    /// 割り当て済みメモリサイズ
    virtual uint64 GetAllocatedMemory() const = 0;

    /// 使用中メモリサイズ
    virtual uint64 GetUsedMemory() const = 0;

    /// 待機フェンス設定（アロケーター再利用タイミング追跡用）
    virtual void SetWaitFence(IRHIFence* fence, uint64 value) = 0;
};

/// アロケータープール（再利用管理）
class RHI_API IRHICommandAllocatorPool
{
public:
    virtual IRHICommandAllocator* Obtain(ERHIQueueType queueType) = 0;
    virtual void Release(IRHICommandAllocator* allocator,
                         IRHIFence* fence, uint64 fenceValue) = 0;
    virtual uint32 ProcessCompletedAllocators() = 0;
};
```

コマンドチェーンの走査:
```cpp
// nextOffset == 0 が終端
RHICommandHeader* cmd = firstCommand;
while (cmd->nextOffset != 0) {
    cmd = reinterpret_cast<RHICommandHeader*>(
        reinterpret_cast<uint8_t*>(cmd) + cmd->nextOffset);
}
```

---

## 2. ディスパッチテーブル経由の実行（NS_RHI_DISPATCH パターン）

`NS_RHI_DISPATCH` マクロ経由でグローバルディスパッチテーブル `GRHIDispatchTable` の
関数ポインタを呼び出す。リンクドリストや `IsImmediate()` チェックは使用しない。

```cpp
/// 開発ビルド: ディスパッチテーブル経由（関数ポインタ1回分）
#define NS_RHI_DISPATCH(func, ...) \
    ::NS::RHI::GRHIDispatchTable.func(__VA_ARGS__)

/// 出荷ビルド: バックエンド関数を直接呼び出し（インライン化可能）
/// #define NS_RHI_DISPATCH(func, ...) \
///     ::NS::RHI::NS_RHI_EXPAND(NS_RHI_STATIC_BACKEND, func)(__VA_ARGS__)

// コマンド構造体の static Execute 内で使用:
static void Execute(IRHICommandContext* ctx, const CmdDraw& cmd)
{
    NS_RHI_DISPATCH(Draw, ctx, cmd.vertexCount, cmd.instanceCount,
                     cmd.startVertex, cmd.startInstance);
}
```

---

## 3. ラムダコマンド

```cpp
template<typename Func>
struct LambdaCommand : CommandBase {
    Func func;
    const char* name;

    LambdaCommand(Func&& f, const char* n)
        : func(std::forward<Func>(f)), name(n) {}

    void Execute(CommandContext& ctx) override { func(ctx); }
    const char* GetName() const override { return name; }
};

// CommandList への追加
template<typename Func>
void CommandList::EnqueueLambda(const char* name, Func&& func) {
    using CmdType = LambdaCommand<std::decay_t<Func>>;
    if (IsImmediate()) {
        func(*context);
    } else {
        auto* cmd = new (allocator.Allocate(sizeof(CmdType), alignof(CmdType)))
            CmdType(std::forward<Func>(func), name);
        LinkCommand(cmd);
    }
}

// 使用例
cmdList.EnqueueLambda("UpdateBuffer", [data](CommandContext& ctx) {
    ctx.UpdateBuffer(buffer, data.data(), data.size());
});
```

---

## 4. リソース参照カウント

基底クラス `IRHIResource` がスレッドセーフな参照カウント、デバッグ名、遅延削除を提供する。

```cpp
class RHI_API IRHIResource
{
public:
    virtual ~IRHIResource();

    /// 参照カウント（スレッドセーフ、atomic）
    uint32 AddRef() const noexcept;
    uint32 Release() const noexcept;
    uint32 GetRefCount() const noexcept;

    /// リソース識別
    ERHIResourceType GetResourceType() const noexcept;
    ResourceId GetResourceId() const noexcept;

    /// デバッグ名
    virtual void SetDebugName(const char* name);
    size_t GetDebugName(char* outBuffer, size_t bufferSize) const;

    /// 遅延削除
    void MarkForDeferredDelete() const noexcept;
    bool IsPendingDelete() const noexcept;

protected:
    explicit IRHIResource(ERHIResourceType type);
    virtual void OnZeroRefCount() const;

private:
    mutable std::atomic<uint32> m_refCount{1};
    // ...
};
```

スマートポインタ `TRefCountPtr<T>`:
```cpp
template <typename T> class TRefCountPtr
{
public:
    TRefCountPtr() noexcept : m_ptr(nullptr) {}
    explicit TRefCountPtr(T* ptr) noexcept;
    TRefCountPtr(const TRefCountPtr& other) noexcept;
    TRefCountPtr(TRefCountPtr&& other) noexcept;
    ~TRefCountPtr() noexcept;

    T* operator->() const noexcept;
    T* Get() const noexcept;
    T* Detach() noexcept;
    void Attach(T* ptr) noexcept;
    void Reset(T* ptr = nullptr) noexcept;
    explicit operator bool() const noexcept;
    // ...
};

// 型エイリアス
using RHISamplerRef  = TRefCountPtr<IRHISampler>;
using RHIBufferRef   = TRefCountPtr<IRHIBuffer>;
using RHITextureRef  = TRefCountPtr<IRHITexture>;
using RHIResourceRef = TRefCountPtr<IRHIResource>;
```

---

## 5. ビットフラグ列挙型

`RHI_ENUM_CLASS_FLAGS(EnumType)` マクロで `|`, `&`, `~`, `^` 演算子と
`EnumHasAnyFlags()` / `EnumHasAllFlags()` ヘルパーを一括定義する。

```cpp
/// RHIMacros.h で定義
/// operator|, operator&, operator~, operator^,
/// EnumHasAnyFlags(), EnumHasAllFlags() を一括定義
#define RHI_ENUM_CLASS_FLAGS(EnumType)                    \
    inline constexpr EnumType operator|(EnumType a, EnumType b) { ... } \
    inline constexpr EnumType operator&(EnumType a, EnumType b) { ... } \
    inline constexpr EnumType operator~(EnumType a) { ... }             \
    inline constexpr EnumType operator^(EnumType a, EnumType b) { ... } \
    inline constexpr EnumType& operator|=(EnumType& a, EnumType b) { ... } \
    inline constexpr EnumType& operator&=(EnumType& a, EnumType b) { ... } \
    inline constexpr bool EnumHasAnyFlags(EnumType a, EnumType b) { ... } \
    inline constexpr bool EnumHasAllFlags(EnumType a, EnumType b) { ... }

// 使用例
enum class ERHIBufferUsage : uint32
{
    None           = 0,
    VertexBuffer   = 1 << 0,
    IndexBuffer    = 1 << 1,
    ConstantBuffer = 1 << 2,
    ShaderResource = 1 << 3,
    UnorderedAccess= 1 << 4,
    IndirectArgs   = 1 << 7,
    // ...
};
RHI_ENUM_CLASS_FLAGS(ERHIBufferUsage)

// フラグチェック
inline bool IsVertexOrIndexBuffer(ERHIBufferUsage usage)
{
    return EnumHasAnyFlags(usage,
                           ERHIBufferUsage::VertexBuffer | ERHIBufferUsage::IndexBuffer);
}
```

---

## 6. ディスクリプタハンドルの抽象化

```cpp
// 型安全なハンドル（タグで区別）
template<typename Tag>
struct Handle {
    uint32_t index = UINT32_MAX;
    bool IsValid() const { return index != UINT32_MAX; }
    explicit operator bool() const { return IsValid(); }
    bool operator==(Handle other) const { return index == other.index; }
    bool operator!=(Handle other) const { return index != other.index; }
};

struct SRVTag {};
struct UAVTag {};
struct SamplerTag {};
using SRVHandle = Handle<SRVTag>;
using UAVHandle = Handle<UAVTag>;
using SamplerHandle = Handle<SamplerTag>;

// フリーリストアロケータ
class HandleAllocator {
public:
    HandleAllocator(uint32_t maxHandles) : maxCount(maxHandles) {
        freeList.reserve(maxHandles);
        for (uint32_t i = maxHandles; i > 0; --i)
            freeList.push_back(i - 1);
    }

    uint32_t Allocate() {
        if (freeList.empty()) return UINT32_MAX;
        uint32_t index = freeList.back();
        freeList.pop_back();
        return index;
    }

    void Free(uint32_t index) { freeList.push_back(index); }

private:
    uint32_t maxCount;
    std::vector<uint32_t> freeList;
};
```

---

## 7. パイプライン状態のハッシュ化

```cpp
struct PipelineStateKey {
    uint64_t shaderHash;
    uint32_t blendState;
    uint32_t depthStencilState;
    uint32_t rasterizerState;
    uint32_t inputLayout;
    uint32_t primitiveType;
    uint32_t renderTargetFormats[8];
    uint32_t depthFormat;

    bool operator==(const PipelineStateKey& other) const {
        return memcmp(this, &other, sizeof(*this)) == 0;
    }
};

struct PipelineStateKeyHash {
    size_t operator()(const PipelineStateKey& key) const {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&key);
        size_t hash = 14695981039346656037ull;  // FNV-1a
        for (size_t i = 0; i < sizeof(key); ++i) {
            hash ^= data[i];
            hash *= 1099511628211ull;
        }
        return hash;
    }
};

class PipelineStateCache {
public:
    PipelineState* GetOrCreate(const PipelineStateKey& key) {
        auto it = cache.find(key);
        if (it != cache.end()) return it->second.get();
        auto pso = CreatePipelineState(key);
        auto* ptr = pso.get();
        cache[key] = std::move(pso);
        return ptr;
    }
private:
    std::unordered_map<PipelineStateKey, std::unique_ptr<PipelineState>,
                       PipelineStateKeyHash> cache;
};
```

---

## 8. スコープガード

```cpp
template<typename Func>
class ScopeGuard {
public:
    explicit ScopeGuard(Func&& func)
        : func(std::forward<Func>(func)), active(true) {}
    ~ScopeGuard() { if (active) func(); }
    void Dismiss() { active = false; }
    ScopeGuard(ScopeGuard&& other) noexcept
        : func(std::move(other.func)), active(other.active) {
        other.active = false;
    }
private:
    Func func;
    bool active;
};

template<typename Func>
ScopeGuard<Func> MakeScopeGuard(Func&& func) {
    return ScopeGuard<Func>(std::forward<Func>(func));
}

#define SCOPE_EXIT(code) \
    auto CONCAT(scopeGuard_, __LINE__) = MakeScopeGuard([&]() { code; })

// 使用例
void* mappedData = buffer->Map();
SCOPE_EXIT(buffer->Unmap());
// ... 処理 ... 関数終了時に自動Unmap
```

---

## 9. GPUプロファイル/ブレッドクラムスコープ

### 9.1 GPUプロファイルスコープ

```cpp
/// RAIIプロファイルスコープ
class RHIGPUProfileScope
{
public:
    RHIGPUProfileScope(IRHIGPUProfiler* profiler,
                       IRHICommandContext* context,
                       const char* name,
                       ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom);
    ~RHIGPUProfileScope();
};

/// マクロ（NS_GPU_PROFILING_ENABLED でコンパイル時除去）
#define RHI_GPU_PROFILE_SCOPE(Profiler, Context, Name) \
    ::NS::RHI::RHIGPUProfileScope \
        NS_MACRO_CONCATENATE(_gpuProfileScope, __LINE__)(Profiler, Context, Name)

// 使用例
void RenderScene(IRHIGPUProfiler* profiler, IRHICommandContext* ctx) {
    RHI_GPU_PROFILE_SCOPE(profiler, ctx, "RenderScene");
    {
        RHI_GPU_PROFILE_SCOPE(profiler, ctx, "ShadowPass");
    }
}
```

### 9.2 ブレッドクラムスコープ（クラッシュ診断用）

```cpp
/// RAII Breadcrumb スコープ
class RHIBreadcrumbScope
{
public:
    RHIBreadcrumbScope(IRHICommandContext* context,
                       RHIBreadcrumbAllocator* allocator,
                       const char* name,
                       const char* sourceFile = nullptr,
                       uint32 sourceLine = 0);
    ~RHIBreadcrumbScope();
};

/// マクロ（NS_RHI_BREADCRUMBS_FULL || NS_RHI_BREADCRUMBS_MINIMAL で有効）
#define RHI_BREADCRUMB_EVENT(context, allocator, name) \
    ::NS::RHI::RHIBreadcrumbScope \
        NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)(context, allocator, name, __FILE__, __LINE__)
```

---

## 10. バリア/遷移のバッチ処理

```cpp
struct TransitionInfo {
    Resource* resource;
    ResourceState before;
    ResourceState after;
    uint32_t subresource = ALL_SUBRESOURCES;
    static constexpr uint32_t ALL_SUBRESOURCES = ~0u;
};

class BarrierBatcher {
public:
    void Add(Resource* res, ResourceState before, ResourceState after) {
        // 同一リソースの連続遷移を統合: A→B, B→C => A→C
        for (auto& pending : pendingBarriers) {
            if (pending.resource == res) {
                pending.after = after;
                return;
            }
        }
        pendingBarriers.push_back({res, before, after});
    }

    void Flush(CommandContext& ctx) {
        if (pendingBarriers.empty()) return;
        ctx.ResourceBarriers(pendingBarriers);
        pendingBarriers.clear();
    }

private:
    std::vector<TransitionInfo> pendingBarriers;
};
```

---

## 11. 型安全なパラメータバインディング

```cpp
struct ShaderParameter {
    const char* name;
    uint32_t offset;
    uint32_t size;
};

// コンパイル時パラメータレイアウト
struct MyShaderParams {
    float4 color;
    float intensity;
    float2 uvScale;

    static std::vector<ShaderParameter> DefineParameters() {
        return {
            {"color",     offsetof(MyShaderParams, color),     sizeof(float4)},
            {"intensity", offsetof(MyShaderParams, intensity), sizeof(float)},
            {"uvScale",   offsetof(MyShaderParams, uvScale),   sizeof(float2)},
        };
    }
};

template<typename T>
void BindParameters(CommandList& cmd, const T& params) {
    cmd.SetConstants(&params, sizeof(T));
}
```

---

## 12. ArrayView（非所有配列参照）

```cpp
template<typename T>
class ArrayView {
public:
    ArrayView() : data_(nullptr), size_(0) {}
    ArrayView(const T* data, size_t size) : data_(data), size_(size) {}
    ArrayView(const std::vector<T>& vec) : data_(vec.data()), size_(vec.size()) {}

    template<size_t N>
    ArrayView(const T (&arr)[N]) : data_(arr), size_(N) {}

    const T* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    const T& operator[](size_t i) const { return data_[i]; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

private:
    const T* data_;
    size_t size_;
};

// 使用例: 配列でもvectorでも受け取れる
void SetBarriers(ArrayView<TransitionInfo> barriers);
```

---

## 13. ディスパッチテーブルパターン

### 13.1 テーブル定義（コンテキスト操作中心）

ディスパッチテーブルはファクトリではなくコンテキスト操作の関数ポインタ集合。
第1引数は常にコンテキストポインタ（`IRHICommandContextBase*`、`IRHIComputeContext*`、
`IRHICommandContext*`、`IRHIUploadContext*` のいずれか）。

```cpp
struct RHI_API RHIDispatchTable
{
    // Base: プロパティ
    IRHIDevice* (*GetDevice)(IRHICommandContextBase* ctx) = nullptr;
    ERHIQueueType (*GetQueueType)(IRHICommandContextBase* ctx) = nullptr;

    // Base: ライフサイクル
    void (*Begin)(IRHICommandContextBase* ctx, IRHICommandAllocator* allocator) = nullptr;
    IRHICommandList* (*Finish)(IRHICommandContextBase* ctx) = nullptr;
    void (*Reset)(IRHICommandContextBase* ctx) = nullptr;

    // Base: バリア
    void (*TransitionResource)(IRHICommandContextBase* ctx,
                               IRHIResource* resource,
                               ERHIAccess stateBefore,
                               ERHIAccess stateAfter) = nullptr;
    void (*FlushBarriers)(IRHICommandContextBase* ctx) = nullptr;

    // Compute: パイプライン
    void (*SetComputePipelineState)(IRHIComputeContext* ctx,
                                    IRHIComputePipelineState* pso) = nullptr;
    void (*Dispatch)(IRHIComputeContext* ctx,
                     uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = nullptr;

    // Graphics: 描画
    void (*Draw)(IRHICommandContext* ctx,
                 uint32 vertexCount, uint32 instanceCount,
                 uint32 startVertex, uint32 startInstance) = nullptr;
    void (*DrawIndexed)(IRHICommandContext* ctx,
                        uint32 indexCount, uint32 instanceCount,
                        uint32 startIndex, int32 baseVertex,
                        uint32 startInstance) = nullptr;

    // Upload
    void (*UploadBuffer)(IRHIUploadContext* ctx,
                         IRHIBuffer* dst, uint64 dstOffset,
                         const void* srcData, uint64 srcSize) = nullptr;

    // オプショナル（null許容）
    void (*DispatchMesh)(IRHICommandContext* ctx,
                         uint32 x, uint32 y, uint32 z) = nullptr;
    void (*DispatchRays)(IRHICommandContext* ctx,
                         const RHIDispatchRaysDesc* desc) = nullptr;
    // ... 100+ エントリ
};

/// グローバルディスパッチテーブル
extern RHI_API RHIDispatchTable GRHIDispatchTable;
```

### 13.2 出荷モード切替

```cpp
// 開発ビルド: ディスパッチテーブル経由（関数ポインタ1回分）
#define NS_RHI_DISPATCH(func, ...) \
    ::NS::RHI::GRHIDispatchTable.func(__VA_ARGS__)

// 出荷ビルド: コンパイル時バックエンド選択（ゼロオーバーヘッド）
// NS_RHI_STATIC_BACKEND が定義されている場合
#define NS_RHI_DISPATCH(func, ...) \
    ::NS::RHI::NS_RHI_EXPAND(NS_RHI_STATIC_BACKEND, func)(__VA_ARGS__)

// 使用側（モードに関係なく同一コード）
NS_RHI_DISPATCH(Draw, ctx, vertexCount, instanceCount, startVertex, startInstance);
NS_RHI_DISPATCH(Dispatch, ctx, groupCountX, groupCountY, groupCountZ);
```

### 13.3 テーブル検証

`IsValid()` メンバ関数で必須エントリを検証。
オプショナル機能は `Has*Support()` メンバで確認。

```cpp
struct RHI_API RHIDispatchTable
{
    // ...

    /// 必須エントリが全て設定済みか
    [[nodiscard]] bool IsValid() const
    {
        return GetDevice != nullptr
            && Begin != nullptr
            && Finish != nullptr
            && TransitionResource != nullptr
            && Draw != nullptr
            && DrawIndexed != nullptr
            && Dispatch != nullptr
            // ... 全必須エントリ
            ;
    }

    /// オプショナル機能チェック
    [[nodiscard]] bool HasMeshShaderSupport() const;
    [[nodiscard]] bool HasRayTracingSupport() const;
    [[nodiscard]] bool HasWorkGraphSupport() const;
    [[nodiscard]] bool HasVariableRateShadingSupport() const;
};
```

---

## パターン一覧

| パターン | 用途 |
|----------|------|
| RHICommandHeader + POD + static Execute | 描画コマンドの記録・再生 |
| NS_RHI_DISPATCH パターン | ディスパッチテーブル経由のバックエンド呼び出し |
| IRHICommandAllocator | コマンドメモリ管理（オフセットチェーン） |
| IRHIResource + TRefCountPtr | リソース寿命管理 |
| RHI_ENUM_CLASS_FLAGS + EnumHasAnyFlags | 型安全なビットフラグ |
| ハンドル型 | 型安全なディスクリプタインデックス |
| パイプラインキャッシュ | PSO重複作成防止 |
| スコープガード | RAII リソース管理 |
| RHIGPUProfileScope / RHIBreadcrumbScope | プロファイリング / クラッシュ診断 |
| バリアバッチャー | 遷移の最適化 |
| ArrayView | 非所有配列参照 |
| RHIDispatchTable + GRHIDispatchTable | バックエンド切替（開発: ランタイム / 出荷: コンパイル時） |
