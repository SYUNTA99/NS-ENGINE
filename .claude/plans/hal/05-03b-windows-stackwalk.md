# 05-03b: Windowsスタックウォーク実装

## 目的

Windows固有のスタックトレース取得を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション4「スタックウォーク」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 05-03a スタックウォークインターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `RtlCaptureStackBackTrace`
- `<dbghelp.h>` - `SymFromAddr`, `SymGetLineFromAddr64`, `SymInitialize`

## 必要なライブラリ（Windows）

- `dbghelp.lib`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformStackWalk.h`
- `source/engine/hal/Public/HAL/PlatformStackWalk.h`
- `source/engine/hal/Private/Windows/WindowsPlatformStackWalk.cpp`

## TODO

- [ ] `WindowsPlatformStackWalk::CaptureStackBackTrace` 実装
- [ ] `WindowsPlatformStackWalk::InitStackWalking` 実装（SymInitialize）
- [ ] `WindowsPlatformStackWalk::ProgramCounterToSymbolInfo` 実装
- [ ] typedef `PlatformStackWalk` 定義

## 実装内容

### WindowsPlatformStackWalk.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformStackWalk.h"

namespace NS
{
    struct WindowsPlatformStackWalk : public GenericPlatformStackWalk
    {
        static int32 CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount = 0);
        static bool ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo);
        static void InitStackWalking();
    };

    typedef WindowsPlatformStackWalk PlatformStackWalk;
}
```

### WindowsPlatformStackWalk.cpp

```cpp
#include "Windows/WindowsPlatformStackWalk.h"
#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

namespace NS
{
    static bool s_stackWalkInitialized = false;

    void WindowsPlatformStackWalk::InitStackWalking()
    {
        if (s_stackWalkInitialized) return;

        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
        SymInitialize(process, nullptr, TRUE);

        s_stackWalkInitialized = true;
    }

    int32 WindowsPlatformStackWalk::CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount)
    {
        return static_cast<int32>(::RtlCaptureStackBackTrace(
            skipCount + 1,  // この関数自体をスキップ
            maxDepth,
            reinterpret_cast<void**>(backTrace),
            nullptr
        ));
    }

    bool WindowsPlatformStackWalk::ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo)
    {
        if (!s_stackWalkInitialized)
        {
            InitStackWalking();
        }

        HANDLE process = GetCurrentProcess();
        DWORD64 address = static_cast<DWORD64>(programCounter);

        // シンボル情報
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        outInfo.programCounter = programCounter;
        outInfo.lineNumber = 0;
        outInfo.moduleName[0] = '\0';
        outInfo.functionName[0] = '\0';
        outInfo.filename[0] = '\0';

        if (SymFromAddr(process, address, nullptr, symbol))
        {
            strncpy_s(outInfo.functionName, symbol->Name, _TRUNCATE);
        }

        // 行情報
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement;

        if (SymGetLineFromAddr64(process, address, &displacement, &line))
        {
            strncpy_s(outInfo.filename, line.FileName, _TRUNCATE);
            outInfo.lineNumber = static_cast<int32>(line.LineNumber);
        }

        // モジュール情報
        IMAGEHLP_MODULE64 module;
        module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        if (SymGetModuleInfo64(process, address, &module))
        {
            strncpy_s(outInfo.moduleName, module.ModuleName, _TRUNCATE);
        }

        return outInfo.functionName[0] != '\0';
    }
}
```

## 検証

- `PlatformStackWalk::CaptureStackBackTrace()` が有効なアドレスを返す
- `PlatformStackWalk::ProgramCounterToSymbolInfo()` が関数名を解決（デバッグビルド）

## 注意事項

- `dbghelp.lib` のリンクが必要
- シンボル解決はデバッグ情報（PDB）が必要
- `InitStackWalking()` はスレッドセーフではない（起動時に一度だけ呼ぶ）

## 次のサブ計画

→ 05-04a: クラッシュコンテキストインターフェース
