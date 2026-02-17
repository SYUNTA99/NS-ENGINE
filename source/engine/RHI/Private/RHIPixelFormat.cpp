/// @file RHIPixelFormat.cpp
/// @brief ピクセルフォーマット情報テーブル・ヘルパー関数実装
#include "RHIPixelFormat.h"

namespace NS::RHI
{
    //=========================================================================
    // フォーマット情報テーブル
    //=========================================================================

    // clang-format off
    static const RHIFormatInfo s_formatInfoTable[] =
    {
        // format, name, bpp, ch, r, g, b, a, d, s, block, category, type, compression

        // Unknown
        { .format=ERHIPixelFormat::Unknown, .name="Unknown", .bytesPerPixelOrBlock=0, .channelCount=0, .redBits=0, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Unknown, .type=ERHIFormatType::Typeless, .compression=ERHICompressionFamily::None },

        // R8
        { .format=ERHIPixelFormat::R8_UNORM,  .name="R8_UNORM",  .bytesPerPixelOrBlock=1, .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8_SNORM,  .name="R8_SNORM",  .bytesPerPixelOrBlock=1, .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8_UINT,   .name="R8_UINT",   .bytesPerPixelOrBlock=1, .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8_SINT,   .name="R8_SINT",   .bytesPerPixelOrBlock=1, .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },

        // R16
        { .format=ERHIPixelFormat::R16_UNORM, .name="R16_UNORM", .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=16, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16_SNORM, .name="R16_SNORM", .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=16, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16_UINT,  .name="R16_UINT",  .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=16, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16_SINT,  .name="R16_SINT",  .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=16, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16_FLOAT, .name="R16_FLOAT", .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=16, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // R32
        { .format=ERHIPixelFormat::R32_UINT,  .name="R32_UINT",  .bytesPerPixelOrBlock=4, .channelCount=1, .redBits=32, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32_SINT,  .name="R32_SINT",  .bytesPerPixelOrBlock=4, .channelCount=1, .redBits=32, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32_FLOAT, .name="R32_FLOAT", .bytesPerPixelOrBlock=4, .channelCount=1, .redBits=32, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // RG8
        { .format=ERHIPixelFormat::R8G8_UNORM, .name="R8G8_UNORM", .bytesPerPixelOrBlock=2, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8_SNORM, .name="R8G8_SNORM", .bytesPerPixelOrBlock=2, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8_UINT,  .name="R8G8_UINT",  .bytesPerPixelOrBlock=2, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8_SINT,  .name="R8G8_SINT",  .bytesPerPixelOrBlock=2, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },

        // RG16
        { .format=ERHIPixelFormat::R16G16_UNORM, .name="R16G16_UNORM", .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=16, .greenBits=16, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16_SNORM, .name="R16G16_SNORM", .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=16, .greenBits=16, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16_UINT,  .name="R16G16_UINT",  .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=16, .greenBits=16, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16_SINT,  .name="R16G16_SINT",  .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=16, .greenBits=16, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16_FLOAT, .name="R16G16_FLOAT", .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=16, .greenBits=16, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // RG32
        { .format=ERHIPixelFormat::R32G32_UINT,  .name="R32G32_UINT",  .bytesPerPixelOrBlock=8, .channelCount=2, .redBits=32, .greenBits=32, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32_SINT,  .name="R32G32_SINT",  .bytesPerPixelOrBlock=8, .channelCount=2, .redBits=32, .greenBits=32, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32_FLOAT, .name="R32G32_FLOAT", .bytesPerPixelOrBlock=8, .channelCount=2, .redBits=32, .greenBits=32, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // RGB32
        { .format=ERHIPixelFormat::R32G32B32_UINT,  .name="R32G32B32_UINT",  .bytesPerPixelOrBlock=12, .channelCount=3, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32B32_SINT,  .name="R32G32B32_SINT",  .bytesPerPixelOrBlock=12, .channelCount=3, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32B32_FLOAT, .name="R32G32B32_FLOAT", .bytesPerPixelOrBlock=12, .channelCount=3, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // R11G11B10
        { .format=ERHIPixelFormat::R11G11B10_FLOAT, .name="R11G11B10_FLOAT", .bytesPerPixelOrBlock=4, .channelCount=3, .redBits=11, .greenBits=11, .blueBits=10, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // RGBA8
        { .format=ERHIPixelFormat::R8G8B8A8_UNORM,      .name="R8G8B8A8_UNORM",      .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8B8A8_UNORM_SRGB,  .name="R8G8B8A8_UNORM_SRGB", .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::sRGB, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8B8A8_SNORM,       .name="R8G8B8A8_SNORM",      .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8B8A8_UINT,        .name="R8G8B8A8_UINT",       .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R8G8B8A8_SINT,        .name="R8G8B8A8_SINT",       .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },

        // BGRA8
        { .format=ERHIPixelFormat::B8G8R8A8_UNORM,      .name="B8G8R8A8_UNORM",      .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::B8G8R8A8_UNORM_SRGB, .name="B8G8R8A8_UNORM_SRGB", .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::sRGB, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::None },

        // R10G10B10A2
        { .format=ERHIPixelFormat::R10G10B10A2_UNORM, .name="R10G10B10A2_UNORM", .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=10, .greenBits=10, .blueBits=10, .alphaBits=2, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R10G10B10A2_UINT,  .name="R10G10B10A2_UINT",  .bytesPerPixelOrBlock=4, .channelCount=4, .redBits=10, .greenBits=10, .blueBits=10, .alphaBits=2, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },

        // RGBA16
        { .format=ERHIPixelFormat::R16G16B16A16_UNORM, .name="R16G16B16A16_UNORM", .bytesPerPixelOrBlock=8, .channelCount=4, .redBits=16, .greenBits=16, .blueBits=16, .alphaBits=16, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::UNORM, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16B16A16_SNORM, .name="R16G16B16A16_SNORM", .bytesPerPixelOrBlock=8, .channelCount=4, .redBits=16, .greenBits=16, .blueBits=16, .alphaBits=16, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::SNORM, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16B16A16_UINT,  .name="R16G16B16A16_UINT",  .bytesPerPixelOrBlock=8, .channelCount=4, .redBits=16, .greenBits=16, .blueBits=16, .alphaBits=16, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16B16A16_SINT,  .name="R16G16B16A16_SINT",  .bytesPerPixelOrBlock=8, .channelCount=4, .redBits=16, .greenBits=16, .blueBits=16, .alphaBits=16, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R16G16B16A16_FLOAT, .name="R16G16B16A16_FLOAT", .bytesPerPixelOrBlock=8, .channelCount=4, .redBits=16, .greenBits=16, .blueBits=16, .alphaBits=16, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // RGBA32
        { .format=ERHIPixelFormat::R32G32B32A32_UINT,  .name="R32G32B32A32_UINT",  .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=32, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::UInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32B32A32_SINT,  .name="R32G32B32A32_SINT",  .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=32, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Integer, .type=ERHIFormatType::SInt, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::R32G32B32A32_FLOAT, .name="R32G32B32A32_FLOAT", .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=32, .greenBits=32, .blueBits=32, .alphaBits=32, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Float, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },

        // Depth/Stencil
        { .format=ERHIPixelFormat::D16_UNORM,             .name="D16_UNORM",             .bytesPerPixelOrBlock=2, .channelCount=1, .redBits=0, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=16, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::DepthStencil, .type=ERHIFormatType::Depth, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::D24_UNORM_S8_UINT,     .name="D24_UNORM_S8_UINT",     .bytesPerPixelOrBlock=4, .channelCount=2, .redBits=0, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=24, .stencilBits=8, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::DepthStencil, .type=ERHIFormatType::DepthStencil, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::D32_FLOAT,             .name="D32_FLOAT",             .bytesPerPixelOrBlock=4, .channelCount=1, .redBits=0, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=32, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::DepthStencil, .type=ERHIFormatType::Depth, .compression=ERHICompressionFamily::None },
        { .format=ERHIPixelFormat::D32_FLOAT_S8X24_UINT,  .name="D32_FLOAT_S8X24_UINT",  .bytesPerPixelOrBlock=8, .channelCount=2, .redBits=0, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=32, .stencilBits=8, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::DepthStencil, .type=ERHIFormatType::DepthStencil, .compression=ERHICompressionFamily::None },

        // BC1-BC7 (4x4ブロック)
        { .format=ERHIPixelFormat::BC1_UNORM,      .name="BC1_UNORM",      .bytesPerPixelOrBlock=8,  .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=1, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC1_UNORM_SRGB, .name="BC1_UNORM_SRGB", .bytesPerPixelOrBlock=8,  .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=1, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC2_UNORM,      .name="BC2_UNORM",      .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=4, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC2_UNORM_SRGB, .name="BC2_UNORM_SRGB", .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=4, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC3_UNORM,      .name="BC3_UNORM",      .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC3_UNORM_SRGB, .name="BC3_UNORM_SRGB", .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=5, .greenBits=6, .blueBits=5, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC4_UNORM,      .name="BC4_UNORM",      .bytesPerPixelOrBlock=8,  .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC4_SNORM,      .name="BC4_SNORM",      .bytesPerPixelOrBlock=8,  .channelCount=1, .redBits=8, .greenBits=0, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC5_UNORM,      .name="BC5_UNORM",      .bytesPerPixelOrBlock=16, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC5_SNORM,      .name="BC5_SNORM",      .bytesPerPixelOrBlock=16, .channelCount=2, .redBits=8, .greenBits=8, .blueBits=0, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::SNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC6H_UF16,      .name="BC6H_UF16",      .bytesPerPixelOrBlock=16, .channelCount=3, .redBits=16,.greenBits=16,.blueBits=16, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC6H_SF16,      .name="BC6H_SF16",      .bytesPerPixelOrBlock=16, .channelCount=3, .redBits=16,.greenBits=16,.blueBits=16, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC7_UNORM,      .name="BC7_UNORM",      .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::UNorm, .compression=ERHICompressionFamily::BC },
        { .format=ERHIPixelFormat::BC7_UNORM_SRGB, .name="BC7_UNORM_SRGB", .bytesPerPixelOrBlock=16, .channelCount=4, .redBits=8, .greenBits=8, .blueBits=8, .alphaBits=8, .depthBits=0, .stencilBits=0, .blockSize={.width=4,.height=4,.depth=1}, .category=ERHIFormatCategory::Compressed, .type=ERHIFormatType::sRGB, .compression=ERHICompressionFamily::BC },

        // Special
        { .format=ERHIPixelFormat::R9G9B9E5_SHAREDEXP, .name="R9G9B9E5_SHAREDEXP", .bytesPerPixelOrBlock=4, .channelCount=3, .redBits=9, .greenBits=9, .blueBits=9, .alphaBits=0, .depthBits=0, .stencilBits=0, .blockSize={.width=1,.height=1,.depth=1}, .category=ERHIFormatCategory::Special, .type=ERHIFormatType::Float, .compression=ERHICompressionFamily::None },
    };
    // clang-format on

    static constexpr uint32 kFormatInfoCount =
        static_cast<uint32>(sizeof(s_formatInfoTable) / sizeof(s_formatInfoTable[0]));

    static const RHIFormatInfo s_unknownFormat = {};

    //=========================================================================
    // フォーマット情報取得関数
    //=========================================================================

    const RHIFormatInfo& GetFormatInfo(ERHIPixelFormat format)
    {
        auto const index = static_cast<uint32>(format);
        if (index < kFormatInfoCount) {
            return s_formatInfoTable[index];
}
        return s_unknownFormat;
    }

    const char* GetFormatName(ERHIPixelFormat format)
    {
        const auto& info = GetFormatInfo(format);
        return (info.name != nullptr) ? info.name : "Unknown";
    }

    uint32 GetFormatBytesPerPixelOrBlock(ERHIPixelFormat format)
    {
        return GetFormatInfo(format).bytesPerPixelOrBlock;
    }

    RHIBlockSize GetFormatBlockSize(ERHIPixelFormat format)
    {
        return GetFormatInfo(format).blockSize;
    }

    uint32 CalculateRowPitch(ERHIPixelFormat format, uint32 width)
    {
        const auto& info = GetFormatInfo(format);
        if (info.IsCompressed())
        {
            // ブロック数 * ブロックサイズ
            uint32 const blockWidth = info.blockSize.width;
            uint32 const numBlocksX = (width + blockWidth - 1) / blockWidth;
            return numBlocksX * info.bytesPerPixelOrBlock;
        }
        return width * info.bytesPerPixelOrBlock;
    }

    uint32 CalculateSlicePitch(ERHIPixelFormat format, uint32 width, uint32 height)
    {
        const auto& info = GetFormatInfo(format);
        uint32 const rowPitch = CalculateRowPitch(format, width);

        if (info.IsCompressed())
        {
            uint32 const blockHeight = info.blockSize.height;
            uint32 const numBlocksY = (height + blockHeight - 1) / blockHeight;
            return rowPitch * numBlocksY;
        }
        return rowPitch * height;
    }

    uint64 CalculateSubresourceSize(ERHIPixelFormat format, uint32 width, uint32 height, uint32 depth)
    {
        uint32 const slicePitch = CalculateSlicePitch(format, width, height);
        return static_cast<uint64>(slicePitch) * depth;
    }

} // namespace NS::RHI
