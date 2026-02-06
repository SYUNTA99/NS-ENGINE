# 05-04b: Windowsクラッシュコンテキスト実装

## 目的

Windows固有のクラッシュコンテキスト収集を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション3「クラッシュ処理」

## 依存（commonモジュール）

- `common/utility/macros.h` - 文字列操作

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 05-03b Windowsスタックウォーク（オプション：スタックトレース付きクラッシュ）
- 05-04a クラッシュコンテキストインターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `SetUnhandledExceptionFilter`, `EXCEPTION_POINTERS`
- `<errhandlingapi.h>` - 例外処理

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformCrashContext.h`
- `source/engine/hal/Public/HAL/PlatformCrashContext.h`
- `source/engine/hal/Private/Windows/WindowsPlatformCrashContext.cpp`

## TODO

- [ ] `WindowsPlatformCrashContext` クラス実装
- [ ] `SetUnhandledExceptionFilter` 呼び出し
- [ ] 例外情報からコンテキスト収集
- [ ] typedef `PlatformCrashContext` 定義

## 実装内容

### WindowsPlatformCrashContext.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"

namespace NS
{
    class WindowsPlatformCrashContext : public GenericPlatformCrashContext
    {
    public:
        using GenericPlatformCrashContext::GenericPlatformCrashContext;

        /// Windows固有のコンテキストキャプチャ
        void CaptureContext() override;

        /// Windowsの未処理例外フィルターを設定
        static void SetUnhandledExceptionFilter();

        /// 例外ポインタからコンテキストを作成
        static void CaptureFromException(void* exceptionPointers);

    private:
        static long WINAPI UnhandledExceptionFilter(void* exceptionPointers);
    };

    typedef WindowsPlatformCrashContext PlatformCrashContext;
}
```

### WindowsPlatformCrashContext.cpp

```cpp
#include "Windows/WindowsPlatformCrashContext.h"
#include <windows.h>
#include <cstring>

namespace NS
{
    // 静的メンバ初期化
    TCHAR GenericPlatformCrashContext::s_engineVersion[64] = TEXT("Unknown");
    CrashHandlerFunc GenericPlatformCrashContext::s_crashHandler = nullptr;

    GenericPlatformCrashContext::GenericPlatformCrashContext(CrashContextType type)
        : m_type(type)
    {
        m_errorMessage[0] = TEXT('\0');
    }

    void GenericPlatformCrashContext::CaptureContext()
    {
        // 基底クラスでは何もしない
    }

    void GenericPlatformCrashContext::SetErrorMessage(const TCHAR* message)
    {
        wcsncpy_s(m_errorMessage, message, _TRUNCATE);
    }

    void GenericPlatformCrashContext::SetEngineVersion(const TCHAR* version)
    {
        wcsncpy_s(s_engineVersion, version, _TRUNCATE);
    }

    void GenericPlatformCrashContext::SetCrashHandler(CrashHandlerFunc handler)
    {
        s_crashHandler = handler;
    }

    CrashHandlerFunc GenericPlatformCrashContext::GetCrashHandler()
    {
        return s_crashHandler;
    }

    // WindowsPlatformCrashContext
    void WindowsPlatformCrashContext::CaptureContext()
    {
        // TODO: Windows固有のコンテキスト収集
        // - レジスタ状態
        // - スタックトレース
        // - ロードされたモジュール一覧
    }

    void WindowsPlatformCrashContext::SetUnhandledExceptionFilter()
    {
        ::SetUnhandledExceptionFilter(
            reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(UnhandledExceptionFilter)
        );
    }

    long WINAPI WindowsPlatformCrashContext::UnhandledExceptionFilter(void* exceptionPointers)
    {
        CaptureFromException(exceptionPointers);

        if (s_crashHandler)
        {
            s_crashHandler(exceptionPointers);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void WindowsPlatformCrashContext::CaptureFromException(void* exceptionPointers)
    {
        EXCEPTION_POINTERS* ep = static_cast<EXCEPTION_POINTERS*>(exceptionPointers);
        if (!ep) return;

        // 例外コードから種類を判定
        CrashContextType type = CrashContextType::Crash;

        switch (ep->ExceptionRecord->ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_STACK_OVERFLOW:
            type = CrashContextType::Crash;
            break;
        default:
            type = CrashContextType::Crash;
            break;
        }

        WindowsPlatformCrashContext context(type);
        context.CaptureContext();
    }
}
```

### PlatformCrashContext.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"
#include COMPILED_PLATFORM_HEADER(PlatformCrashContext.h)
```

## 検証

- `PlatformCrashContext::SetUnhandledExceptionFilter()` が正常に設定
- クラッシュ時にハンドラが呼ばれる（テスト困難だがデバッガで確認可能）

## 注意事項

- クラッシュハンドラ内では最小限の処理のみ行う
- メモリ確保は避ける（クラッシュ原因がOOMの可能性）
- スタックオーバーフロー時は特別な処理が必要

## 次のサブ計画

→ 06-01: Mallocインターフェース
