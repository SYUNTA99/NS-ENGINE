/// @file RHIHDR.cpp
/// @brief HDR変換ヘルパー実装
#include "RHIHDR.h"
#include <algorithm>
#include <cmath>

namespace NS::RHI
{
    //=========================================================================
    // RHIHDRHelper
    //=========================================================================

    float RHIHDRHelper::LinearToPQ(float linear)
    {
        // ST.2084 PQ EOTF inverse
        // 入力: リニア輝度 (0..10000 nits / 10000)
        constexpr float m1 = 0.1593017578125F;
        constexpr float m2 = 78.84375F;
        constexpr float c1 = 0.8359375F;
        constexpr float c2 = 18.8515625F;
        constexpr float c3 = 18.6875F;

        float const Ym1 = std::pow(linear, m1);
        return std::pow((c1 + c2 * Ym1) / (1.0F + c3 * Ym1), m2);
    }

    float RHIHDRHelper::PQToLinear(float pq)
    {
        // ST.2084 PQ EOTF
        constexpr float m1 = 0.1593017578125F;
        constexpr float m2 = 78.84375F;
        constexpr float c1 = 0.8359375F;
        constexpr float c2 = 18.8515625F;
        constexpr float c3 = 18.6875F;

        float const Nm2 = std::pow(pq, 1.0F / m2);
        float num = Nm2 - c1;
        num = std::max(num, 0.0F);
        return std::pow(num / (c2 - c3 * Nm2), 1.0F / m1);
    }

    float RHIHDRHelper::LinearToHLG(float linear)
    {
        // Hybrid Log-Gamma OETF
        constexpr float a = 0.17883277F;
        constexpr float b = 0.28466892F;
        constexpr float c = 0.55991073F;

        if (linear <= 1.0F / 12.0F)
        {
            return std::sqrt(3.0F * linear);
        }
        return a * std::log(12.0F * linear - b) + c;
    }

    float RHIHDRHelper::HLGToLinear(float hlg)
    {
        // Hybrid Log-Gamma EOTF
        constexpr float a = 0.17883277F;
        constexpr float b = 0.28466892F;
        constexpr float c = 0.55991073F;

        if (hlg <= 0.5F)
        {
            return (hlg * hlg) / 3.0F;
        }
        return (std::exp((hlg - c) / a) + b) / 12.0F;
    }

    float RHIHDRHelper::sRGBToLinear(float srgb)
    {
        if (srgb <= 0.04045F)
        {
            return srgb / 12.92F;
        }
        return std::pow((srgb + 0.055F) / 1.055F, 2.4F);
    }

    float RHIHDRHelper::LinearTosRGB(float linear)
    {
        if (linear <= 0.0031308F)
        {
            return linear * 12.92F;
        }
        return 1.055F * std::pow(linear, 1.0F / 2.4F) - 0.055F;
    }

    ERHIPixelFormat RHIHDRHelper::SelectOptimalHDRFormat(const RHIHDROutputCapabilities& capabilities)
    {
        if (!capabilities.supportsHDR)
        {
            return ERHIPixelFormat::R8G8B8A8_UNORM;
        }

        if (capabilities.supportsScRGB)
        {
            return ERHIPixelFormat::R16G16B16A16_FLOAT;
        }

        if (capabilities.supportsHDR10)
        {
            return ERHIPixelFormat::R10G10B10A2_UNORM;
        }

        return capabilities.recommendedFormat;
    }

    ERHIColorSpace RHIHDRHelper::SelectOptimalColorSpace(const RHIHDROutputCapabilities& capabilities)
    {
        if (!capabilities.supportsHDR)
        {
            return ERHIColorSpace::sRGB;
        }

        if (capabilities.supportsScRGB)
        {
            return ERHIColorSpace::scRGB;
        }

        if (capabilities.supportsHDR10)
        {
            return ERHIColorSpace::HDR10_ST2084;
        }

        return capabilities.recommendedColorSpace;
    }

} // namespace NS::RHI
