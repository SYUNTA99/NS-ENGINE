# 08-01a: OutputDeviceインターフェース

## 目的

出力デバイスの基底インターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション1「概要」

## 依存（commonモジュール）

（なし）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/OutputDevice.h`

## TODO

- [ ] `LogVerbosity` 列挙型定義
- [ ] `OutputDevice` 基底クラス定義
- [ ] 仮想メソッド宣言

## 実装内容

```cpp
// OutputDevice.h
#pragma once

#include "HAL/PlatformTypes.h"

namespace NS
{
    /// ログ詳細度レベル
    enum class LogVerbosity
    {
        NoLogging = 0,  ///< ログ出力なし
        Fatal,          ///< 致命的エラー（クラッシュ前提）
        Error,          ///< エラー
        Warning,        ///< 警告
        Display,        ///< 表示（重要情報）
        Log,            ///< 通常ログ
        Verbose,        ///< 詳細ログ
        VeryVerbose,    ///< 非常に詳細なログ
        All             ///< すべて
    };

    /// 出力デバイス基底クラス
    ///
    /// ログやデバッグ出力の抽象化。
    /// 派生クラスでファイル、コンソール、デバッガ等に出力。
    class OutputDevice
    {
    public:
        virtual ~OutputDevice() = default;

        /// メッセージをシリアライズ（派生クラスで実装）
        /// @param message 出力メッセージ
        /// @param verbosity 詳細度レベル
        virtual void Serialize(const TCHAR* message, LogVerbosity verbosity) = 0;

        /// 通常ログ出力
        void Log(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Log);
        }

        /// 警告出力
        void LogWarning(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Warning);
        }

        /// エラー出力
        void LogError(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Error);
        }

        /// 致命的エラー出力
        void LogFatal(const TCHAR* message)
        {
            Serialize(message, LogVerbosity::Fatal);
        }

        /// バッファフラッシュ
        virtual void Flush() {}

        /// デバイスが有効かどうか
        virtual bool CanBeUsedOnAnyThread() const { return false; }
    };
}
```

## 検証

- `LogVerbosity` 列挙型が定義される
- `OutputDevice` が抽象クラスとして機能

## 次のサブ計画

→ 08-01b: OutputDeviceDebug実装
