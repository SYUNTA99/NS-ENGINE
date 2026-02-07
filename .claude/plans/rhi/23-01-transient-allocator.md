# 23-01: Transientアロケーター

## 概要

フレーム内でのみ有効な一時リソースを効率的に割り当てるアロケーター、
メモリエイリアシングにより、異なるリソースが同じ物理メモリを共有、

## ファイル

- `Source/Engine/RHICore/Public/RHITransientAllocator.h`
- `Source/Engine/RHI/Public/IRHITransientResourceAllocator.h`

## 依存

- 03-02-buffer-interface.md (IRHIBuffer)
- 04-02-texture-interface.md (IRHITexture)
- 11-01-heap-types.md (ERHIHeapType)

## 定義

```cpp
namespace NS::RHI
{

// ════════════════════════════════════════════════════════════════
// Transientリソース寿命
// ════════════════════════════════════════════════════════════════

/// Transientリソースの使用範囲
struct RHITransientResourceLifetime
{
    uint32 firstPassIndex = 0;      ///< 最初に使用するパスのインデックス
    uint32 lastPassIndex = 0;       ///< 最後に使用するパスのインデックス

    bool Overlaps(const RHITransientResourceLifetime& other) const
    {
        return !(lastPassIndex < other.firstPassIndex ||
                 other.lastPassIndex < firstPassIndex);
    }
};

// ════════════════════════════════════════════════════════════════
// Transientリソース記述
// ════════════════════════════════════════════════════════════════

/// Transientバッファ記述
struct RHITransientBufferDesc
{
    uint64 size = 0;
    ERHIBufferUsage usage = ERHIBufferUsage::Default;
    RHITransientResourceLifetime lifetime;
    const char* debugName = nullptr;
};

/// Transientテクスチャ記述
struct RHITransientTextureDesc
{
    uint32 width = 1;
    uint32 height = 1;
    uint32 depth = 1;
    ERHIPixelFormat format = ERHIPixelFormat::Unknown;
    ERHITextureUsage usage = ERHITextureUsage::Default;
    uint32 mipLevels = 1;
    uint32 sampleCount = 1;
    ERHITextureDimension dimension = ERHITextureDimension::Texture2D;
    RHITransientResourceLifetime lifetime;
    const char* debugName = nullptr;

    /// 必要なメモリサイズ概算
    uint64 EstimateMemorySize() const;
};

// ════════════════════════════════════════════════════════════════
// Transientリソースハンドル
// ════════════════════════════════════════════════════════════════

/// Transientバッファハンドル
/// 実際のIRHIBufferへのアクセスはAcquire後のみ有効
class RHITransientBuffer
{
public:
    RHITransientBuffer() = default;

    bool IsValid() const { return m_allocator != nullptr; }

    /// 実際のバッファを取得（使用範囲内でのみ呼び出し可）。
    IRHIBuffer* GetBuffer() const;

    /// バッファサイズ
    uint64 GetSize() const { return m_desc.size; }

private:
    friend class IRHITransientResourceAllocator;

    IRHITransientResourceAllocator* m_allocator = nullptr;
    uint32 m_handle = 0;
    RHITransientBufferDesc m_desc;
};

/// Transientテクスチャハンドル
class RHITransientTexture
{
public:
    RHITransientTexture() = default;

    bool IsValid() const { return m_allocator != nullptr; }

    /// 実際のテクスチャを取得（使用範囲内でのみ呼び出し可）。
    IRHITexture* GetTexture() const;

    /// テクスチャ寸法
    uint32 GetWidth() const { return m_desc.width; }
    uint32 GetHeight() const { return m_desc.height; }
    ERHIPixelFormat GetFormat() const { return m_desc.format; }

private:
    friend class IRHITransientResourceAllocator;

    IRHITransientResourceAllocator* m_allocator = nullptr;
    uint32 m_handle = 0;
    RHITransientTextureDesc m_desc;
};

// ════════════════════════════════════════════════════════════════
// アロケーターインターフェース
// ════════════════════════════════════════════════════════════════

/// Transientリソースアロケーター統計
struct RHITransientAllocatorStats
{
    uint64 totalHeapSize = 0;           ///< ヒープ総サイズ
    uint64 peakUsedMemory = 0;          ///< ピーク使用メモリ
    uint64 currentUsedMemory = 0;       ///< 現在の使用メモリ
    uint64 aliasedMemorySaved = 0;      ///< エイリアシングで節約されたメモリ
    uint32 allocatedBuffers = 0;        ///< 割り当て済みバッファ数
    uint32 allocatedTextures = 0;       ///< 割り当て済みテクスチャ数
    uint32 reusedResources = 0;         ///< 再利用されたリソース数
};

/// Transientリソースアロケーター
/// フレーム内の使い捨てのリソースを効率的に管理
class IRHITransientResourceAllocator
{
public:
    virtual ~IRHITransientResourceAllocator() = default;

    // ────────────────────────────────────────────────────────────
    // フレーム管理
    // ────────────────────────────────────────────────────────────

    /// フレーム開始（リソースリセット）。
    virtual void BeginFrame() = 0;

    /// フレーム終了
    virtual void EndFrame() = 0;

    // ────────────────────────────────────────────────────────────
    // リソース割り当て
    // ────────────────────────────────────────────────────────────

    /// Transientバッファ割り当て
    virtual RHITransientBuffer AllocateBuffer(const RHITransientBufferDesc& desc) = 0;

    /// Transientテクスチャ割り当て
    virtual RHITransientTexture AllocateTexture(const RHITransientTextureDesc& desc) = 0;

    // ────────────────────────────────────────────────────────────
    // リソースアクセス
    // ────────────────────────────────────────────────────────────

    /// パス開始時にリソースをアクティブ化
    /// エイリアシングバリアが必要な場合に挿入
    virtual void AcquireResources(
        IRHICommandContext* context,
        uint32 passIndex) = 0;

    /// パス終了時にリソースを解放
    virtual void ReleaseResources(
        IRHICommandContext* context,
        uint32 passIndex) = 0;

    // ────────────────────────────────────────────────────────────
    // 統計
    // ────────────────────────────────────────────────────────────

    /// 統計情報取得
    virtual RHITransientAllocatorStats GetStats() const = 0;

    /// メモリ使用量のヒストグラムをログ出力
    virtual void DumpMemoryUsage() const = 0;

protected:
    /// ハンドルからバッファ取得（内部用）。
    virtual IRHIBuffer* GetBufferInternal(uint32 handle) const = 0;

    /// ハンドルからテクスチャ取得（内部用）。
    virtual IRHITexture* GetTextureInternal(uint32 handle) const = 0;
};

using RHITransientResourceAllocatorRef = TRefCountPtr<IRHITransientResourceAllocator>;

// ════════════════════════════════════════════════════════════════
// ファクトリ
// ════════════════════════════════════════════════════════════════

struct RHITransientAllocatorDesc
{
    uint64 initialHeapSize = 256 * 1024 * 1024;     ///< 初期ヒープサイズ（256MB）。
    uint64 maxHeapSize = 1024 * 1024 * 1024;        ///< 最大ヒープサイズ（GB）。
    bool allowGrowth = true;                         ///< ヒープの成長を許可
    const char* debugName = "TransientHeap";
};

// IRHIDeviceへの追加
// virtual RHITransientResourceAllocatorRef CreateTransientAllocator(
//     const RHITransientAllocatorDesc& desc) = 0;

} // namespace NS::RHI
```

