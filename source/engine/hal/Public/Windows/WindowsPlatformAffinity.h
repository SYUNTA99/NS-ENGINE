/// @file WindowsPlatformAffinity.h
/// @brief Windows固有のスレッドアフィニティ・優先度管理実装
#pragma once

#include "GenericPlatform/GenericPlatformAffinity.h"

namespace NS
{
    /// Windows固有のアフィニティ管理
    ///
    /// ハイブリッドCPU（Intel 12th+）対応：
    /// - ゲームスレッド/レンダリング → Pコア優先
    /// - バックグラウンド/ロード → Eコア優先
    struct WindowsPlatformAffinity : public GenericPlatformAffinity
    {
        static uint64 GetAffinityMask(ThreadType type);
        static ThreadPriority GetDefaultPriority(ThreadType type);
        static const CPUTopology& GetCPUTopology();

        static bool SetCurrentThreadAffinity(uint64 mask);
        static bool SetCurrentThreadPriority(ThreadPriority priority);
        static uint32 GetCurrentProcessorNumber();

        static void Sleep(uint32 milliseconds);
        static void YieldThread();

    private:
        static int32 ToWindowsPriority(ThreadPriority priority);
        static void InitializeTopology();

        static CPUTopology s_topology;
        static bool s_topologyInitialized;
    };

    /// 現在のプラットフォームのアフィニティ管理
    using PlatformAffinity = WindowsPlatformAffinity;
} // namespace NS
