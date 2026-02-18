/// @file OutputDevice.h
/// @brief 出力デバイス基底クラス
#pragma once

#include "HAL/PlatformTypes.h"

namespace NS
{
    /// ログ詳細度レベル
    enum class LogVerbosity
    {
        NoLogging = 0, ///< ログ出力なし
        Fatal,         ///< 致命的エラー（クラッシュ前提）
        Error,         ///< エラー
        Warning,       ///< 警告
        Display,       ///< 表示（重要情報）
        Log,           ///< 通常ログ
        Verbose,       ///< 詳細ログ
        VeryVerbose,   ///< 非常に詳細なログ
        All            ///< すべて
    };

    /// 出力デバイス基底クラス
    ///
    /// ログやデバッグ出力の抽象化。
    /// 派生クラスでファイル、コンソール、デバッガ等に出力。
    ///
    /// @code
    /// OutputDeviceDebug debug;
    /// debug.Log(TEXT("Hello, World!"));
    /// debug.LogWarning(TEXT("This is a warning"));
    /// @endcode
    class OutputDevice
    {
    public:
        OutputDevice() = default;
        virtual ~OutputDevice() = default;
        NS_DISALLOW_COPY_AND_MOVE(OutputDevice);

    public:
        /// メッセージをシリアライズ（派生クラスで実装）
        /// @param message 出力メッセージ
        /// @param verbosity 詳細度レベル
        virtual void Serialize(const TCHAR* message, LogVerbosity verbosity) = 0;

        /// 通常ログ出力
        void Log(const TCHAR* message) { Serialize(message, LogVerbosity::Log); }

        /// 警告出力
        void LogWarning(const TCHAR* message) { Serialize(message, LogVerbosity::Warning); }

        /// エラー出力
        void LogError(const TCHAR* message) { Serialize(message, LogVerbosity::Error); }

        /// 致命的エラー出力
        void LogFatal(const TCHAR* message) { Serialize(message, LogVerbosity::Fatal); }

        /// バッファフラッシュ
        virtual void Flush() {}

        /// デバイスが任意のスレッドから使用可能かどうか
        [[nodiscard]] virtual bool CanBeUsedOnAnyThread() const { return false; }
    };

    /// デバッガ出力デバイス
    ///
    /// Visual Studioの出力ウィンドウ等に出力する。
    class OutputDeviceDebug : public OutputDevice
    {
    public:
        OutputDeviceDebug() = default;
        ~OutputDeviceDebug() override = default;
        NS_DISALLOW_COPY_AND_MOVE(OutputDeviceDebug);

    public:
        void Serialize(const TCHAR* message, LogVerbosity verbosity) override;
        void Flush() override;
        [[nodiscard]] bool CanBeUsedOnAnyThread() const override { return true; }

        /// デバッガが接続されているかどうか
        static bool IsDebuggerPresent();
    };

    /// コンソール出力デバイス
    ///
    /// 標準出力に出力する。
    class OutputDeviceConsole : public OutputDevice
    {
    public:
        OutputDeviceConsole() = default;
        ~OutputDeviceConsole() override = default;
        NS_DISALLOW_COPY_AND_MOVE(OutputDeviceConsole);

    public:
        void Serialize(const TCHAR* message, LogVerbosity verbosity) override;
        void Flush() override;
        [[nodiscard]] bool CanBeUsedOnAnyThread() const override { return true; }
    };
} // namespace NS
