/// @file HardwareSurveyResults.h
/// @brief ハードウェアサーベイ結果構造体 (12-03)
#pragma once

#include "HAL/PlatformTypes.h"
#include <cstdint>
#include <string>

namespace NS
{
    /// ハードウェアサーベイ結果
    struct HardwareSurveyResults
    {
        // CPU
        int32_t cpuCount = 0;
        std::wstring cpuBrand;
        float cpuClockGHz = 0.0F;

        // メモリ
        float memoryGb = 0.0F;

        // ラップトップ判定
        bool bIsLaptop = false;

        // GPU
        std::wstring gpuAdpater; // ※UE5タイポ保存: Adapter → Adpater
        uint32_t gpuVendorId = 0;
        uint32_t gpuDeviceId = 0;
        uint64_t gpuDedicatedVram = 0;

        // ディスプレイ
        int32_t displayCount = 0;
        int32_t primaryDisplayWidth = 0;
        int32_t primaryDisplayHeight = 0;

        // OS
        std::wstring osVersion;
        std::wstring osLanguage;

        // パフォーマンスインデックス (WinSAT)
        float cpuPerformanceIndex = -1.0F;
        float gpuPerformanceIndex = -1.0F;
        float memoryPerformanceIndex = -1.0F;
        float diskPerformanceIndex = -1.0F;

        /// 結果が有効か
        [[nodiscard]] bool IsValid() const { return cpuCount > 0; }
    };

    /// ベンチマーク結果
    struct SynthBenchmarkResults
    {
        float cpuStats = -1.0F;
        float gpuStats = -1.0F;

        /// パフォーマンスグレード計算 (0-100)
        [[nodiscard]] float ComputePerformanceIndex() const
        {
            if (cpuStats < 0 || gpuStats < 0) {
                return -1.0F;
}
            return (cpuStats + gpuStats) * 0.5F;
        }
    };

} // namespace NS
