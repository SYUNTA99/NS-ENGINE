# 08-01b: OutputDeviceDebug実装

## 目的

デバッガ出力用のOutputDeviceを実装する。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション2「デバッグ出力」

## 依存（commonモジュール）

- `common/utility/macros.h` - プラットフォーム検出

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`）
- 08-01a OutputDeviceインターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `OutputDebugStringW`, `IsDebuggerPresent`

## 変更対象

変更:
- `source/engine/hal/Public/HAL/OutputDevice.h`（クラス追加）

新規作成:
- `source/engine/hal/Private/HAL/OutputDevice.cpp`

## TODO

- [ ] `OutputDeviceDebug` クラス定義
- [ ] `Serialize` 実装（`OutputDebugStringW`）
- [ ] デバッガ存在チェック

## 実装内容

### OutputDevice.h（追加）

```cpp
// OutputDevice.h に追加

namespace NS
{
    /// デバッガ出力デバイス
    ///
    /// Visual Studioの出力ウィンドウ等に出力する。
    class OutputDeviceDebug : public OutputDevice
    {
    public:
        OutputDeviceDebug() = default;
        ~OutputDeviceDebug() override = default;

        void Serialize(const TCHAR* message, LogVerbosity verbosity) override;
        void Flush() override;
        bool CanBeUsedOnAnyThread() const override { return true; }

        /// デバッガが接続されているかどうか
        static bool IsDebuggerPresent();
    };
}
```

### OutputDevice.cpp

```cpp
#include "HAL/OutputDevice.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace NS
{
    void OutputDeviceDebug::Serialize(const TCHAR* message, LogVerbosity verbosity)
    {
        if (message == nullptr)
        {
            return;
        }

#if PLATFORM_WINDOWS
        // Verbosityに応じたプレフィックスを追加（オプション）
        const TCHAR* prefix = TEXT("");
        switch (verbosity)
        {
        case LogVerbosity::Fatal:
            prefix = TEXT("[FATAL] ");
            break;
        case LogVerbosity::Error:
            prefix = TEXT("[ERROR] ");
            break;
        case LogVerbosity::Warning:
            prefix = TEXT("[WARN] ");
            break;
        default:
            break;
        }

        // デバッガに出力
        if (prefix[0] != TEXT('\0'))
        {
            ::OutputDebugStringW(prefix);
        }
        ::OutputDebugStringW(message);
        ::OutputDebugStringW(TEXT("\n"));
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
}
```

## 検証

- Visual Studioの出力ウィンドウにログが表示される
- `IsDebuggerPresent()` がデバッグ時にtrueを返す
- 各Verbosityに応じたプレフィックスが付く

## 注意事項

- `OutputDebugStringW` はデバッガがない場合は無視される
- スレッドセーフ（`CanBeUsedOnAnyThread() = true`）
- 改行は自動追加

## 次のサブ計画

→ 08-02a: ScopedNamedEventクラス
