# 03-03: PlatformTime（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [03-03a: 時間インターフェース](03-03a-time-interface.md)
> - [03-03b: Windows時間実装](03-03b-windows-time.md)

---

# 03-03: PlatformTime（旧）

## 目的

時間管理の抽象化レイヤーを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション5「時間管理」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`, `uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型
- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `LARGE_INTEGER`, `QueryPerformanceCounter`, `QueryPerformanceFrequency`

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformTime.h`
- `source/engine/hal/Public/Windows/WindowsPlatformTime.h`
- `source/engine/hal/Public/HAL/PlatformTime.h`
- `source/engine/hal/Private/Windows/WindowsPlatformTime.cpp`

## TODO

- [ ] `GenericPlatformTime` 構造体（インターフェース定義）
- [ ] `WindowsPlatformTime` 構造体（QueryPerformanceCounter使用）
- [ ] `PlatformTime.h` エントリポイント
- [ ] 高精度タイマー初期化

## 実装内容

```cpp
// GenericPlatformTime.h
namespace NS
{
    struct GenericPlatformTime
    {
        static double InitTiming();

        static FORCEINLINE double Seconds();
        static FORCEINLINE uint64 Cycles64();

        static double GetSecondsPerCycle64();

        static void SystemTime(int32& year, int32& month, int32& dayOfWeek,
                              int32& day, int32& hour, int32& min, int32& sec, int32& msec);
        static void UtcTime(int32& year, int32& month, int32& dayOfWeek,
                           int32& day, int32& hour, int32& min, int32& sec, int32& msec);
    };
}

// WindowsPlatformTime.h
namespace NS
{
    struct WindowsPlatformTime : public GenericPlatformTime
    {
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
    };

    typedef WindowsPlatformTime PlatformTime;
}
```

## 検証

- Seconds() が正確な経過時間を返す
- Cycles64() が単調増加
- InitTiming() 後に GetSecondsPerCycle64() が有効な値を返す

## 実装詳細

### WindowsPlatformTime.cpp

```cpp
static double s_secondsPerCycle64 = 0.0;

double WindowsPlatformTime::InitTiming()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    s_secondsPerCycle64 = 1.0 / (double)frequency.QuadPart;
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
```

## 注意事項

- `InitTiming()` はエンジン起動時に一度だけ呼び出す
- `QueryPerformanceCounter` は高精度（マイクロ秒以下）
- `s_secondsPerCycle64` はスレッドセーフではないがInitTiming後は読み取り専用
