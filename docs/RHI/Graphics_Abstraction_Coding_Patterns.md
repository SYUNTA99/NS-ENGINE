# グラフィックスAPI抽象層 - コーディングパターン集

D3D12/Vulkan/Metal等を抽象化するレイヤーで使われる汎用的なC++パターン。

---

## 0. 共通ユーティリティ

```cpp
// トークン連結マクロ
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

// 数学型（DirectXMathやglm等を使用、または自前定義）
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct float4x4 { float m[4][4]; };

// または
#include <DirectXMath.h>
using float2 = DirectX::XMFLOAT2;
using float4 = DirectX::XMFLOAT4;

// または
#include <glm/glm.hpp>
using float2 = glm::vec2;
using float4 = glm::vec4;
```

---

## 1. コマンドパターン

### 1.1 コマンド基底クラス

```cpp
// コマンド基底
struct CommandBase
{
    CommandBase* next = nullptr;

    virtual ~CommandBase() = default;
    virtual void Execute(class CommandContext& ctx) = 0;
    virtual const char* GetName() const = 0;
};

// CRTP でボイラープレート削減
template<typename Derived, typename NameTag>
struct Command : CommandBase
{
    void ExecuteAndDestruct(CommandContext& ctx)
    {
        static_cast<Derived*>(this)->Execute(ctx);
        static_cast<Derived*>(this)->~Derived();
    }

    const char* GetName() const override
    {
        return NameTag::Name();
    }
};
```

### 1.2 コマンド定義マクロ

```cpp
// コマンド定義を簡略化するマクロ
#define DEFINE_COMMAND(Name) \
    struct Name##_Tag { static const char* Name() { return #Name; } }; \
    struct Name : Command<Name, Name##_Tag>

// 使用例
DEFINE_COMMAND(CmdDrawPrimitive)
{
    uint32_t baseVertex;
    uint32_t numPrimitives;
    uint32_t numInstances;

    CmdDrawPrimitive(uint32_t base, uint32_t prims, uint32_t inst)
        : baseVertex(base), numPrimitives(prims), numInstances(inst) {}

    void Execute(CommandContext& ctx) override
    {
        ctx.DrawPrimitive(baseVertex, numPrimitives, numInstances);
    }
};

DEFINE_COMMAND(CmdSetViewport)
{
    float x, y, width, height, minDepth, maxDepth;

    void Execute(CommandContext& ctx) override
    {
        ctx.SetViewport(x, y, width, height, minDepth, maxDepth);
    }
};

DEFINE_COMMAND(CmdTransition)
{
    Resource* resource;
    ResourceState before;
    ResourceState after;

    void Execute(CommandContext& ctx) override
    {
        ctx.Transition(resource, before, after);
    }
};
```

### 1.3 コマンドアロケータ

```cpp
// リニアアロケータ（フレーム毎にリセット）
class LinearAllocator
{
public:
    LinearAllocator(size_t capacity)
        : buffer(new uint8_t[capacity])
        , capacity(capacity)
        , offset(0)
    {}

    void* Allocate(size_t size, size_t alignment)
    {
        size_t aligned = (offset + alignment - 1) & ~(alignment - 1);
        if (aligned + size > capacity) return nullptr;

        void* ptr = buffer.get() + aligned;
        offset = aligned + size;
        return ptr;
    }

    void Reset() { offset = 0; }

private:
    std::unique_ptr<uint8_t[]> buffer;
    size_t capacity;
    size_t offset;
};

// コマンド割り当てマクロ
#define ALLOC_CMD(allocator, Type, ...) \
    new (allocator.Allocate(sizeof(Type), alignof(Type))) Type(__VA_ARGS__)
```

---

## 2. 即時実行 vs 遅延実行（Bypass パターン）

