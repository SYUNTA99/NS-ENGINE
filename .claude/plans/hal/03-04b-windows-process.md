# 03-04b: Windowsプロセス実装

## 目的

Windows固有のプロセス管理を実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション2「プロセス管理」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 03-04a プロセスインターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `Sleep`, `LoadLibraryW`, `FreeLibrary`, `GetProcAddress`
- `<processthreadsapi.h>` - `GetCurrentProcessId`, `SetThreadAffinityMask`, `SetThreadPriority`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformProcess.h`
- `source/engine/hal/Public/HAL/PlatformProcess.h`
- `source/engine/hal/Private/Windows/WindowsPlatformProcess.cpp`

## TODO

- [ ] `WindowsPlatformProcess::Sleep` 実装
- [ ] `WindowsPlatformProcess::SleepNoStats` 実装
- [ ] `WindowsPlatformProcess::YieldThread` 実装
- [ ] `WindowsPlatformProcess::GetDllHandle` 実装
- [ ] `WindowsPlatformProcess::FreeDllHandle` 実装
- [ ] `WindowsPlatformProcess::GetDllExport` 実装
- [ ] `WindowsPlatformProcess::GetCurrentProcessId` 実装
- [ ] typedef `PlatformProcess` 定義

## 実装内容

### WindowsPlatformProcess.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformProcess.h"

namespace NS
{
    struct WindowsPlatformProcess : public GenericPlatformProcess
    {
        static void Sleep(float seconds);
        static void SleepNoStats(float seconds);
        static void SleepInfinite();
        static void YieldThread();

        static void* GetDllHandle(const TCHAR* filename);
        static void FreeDllHandle(void* dllHandle);
        static void* GetDllExport(void* dllHandle, const TCHAR* procName);

        static uint32 GetCurrentProcessId();
        static uint32 GetCurrentCoreNumber();

        static void SetThreadAffinityMask(uint64 mask);
        static void SetThreadPriority(int32 priority);
    };

    typedef WindowsPlatformProcess PlatformProcess;
}
```

### WindowsPlatformProcess.cpp

```cpp
#include "Windows/WindowsPlatformProcess.h"
#include <windows.h>
#include <processthreadsapi.h>

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
        // GetProcAddress はANSI文字列を要求
        char procNameAnsi[256];
        WideCharToMultiByte(CP_ACP, 0, procName, -1, procNameAnsi, 256, nullptr, nullptr);
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
}
```

## 検証

- `PlatformProcess::Sleep(0.1f)` が約100ms待機
- `PlatformProcess::GetCurrentProcessId()` が有効なPIDを返す
- DLLのロード/アンロードが正常動作

## 注意事項

- `GetProcAddress` はANSI文字列のみ受け付ける
- `Sleep(0)` は他スレッドに実行権を譲る（YieldThreadと同等ではない）

## 次のサブ計画

→ 03-05a: ファイルインターフェース
