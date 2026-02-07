/// @file RHIHDR.h
/// @brief HDR（High Dynamic Range）サポート
/// @details HDRメタデータ、カラースペース、出力能力、トーンマッピング、Auto HDRを提供。
/// @see 12-04-hdr.md
#pragma once

#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHIHDRMetadataType (12-04)
    //=========================================================================

    /// HDRメタデータタイプ
    enum class ERHIHDRMetadataType : uint8
    {
        None,
        HDR10,       ///< ST.2086 + MaxCLL/MaxFALL
        HDR10Plus,   ///< HDR10+
        DolbyVision, ///< Dolby Vision
    };

    //=========================================================================
    // RHIColorPrimaries (12-04)
    //=========================================================================

    /// 色域プライマリ
    struct RHI_API RHIColorPrimaries
    {
        float redX = 0.0f;
        float redY = 0.0f;
        float greenX = 0.0f;
        float greenY = 0.0f;
        float blueX = 0.0f;
        float blueY = 0.0f;
        float whiteX = 0.0f;
        float whiteY = 0.0f;

        static RHIColorPrimaries Rec709()
        {
            RHIColorPrimaries p;
            p.redX = 0.64f;
            p.redY = 0.33f;
            p.greenX = 0.30f;
            p.greenY = 0.60f;
            p.blueX = 0.15f;
            p.blueY = 0.06f;
            p.whiteX = 0.3127f;
            p.whiteY = 0.3290f;
            return p;
        }

        static RHIColorPrimaries DCIP3()
        {
            RHIColorPrimaries p;
            p.redX = 0.680f;
            p.redY = 0.320f;
            p.greenX = 0.265f;
            p.greenY = 0.690f;
            p.blueX = 0.150f;
            p.blueY = 0.060f;
            p.whiteX = 0.3127f;
            p.whiteY = 0.3290f;
            return p;
        }

        static RHIColorPrimaries Rec2020()
        {
            RHIColorPrimaries p;
            p.redX = 0.708f;
            p.redY = 0.292f;
            p.greenX = 0.170f;
            p.greenY = 0.797f;
            p.blueX = 0.131f;
            p.blueY = 0.046f;
            p.whiteX = 0.3127f;
            p.whiteY = 0.3290f;
            return p;
        }
    };

    //=========================================================================
    // RHIHDRMetadata (12-04)
    //=========================================================================

    /// HDRメタデータ
    struct RHI_API RHIHDRMetadata
    {
        ERHIHDRMetadataType type = ERHIHDRMetadataType::None;
        RHIColorPrimaries colorPrimaries;
        float maxMasteringLuminance = 1000.0f;    ///< nits
        float minMasteringLuminance = 0.001f;     ///< nits
        float maxContentLightLevel = 1000.0f;     ///< nits
        float maxFrameAverageLightLevel = 400.0f; ///< nits

        bool IsValid() const
        {
            if (type == ERHIHDRMetadataType::None)
                return true;
            return minMasteringLuminance >= 0.0f && maxMasteringLuminance > minMasteringLuminance &&
                   maxContentLightLevel > 0.0f && maxFrameAverageLightLevel > 0.0f &&
                   maxFrameAverageLightLevel <= maxContentLightLevel;
        }
    };

    //=========================================================================
    // ERHIColorSpace (12-04)
    //=========================================================================

    /// カラースペース
    enum class ERHIColorSpace : uint8
    {
        sRGB,         ///< sRGB
        sRGBLinear,   ///< リニアsRGB
        scRGB,        ///< 拡張sRGB（HDR）
        HDR10_ST2084, ///< HDR10（ST.2084 + Rec.2020）
        HLG,          ///< Hybrid Log-Gamma
        ACEScg,       ///< ACEScg
        Custom,       ///< カスタム
    };

    //=========================================================================
    // RHIHDROutputCapabilities (12-04)
    //=========================================================================

    /// HDR出力能力
    struct RHI_API RHIHDROutputCapabilities
    {
        bool supportsHDR = false;
        bool supportsHDR10 = false;
        bool supportsDolbyVision = false;
        bool supportsHLG = false;
        bool supportsScRGB = false;
        bool supportsRec2020 = false;
        float minLuminance = 0.0f;          ///< nits
        float maxLuminance = 0.0f;          ///< nits
        float maxFullFrameLuminance = 0.0f; ///< nits
        uint32 bitsPerColor = 8;
        ERHIColorSpace recommendedColorSpace = ERHIColorSpace::sRGB;
        ERHIPixelFormat recommendedFormat = ERHIPixelFormat::R8G8B8A8_UNORM;
    };

    //=========================================================================
    // トーンマッピング (12-04)
    //=========================================================================

    /// トーンマッピングモード
    enum class ERHIToneMappingMode : uint8
    {
        None,       ///< なし
        Reinhard,   ///< Reinhard
        ACES,       ///< ACES Filmic
        AgX,        ///< AgX
        Uncharted2, ///< Uncharted 2
        Custom,     ///< カスタム
    };

    /// HDRトーンマッピング設定
    struct RHI_API RHIToneMappingSettings
    {
        ERHIToneMappingMode mode = ERHIToneMappingMode::ACES;
        float exposure = 1.0f;      ///< 露出補正
        float whitePoint = 1000.0f; ///< ホワイトポイント（nits）
        float paperWhite = 200.0f;  ///< ペーパーホワイト（nits）
        float contrast = 1.0f;
        float saturation = 1.0f;
    };

    //=========================================================================
    // RHIHDRHelper (12-04)
    //=========================================================================

    /// HDR変換ヘルパー
    class RHI_API RHIHDRHelper
    {
    public:
        static float LinearToPQ(float linear);
        static float PQToLinear(float pq);
        static float LinearToHLG(float linear);
        static float HLGToLinear(float hlg);
        static float sRGBToLinear(float srgb);
        static float LinearTosRGB(float linear);

        /// 最適なHDRフォーマット選択
        static ERHIPixelFormat SelectOptimalHDRFormat(const RHIHDROutputCapabilities& capabilities);

        /// 最適なカラースペース選択
        static ERHIColorSpace SelectOptimalColorSpace(const RHIHDROutputCapabilities& capabilities);
    };

    //=========================================================================
    // RHIAutoHDRSettings (12-04)
    //=========================================================================

    /// Auto HDR設定
    struct RHI_API RHIAutoHDRSettings
    {
        bool enabled = false;
        float maxBoost = 2.0f;           ///< SDR最大輝度ブースト
        float intensity = 1.0f;          ///< 輝度拡張の強度
        float highlightThreshold = 0.8f; ///< ハイライト拡張閾値
        float shadowRetention = 0.5f;    ///< シャドウ保持強度
    };

} // namespace NS::RHI