```cpp
class CommandList
{
public:
    // 即時実行モードかどうか
    bool IsImmediate() const { return immediateMode; }

    void DrawPrimitive(uint32_t base, uint32_t prims, uint32_t instances)
    {
        if (IsImmediate())
        {
            // 即時実行
            context->DrawPrimitive(base, prims, instances);
        }
        else
        {
            // コマンド記録
            ALLOC_CMD(allocator, CmdDrawPrimitive, base, prims, instances);
            LinkCommand(cmd);
        }
    }

    void SetViewport(float x, float y, float w, float h)
    {
        if (IsImmediate())
        {
            context->SetViewport(x, y, w, h, 0.0f, 1.0f);
        }
        else
        {
            auto* cmd = ALLOC_CMD(allocator, CmdSetViewport);
            cmd->x = x; cmd->y = y; cmd->width = w; cmd->height = h;
            cmd->minDepth = 0.0f; cmd->maxDepth = 1.0f;
            LinkCommand(cmd);
        }
    }

private:
    void LinkCommand(CommandBase* cmd)
    {
        *commandTail = cmd;
        commandTail = &cmd->next;
    }

    LinearAllocator allocator{1024 * 1024};  // 1MB
    CommandBase* commandHead = nullptr;
    CommandBase** commandTail = &commandHead;
    CommandContext* context = nullptr;
    bool immediateMode = false;
};
```

---

## 3. ラムダコマンド

```cpp
// 任意のラムダをコマンドとして記録
template<typename Func>
struct LambdaCommand : CommandBase
{
    Func func;
    const char* name;

    LambdaCommand(Func&& f, const char* n)
        : func(std::forward<Func>(f)), name(n) {}

    void Execute(CommandContext& ctx) override { func(ctx); }
    const char* GetName() const override { return name; }
};

class CommandList
{
public:
    template<typename Func>
    void EnqueueLambda(const char* name, Func&& func)
    {
        using CmdType = LambdaCommand<std::decay_t<Func>>;

        if (IsImmediate())
        {
            func(*context);
        }
        else
        {
            auto* cmd = new (allocator.Allocate(sizeof(CmdType), alignof(CmdType)))
                CmdType(std::forward<Func>(func), name);
            LinkCommand(cmd);
        }
    }
};

// 使用例
cmdList.EnqueueLambda("CustomWork", [data](CommandContext& ctx) {
    ctx.UpdateBuffer(buffer, data.data(), data.size());
});
```

---

## 4. リソース参照カウント

```cpp
// 参照カウント基底
class RefCounted
{
public:
    void AddRef() { ++refCount; }

    void Release()
    {
        if (--refCount == 0)
            delete this;
    }

    uint32_t GetRefCount() const { return refCount; }

protected:
    virtual ~RefCounted() = default;

private:
    std::atomic<uint32_t> refCount{1};
};

// スマートポインタ
template<typename T>
class Ref
{
public:
    Ref() : ptr(nullptr) {}
    Ref(T* p) : ptr(p) {}
    Ref(const Ref& other) : ptr(other.ptr) { if (ptr) ptr->AddRef(); }
    Ref(Ref&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
    ~Ref() { if (ptr) ptr->Release(); }

    Ref& operator=(const Ref& other)
    {
        if (other.ptr) other.ptr->AddRef();
        if (ptr) ptr->Release();
        ptr = other.ptr;
        return *this;
    }

    T* operator->() const { return ptr; }
    T* Get() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }

private:
    T* ptr;
};

// 使用
using BufferRef = Ref<Buffer>;
using TextureRef = Ref<Texture>;
```

---

## 5. ビットフラグ列挙型

