/// @file WindowsPlatformCrashContext.h
/// @brief Windows固有のクラッシュコンテキスト実装
#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"

namespace NS
{
    /// Windows固有のクラッシュコンテキスト実装
    ///
    /// SetUnhandledExceptionFilterを使用した例外捕捉。
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
        static long __stdcall UnhandledExceptionFilter(void* exceptionPointers);
    };

    /// 現在のプラットフォームのクラッシュコンテキスト
    using PlatformCrashContext = WindowsPlatformCrashContext;
} // namespace NS
