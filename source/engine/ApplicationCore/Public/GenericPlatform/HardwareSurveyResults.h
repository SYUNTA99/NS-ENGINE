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
        int32_t CPUCount = 0;
        std::wstring CPUBrand;
        float CPUClockGHz = 0.0f;

        // メモリ
        float MemoryGB = 0.0f;

        // ラップトップ判定
        bool bIsLaptop = false;

        // GPU
        std::wstring GPUAdpater; // ※UE5タイポ保存: Adapter → Adpater
        uint32_t GPUVendorId = 0;
        uint32_t GPUDeviceId = 0;
        uint64_t GPUDedicatedVRAM = 0;

        // ディスプレイ
        int32_t DisplayCount = 0;
        int32_t PrimaryDisplayWidth = 0;
        int32_t PrimaryDisplayHeight = 0;

        // OS
        std::wstring OSVersion;
        std::wstring OSLanguage;

        // パフォーマンスインデックス (WinSAT)
        float CPUPerformanceIndex = -1.0f;
        float GPUPerformanceIndex = -1.0f;
        float MemoryPerformanceIndex = -1.0f;
        float DiskPerformanceIndex = -1.0f;

        /// 結果が有効か
        bool IsValid() const { return CPUCount > 0; }
    };

    /// ベンチマーク結果
    struct SynthBenchmarkResults
    {
        float CPUStats = -1.0f;
        float GPUStats = -1.0f;

        /// パフォーマンスグレード計算 (0-100)
        float ComputePerformanceIndex() const
        {
            if (CPUStats < 0 || GPUStats < 0)
                return -1.0f;
            return (CPUStats + GPUStats) * 0.5f;
        }
    };

} // namespace NS