```cpp
// enum class でビット演算を有効化
template<typename E>
struct EnableBitOps : std::false_type {};

#define ENABLE_BITMASK_OPERATORS(Enum) \
    template<> struct EnableBitOps<Enum> : std::true_type {}

template<typename E>
constexpr std::enable_if_t<EnableBitOps<E>::value, E>
operator|(E lhs, E rhs)
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template<typename E>
constexpr std::enable_if_t<EnableBitOps<E>::value, E>
operator&(E lhs, E rhs)
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template<typename E>
constexpr std::enable_if_t<EnableBitOps<E>::value, bool>
HasFlag(E value, E flag)
{
    return (value & flag) == flag;
}

template<typename E>
constexpr std::enable_if_t<EnableBitOps<E>::value, bool>
HasAnyFlag(E value, E flags)
{
    using U = std::underlying_type_t<E>;
    return (static_cast<U>(value) & static_cast<U>(flags)) != 0;
}

// 使用
enum class BufferUsage : uint32_t
{
    None           = 0,
    VertexBuffer   = 1 << 0,
    IndexBuffer    = 1 << 1,
    ConstantBuffer = 1 << 2,
    ShaderResource = 1 << 3,
    UnorderedAccess= 1 << 4,
    IndirectArgs   = 1 << 5,
};
ENABLE_BITMASK_OPERATORS(BufferUsage);

// 使用例
BufferUsage usage = BufferUsage::VertexBuffer | BufferUsage::ShaderResource;
if (HasFlag(usage, BufferUsage::VertexBuffer)) { ... }
```

---

## 6. 記述子（ディスクリプタ）の抽象化

```cpp
// ハンドル型（型安全なインデックス）
template<typename Tag>
struct Handle
{
    uint32_t index = UINT32_MAX;

    bool IsValid() const { return index != UINT32_MAX; }
    explicit operator bool() const { return IsValid(); }

    bool operator==(Handle other) const { return index == other.index; }
    bool operator!=(Handle other) const { return index != other.index; }
};

// タグ定義
struct SRVTag {};
struct UAVTag {};
struct SamplerTag {};

using SRVHandle = Handle<SRVTag>;
using UAVHandle = Handle<UAVTag>;
using SamplerHandle = Handle<SamplerTag>;

// フリーリストアロケータ
class HandleAllocator
{
public:
    HandleAllocator(uint32_t maxHandles)
        : maxCount(maxHandles)
    {
        freeList.reserve(maxHandles);
        for (uint32_t i = maxHandles; i > 0; --i)
            freeList.push_back(i - 1);
    }

    uint32_t Allocate()
    {
        if (freeList.empty()) return UINT32_MAX;
        uint32_t index = freeList.back();
        freeList.pop_back();
        return index;
    }

    void Free(uint32_t index)
    {
        freeList.push_back(index);
    }

private:
    uint32_t maxCount;
    std::vector<uint32_t> freeList;
};
```

---

## 7. パイプライン状態のハッシュ化

```cpp
// パイプライン状態のキー
struct PipelineStateKey
{
    uint64_t shaderHash;
    uint32_t blendState;
    uint32_t depthStencilState;
    uint32_t rasterizerState;
    uint32_t inputLayout;
    uint32_t primitiveType;
    uint32_t renderTargetFormats[8];
    uint32_t depthFormat;

    bool operator==(const PipelineStateKey& other) const
    {
        return memcmp(this, &other, sizeof(*this)) == 0;
    }
};

// ハッシュ関数
struct PipelineStateKeyHash
{
    size_t operator()(const PipelineStateKey& key) const
    {
        // FNV-1a ハッシュ
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&key);
        size_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < sizeof(key); ++i)
        {
            hash ^= data[i];
            hash *= 1099511628211ull;
        }
        return hash;
    }
};

// キャッシュ
class PipelineStateCache
{
public:
    PipelineState* GetOrCreate(const PipelineStateKey& key)
    {
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second.get();

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
// RAII スコープガード
template<typename Func>
class ScopeGuard
{
public:
    explicit ScopeGuard(Func&& func)
        : func(std::forward<Func>(func)), active(true) {}

    ~ScopeGuard() { if (active) func(); }

    void Dismiss() { active = false; }

    ScopeGuard(ScopeGuard&& other) noexcept
        : func(std::move(other.func)), active(other.active)
    {
        other.active = false;
    }

private:
    Func func;
    bool active;
};

template<typename Func>
ScopeGuard<Func> MakeScopeGuard(Func&& func)
{
    return ScopeGuard<Func>(std::forward<Func>(func));
}

#define SCOPE_EXIT(code) \
    auto CONCAT(scopeGuard_, __LINE__) = MakeScopeGuard([&]() { code; })

// 使用例
void* mappedData = buffer->Map();
SCOPE_EXIT(buffer->Unmap());

// 処理...
// 関数終了時に自動的にUnmap
```

