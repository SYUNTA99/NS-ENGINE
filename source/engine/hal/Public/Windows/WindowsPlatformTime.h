/// @file WindowsPlatformTime.h
/// @brief Windows固有の時間管理実装
#pragma once

// windows.hを先にインクルード（TEXTマクロの衝突を回避）
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "GenericPlatform/GenericPlatformTime.h"

namespace NS
{
    /// Windows固有の時間管理実装
    ///
    /// QueryPerformanceCounter/QueryPerformanceFrequencyを使用。
    struct WindowsPlatformTime : public GenericPlatformTime
    {
        // =====================================================================
        // 初期化
        // =====================================================================

        /// タイミングシステム初期化
        static double InitTiming();

        /// 初期化済みかどうか
        static bool IsInitialized();

        // =====================================================================
        // 高精度タイマー
        // =====================================================================

        static FORCEINLINE double Seconds()
        {
            LARGE_INTEGER cycles;
            QueryPerformanceCounter(&cycles);
            return static_cast<double>(cycles.QuadPart) * GetSecondsPerCycle64();
        }

        static FORCEINLINE uint64 Cycles64()
        {
            LARGE_INTEGER cycles;
            QueryPerformanceCounter(&cycles);
            return static_cast<uint64>(cycles.QuadPart);
        }

        static FORCEINLINE double GetSecondsPerCycle64() { return s_secondsPerCycle; }

        // =====================================================================
        // システム時刻
        // =====================================================================

        static void GetLocalTime(DateTime& outDateTime);
        static void GetUtcTime(DateTime& outDateTime);

        static void SystemTime(
            int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec);

        static void UtcTime(
            int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec);

        // =====================================================================
        // ユーティリティ
        // =====================================================================

        static int64 GetUnixTimestamp();
        static int64 GetUnixTimestampMillis();
    };

    /// 現在のプラットフォームの時間管理
    using PlatformTime = WindowsPlatformTime;
} // namespace NS
