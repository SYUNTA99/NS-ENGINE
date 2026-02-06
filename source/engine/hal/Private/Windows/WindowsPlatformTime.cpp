/// @file WindowsPlatformTime.cpp
/// @brief Windows固有の時間管理実装

// windows.hはWindowsPlatformTime.hで既にインクルードされる
#include "Windows/WindowsPlatformTime.h"

namespace NS
{
    // 静的メンバの実体
    double GenericPlatformTime::s_secondsPerCycle = 0.0;
    bool GenericPlatformTime::s_initialized = false;

    double WindowsPlatformTime::InitTiming()
    {
        if (s_initialized)
        {
            return s_secondsPerCycle;
        }

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        s_secondsPerCycle = 1.0 / static_cast<double>(frequency.QuadPart);
        s_initialized = true;
        return s_secondsPerCycle;
    }

    bool WindowsPlatformTime::IsInitialized()
    {
        return s_initialized;
    }

    void WindowsPlatformTime::GetLocalTime(DateTime& outDateTime)
    {
        SYSTEMTIME st;
        ::GetLocalTime(&st);
        outDateTime.year = static_cast<int32>(st.wYear);
        outDateTime.month = static_cast<int32>(st.wMonth);
        outDateTime.day = static_cast<int32>(st.wDay);
        outDateTime.dayOfWeek = static_cast<int32>(st.wDayOfWeek);
        outDateTime.hour = static_cast<int32>(st.wHour);
        outDateTime.minute = static_cast<int32>(st.wMinute);
        outDateTime.second = static_cast<int32>(st.wSecond);
        outDateTime.millisecond = static_cast<int32>(st.wMilliseconds);
    }

    void WindowsPlatformTime::GetUtcTime(DateTime& outDateTime)
    {
        SYSTEMTIME st;
        ::GetSystemTime(&st);
        outDateTime.year = static_cast<int32>(st.wYear);
        outDateTime.month = static_cast<int32>(st.wMonth);
        outDateTime.day = static_cast<int32>(st.wDay);
        outDateTime.dayOfWeek = static_cast<int32>(st.wDayOfWeek);
        outDateTime.hour = static_cast<int32>(st.wHour);
        outDateTime.minute = static_cast<int32>(st.wMinute);
        outDateTime.second = static_cast<int32>(st.wSecond);
        outDateTime.millisecond = static_cast<int32>(st.wMilliseconds);
    }

    void WindowsPlatformTime::SystemTime(
        int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec)
    {
        SYSTEMTIME st;
        ::GetLocalTime(&st);
        year = static_cast<int32>(st.wYear);
        month = static_cast<int32>(st.wMonth);
        dayOfWeek = static_cast<int32>(st.wDayOfWeek);
        day = static_cast<int32>(st.wDay);
        hour = static_cast<int32>(st.wHour);
        min = static_cast<int32>(st.wMinute);
        sec = static_cast<int32>(st.wSecond);
        msec = static_cast<int32>(st.wMilliseconds);
    }

    void WindowsPlatformTime::UtcTime(
        int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec)
    {
        SYSTEMTIME st;
        ::GetSystemTime(&st);
        year = static_cast<int32>(st.wYear);
        month = static_cast<int32>(st.wMonth);
        dayOfWeek = static_cast<int32>(st.wDayOfWeek);
        day = static_cast<int32>(st.wDay);
        hour = static_cast<int32>(st.wHour);
        min = static_cast<int32>(st.wMinute);
        sec = static_cast<int32>(st.wSecond);
        msec = static_cast<int32>(st.wMilliseconds);
    }

    int64 WindowsPlatformTime::GetUnixTimestamp()
    {
        FILETIME ft;
        ::GetSystemTimeAsFileTime(&ft);

        // FILETIMEは1601-01-01からの100ns単位
        // Unixタイムスタンプは1970-01-01からの秒単位
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        // 1601-01-01から1970-01-01までの100ns数: 116444736000000000
        constexpr uint64 kEpochDifference = 116444736000000000ULL;
        return static_cast<int64>((uli.QuadPart - kEpochDifference) / 10000000ULL);
    }

    int64 WindowsPlatformTime::GetUnixTimestampMillis()
    {
        FILETIME ft;
        ::GetSystemTimeAsFileTime(&ft);

        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        constexpr uint64 kEpochDifference = 116444736000000000ULL;
        return static_cast<int64>((uli.QuadPart - kEpochDifference) / 10000ULL);
    }
} // namespace NS
