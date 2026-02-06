/// @file WindowsPlatformProcess.cpp
/// @brief Windows固有のプロセス管理実装

// windows.hを先にインクルード
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Windows/WindowsPlatformProcess.h"

namespace NS
{
    void WindowsPlatformProcess::Sleep(float seconds)
    {
        SleepNoStats(seconds);
    }

    void WindowsPlatformProcess::SleepNoStats(float seconds)
    {
        ::Sleep(static_cast<DWORD>(seconds * 1000.0f));
    }

    void WindowsPlatformProcess::SleepInfinite()
    {
        ::Sleep(INFINITE);
    }

    void WindowsPlatformProcess::YieldThread()
    {
        ::SwitchToThread();
    }

    void* WindowsPlatformProcess::GetDllHandle(const TCHAR* filename)
    {
        return ::LoadLibraryW(filename);
    }

    void WindowsPlatformProcess::FreeDllHandle(void* dllHandle)
    {
        if (dllHandle)
        {
            ::FreeLibrary(static_cast<HMODULE>(dllHandle));
        }
    }

    void* WindowsPlatformProcess::GetDllExport(void* dllHandle, const TCHAR* procName)
    {
        // GetProcAddressはANSI文字列を要求
        char procNameAnsi[256];
        WideCharToMultiByte(CP_ACP, 0, procName, -1, procNameAnsi, sizeof(procNameAnsi), nullptr, nullptr);
        return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(dllHandle), procNameAnsi));
    }

    uint32 WindowsPlatformProcess::GetCurrentProcessId()
    {
        return ::GetCurrentProcessId();
    }

    uint32 WindowsPlatformProcess::GetCurrentCoreNumber()
    {
        return ::GetCurrentProcessorNumber();
    }

    void WindowsPlatformProcess::SetThreadAffinityMask(uint64 mask)
    {
        ::SetThreadAffinityMask(::GetCurrentThread(), static_cast<DWORD_PTR>(mask));
    }

    void WindowsPlatformProcess::SetThreadPriority(int32 priority)
    {
        ::SetThreadPriority(::GetCurrentThread(), priority);
    }
} // namespace NS
