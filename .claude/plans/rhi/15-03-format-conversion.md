# 15-03: フォーマット変換

## 目的

ピクセルフォーマット間の変換機能を定義する。

## 参照ドキュメント

- 15-01-pixel-format-enum.md (ERHIPixelFormat)
- 15-02-format-info.md (RHIFormatInfo)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIFormatConversion.h`

## TODO

### 1. フォーマット変換可能性

```cpp
#pragma once

#include "RHIPixelFormat.h"

namespace NS::RHI
{
    /// フォーマット変換タイプ
    enum class ERHIFormatConversionType : uint8
    {
        /// 変換不可
        None,

        /// 直接キャスト可能（同一メモリレイアウト）
        DirectCast,

        /// 型変換が必要（同一チャンネル構（。
        TypeCast,

        /// チャンネル変換が必要。
        ChannelConversion,

        /// 圧縮/展開が必要。
        Compression,

        /// フル変換が必要。
        Full,
    };

    /// 変換可能性チェック
    namespace RHIFormatConversion
    {
        /// 変換タイプ取得
        RHI_API ERHIFormatConversionType GetConversionType(
            ERHIPixelFormat srcFormat,
            ERHIPixelFormat dstFormat);

        /// 直接キャスト可能か
        inline bool CanDirectCast(ERHIPixelFormat srcFormat, ERHIPixelFormat dstFormat) {
            return GetConversionType(srcFormat, dstFormat) == ERHIFormatConversionType::DirectCast;
        }

        /// 変換可能か
        inline bool CanConvert(ERHIPixelFormat srcFormat, ERHIPixelFormat dstFormat) {
            return GetConversionType(srcFormat, dstFormat) != ERHIFormatConversionType::None;
        }
    }
}
```

- [ ] ERHIFormatConversionType 列挙型
- [ ] GetConversionType

### 2. 互換フォーマットグループ

```cpp
namespace NS::RHI
{
    /// 互換フォーマットグループ
    /// 直接キャスト可能なフォーマットのグループ
    enum class ERHIFormatCompatibilityGroup : uint8
    {
        None,

        /// R8グループ
        R8,

        /// R16グループ
        R16,

        /// R32グループ
        R32,

        /// RG8グループ
        RG8,

        /// RG16グループ
        RG16,

        /// RG32グループ
        RG32,

        /// RGBA8グループ
        RGBA8,

        /// RGBA16グループ
        RGBA16,

        /// RGBA32グループ
        RGBA32,

        /// BC1グループ
        BC1,

        /// BC2グループ
        BC2,

        /// BC3グループ
        BC3,

        /// BC4グループ
        BC4,

        /// BC5グループ
        BC5,

        /// BC6Hグループ
        BC6H,

        /// BC7グループ
        BC7,

        /// D24S8グループ
        D24S8,

        /// D32S8グループ
        D32S8,
    };

    /// フォーマット互換グループ取得
    RHI_API ERHIFormatCompatibilityGroup GetFormatCompatibilityGroup(ERHIPixelFormat format);

    /// 同一互換グループか
    inline bool AreFormatsCompatible(ERHIPixelFormat a, ERHIPixelFormat b) {
        auto groupA = GetFormatCompatibilityGroup(a);
        auto groupB = GetFormatCompatibilityGroup(b);
        return groupA != ERHIFormatCompatibilityGroup::None && groupA == groupB;
    }
}
```

- [ ] ERHIFormatCompatibilityGroup 列挙型
- [ ] GetFormatCompatibilityGroup

### 3. デプスステンシルフォーマット変換

```cpp
namespace NS::RHI
{
    /// デプスステンシルフォーマットヘルパー。
    namespace RHIDepthStencilFormat
    {
        /// デプス専用フォーマットか
        inline bool IsDepthOnly(ERHIPixelFormat format) {
            return format == ERHIPixelFormat::D16_UNORM ||
                   format == ERHIPixelFormat::D32_FLOAT;
        }

        /// ステンシル付きか
        inline bool HasStencil(ERHIPixelFormat format) {
            return format == ERHIPixelFormat::D24_UNORM_S8_UINT ||
                   format == ERHIPixelFormat::D32_FLOAT_S8X24_UINT;
        }

