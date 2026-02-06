/// @file OutputDevice.cpp
/// @brief 出力デバイス実装

// windows.hを最初にインクルードしてTEXTマクロ衝突を回避
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "HAL/OutputDevice.h"
#include "HAL/Platform.h"

#include <cstdio>
#include <cwchar>

namespace NS
{
    // =========================================================================
    // OutputDeviceDebug
    // =========================================================================

    void OutputDeviceDebug::Serialize(const TCHAR* message, LogVerbosity verbosity)
    {
        if (message == nullptr)
        {
            return;
        }

#if PLATFORM_WINDOWS
        const TCHAR* prefix = L"";
        switch (verbosity)
        {
        case LogVerbosity::Fatal:
            prefix = L"[FATAL] ";
            break;
        case LogVerbosity::Error:
            prefix = L"[ERROR] ";
            break;
        case LogVerbosity::Warning:
            prefix = L"[WARN] ";
            break;
        default:
            break;
        }

        if (prefix[0] != L'\0')
        {
            ::OutputDebugStringW(prefix);
        }
        ::OutputDebugStringW(message);
        ::OutputDebugStringW(L"\n");
#else
        NS_UNUSED(verbosity);
#endif
    }

    void OutputDeviceDebug::Flush()
    {
        // OutputDebugString は即時出力なのでフラッシュ不要
    }

    bool OutputDeviceDebug::IsDebuggerPresent()
    {
#if PLATFORM_WINDOWS
        return ::IsDebuggerPresent() != 0;
#else
        return false;
#endif
    }

    // =========================================================================
    // OutputDeviceConsole
    // =========================================================================

    void OutputDeviceConsole::Serialize(const TCHAR* message, LogVerbosity verbosity)
    {
        if (message == nullptr)
        {
            return;
        }

        FILE* output = stdout;
        const char* prefix = "";

        switch (verbosity)
        {
        case LogVerbosity::Fatal:
            output = stderr;
            prefix = "[FATAL] ";
            break;
        case LogVerbosity::Error:
            output = stderr;
            prefix = "[ERROR] ";
            break;
        case LogVerbosity::Warning:
            output = stderr;
            prefix = "[WARN] ";
            break;
        default:
            break;
        }

        if (prefix[0] != '\0')
        {
            std::fputs(prefix, output);
        }

#if PLATFORM_WINDOWS
        // ワイド文字列をコンソールに出力
        std::fwprintf(output, L"%ls\n", message);
#else
        std::fprintf(output, "%s\n", message);
#endif
    }

    void OutputDeviceConsole::Flush()
    {
        std::fflush(stdout);
        std::fflush(stderr);
    }
} // namespace NS
