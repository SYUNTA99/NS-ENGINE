/// @file RHIFormatConversion.h
/// @brief ピクセルフォーマット変換・互換性・デプスステンシル/HDRヘルパー
/// @details フォーマット間の変換可能性判定、互換グループ、デプスステンシル変換、
///          HDRフォーマット選択、プラットフォーム変換を提供。
/// @see 15-03-format-conversion.md
#pragma once

#include "RHIMacros.h"
#include "RHIPixelFormat.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIFormatConversionType (15-03)
    //=========================================================================

    /// フォーマット変換タイプ
    enum class ERHIFormatConversionType : uint8
    {
        None,              ///< 変換不可
        DirectCast,        ///< 直接キャスト可能（同一メモリレイアウト）
        TypeCast,          ///< 型変換が必要（同一チャンネル構成）
        ChannelConversion, ///< チャンネル変換が必要
        Compression,       ///< 圧縮/展開が必要
        Full,              ///< フル変換が必要
    };

    //=========================================================================
    // RHIFormatConversion (15-03)
    //=========================================================================

    /// フォーマット変換ヘルパー
    namespace RHIFormatConversion
    {
        /// 変換タイプ取得
        RHI_API ERHIFormatConversionType GetConversionType(ERHIPixelFormat srcFormat, ERHIPixelFormat dstFormat);

        /// 直接キャスト可能か
        inline bool CanDirectCast(ERHIPixelFormat srcFormat, ERHIPixelFormat dstFormat)
        {
            return GetConversionType(srcFormat, dstFormat) == ERHIFormatConversionType::DirectCast;
        }

        /// 変換可能か
        inline bool CanConvert(ERHIPixelFormat srcFormat, ERHIPixelFormat dstFormat)
        {
            return GetConversionType(srcFormat, dstFormat) != ERHIFormatConversionType::None;
        }
    } // namespace RHIFormatConversion

    //=========================================================================
    // ERHIFormatCompatibilityGroup (15-03)
    //=========================================================================

    /// 互換フォーマットグループ
    /// 直接キャスト可能なフォーマットのグループ
    enum class ERHIFormatCompatibilityGroup : uint8
    {
        None,
        R8,
        R16,
        R32,
        RG8,
        RG16,
        RG32,
        RGBA8,
        RGBA16,
        RGBA32,
        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,
        D24S8,
        D32S8,
    };

    /// フォーマット互換グループ取得
    RHI_API ERHIFormatCompatibilityGroup GetFormatCompatibilityGroup(ERHIPixelFormat format);

    /// 同一互換グループか
    inline bool AreFormatsCompatible(ERHIPixelFormat a, ERHIPixelFormat b)
    {
        auto groupA = GetFormatCompatibilityGroup(a);
        auto groupB = GetFormatCompatibilityGroup(b);
        return groupA != ERHIFormatCompatibilityGroup::None && groupA == groupB;
    }

    //=========================================================================
    // RHIDepthStencilFormat (15-03)
    //=========================================================================

    /// デプスステンシルフォーマットヘルパー
    namespace RHIDepthStencilFormat
    {
        /// デプス専用フォーマットか
        inline bool IsDepthOnly(ERHIPixelFormat format)
        {
            return format == ERHIPixelFormat::D16_UNORM || format == ERHIPixelFormat::D32_FLOAT;
        }

        /// ステンシル付きか
        inline bool HasStencil(ERHIPixelFormat format)
        {
            return format == ERHIPixelFormat::D24_UNORM_S8_UINT || format == ERHIPixelFormat::D32_FLOAT_S8X24_UINT;
        }

        /// デプス読み取り用SRVフォーマット取得
        inline ERHIPixelFormat GetDepthSRVFormat(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::D16_UNORM:
                return ERHIPixelFormat::R16_UNORM;
            case ERHIPixelFormat::D24_UNORM_S8_UINT:
                return ERHIPixelFormat::R32_FLOAT;
            case ERHIPixelFormat::D32_FLOAT:
                return ERHIPixelFormat::R32_FLOAT;
            case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
                return ERHIPixelFormat::R32_FLOAT;
            default:
                return ERHIPixelFormat::Unknown;
            }
        }

        /// ステンシル読み取り用SRVフォーマット取得
        inline ERHIPixelFormat GetStencilSRVFormat(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::D24_UNORM_S8_UINT:
                return ERHIPixelFormat::R8_UINT;
            case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
                return ERHIPixelFormat::R8_UINT;
            default:
                return ERHIPixelFormat::Unknown;
            }
        }

        /// 推奨デプスフォーマット取得
        inline ERHIPixelFormat GetRecommendedDepthFormat(bool needsStencil, bool highPrecision)
        {
            if (needsStencil)
                return highPrecision ? ERHIPixelFormat::D32_FLOAT_S8X24_UINT : ERHIPixelFormat::D24_UNORM_S8_UINT;
            else
                return highPrecision ? ERHIPixelFormat::D32_FLOAT : ERHIPixelFormat::D16_UNORM;
        }
    } // namespace RHIDepthStencilFormat

    //=========================================================================
    // RHIHDRFormat (15-03)
    //=========================================================================

    /// HDRフォーマットヘルパー
    namespace RHIHDRFormat
    {
        /// HDRフォーマットか
        inline bool IsHDR(ERHIPixelFormat format)
        {
            switch (format)
            {
            case ERHIPixelFormat::R16_FLOAT:
            case ERHIPixelFormat::R16G16_FLOAT:
            case ERHIPixelFormat::R16G16B16A16_FLOAT:
            case ERHIPixelFormat::R32_FLOAT:
            case ERHIPixelFormat::R32G32_FLOAT:
            case ERHIPixelFormat::R32G32B32_FLOAT:
            case ERHIPixelFormat::R32G32B32A32_FLOAT:
            case ERHIPixelFormat::R11G11B10_FLOAT:
            case ERHIPixelFormat::R9G9B9E5_SHAREDEXP:
            case ERHIPixelFormat::R10G10B10A2_UNORM:
            case ERHIPixelFormat::BC6H_UF16:
            case ERHIPixelFormat::BC6H_SF16:
                return true;
            default:
                return false;
            }
        }

        /// 推奨HDRフォーマット取得
        inline ERHIPixelFormat GetRecommendedHDRFormat(bool needsAlpha, bool highPrecision)
        {
            if (needsAlpha)
                return highPrecision ? ERHIPixelFormat::R32G32B32A32_FLOAT : ERHIPixelFormat::R16G16B16A16_FLOAT;
            else
                return highPrecision ? ERHIPixelFormat::R32G32B32_FLOAT : ERHIPixelFormat::R11G11B10_FLOAT;
        }

        /// HDR10出力フォーマット取得
        inline ERHIPixelFormat GetHDR10DisplayFormat()
        {
            return ERHIPixelFormat::R10G10B10A2_UNORM;
        }
    } // namespace RHIHDRFormat

}} // namespace NS::RHI
