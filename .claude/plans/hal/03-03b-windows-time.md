# 03-03b: Windows時間実装

## 目的

Windows固有の時間管理を実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション5「時間管理」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 03-03a 時間インターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `QueryPerformanceCounter`, `QueryPerformanceFrequency`
- `<sysinfoapi.h>` - `GetLocalTime`, `GetSystemTime`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformTime.h`
- `source/engine/hal/Public/HAL/PlatformTime.h`
- `source/engine/hal/Private/Windows/WindowsPlatformTime.cpp`

## TODO

- [ ] `WindowsPlatformTime::InitTiming` 実装
- [ ] `WindowsPlatformTime::Seconds` 実装（インライン）
- [ ] `WindowsPlatformTime::Cycles64` 実装（インライン）
- [ ] `WindowsPlatformTime::SystemTime` 実装
- [ ] `WindowsPlatformTime::UtcTime` 実装
- [ ] typedef `PlatformTime` 定義

## 実装内容

### WindowsPlatformTime.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformTime.h"
#include <windows.h>

namespace NS
{
    struct WindowsPlatformTime : public GenericPlatformTime
    {
        static double InitTiming();
        static double GetSecondsPerCycle64();

        static FORCEINLINE double Seconds()
        {
            LARGE_INTEGER cycles;
            QueryPerformanceCounter(&cycles);
            return cycles.QuadPart * GetSecondsPerCycle64();
        }

        static FORCEINLINE uint64 Cycles64()
        {
            LARGE_INTEGER cycles;
            QueryPerformanceCounter(&cycles);
            return cycles.QuadPart;
        }

        static void SystemTime(int32& year, int32& month, int32& dayOfWeek,
                               int32& day, int32& hour, int32& min, int32& sec, int32& msec);

        static void UtcTime(int32& year, int32& month, int32& dayOfWeek,
                            int32& day, int32& hour, int32& min, int32& sec, int32& msec);
    };

    typedef WindowsPlatformTime PlatformTime;
}
```

### WindowsPlatformTime.cpp

```cpp
#include "Windows/WindowsPlatformTime.h"

namespace NS
{
    static double s_secondsPerCycle64 = 0.0;

    double WindowsPlatformTime::InitTiming()
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        s_secondsPerCycle64 = 1.0 / static_cast<double>(frequency.QuadPart);
        return s_secondsPerCycle64;
    }

    double WindowsPlatformTime::GetSecondsPerCycle64()
    {
        return s_secondsPerCycle64;
    }

    void WindowsPlatformTime::SystemTime(int32& year, int32& month, int32& dayOfWeek,
                                          int32& day, int32& hour, int32& min, int32& sec, int32& msec)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        year = st.wYear;
        month = st.wMonth;
        dayOfWeek = st.wDayOfWeek;
        day = st.wDay;
        hour = st.wHour;
        min = st.wMinute;
        sec = st.wSecond;
        msec = st.wMilliseconds;
    }

    void WindowsPlatformTime::UtcTime(int32& year, int32& month, int32& dayOfWeek,
                                       int32& day, int32& hour, int32& min, int32& sec, int32& msec)
    {
        SYSTEMTIME st;
        GetSystemTime(&st);
        year = st.wYear;
        month = st.wMonth;
        dayOfWeek = st.wDayOfWeek;
        day = st.wDay;
        hour = st.wHour;
        min = st.wMinute;
        sec = st.wSecond;
        msec = st.wMilliseconds;
    }
}
```

## 検証

- `PlatformTime::InitTiming()` が正の値を返す
- `PlatformTime::Seconds()` が呼び出すたびに増加
- `PlatformTime::Cycles64()` が単調増加

## 注意事項

- `InitTiming()` はエンジン起動時に一度だけ呼び出す
- `GetSecondsPerCycle64()` は `InitTiming()` 後でないと0を返す

## 次のサブ計画

→ 03-04a: プロセスインターフェース
