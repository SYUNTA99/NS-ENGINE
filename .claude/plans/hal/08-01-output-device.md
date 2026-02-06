> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [08-01a: OutputDeviceインターフェース](08-01a-output-device-interface.md)
> - [08-01b: OutputDeviceDebug実装](08-01b-output-device-debug.md)

# 08-01: OutputDevice（旧版）

## 目的

出力デバイス抽象化を実装する。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション1「概要」

## 依存（commonモジュール）

（commonモジュールへの直接依存なし）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/OutputDevice.h`

## TODO

- [ ] `OutputDevice` 基底クラス
- [ ] `LogVerbosity` 列挙型
- [ ] ログ出力メソッド
- [ ] フォーマット付き出力

## 実装内容

```cpp
// OutputDevice.h
#pragma once

namespace NS
{
    enum class LogVerbosity
    {
        NoLogging = 0,
        Fatal,
        Error,
        Warning,
        Display,
        Log,
        Verbose,
        VeryVerbose,
        All
    };

    class OutputDevice
    {
    public:
        virtual ~OutputDevice() = default;

        virtual void Serialize(const TCHAR* message, LogVerbosity verbosity) = 0;

        void Log(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Log);
        }

        void LogWarning(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Warning);
        }

        void LogError(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Error);
        }

        virtual void Flush() {}
    };

    class OutputDeviceDebug : public OutputDevice
    {
    public:
        void Serialize(const TCHAR* message, LogVerbosity verbosity) override;
    };
}
```

## 検証

- ログ出力がデバッガに表示される
- 各Verbosityレベルが正しく機能する
- `OutputDeviceDebug::Serialize()` が `OutputDebugStringW()` を呼ぶ

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `OutputDebugStringW`

## 注意事項

- `OutputDevice` は基底クラス（ファイル、コンソール等に派生可能）
- `LogVerbosity::Fatal` は致命的エラー（クラッシュ前提）
- Verbosityによるフィルタリングは上位層で行う

## 次のサブ計画

→ 08-02: 名前付きイベント
