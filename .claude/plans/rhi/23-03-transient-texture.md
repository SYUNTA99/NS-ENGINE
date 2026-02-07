# 23-03: Transientテクスチャ

## 概要

フレーム内の一時的に使用するテクスチャリソース、
GBuffer、シャドウマップ、ポストプロセス中間バッファ等に使用、

## ファイル

- `Source/Engine/RHICore/Public/RHITransientTexture.h`

## 依存

- 23-01-transient-allocator.md (Transientアロケーター)
- 04-02-texture-interface.md (IRHITexture)

## 定義

```cpp
namespace NS::RHI
{

/// Transientテクスチャ使用フラグ
enum class ERHITransientTextureUsage : uint32
{
    None = 0,
    RenderTarget = 1 << 0,      ///< レンダーターゲット
    DepthStencil = 1 << 1,      ///< デプスステンシル
    ShaderResource = 1 << 2,    ///< シェーダーリソース
    UnorderedAccess = 1 << 3,   ///< UAV
    CopySource = 1 << 4,        ///< コピー元
    CopyDest = 1 << 5,          ///< コピー先
};
NS_ENUM_FLAGS(ERHITransientTextureUsage);

/// Transientテクスチャ記述
struct RHITransientTextureCreateInfo
{
    uint32 width = 1;
    uint32 height = 1;
    uint32 depth = 1;
    uint32 mipLevels = 1;
    uint32 arraySize = 1;
    ERHIPixelFormat format = ERHIPixelFormat::RGBA8_UNORM;
    ERHITransientTextureUsage usage = ERHITransientTextureUsage::None;
    ERHITextureDimension dimension = ERHITextureDimension::Texture2D;
    uint32 sampleCount = 1;
    RHIClearValue clearValue;
    const char* debugName = nullptr;

    /// 必要メモリサイズの概算
    uint64 EstimateMemorySize() const;
};

/// Transientテクスチャハンドル
class RHITransientTextureHandle
{
public:
    RHITransientTextureHandle() = default;

    bool IsValid() const { return m_handle != kInvalidHandle; }

    /// 実際のテクスチャ取得（Acquire後のみ有効）。
    IRHITexture* GetTexture() const;

    /// SRV取得
    IRHIShaderResourceView* GetSRV() const;

    /// RTV取得
    IRHIRenderTargetView* GetRTV() const;

    /// DSV取得
    IRHIDepthStencilView* GetDSV() const;

    /// UAV取得
    IRHIUnorderedAccessView* GetUAV() const;

    /// プロパティ
    uint32 GetWidth() const { return m_info.width; }
    uint32 GetHeight() const { return m_info.height; }
    ERHIPixelFormat GetFormat() const { return m_info.format; }

private:
    friend class RHITransientResourceSystem;

    static constexpr uint32 kInvalidHandle = UINT32_MAX;

    uint32 m_handle = kInvalidHandle;
    RHITransientTextureCreateInfo m_info;
    IRHITexture* m_acquiredTexture = nullptr;
};

// ════════════════════════════════════════════════════════════════
// 便利なTransientテクスチャ生成ヘルパー
// ════════════════════════════════════════════════════════════════

namespace RHITransientTextures
{
    /// レンダーターゲット用
    inline RHITransientTextureCreateInfo RenderTarget(
        uint32 width, uint32 height,
        ERHIPixelFormat format,
        const char* name = nullptr)
    {
        return {
            width, height, 1, 1, 1,
            format,
            ERHITransientTextureUsage::RenderTarget | ERHITransientTextureUsage::ShaderResource,
            ERHITextureDimension::Texture2D,
            1, {}, name
        };
    }

    /// デプスバッファ用
    inline RHITransientTextureCreateInfo DepthBuffer(
        uint32 width, uint32 height,
        ERHIPixelFormat format = ERHIPixelFormat::D32_FLOAT,
        const char* name = nullptr)
    {
        return {
            width, height, 1, 1, 1,
            format,
            ERHITransientTextureUsage::DepthStencil | ERHITransientTextureUsage::ShaderResource,
            ERHITextureDimension::Texture2D,
            1, { .depthStencil = { 1.0f, 0 } }, name
        };
    }

    /// UAVテクスチャ用
    inline RHITransientTextureCreateInfo UAVTexture(
        uint32 width, uint32 height,
        ERHIPixelFormat format,
        const char* name = nullptr)
    {
        return {
            width, height, 1, 1, 1,
            format,
            ERHITransientTextureUsage::UnorderedAccess | ERHITransientTextureUsage::ShaderResource,
            ERHITextureDimension::Texture2D,
            1, {}, name
        };
    }

    /// MSAA レンダーターゲット
    inline RHITransientTextureCreateInfo MSAARenderTarget(
        uint32 width, uint32 height,
        ERHIPixelFormat format,
        uint32 sampleCount,
        const char* name = nullptr)
    {
        return {
            width, height, 1, 1, 1,
            format,
            ERHITransientTextureUsage::RenderTarget,
            ERHITextureDimension::Texture2D,
            sampleCount, {}, name
        };
    }

    /// シャドウマップ用
    inline RHITransientTextureCreateInfo ShadowMap(
        uint32 size,
        const char* name = nullptr)
    {
        return {
            size, size, 1, 1, 1,
            ERHIPixelFormat::D32_FLOAT,
            ERHITransientTextureUsage::DepthStencil | ERHITransientTextureUsage::ShaderResource,
            ERHITextureDimension::Texture2D,
            1, { .depthStencil = { 1.0f, 0 } }, name
        };
    }

    /// カスケードシャドウマップ用（配列）
    inline RHITransientTextureCreateInfo CascadeShadowMap(
        uint32 size, uint32 cascadeCount,
        const char* name = nullptr)
    {
        return {
            size, size, 1, 1, cascadeCount,
            ERHIPixelFormat::D32_FLOAT,
            ERHITransientTextureUsage::DepthStencil | ERHITransientTextureUsage::ShaderResource,
            ERHITextureDimension::Texture2DArray,
            1, { .depthStencil = { 1.0f, 0 } }, name
        };
    }

    /// GBufferセット
    struct GBufferSet
    {
        RHITransientTextureCreateInfo albedo;
        RHITransientTextureCreateInfo normal;
        RHITransientTextureCreateInfo material;
        RHITransientTextureCreateInfo depth;
    };

    inline GBufferSet CreateGBufferSet(uint32 width, uint32 height)
    {
        return {
            RenderTarget(width, height, ERHIPixelFormat::RGBA8_UNORM, "GBuffer_Albedo"),
            RenderTarget(width, height, ERHIPixelFormat::RGB10A2_UNORM, "GBuffer_Normal"),
            RenderTarget(width, height, ERHIPixelFormat::RGBA8_UNORM, "GBuffer_Material"),
            DepthBuffer(width, height, ERHIPixelFormat::D32_FLOAT, "GBuffer_Depth")
        };
    }
}

} // namespace NS::RHI
```

