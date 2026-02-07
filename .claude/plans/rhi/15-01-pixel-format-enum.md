# 15-01: ピクセルフォーマット列挙型

## 目的

ピクセルフォーマットの列挙型を定義する。

## 参照ドキュメント

- 01-09-enums-texture.md (テクスチャ関連列挙型

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPixelFormat.h`

## TODO

### 1. ピクセルフォーマット列挙型

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// ピクセルフォーマット
    enum class ERHIPixelFormat : uint16
    {
        Unknown = 0,

        //=====================================================================
        // R（チャンネル）。
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
        // RG（チャンネル）。
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
        // RGB（チャンネル）。
        //=====================================================================

        R32G32B32_UINT,
        R32G32B32_SINT,
        R32G32B32_FLOAT,

        R11G11B10_FLOAT,

        //=====================================================================
        // RGBA（チャンネル）。
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
        // 圧縮フォーマット（BC）。
        //=====================================================================

        BC1_UNORM,          // DXT1
        BC1_UNORM_SRGB,
        BC2_UNORM,          // DXT3
        BC2_UNORM_SRGB,
        BC3_UNORM,          // DXT5
        BC3_UNORM_SRGB,
        BC4_UNORM,          // ATI1/3Dc+
        BC4_SNORM,
        BC5_UNORM,          // ATI2/3Dc
        BC5_SNORM,
        BC6H_UF16,          // HDR
        BC6H_SF16,
        BC7_UNORM,          // 高品質
        BC7_UNORM_SRGB,

        //=====================================================================
        // 特殊フォーマット
        //=====================================================================

        R9G9B9E5_SHAREDEXP, // 共有指数HDR

        /// フォーマット数
        Count,
    };
}
```

- [ ] ERHIPixelFormat 列挙型

### 2. フォーマットカテゴリ

```cpp
namespace NS::RHI
{
    /// フォーマットカテゴリ
    enum class ERHIFormatCategory : uint8
    {
        Unknown,
        Integer,        // 整数型
        Float,          // 浮動小数点型
        UNORM,          // 正規化符号なし
        SNORM,          // 正規化符号付き
        sRGB,           // sRGBガンマ
        DepthStencil,   // デプス/ステンシル
        Compressed,     // 圧縮
        Special,        // 特殊
    };

    /// フォーマットタイプ
    enum class ERHIFormatType : uint8
    {
        Typeless,       // 型なし
        UNorm,          // 正規化符号なし [0, 1]
        SNorm,          // 正規化符号付き [-1, 1]
        UInt,           // 符号なし整数
        SInt,           // 符号付き整数
        Float,          // 浮動小数点
        sRGB,           // sRGBガンマ
        Depth,          // デプス
        Stencil,        // ステンシル
        DepthStencil,   // デプス+ステンシル
    };
}
```

- [ ] ERHIFormatCategory 列挙型
- [ ] ERHIFormatType 列挙型

### 3. 圧縮フォーマットファミリー

```cpp
namespace NS::RHI
{
    /// 圧縮フォーマットファミリー
    enum class ERHICompressionFamily : uint8
    {
        None,
        BC,             // Block Compression (DXT/S3TC)
        ASTC,           // Adaptive Scalable Texture Compression
        ETC,            // Ericsson Texture Compression
        PVRTC,          // PowerVR Texture Compression
    };

    /// ブロックサイズ
    struct RHI_API RHIBlockSize
    {
        uint8 width = 1;
        uint8 height = 1;
        uint8 depth = 1;

        /// 4x4ブロック
        static RHIBlockSize Block4x4() { return {4, 4, 1}; }

        /// 1x1（非圧縮）。
        static RHIBlockSize Uncompressed() { return {1, 1, 1}; }
    };
}
```

- [ ] ERHICompressionFamily 列挙型
- [ ] RHIBlockSize 構造体

### 4. sRGBペア

```cpp
namespace NS::RHI
{
    /// sRGB変換ヘルパー
    namespace RHIPixelFormatSRGB
    {
        /// sRGB版があるか
        inline bool HasSRGBVariant(ERHIPixelFormat format)
        {
            switch (format) {
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
            switch (format) {
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
            switch (format) {
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
            switch (format) {
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
    }
}
```

- [ ] RHIPixelFormatSRGB ヘルパー

## 検証方法

- [ ] フォーマットの挙の網羅性
- [ ] カテゴリ類の正確性
- [ ] sRGB変換の正確性
