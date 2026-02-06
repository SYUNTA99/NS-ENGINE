# 03-04: PlatformProcess（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [03-04a: プロセスインターフェース](03-04a-process-interface.md)
> - [03-04b: Windowsプロセス実装](03-04b-windows-process.md)

---

# 03-04: PlatformProcess（旧）

## 目的

プロセス管理の抽象化レイヤーを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション2「プロセス管理」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`, `uint64`, `int32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformProcess.h`
- `source/engine/hal/Public/Windows/WindowsPlatformProcess.h`
- `source/engine/hal/Public/HAL/PlatformProcess.h`
- `source/engine/hal/Private/Windows/WindowsPlatformProcess.cpp`

## TODO

- [ ] `GenericPlatformProcess` 構造体（スリープ、DLL管理）
- [ ] `WindowsPlatformProcess` 構造体（Win32 API使用）
- [ ] `PlatformProcess.h` エントリポイント
- [ ] DLLハンドル型定義

## 実装内容

```cpp
// GenericPlatformProcess.h
namespace NS
{
    struct GenericPlatformProcess
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
}

// WindowsPlatformProcess.h
namespace NS
{
    struct WindowsPlatformProcess : public GenericPlatformProcess
    {
        static void Sleep(float seconds)
        {
            ::Sleep((DWORD)(seconds * 1000.0f));
        }

        static void* GetDllHandle(const TCHAR* filename)
        {
            return ::LoadLibraryW(filename);
        }
        // ...
    };

    typedef WindowsPlatformProcess PlatformProcess;
}
```

## 検証

- Sleep が指定時間待機
- DLL ロード/アンロードが正常動作
- GetCurrentProcessId() が有効なPIDを返す

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `Sleep`, `LoadLibraryW`, `FreeLibrary`, `GetProcAddress`
- `<processthreadsapi.h>` - `GetCurrentProcessId`, `SetThreadAffinityMask`

## 実装詳細

### WindowsPlatformProcess.cpp

```cpp
void WindowsPlatformProcess::SleepNoStats(float seconds)
{
    ::Sleep((DWORD)(seconds * 1000.0f));
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
        ::FreeLibrary((HMODULE)dllHandle);
    }
}

void* WindowsPlatformProcess::GetDllExport(void* dllHandle, const TCHAR* procName)
{
    // ANSI変換が必要
    char procNameAnsi[256];
    WideCharToMultiByte(CP_ACP, 0, procName, -1, procNameAnsi, 256, nullptr, nullptr);
    return ::GetProcAddress((HMODULE)dllHandle, procNameAnsi);
}
```

## 注意事項

- `GetDllExport` はANSI文字列が必要（Win32 API制限）
- `Sleep(0)` は他スレッドに実行権を譲る
- `SleepInfinite` は `INFINITE` を使用
