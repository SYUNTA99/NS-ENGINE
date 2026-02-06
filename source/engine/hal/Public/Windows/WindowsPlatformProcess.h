/// @file WindowsPlatformProcess.h
/// @brief Windows固有のプロセス管理実装
#pragma once

#include "GenericPlatform/GenericPlatformProcess.h"

namespace NS
{
    /// Windows固有のプロセス管理実装
    struct WindowsPlatformProcess : public GenericPlatformProcess
    {
        // =====================================================================
        // スリープ
        // =====================================================================

        static void Sleep(float seconds);
        static void SleepNoStats(float seconds);
        static void SleepInfinite();
        static void YieldThread();

        // =====================================================================
        // DLL管理
        // =====================================================================

        static void* GetDllHandle(const TCHAR* filename);
        static void FreeDllHandle(void* dllHandle);
        static void* GetDllExport(void* dllHandle, const TCHAR* procName);

        // =====================================================================
        // プロセス情報
        // =====================================================================

        static uint32 GetCurrentProcessId();
        static uint32 GetCurrentCoreNumber();

        // =====================================================================
        // スレッド制御
        // =====================================================================

        static void SetThreadAffinityMask(uint64 mask);
        static void SetThreadPriority(int32 priority);
    };

    /// 現在のプラットフォームのプロセス管理
    using PlatformProcess = WindowsPlatformProcess;
} // namespace NS
