/// @file RHITransientTexture.h
/// @brief Transientテクスチャハンドル・ヘルパー
/// @details フレーム内一時テクスチャのハンドルと便利な生成ヘルパーを提供。
/// @see 23-03-transient-texture.md
#pragma once

#include "IRHITexture.h"
#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    // 前方宣言
    class IRHIShaderResourceView;
    class IRHIRenderTargetView;
    class IRHIDepthStencilView;
    class IRHIUnorderedAccessView;

    //=========================================================================
    // ERHITransientTextureUsage (23-03)
    //=========================================================================

    /// Transientテクスチャ使用フラグ
    enum class ERHITransientTextureUsage : uint32
    {
        None = 0,
        RenderTarget = 1 << 0,    ///< レンダーターゲット
        DepthStencil = 1 << 1,    ///< デプスステンシル
        ShaderResource = 1 << 2,  ///< シェーダーリソース
        UnorderedAccess = 1 << 3, ///< UAV
        CopySource = 1 << 4,      ///< コピー元
        CopyDest = 1 << 5,        ///< コピー先
    };
    RHI_ENUM_CLASS_FLAGS(ERHITransientTextureUsage)

    //=========================================================================
    // RHITransientTextureCreateInfo (23-03)
    //=========================================================================

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

    //=========================================================================
    // RHITransientTextureHandle (23-03)
    //=========================================================================

    /// Transientテクスチャハンドル
    class RHITransientTextureHandle
    {
    public:
        RHITransientTextureHandle() = default;

        bool IsValid() const { return m_handle != kInvalidHandle; }

        /// 実際のテクスチャ取得（Acquire後のみ有効）
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

    //=========================================================================
    // RHITransientTextures ヘルパー (23-03)
    //=========================================================================

    /// 便利なTransientテクスチャ生成ヘルパー
    namespace RHITransientTextures
    {
        /// レンダーターゲット用
        inline RHITransientTextureCreateInfo RenderTarget(uint32 width,
                                                          uint32 height,
                                                          ERHIPixelFormat format,
                                                          const char* name = nullptr)
        {
            RHITransientTextureCreateInfo info;
            info.width = width;
            info.height = height;
            info.format = format;
            info.usage = ERHITransientTextureUsage::RenderTarget | ERHITransientTextureUsage::ShaderResource;
            info.debugName = name;
            return info;
        }

        /// デプスバッファ用
        inline RHITransientTextureCreateInfo DepthBuffer(uint32 width,
                                                         uint32 height,
                                                         ERHIPixelFormat format = ERHIPixelFormat::D32_Float,
                                                         const char* name = nullptr)
        {
            RHITransientTextureCreateInfo info;
            info.width = width;
            info.height = height;
            info.format = format;
            info.usage = ERHITransientTextureUsage::DepthStencil | ERHITransientTextureUsage::ShaderResource;
            info.clearValue = RHIClearValue::Depth(1.0f, 0);
            info.debugName = name;
            return info;
        }

        /// UAVテクスチャ用
        inline RHITransientTextureCreateInfo UAVTexture(uint32 width,
                                                        uint32 height,
                                                        ERHIPixelFormat format,
                                                        const char* name = nullptr)
        {
            RHITransientTextureCreateInfo info;
            info.width = width;
            info.height = height;
            info.format = format;
            info.usage = ERHITransientTextureUsage::UnorderedAccess | ERHITransientTextureUsage::ShaderResource;
            info.debugName = name;
            return info;
        }

        /// MSAAレンダーターゲット
        inline RHITransientTextureCreateInfo MSAARenderTarget(
            uint32 width, uint32 height, ERHIPixelFormat format, uint32 sampleCount, const char* name = nullptr)
        {
            RHITransientTextureCreateInfo info;
            info.width = width;
            info.height = height;
            info.format = format;
            info.usage = ERHITransientTextureUsage::RenderTarget;
            info.sampleCount = sampleCount;
            info.debugName = name;
            return info;
        }

        /// シャドウマップ用
        inline RHITransientTextureCreateInfo ShadowMap(uint32 size, const char* name = nullptr)
        {
            return DepthBuffer(size, size, ERHIPixelFormat::D32_Float, name);
        }

        /// カスケードシャドウマップ用（配列）
        inline RHITransientTextureCreateInfo CascadeShadowMap(uint32 size,
                                                              uint32 cascadeCount,
                                                              const char* name = nullptr)
        {
            RHITransientTextureCreateInfo info;
            info.width = size;
            info.height = size;
            info.arraySize = cascadeCount;
            info.format = ERHIPixelFormat::D32_Float;
            info.usage = ERHITransientTextureUsage::DepthStencil | ERHITransientTextureUsage::ShaderResource;
            info.dimension = ERHITextureDimension::Texture2DArray;
            info.clearValue = RHIClearValue::Depth(1.0f, 0);
            info.debugName = name;
            return info;
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
            return {RenderTarget(width, height, ERHIPixelFormat::RGBA8_UNORM, "GBuffer_Albedo"),
                    RenderTarget(width, height, ERHIPixelFormat::RGB10A2_UNORM, "GBuffer_Normal"),
                    RenderTarget(width, height, ERHIPixelFormat::RGBA8_UNORM, "GBuffer_Material"),
                    DepthBuffer(width, height, ERHIPixelFormat::D32_Float, "GBuffer_Depth")};
        }
    } // namespace RHITransientTextures

}} // namespace NS::RHI
