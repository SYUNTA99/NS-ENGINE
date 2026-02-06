/// @file GenericPlatformCrashContext.h
/// @brief プラットフォーム非依存のクラッシュコンテキストインターフェース
#pragma once

#include "HAL/PlatformTypes.h"

namespace NS
{
    /// クラッシュコンテキストの種類
    enum class CrashContextType
    {
        Crash,           ///< 一般的なクラッシュ
        Assert,          ///< アサーション失敗
        Ensure,          ///< Ensure失敗（継続可能）
        Stall,           ///< ストール（長時間ブロック）
        GPUCrash,        ///< GPUクラッシュ
        Hang,            ///< ハング（応答なし）
        OutOfMemory,     ///< メモリ不足
        AbnormalShutdown ///< 異常終了
    };

    /// クラッシュハンドラ関数型
    using CrashHandlerFunc = void (*)(void* exceptionInfo);

    /// プラットフォーム非依存のクラッシュコンテキストクラス
    ///
    /// ## 使用パターン
    ///
    /// ```cpp
    /// // 起動時にハンドラを設定
    /// PlatformCrashContext::SetUnhandledExceptionFilter();
    /// PlatformCrashContext::SetCrashHandler([](void* info) {
    ///     // クラッシュログ出力等
    /// });
    /// ```
    class GenericPlatformCrashContext
    {
    public:
        explicit GenericPlatformCrashContext(CrashContextType type);
        virtual ~GenericPlatformCrashContext() = default;

        /// コンテキスト種類を取得
        CrashContextType GetType() const { return m_type; }

        /// コンテキスト情報をキャプチャ
        virtual void CaptureContext();

        /// エラーメッセージを設定
        virtual void SetErrorMessage(const TCHAR* message);

        /// エラーメッセージを取得
        const TCHAR* GetErrorMessage() const { return m_errorMessage; }

        // =====================================================================
        // 静的メソッド
        // =====================================================================

        /// エンジンバージョンを設定
        static void SetEngineVersion(const TCHAR* version);

        /// クラッシュハンドラを設定
        static void SetCrashHandler(CrashHandlerFunc handler);

        /// クラッシュハンドラを取得
        static CrashHandlerFunc GetCrashHandler();

    protected:
        CrashContextType m_type;
        TCHAR m_errorMessage[1024];

        static TCHAR s_engineVersion[64];
        static CrashHandlerFunc s_crashHandler;
    };
} // namespace NS
