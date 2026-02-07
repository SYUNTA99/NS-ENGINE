/// @file RHIPixelFormat.h
/// @brief ピクセルフォーマット列挙型・ヘルパー
/// @details テクスチャ/レンダーターゲットのフォーマット定義、カテゴリ分類、sRGB変換を提供。
/// @see 15-01-pixel-format-enum.md
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHIPixelFormat: ピクセルフォーマット
    //=========================================================================

    /// ピクセルフォーマット
    enum class ERHIPixelFormat : uint16
    {
        Unknown = 0,

        //=====================================================================
        // R（1チャンネル）
        //=====================================================================

        R8_UNORM,
        R8_SNORM,
        R8_UINT,
        R8_SINT,

        R16_UNORM,
        R16_SNORM,
        R16_UINT,
        R16_SINT,
        R16_FLOAT,

        R32_UINT,
        R32_SINT,
        R32_FLOAT,

        //=====================================================================
        // RG（2チャンネル）
        //=====================================================================

        R8G8_UNORM,
        R8G8_SNORM,
        R8G8_UINT,
        R8G8_SINT,

        R16G16_UNORM,
        R16G16_SNORM,
        R16G16_UINT,
        R16G16_SINT,
        R16G16_FLOAT,

        R32G32_UINT,
        R32G32_SINT,
        R32G32_FLOAT,

        //=====================================================================
        // RGB（3チャンネル）
        //=====================================================================

        R32G32B32_UINT,
        R32G32B32_SINT,
        R32G32B32_FLOAT,

        R11G11B10_FLOAT,

        //=====================================================================
        // RGBA（4チャンネル）
        //=====================================================================

        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_SNORM,
        R8G8B8A8_UINT,
        R8G8B8A8_SINT,

        B8G8R8A8_UNORM,
        B8G8R8A8_UNORM_SRGB,

        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,

        R16G16B16A16_UNORM,
        R16G16B16A16_SNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SINT,
        R16G16B16A16_FLOAT,

        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32A32_FLOAT,

        //=====================================================================
        // デプス/ステンシル
        //=====================================================================

        D16_UNORM,
        D24_UNORM_S8_UINT,
        D32_FLOAT,
        D32_FLOAT_S8X24_UINT,

        //=====================================================================
        // 圧縮フォーマット（BC）
        //=====================================================================

        BC1_UNORM, // DXT1
        BC1_UNORM_SRGB,
        BC2_UNORM, // DXT3
        BC2_UNORM_SRGB,
        BC3_UNORM, // DXT5
        BC3_UNORM_SRGB,
        BC4_UNORM, // ATI1/3Dc+
        BC4_SNORM,
        BC5_UNORM, // ATI2/3Dc
        BC5_SNORM,
        BC6H_UF16, // HDR
        BC6H_SF16,
        BC7_UNORM, // 高品質
        BC7_UNORM_SRGB,

        //=====================================================================
        // 特殊フォーマット
        //=====================================================================

        R9G9B9E5_SHAREDEXP, // 共有指数HDR

        /// フォーマット数
        Count,
    };

    //=========================================================================
    // ERHIFormatCategory / ERHIFormatType
    //=========================================================================

    /// フォーマットカテゴリ
    enum class ERHIFormatCategory : uint8
    {
        Unknown,
        Integer,      // 整数型
        Float,        // 浮動小数点型
        UNORM,        // 正規化符号なし
        SNORM,        // 正規化符号付き
        sRGB,         // sRGBガンマ
        DepthStencil, // デプス/ステンシル
        Compressed,   // 圧縮
        Special,      // 特殊
    };

    /// フォーマットタイプ
    enum class ERHIFormatType : uint8
    {
        Typeless,     // 型なし
        UNorm,        // 正規化符号なし [0, 1]
        SNorm,        // 正規化符号付き [-1, 1]
        UInt,         // 符号なし整数
        SInt,         // 符号付き整数
        Float,        // 浮動小数点
        sRGB,         // sRGBガンマ
        Depth,        // デプス
        Stencil,      // ステンシル
        DepthStencil, // デプス+ステンシル
    };

    //=========================================================================
    // ERHICompressionFamily / RHIBlockSize
    //=========================================================================

    /// 圧縮フォーマットファミリー
    enum class ERHICompressionFamily : uint8
    {
        None,
        BC,    // Block Compression (DXT/S3TC)
        ASTC,  // Adaptive Scalable Texture Compression
        ETC,   // Ericsson Texture Compression
        PVRTC, // PowerVR Texture Compression
    };

    /// ブロックサイズ
    struct RHI_API RHIBlockSize
    {
        uint8 width = 1;
        uint8 height = 1;
        uint8 depth = 1;

        /// 4x4ブロック
        static RHIBlockSize Block4x4() { return {4, 4, 1}; }

        /// 1x1（非圧縮）
        static RHIBlockSize Uncompressed() { return {1, 1, 1}; }
    };

    //=========================================================================
    // sRGB変換ヘルパー
    //=========================================================================

    namespace RHIPixelFormatSRGB
    {
        /// sRGB版があるか
        inline bool HasSRGBVariant(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::R8G8B8A8_UNORM:
            case ERHIPixelFormat::B8G8R8A8_UNORM:
            case ERHIPixelFormat::BC1_UNORM:
            case ERHIPixelFormat::BC2_UNORM:
            case ERHIPixelFormat::BC3_UNORM:
            case ERHIPixelFormat::BC7_UNORM:
                return true;
            default:
                return false;
            }
        }

        /// sRGB版を取得
        inline ERHIPixelFormat ToSRGB(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::R8G8B8A8_UNORM:
                return ERHIPixelFormat::R8G8B8A8_UNORM_SRGB;
            case ERHIPixelFormat::B8G8R8A8_UNORM:
                return ERHIPixelFormat::B8G8R8A8_UNORM_SRGB;
            case ERHIPixelFormat::BC1_UNORM:
                return ERHIPixelFormat::BC1_UNORM_SRGB;
            case ERHIPixelFormat::BC2_UNORM:
                return ERHIPixelFormat::BC2_UNORM_SRGB;
            case ERHIPixelFormat::BC3_UNORM:
                return ERHIPixelFormat::BC3_UNORM_SRGB;
            case ERHIPixelFormat::BC7_UNORM:
                return ERHIPixelFormat::BC7_UNORM_SRGB;
            default:
                return format;
            }
        }

        /// リニア版を取得
        inline ERHIPixelFormat ToLinear(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::R8G8B8A8_UNORM_SRGB:
                return ERHIPixelFormat::R8G8B8A8_UNORM;
            case ERHIPixelFormat::B8G8R8A8_UNORM_SRGB:
                return ERHIPixelFormat::B8G8R8A8_UNORM;
            case ERHIPixelFormat::BC1_UNORM_SRGB:
                return ERHIPixelFormat::BC1_UNORM;
            case ERHIPixelFormat::BC2_UNORM_SRGB:
                return ERHIPixelFormat::BC2_UNORM;
            case ERHIPixelFormat::BC3_UNORM_SRGB:
                return ERHIPixelFormat::BC3_UNORM;
            case ERHIPixelFormat::BC7_UNORM_SRGB:
                return ERHIPixelFormat::BC7_UNORM;
            default:
                return format;
            }
        }

        /// sRGBフォーマットか
        inline bool IsSRGB(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::R8G8B8A8_UNORM_SRGB:
            case ERHIPixelFormat::B8G8R8A8_UNORM_SRGB:
            case ERHIPixelFormat::BC1_UNORM_SRGB:
            case ERHIPixelFormat::BC2_UNORM_SRGB:
            case ERHIPixelFormat::BC3_UNORM_SRGB:
            case ERHIPixelFormat::BC7_UNORM_SRGB:
                return true;
            default:
                return false;
            }
        }
    } // namespace RHIPixelFormatSRGB

    //=========================================================================
    // RHIFormatInfo (15-02)
    //=========================================================================

    /// フォーマット情報
    struct RHI_API RHIFormatInfo
    {
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;
        const char* name = nullptr;
        uint8 bytesPerPixelOrBlock = 0;
        uint8 channelCount = 0;
        uint8 redBits = 0;
        uint8 greenBits = 0;
        uint8 blueBits = 0;
        uint8 alphaBits = 0;
        uint8 depthBits = 0;
        uint8 stencilBits = 0;
        RHIBlockSize blockSize;
        ERHIFormatCategory category = ERHIFormatCategory::Unknown;
        ERHIFormatType type = ERHIFormatType::Typeless;
        ERHICompressionFamily compression = ERHICompressionFamily::None;

        //=====================================================================
        // ヘルパー
        //=====================================================================

        bool IsCompressed() const { return compression != ERHICompressionFamily::None; }
        bool IsDepth() const { return depthBits > 0; }
        bool IsStencil() const { return stencilBits > 0; }
        bool IsDepthStencil() const { return IsDepth() || IsStencil(); }
        bool IsSRGB() const { return category == ERHIFormatCategory::sRGB; }
        bool IsFloat() const { return type == ERHIFormatType::Float; }
        bool IsInteger() const { return type == ERHIFormatType::UInt || type == ERHIFormatType::SInt; }
        bool IsNormalized() const { return type == ERHIFormatType::UNorm || type == ERHIFormatType::SNorm; }
        bool HasAlpha() const { return alphaBits > 0; }
        uint32 GetTotalBits() const { return bytesPerPixelOrBlock * 8; }
    };

    //=========================================================================
    // フォーマット情報取得関数 (15-02)
    //=========================================================================

    /// フォーマット情報取得
    RHI_API const RHIFormatInfo& GetFormatInfo(ERHIPixelFormat format);

    /// フォーマット名取得
    RHI_API const char* GetFormatName(ERHIPixelFormat format);

    /// ピクセル/ブロックサイズ取得
    RHI_API uint32 GetFormatBytesPerPixelOrBlock(ERHIPixelFormat format);

    /// ブロックサイズ取得
    RHI_API RHIBlockSize GetFormatBlockSize(ERHIPixelFormat format);

    /// 行ピッチ計算
    RHI_API uint32 CalculateRowPitch(ERHIPixelFormat format, uint32 width);

    /// スライスピッチ計算
    RHI_API uint32 CalculateSlicePitch(ERHIPixelFormat format, uint32 width, uint32 height);

    /// サブリソースサイズ計算
    RHI_API uint64 CalculateSubresourceSize(
        ERHIPixelFormat format,
        uint32 width,
        uint32 height,
        uint32 depth = 1);

    //=========================================================================
    // ERHIFormatSupportFlags (15-02)
    //=========================================================================

    /// 詳細フォーマットサポートフラグ
    enum class ERHIFormatSupportFlags : uint32
    {
        None = 0,
        Buffer = 1 << 0,
        IndexBuffer = 1 << 1,
        VertexBuffer = 1 << 2,
        Texture1D = 1 << 3,
        Texture2D = 1 << 4,
        Texture3D = 1 << 5,
        TextureCube = 1 << 6,
        ShaderLoad = 1 << 7,
        ShaderSample = 1 << 8,
        ShaderSampleComparison = 1 << 9,
        ShaderGather = 1 << 10,
        ShaderGatherComparison = 1 << 11,
        UAVLoad = 1 << 12,
        UAVStore = 1 << 13,
        UAVAtomics = 1 << 14,
        RenderTarget = 1 << 15,
        RenderTargetBlend = 1 << 16,
        DepthStencil = 1 << 17,
        Display = 1 << 18,
        Multisample2x = 1 << 19,
        Multisample4x = 1 << 20,
        Multisample8x = 1 << 21,
        Multisample16x = 1 << 22,
        MultisampleResolve = 1 << 23,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIFormatSupportFlags)

    //=========================================================================
    // RHIMSAASupportInfo (15-02)
    //=========================================================================

    /// MSAAサポート情報
    struct RHI_API RHIMSAASupportInfo
    {
        /// サポートされるサンプル数（ビットマスク）
        uint32 supportedSampleCounts = 0;
        uint32 maxSampleCount = 1;
        uint32 qualityLevels[5] = {}; ///< 1x, 2x, 4x, 8x, 16x

        bool IsSupported(uint32 sampleCount) const
        {
            switch (sampleCount)
            {
            case 1:
                return true;
            case 2:
                return (supportedSampleCounts & (1 << 1)) != 0;
            case 4:
                return (supportedSampleCounts & (1 << 2)) != 0;
            case 8:
                return (supportedSampleCounts & (1 << 3)) != 0;
            case 16:
                return (supportedSampleCounts & (1 << 4)) != 0;
            default:
                return false;
            }
        }
    };

} // namespace NS::RHI