## 使用例

```cpp
// フレームグラフでの使用
void RenderFrame(IRHITransientResourceAllocator* transientAllocator)
{
    transientAllocator->BeginFrame();

    // GBuffer割り当て（パス0-2で使用）。
    auto gbufferAlbedo = transientAllocator->AllocateTexture({
        .width = screenWidth,
        .height = screenHeight,
        .format = ERHIPixelFormat::RGBA8_UNORM,
        .usage = ERHITextureUsage::RenderTarget | ERHITextureUsage::ShaderResource,
        .lifetime = { 0, 2 },
        .debugName = "GBuffer_Albedo"
    });

    // シャドウマップ割り当て（パス0のみ）。
    auto shadowMap = transientAllocator->AllocateTexture({
        .width = 2048,
        .height = 2048,
        .format = ERHIPixelFormat::D32_FLOAT,
        .usage = ERHITextureUsage::DepthStencil | ERHITextureUsage::ShaderResource,
        .lifetime = { 0, 0 },
        .debugName = "ShadowMap"
    });

    // SSAOバッファ（パス2-3で使用）。
    // シャドウマップと同じメモリをエイリアス可能
    auto ssaoBuffer = transientAllocator->AllocateTexture({
        .width = screenWidth / 2,
        .height = screenHeight / 2,
        .format = ERHIPixelFormat::R8_UNORM,
        .usage = ERHITextureUsage::RenderTarget | ERHITextureUsage::ShaderResource,
        .lifetime = { 2, 3 },
        .debugName = "SSAO"
    });

    // パス0: シャドウマップ描画
    transientAllocator->AcquireResources(context, 0);
    RenderShadowMap(shadowMap.GetTexture());

    // パス1: GBuffer描画
    transientAllocator->AcquireResources(context, 1);
    RenderGBuffer(gbufferAlbedo.GetTexture());

    // パス2: SSAO（シャドウマップとメモリ共有）
    transientAllocator->AcquireResources(context, 2);
    RenderSSAO(ssaoBuffer.GetTexture());

    transientAllocator->EndFrame();
}
```