---

## 9. GPUイベント/マーカー

```cpp
// プロファイリング用スコープ
class ScopedGPUEvent
{
public:
    ScopedGPUEvent(CommandList& cmdList, const char* name)
        : cmdList(cmdList)
    {
        cmdList.BeginEvent(name);
    }

    ~ScopedGPUEvent()
    {
        cmdList.EndEvent();
    }

private:
    CommandList& cmdList;
};

#define GPU_EVENT(cmdList, name) \
    ScopedGPUEvent CONCAT(gpuEvent_, __LINE__)(cmdList, name)

// 使用例
void RenderScene(CommandList& cmdList)
{
    GPU_EVENT(cmdList, "RenderScene");

    {
        GPU_EVENT(cmdList, "ShadowPass");
        // シャドウ描画
    }

    {
        GPU_EVENT(cmdList, "GBuffer");
        // GBuffer描画
    }
}
```

---

## 10. バリア/遷移のバッチ処理

```cpp
// 遷移情報
struct TransitionInfo
{
    Resource* resource;
    ResourceState before;
    ResourceState after;
    uint32_t subresource = ALL_SUBRESOURCES;

    static constexpr uint32_t ALL_SUBRESOURCES = ~0u;
};

// バリアバッチャー
class BarrierBatcher
{
public:
    void Add(Resource* res, ResourceState before, ResourceState after)
    {
        // 同一リソースの連続遷移を最適化
        for (auto& pending : pendingBarriers)
        {
            if (pending.resource == res)
            {
                // 統合: A->B, B->C => A->C
                pending.after = after;
                return;
            }
        }
        pendingBarriers.push_back({res, before, after});
    }

    void Flush(CommandContext& ctx)
    {
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
// シェーダーパラメータ記述
struct ShaderParameter
{
    const char* name;
    uint32_t offset;
    uint32_t size;
};

// コンパイル時パラメータレイアウト
template<typename T>
struct ParameterLayout
{
    static const std::vector<ShaderParameter>& GetParameters()
    {
        static std::vector<ShaderParameter> params = T::DefineParameters();
        return params;
    }
};

// 使用例
struct MyShaderParams
{
    float4 color;
    float intensity;
    float2 uvScale;

    static std::vector<ShaderParameter> DefineParameters()
    {
        return {
            {"color",     offsetof(MyShaderParams, color),     sizeof(float4)},
            {"intensity", offsetof(MyShaderParams, intensity), sizeof(float)},
            {"uvScale",   offsetof(MyShaderParams, uvScale),   sizeof(float2)},
        };
    }
};

// バインド
template<typename T>
void BindParameters(CommandList& cmd, const T& params)
{
    cmd.SetConstants(&params, sizeof(T));
}
```

---

## 12. ArrayView（非所有配列参照）

```cpp
template<typename T>
class ArrayView
{
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

// 使用
void SetBarriers(ArrayView<TransitionInfo> barriers);

// 呼び出し
TransitionInfo arr[] = { ... };
SetBarriers(arr);  // 配列から

std::vector<TransitionInfo> vec = { ... };
SetBarriers(vec);  // vectorから
```

---

## まとめ

| パターン | 用途 |
|----------|------|
| コマンドパターン + CRTP | 描画コマンドの記録・再生 |
| Bypass パターン | 即時/遅延実行の切り替え |
| リニアアロケータ | フレーム毎のコマンドメモリ |
| 参照カウント + スマートポインタ | リソース寿命管理 |
| ビットフラグ enum class | 型安全なフラグ |
| ハンドル型 | 型安全な記述子インデックス |
| パイプラインキャッシュ | PSO重複作成防止 |
| スコープガード | RAII リソース管理 |
| GPUイベントスコープ | プロファイリング |
| バリアバッチャー | 遷移の最適化 |
| ArrayView | 非所有配列参照 |

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-02-04 | 初版作成 |
