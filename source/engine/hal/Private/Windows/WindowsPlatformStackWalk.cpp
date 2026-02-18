/// @file WindowsPlatformStackWalk.cpp
/// @brief Windows固有のスタックウォーク実装

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <dbghelp.h>
#pragma warning(pop)

#pragma comment(lib, "dbghelp.lib")

#include "Windows/WindowsPlatformStackWalk.h"

namespace NS
{
    // 静的メンバの実体
    bool GenericPlatformStackWalk::s_initialized = false;

    void WindowsPlatformStackWalk::InitStackWalking()
    {
        if (s_initialized)
        {
            return;
        }

        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
        SymInitialize(process, nullptr, TRUE);

        s_initialized = true;
    }

    bool WindowsPlatformStackWalk::IsInitialized()
    {
        return s_initialized;
    }

    int32 WindowsPlatformStackWalk::CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount)
    {
        if (maxDepth <= 0 || maxDepth > kMaxStackDepth)
        {
            maxDepth = kDefaultStackDepth;
        }

        return static_cast<int32>(::RtlCaptureStackBackTrace(skipCount + 1, // この関数自体をスキップ
                                                             static_cast<DWORD>(maxDepth),
                                                             reinterpret_cast<void**>(backTrace),
                                                             nullptr));
    }

    bool WindowsPlatformStackWalk::ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo)
    {
        if (!s_initialized)
        {
            InitStackWalking();
        }

        // 出力を初期化
        outInfo = ProgramCounterSymbolInfo();
        outInfo.programCounter = programCounter;

        if (programCounter == 0)
        {
            return false;
        }

        HANDLE process = GetCurrentProcess();
        auto const address = static_cast<DWORD64>(programCounter);

        // シンボル情報用バッファ
        alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol) != 0)
        {
            SafeStrCopy(outInfo.functionName, kMaxSymbolNameLength, symbol->Name);
        }

        // 行情報
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;

        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line) != 0)
        {
            SafeStrCopy(outInfo.filename, kMaxFilenameLength, line.FileName);
            outInfo.lineNumber = static_cast<int32>(line.LineNumber);
        }

        // モジュール情報
        IMAGEHLP_MODULE64 module;
        module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        if (SymGetModuleInfo64(process, address, &module) != 0)
        {
            SafeStrCopy(outInfo.moduleName, kMaxModuleNameLength, module.ModuleName);
            outInfo.offsetInModule = address - module.BaseOfImage;
        }

        return outInfo.IsResolved();
    }

    int32 WindowsPlatformStackWalk::ProgramCountersToSymbolInfos(const uint64* programCounters,
                                                                 ProgramCounterSymbolInfo* outInfos,
                                                                 int32 count)
    {
        int32 resolvedCount = 0;
        for (int32 i = 0; i < count; ++i)
        {
            if (ProgramCounterToSymbolInfo(programCounters[i], outInfos[i]))
            {
                resolvedCount++;
            }
        }
        return resolvedCount;
    }
} // namespace NS