### 追加: マルチパイプライン同期

```cpp
namespace NS::RHI
{
    /// Transientリソース同期フェンス
    /// Graphics/AsyncComputeパイプライン間のエイリアシング同期
    struct RHI_API RHITransientAllocationFences
    {
        /// Graphicsパイプラインフェンス
        IRHIFence* graphicsFence = nullptr;
        uint64 graphicsFenceValue = 0;

        /// AsyncComputeパイプラインフェンス
        IRHIFence* asyncComputeFence = nullptr;
        uint64 asyncComputeFenceValue = 0;

        /// Graphics fork/joinフェンス
        IRHIFence* graphicsForkJoinFence = nullptr;
        uint64 graphicsForkJoinFenceValue = 0;
    };

    /// IRHITransientResourceAllocatorへの追加
    class IRHITransientResourceAllocator
    {
    public:
        /// パイプライン対応のリソースアクティブ化
        virtual void AcquireResourcesForPipeline(
            IRHICommandContext* context,
            uint32 passIndex,
            ERHIPipeline pipeline) = 0;

        /// パイプライン対応のリソース解放
        virtual void ReleaseResourcesForPipeline(
            IRHICommandContext* context,
            uint32 passIndex,
            ERHIPipeline pipeline) = 0;

        /// フェンス設定
        virtual void SetAllocationFences(
            const RHITransientAllocationFences& fences) = 0;

        /// AsyncComputeバジェット設定
        virtual void SetAsyncComputeBudget(
            ERHIAsyncComputeBudget budget) = 0;
    };

    /// AsyncComputeバジェット
    enum class ERHIAsyncComputeBudget : uint8
    {
        EAll_4 = 4,        ///< 全リソース使用可能
        EThreeQuarters_3 = 3,
        EHalf_2 = 2,
        EQuarter_1 = 1,
        ENone_0 = 0,       ///< AsyncCompute無効
    };
}
```

- [ ] RHITransientAllocationFences 構造体
- [ ] AcquireResourcesForPipeline / ReleaseResourcesForPipeline
- [ ] ERHIAsyncComputeBudget 列挙型

## 検証

- [ ] メモリエイリアシングの正確性
- [ ] ライフタイム追跡
- [ ] エイリアシングバリアの自動挿入
- [ ] 統計情報の正確性
