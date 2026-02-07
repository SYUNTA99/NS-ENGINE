/// @file RHIVariableRateShading.h
/// @brief Variable Rate Shading (VRS) サポート
/// @details シェーディングレート列挙型、ケイパビリティ情報、VRSイメージ記述を提供。
/// @see 07-07-variable-rate-shading.md
#pragma once

#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHIVRSAxisRate (07-07)
    //=========================================================================

    /// 軸ごとのシェーディングレート
    enum class ERHIVRSAxisRate : uint8
    {
        Rate_1X = 0x0, // フルレート
        Rate_2X = 0x1, // 1/2レート
        Rate_4X = 0x2, // 1/4レート
    };

    //=========================================================================
    // ERHIShadingRate (07-07)
    //=========================================================================

    /// 2Dシェーディングレート（X軸 x Y軸）
    enum class ERHIShadingRate : uint8
    {
        Rate_1x1 = 0x00, // フルレート
        Rate_1x2 = 0x01,
        Rate_2x1 = 0x04,
        Rate_2x2 = 0x05, // デフォルトの低品質
        Rate_2x4 = 0x06,
        Rate_4x2 = 0x09,
        Rate_4x4 = 0x0A, // 最低品質
    };

    /// シェーディングレートをピクセル数に変換
    inline uint32 GetShadingRatePixelCountX(ERHIShadingRate rate)
    {
        return 1u << ((static_cast<uint8>(rate) >> 2) & 0x3);
    }

    inline uint32 GetShadingRatePixelCountY(ERHIShadingRate rate)
    {
        return 1u << (static_cast<uint8>(rate) & 0x3);
    }

    //=========================================================================
    // ERHIVRSCombiner (07-07)
    //=========================================================================

    /// 複数のシェーディングレートソースの組み合わせ方法
    enum class ERHIVRSCombiner : uint8
    {
        Passthrough, // 入力をそのまま出力
        Override,    // 入力で上書き
        Min,         // 最小値（最高品質）
        Max,         // 最大値（最低品質）
        Sum,         // 合計（品質を下げる方向）
    };

    //=========================================================================
    // ERHIVRSImageType (07-07)
    //=========================================================================

    /// VRSイメージデータタイプ
    enum class ERHIVRSImageType : uint8
    {
        NotSupported, // VRSイメージ非サポート
        Palette,      // パレットベース（離散値）
        Fractional,   // 浮動小数点ベース（連続値）
    };

    //=========================================================================
    // RHIVRSCapabilities (07-07)
    //=========================================================================

    /// VRSケイパビリティ情報
    struct RHI_API RHIVRSCapabilities
    {
        bool supportsPipelineVRS = false;
        bool supportsLargerSizes = false;
        bool supportsImageVRS = false;
        bool supportsPerPrimitiveVRS = false;
        bool supportsComplexCombiners = false;
        bool supportsArrayTextures = false;

        uint32 imageTileMinWidth = 0;
        uint32 imageTileMinHeight = 0;
        uint32 imageTileMaxWidth = 0;
        uint32 imageTileMaxHeight = 0;

        ERHIVRSImageType imageType = ERHIVRSImageType::NotSupported;
        ERHIPixelFormat imageFormat = ERHIPixelFormat::Unknown;
    };

    //=========================================================================
    // RHIVRSImageDesc (07-07)
    //=========================================================================

    /// VRSイメージ記述
    struct RHI_API RHIVRSImageDesc
    {
        uint32 targetWidth = 0;
        uint32 targetHeight = 0;
        uint32 tileWidth = 0;  // 0 = 最大を使用
        uint32 tileHeight = 0; // 0 = 最大を使用
        const char* debugName = nullptr;
    };

    //=========================================================================
    // VRSイメージサイズ計算ヘルパー (07-07)
    //=========================================================================

    /// VRSイメージサイズ計算
    inline void CalculateVRSImageSize(const RHIVRSCapabilities& caps,
                                      uint32 targetWidth,
                                      uint32 targetHeight,
                                      uint32& outImageWidth,
                                      uint32& outImageHeight,
                                      uint32 tileWidth = 0,
                                      uint32 tileHeight = 0)
    {
        uint32 tw = (tileWidth > 0) ? tileWidth : caps.imageTileMaxWidth;
        uint32 th = (tileHeight > 0) ? tileHeight : caps.imageTileMaxHeight;

        if (tw == 0)
            tw = 1;
        if (th == 0)
            th = 1;

        outImageWidth = (targetWidth + tw - 1) / tw;
        outImageHeight = (targetHeight + th - 1) / th;
    }

} // namespace NS::RHI