        /// デプス読み取り用SRVフォーマット取得
        inline ERHIPixelFormat GetDepthSRVFormat(ERHIPixelFormat format) {
            switch (format) {
                case ERHIPixelFormat::D16_UNORM:
                    return ERHIPixelFormat::R16_UNORM;
                case ERHIPixelFormat::D24_UNORM_S8_UINT:
                    return ERHIPixelFormat::R32_FLOAT;  // 24bit ↔ 32bit変換
                case ERHIPixelFormat::D32_FLOAT:
                    return ERHIPixelFormat::R32_FLOAT;
                case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
                    return ERHIPixelFormat::R32_FLOAT;
                default:
                    return ERHIPixelFormat::Unknown;
            }
        }

        /// ステンシル読み取り用SRVフォーマット取得
        inline ERHIPixelFormat GetStencilSRVFormat(ERHIPixelFormat format) {
            switch (format) {
                case ERHIPixelFormat::D24_UNORM_S8_UINT:
                    return ERHIPixelFormat::R8_UINT;
                case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
                    return ERHIPixelFormat::R8_UINT;
                default:
                    return ERHIPixelFormat::Unknown;
            }
        }

        /// 推奨デプスフォーマット取得
        inline ERHIPixelFormat GetRecommendedDepthFormat(
            bool needsStencil,
            bool highPrecision)
        {
            if (needsStencil) {
                return highPrecision ?
                    ERHIPixelFormat::D32_FLOAT_S8X24_UINT :
                    ERHIPixelFormat::D24_UNORM_S8_UINT;
            } else {
                return highPrecision ?
                    ERHIPixelFormat::D32_FLOAT :
                    ERHIPixelFormat::D16_UNORM;
            }
        }
    }
}
```

- [ ] RHIDepthStencilFormat ヘルパー

### 4. HDRフォーマット変換

```cpp
namespace NS::RHI
{
    /// HDRフォーマットヘルパー。
    namespace RHIHDRFormat
    {
        /// HDRフォーマットか
        inline bool IsHDR(ERHIPixelFormat format) {
            switch (format) {
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
        inline ERHIPixelFormat GetRecommendedHDRFormat(
            bool needsAlpha,
            bool highPrecision)
        {
            if (needsAlpha) {
                return highPrecision ?
                    ERHIPixelFormat::R32G32B32A32_FLOAT :
                    ERHIPixelFormat::R16G16B16A16_FLOAT;
            } else {
                return highPrecision ?
                    ERHIPixelFormat::R32G32B32_FLOAT :
                    ERHIPixelFormat::R11G11B10_FLOAT;
            }
        }

        /// HDR10出力フォーマット取得
        inline ERHIPixelFormat GetHDR10DisplayFormat() {
            return ERHIPixelFormat::R10G10B10A2_UNORM;
        }
    }
}
```

- [ ] RHIHDRFormat ヘルパー

### 5. プラットフォームフォーマット変換

```cpp
namespace NS::RHI
{
    /// プラットフォームフォーマット変換（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// RHIフォーマットからネイティブフォーマットに変換
        /// @return プラットフォーム固有の値（D3D12ならDXGI_FORMAT）。
        /// @note 実装は内部変換テーブルを使用。テーブル完全性は以下で保証:
        ///       static_assert(sizeof(kConversionTable)/sizeof(kConversionTable[0])
        ///                     == static_cast<size_t>(ERHIPixelFormat::Count))
        virtual uint32 ConvertToNativeFormat(ERHIPixelFormat format) const = 0;

        /// ネイティブフォーマットからRHIフォーマットに変換
        /// @note 実装は内部逆変換テーブルを使用。テーブル完全性は同様にstatic_assertで保証。
        virtual ERHIPixelFormat ConvertFromNativeFormat(uint32 nativeFormat) const = 0;
    };
}
```

- [ ] ConvertToNativeFormat
- [ ] ConvertFromNativeFormat

## 検証方法

- [ ] 変換タイプの正確性
- [ ] 互換グループの正確性
- [ ] デプスステンシル変換
- [ ] HDRフォーマット選択
- [ ] `static_assert(sizeof(kConversionTable) / sizeof(kConversionTable[0]) == static_cast<size_t>(ERHIPixelFormat::Count))` でテーブルサイズ検証
- [ ] 全フォーマットの往復変換テスト（Native→RHI→Native が同値）
- [ ] 未定義フォーマットに対するフォールバック動作テスト
