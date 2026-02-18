/// @file WindowsPlatformProcess.cpp
/// @brief Windows platform process implementation

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Windows/WindowsPlatformProcess.h"
#include <cmath>
#include <string>

namespace NS
{
    void WindowsPlatformProcess::Sleep(float seconds)
    {
        SleepNoStats(seconds);
    }

    void WindowsPlatformProcess::SleepNoStats(float seconds)
    {
        if (!std::isfinite(seconds) || seconds <= 0.0F)
        {
            ::Sleep(0);
            return;
        }

        constexpr float kMaxSleepSeconds = static_cast<float>(MAXDWORD) / 1000.0F;
        if (seconds >= kMaxSleepSeconds)
        {
            ::Sleep(MAXDWORD);
            return;
        }

        ::Sleep(static_cast<DWORD>(seconds * 1000.0F));
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
        if (dllHandle != nullptr)
        {
            ::FreeLibrary(static_cast<HMODULE>(dllHandle));
        }
    }

    void* WindowsPlatformProcess::GetDllExport(void* dllHandle, const TCHAR* procName)
    {
        if ((dllHandle == nullptr) || (procName == nullptr) || *procName == L'\0')
        {
            return nullptr;
        }

        const int kRequiredSize = ::WideCharToMultiByte(CP_ACP, 0, procName, -1, nullptr, 0, nullptr, nullptr);
        if (kRequiredSize <= 0)
        {
            return nullptr;
        }

        std::string procNameAnsi(static_cast<size_t>(kRequiredSize), '\0');
        const int kConvertedSize =
            ::WideCharToMultiByte(CP_ACP, 0, procName, -1, procNameAnsi.data(), kRequiredSize, nullptr, nullptr);
        if (kConvertedSize <= 0)
        {
            return nullptr;
        }

        return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(dllHandle), procNameAnsi.c_str()));
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
