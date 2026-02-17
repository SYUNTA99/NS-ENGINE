/// @file RHIFormatConversion.cpp
/// @brief ピクセルフォーマット変換・互換性判定実装
#include "RHIFormatConversion.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIFormatConversion
    //=========================================================================

    ERHIFormatConversionType RHIFormatConversion::GetConversionType(ERHIPixelFormat srcFormat,
                                                                    ERHIPixelFormat dstFormat)
    {
        if (srcFormat == dstFormat) {
            return ERHIFormatConversionType::DirectCast;
}

        if (srcFormat == ERHIPixelFormat::Unknown || dstFormat == ERHIPixelFormat::Unknown) {
            return ERHIFormatConversionType::None;
}

        // 同一互換グループなら DirectCast
        auto srcGroup = GetFormatCompatibilityGroup(srcFormat);
        auto dstGroup = GetFormatCompatibilityGroup(dstFormat);

        if (srcGroup != ERHIFormatCompatibilityGroup::None && srcGroup == dstGroup) {
            return ERHIFormatConversionType::DirectCast;
}

        // 圧縮フォーマットの判定
        const auto& srcInfo = GetFormatInfo(srcFormat);
        const auto& dstInfo = GetFormatInfo(dstFormat);

        bool const srcCompressed = srcInfo.IsCompressed();
        bool const dstCompressed = dstInfo.IsCompressed();

        if (srcCompressed != dstCompressed) {
            return ERHIFormatConversionType::Compression;
}

        // 同一チャンネル数で型が異なる場合
        if (srcInfo.channelCount == dstInfo.channelCount)
        {
            if (srcInfo.bytesPerPixelOrBlock == dstInfo.bytesPerPixelOrBlock) {
                return ERHIFormatConversionType::TypeCast;
            } 
                return ERHIFormatConversionType::ChannelConversion;
        }

        // チャンネル数が異なる
        return ERHIFormatConversionType::Full;
    }

    //=========================================================================
    // GetFormatCompatibilityGroup
    //=========================================================================

    ERHIFormatCompatibilityGroup GetFormatCompatibilityGroup(ERHIPixelFormat format)
    {
        switch (format)
        {
        // R8
        case ERHIPixelFormat::R8_UNORM:
        case ERHIPixelFormat::R8_SNORM:
        case ERHIPixelFormat::R8_UINT:
        case ERHIPixelFormat::R8_SINT:
            return ERHIFormatCompatibilityGroup::R8;

        // R16
        case ERHIPixelFormat::R16_UNORM:
        case ERHIPixelFormat::R16_SNORM:
        case ERHIPixelFormat::R16_UINT:
        case ERHIPixelFormat::R16_SINT:
        case ERHIPixelFormat::R16_FLOAT:
            return ERHIFormatCompatibilityGroup::R16;

        // R32
        case ERHIPixelFormat::R32_UINT:
        case ERHIPixelFormat::R32_SINT:
        case ERHIPixelFormat::R32_FLOAT:
            return ERHIFormatCompatibilityGroup::R32;

        // RG8
        case ERHIPixelFormat::R8G8_UNORM:
        case ERHIPixelFormat::R8G8_SNORM:
        case ERHIPixelFormat::R8G8_UINT:
        case ERHIPixelFormat::R8G8_SINT:
            return ERHIFormatCompatibilityGroup::RG8;

        // RG16
        case ERHIPixelFormat::R16G16_UNORM:
        case ERHIPixelFormat::R16G16_SNORM:
        case ERHIPixelFormat::R16G16_UINT:
        case ERHIPixelFormat::R16G16_SINT:
        case ERHIPixelFormat::R16G16_FLOAT:
            return ERHIFormatCompatibilityGroup::RG16;

        // RG32
        case ERHIPixelFormat::R32G32_UINT:
        case ERHIPixelFormat::R32G32_SINT:
        case ERHIPixelFormat::R32G32_FLOAT:
            return ERHIFormatCompatibilityGroup::RG32;

        // RGBA8
        case ERHIPixelFormat::R8G8B8A8_UNORM:
        case ERHIPixelFormat::R8G8B8A8_UNORM_SRGB:
        case ERHIPixelFormat::R8G8B8A8_SNORM:
        case ERHIPixelFormat::R8G8B8A8_UINT:
        case ERHIPixelFormat::R8G8B8A8_SINT:
        case ERHIPixelFormat::B8G8R8A8_UNORM:
        case ERHIPixelFormat::B8G8R8A8_UNORM_SRGB:
            return ERHIFormatCompatibilityGroup::RGBA8;

        // RGBA16
        case ERHIPixelFormat::R16G16B16A16_UNORM:
        case ERHIPixelFormat::R16G16B16A16_SNORM:
        case ERHIPixelFormat::R16G16B16A16_UINT:
        case ERHIPixelFormat::R16G16B16A16_SINT:
        case ERHIPixelFormat::R16G16B16A16_FLOAT:
            return ERHIFormatCompatibilityGroup::RGBA16;

        // RGBA32
        case ERHIPixelFormat::R32G32B32A32_UINT:
        case ERHIPixelFormat::R32G32B32A32_SINT:
        case ERHIPixelFormat::R32G32B32A32_FLOAT:
            return ERHIFormatCompatibilityGroup::RGBA32;

        // BC
        case ERHIPixelFormat::BC1_UNORM:
        case ERHIPixelFormat::BC1_UNORM_SRGB:
            return ERHIFormatCompatibilityGroup::BC1;
        case ERHIPixelFormat::BC2_UNORM:
        case ERHIPixelFormat::BC2_UNORM_SRGB:
            return ERHIFormatCompatibilityGroup::BC2;
        case ERHIPixelFormat::BC3_UNORM:
        case ERHIPixelFormat::BC3_UNORM_SRGB:
            return ERHIFormatCompatibilityGroup::BC3;
        case ERHIPixelFormat::BC4_UNORM:
        case ERHIPixelFormat::BC4_SNORM:
            return ERHIFormatCompatibilityGroup::BC4;
        case ERHIPixelFormat::BC5_UNORM:
        case ERHIPixelFormat::BC5_SNORM:
            return ERHIFormatCompatibilityGroup::BC5;
        case ERHIPixelFormat::BC6H_UF16:
        case ERHIPixelFormat::BC6H_SF16:
            return ERHIFormatCompatibilityGroup::BC6H;
        case ERHIPixelFormat::BC7_UNORM:
        case ERHIPixelFormat::BC7_UNORM_SRGB:
            return ERHIFormatCompatibilityGroup::BC7;

        // Depth
        case ERHIPixelFormat::D24_UNORM_S8_UINT:
            return ERHIFormatCompatibilityGroup::D24S8;
        case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
            return ERHIFormatCompatibilityGroup::D32S8;

        default:
            return ERHIFormatCompatibilityGroup::None;
        }
    }

} // namespace NS::RHI
