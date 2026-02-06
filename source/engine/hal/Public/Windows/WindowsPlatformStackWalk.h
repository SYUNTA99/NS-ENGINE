/// @file WindowsPlatformStackWalk.h
/// @brief Windows固有のスタックウォーク実装
#pragma once

#include "GenericPlatform/GenericPlatformStackWalk.h"

namespace NS
{
    /// Windows固有のスタックウォーク実装
    ///
    /// RtlCaptureStackBackTrace、DbgHelp APIを使用。
    struct WindowsPlatformStackWalk : public GenericPlatformStackWalk
    {
        static void InitStackWalking();
        static bool IsInitialized();
        static int32 CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount = 0);
        static bool ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo);
        static int32 ProgramCountersToSymbolInfos(const uint64* programCounters,
                                                  ProgramCounterSymbolInfo* outInfos,
                                                  int32 count);
    };

    /// 現在のプラットフォームのスタックウォーク
    using PlatformStackWalk = WindowsPlatformStackWalk;
} // namespace NS
