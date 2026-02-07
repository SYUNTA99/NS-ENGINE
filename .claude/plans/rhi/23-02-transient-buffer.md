# 23-02: Transientバッファ

## 概要

フレーム内の一時的に使用するバッファリソース、
メモリエイリアシングにより効率的にGPUメモリを再利用、

## ファイル

- `Source/Engine/RHICore/Public/RHITransientBuffer.h`

## 依存

- 23-01-transient-allocator.md (Transientアロケーター)
- 03-02-buffer-interface.md (IRHIBuffer)

## 定義

```cpp
namespace NS::RHI
{

/// Transientバッファ使用フラグ
enum class ERHITransientBufferUsage : uint32
{
    None = 0,
    Vertex = 1 << 0,        ///< 頂点バッファ
    Index = 1 << 1,         ///< インデックスバッファ
    Constant = 1 << 2,      ///< 定数バッファ
    Structured = 1 << 3,    ///< 構造化バッファ
    Raw = 1 << 4,           ///< ByteAddressBuffer
    Indirect = 1 << 5,      ///< Indirect引数
    CopySource = 1 << 6,    ///< コピー元
    CopyDest = 1 << 7,      ///< コピー先
    UAV = 1 << 8,           ///< UAV
};
NS_ENUM_FLAGS(ERHITransientBufferUsage);

/// Transientバッファ記述
struct RHITransientBufferCreateInfo
{
    uint64 size = 0;
    ERHITransientBufferUsage usage = ERHITransientBufferUsage::None;
    uint32 structureByteStride = 0;     ///< 構造化バッファのストライド
    const char* debugName = nullptr;
};

/// Transientバッファハンドル
/// 実際のIRHIBufferはAcquire〜Release間のみ有効
class RHITransientBufferHandle
{
public:
    RHITransientBufferHandle() = default;

    bool IsValid() const { return m_handle != kInvalidHandle; }

    /// 実際のバッファ取得（Acquire後のみ有効）。
    IRHIBuffer* GetBuffer() const;

    /// サイズ取得
    uint64 GetSize() const { return m_info.size; }

    /// 使用フラグ取得
    ERHITransientBufferUsage GetUsage() const { return m_info.usage; }

    /// ハンドルID（デバッグ用）。
    uint32 GetHandleId() const { return m_handle; }

private:
    friend class RHITransientResourceSystem;

    static constexpr uint32 kInvalidHandle = UINT32_MAX;

    uint32 m_handle = kInvalidHandle;
    RHITransientBufferCreateInfo m_info;
    IRHIBuffer* m_acquiredBuffer = nullptr;
};

// ════════════════════════════════════════════════════════════════
// Transientバッファプール
// ════════════════════════════════════════════════════════════════

/// Transientバッファプール
/// 同じサイズ・使用フラグのバッファを再利用
class RHITransientBufferPool
{
public:
    explicit RHITransientBufferPool(IRHIDevice* device);
    ~RHITransientBufferPool();

    /// バッファ取得（プールから、なければ新規作成）。
    RHIBufferRef Acquire(const RHITransientBufferCreateInfo& info);

    /// バッファ返却
    void Release(RHIBufferRef buffer);

    /// フレーム終了時（返却されたバッファを再利用可能にする）。
    void OnFrameEnd();

    /// プールをクリア
    void Clear();

    /// 統計
    uint32 GetPooledBufferCount() const;
    uint64 GetTotalPooledMemory() const;

private:
    struct PoolKey
    {
        uint64 size;
        ERHITransientBufferUsage usage;

        bool operator==(const PoolKey& other) const
        {
            return size == other.size && usage == other.usage;
        }
    };

    struct PoolKeyHash
    {
        size_t operator()(const PoolKey& key) const
        {
            return std::hash<uint64>()(key.size) ^
                   (std::hash<uint32>()(static_cast<uint32>(key.usage)) << 16);
        }
    };

    IRHIDevice* m_device;
    std::unordered_map<PoolKey, std::vector<RHIBufferRef>, PoolKeyHash> m_pools;
    std::vector<std::pair<PoolKey, RHIBufferRef>> m_pendingRelease;
};

// ════════════════════════════════════════════════════════════════
// 便利なTransientバッファ生成ヘルパー
// ════════════════════════════════════════════════════════════════

namespace RHITransientBuffers
{
    /// 頂点バッファ用Transient
    inline RHITransientBufferCreateInfo Vertex(uint64 size, const char* name = nullptr)
    {
        return { size, ERHITransientBufferUsage::Vertex | ERHITransientBufferUsage::CopyDest, 0, name };
    }

    /// インデックスバッファ用Transient
    inline RHITransientBufferCreateInfo Index(uint64 size, const char* name = nullptr)
    {
        return { size, ERHITransientBufferUsage::Index | ERHITransientBufferUsage::CopyDest, 0, name };
    }

    /// 定数バッファ用Transient
    inline RHITransientBufferCreateInfo Constant(uint64 size, const char* name = nullptr)
    {
        return { size, ERHITransientBufferUsage::Constant, 0, name };
    }

    /// 構造化バッファ用Transient
    inline RHITransientBufferCreateInfo Structured(
        uint64 elementCount, uint32 stride, const char* name = nullptr)
    {
        return {
            elementCount * stride,
            ERHITransientBufferUsage::Structured | ERHITransientBufferUsage::UAV,
            stride,
            name
        };
    }

    /// Indirect引数バッファ用Transient
    inline RHITransientBufferCreateInfo Indirect(uint64 size, const char* name = nullptr)
    {
        return {
            size,
            ERHITransientBufferUsage::Indirect | ERHITransientBufferUsage::UAV,
            0,
            name
        };
    }
}

} // namespace NS::RHI
```

## 使用例

```cpp
// フレームグラフでのTransientバッファ使用
void SetupFrameGraph(RHIFrameGraph& fg)
{
    // カリング結果バッファ（Pass 0-1で使用）。
    auto cullingResults = fg.CreateTransientBuffer(
        RHITransientBuffers::Structured(kMaxObjects, sizeof(uint32), "CullingResults"),
        { 0, 1 }  // lifetime
    );

    // Indirect引数（Pass 1のみ）。
    auto indirectArgs = fg.CreateTransientBuffer(
        RHITransientBuffers::Indirect(sizeof(RHIDrawIndexedArguments) * kMaxDraws, "IndirectArgs"),
        { 1, 1 }
    );

    // パス定義
    fg.AddPass("Culling", [=](RHIPassBuilder& builder) {
        builder.WriteBuffer(cullingResults);
        return [=](IRHIComputeContext* ctx) {
            ctx->SetUAV(0, cullingResults.GetBuffer());
            ctx->Dispatch(groups, 1, 1);
        };
    });

    fg.AddPass("Draw", [=](RHIPassBuilder& builder) {
        builder.ReadBuffer(cullingResults);
        builder.WriteBuffer(indirectArgs);
        return [=](IRHIGraphicsContext* ctx) {
            // Indirect引数生成と描画
        };
    });
}
```

## 検証

- [ ] プールからの取得返却
- [ ] フレーム間での再利用
- [ ] 使用フラグの組み合わせ
- [ ] メモリ効率