## 使用例

```cpp
// フレームグラフでのGBuffer使用
void SetupDeferredPipeline(RHIFrameGraph& fg, uint32 width, uint32 height)
{
    auto gbuffer = RHITransientTextures::CreateGBufferSet(width, height);

    auto albedoRT = fg.CreateTransientTexture(gbuffer.albedo, { 0, 1 });
    auto normalRT = fg.CreateTransientTexture(gbuffer.normal, { 0, 1 });
    auto depthRT = fg.CreateTransientTexture(gbuffer.depth, { 0, 2 });

    // シャドウマップ（パス0で作成、パス1で参照）。
    auto shadowMap = fg.CreateTransientTexture(
        RHITransientTextures::ShadowMap(2048, "MainShadow"),
        { 0, 1 }
    );

    // ライティング結果（パス1で作成）。
    // シャドウマップとメモリエイリアス可能
    auto lightingRT = fg.CreateTransientTexture(
        RHITransientTextures::RenderTarget(width, height, ERHIPixelFormat::RGBA16_FLOAT, "Lighting"),
        { 1, 2 }
    );

    // パス定義...
}
```

## 検証

- [ ] 各フォーマットの動作
- [ ] MSAA対応
- [ ] 配列テクスチャ対応
- [ ] クリア値の適用
