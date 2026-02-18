/// @file WindowsPlatformCrashContext.h
/// @brief Windows固有のクラッシュコンテキスト実装
#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"

namespace NS
{
    /// スタックトレースの最大深度
    constexpr int32 kCrashMaxStackDepth = 64;

    /// Windows固有のクラッシュコンテキスト実装
    ///
    /// SetUnhandledExceptionFilterを使用した例外捕捉。
    class WindowsPlatformCrashContext : public GenericPlatformCrashContext
    {
    public:
        using GenericPlatformCrashContext::GenericPlatformCrashContext;

        /// Windows固有のコンテキストキャプチャ
        void CaptureContext() override;

        /// キャプチャされたスタックトレースを取得
        [[nodiscard]] const uint64* GetStackTrace() const { return m_stackTrace; }

        /// キャプチャされたスタック深度を取得
        [[nodiscard]] int32 GetStackDepth() const { return m_stackDepth; }

        /// 例外コードを取得
        uint32 GetExceptionCode() const { return m_exceptionCode; }

        /// 例外アドレスを取得
        [[nodiscard]] uint64 GetExceptionAddress() const { return m_exceptionAddress; }

        /// Windowsの未処理例外フィルターを設定
        static void SetUnhandledExceptionFilter();

        /// 例外ポインタからコンテキストを作成
        static void CaptureFromException(void* exceptionPointers);

    private:
        static long __stdcall UnhandledExceptionFilter(void* exceptionPointers);

        uint64 m_stackTrace[kCrashMaxStackDepth] = {};
        int32 m_stackDepth = 0;
        uint32 m_exceptionCode = 0;
        uint64 m_exceptionAddress = 0;
    };

    /// 現在のプラットフォームのクラッシュコンテキスト
    using PlatformCrashContext = WindowsPlatformCrashContext;
} // namespace NS
