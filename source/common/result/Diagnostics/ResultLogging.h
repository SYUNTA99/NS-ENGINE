/// @file ResultLogging.h
/// @brief Result診断ログ
#pragma once


#include "common/result/Context/SourceLocation.h"
#include "common/result/Core/Result.h"
#include "common/result/Core/ResultConfig.h"
#include "common/result/Utility/ResultFormatter.h"

#include <functional>
#include <string_view>

namespace NS { namespace result {

    /// ログレベル
    enum class ResultLogLevel
    {
        None,
        Error,
        Warning,
        Info,
        Debug,
        Verbose,
    };

    /// ログ設定
    struct ResultLogConfig
    {
        ResultLogLevel level = ResultLogLevel::Error;
        bool logSuccess = false;
        bool logContext = true;
        bool logChain = true;
        bool recordStatistics = true;
        FormatOptions formatOptions{};
    };

    /// Resultログ出力
    class ResultLogger
    {
    public:
        /// ログハンドラの型
        using LogHandler = std::function<void(ResultLogLevel level,
                                              ::NS::Result result,
                                              std::string_view formattedMessage,
                                              std::string_view context,
                                              const SourceLocation& location)>;

        /// シングルトン取得
        [[nodiscard]] static ResultLogger& Instance() noexcept;

        /// 設定を取得/設定
        [[nodiscard]] const ResultLogConfig& GetConfig() const noexcept { return m_config; }
        void SetConfig(const ResultLogConfig& config) noexcept { m_config = config; }

        /// ログレベル設定
        void SetLogLevel(ResultLogLevel level) noexcept { m_config.level = level; }

        /// Resultをログ出力
        void Log(::NS::Result result,
                 std::string_view context = {},
                 SourceLocation location = SourceLocation::FromStd()) noexcept;

        /// 条件付きログ（失敗時のみ）
        void LogIfFailed(::NS::Result result,
                         std::string_view context = {},
                         SourceLocation location = SourceLocation::FromStd()) noexcept;

        /// カスタムログハンドラを設定
        void SetCustomHandler(LogHandler handler) noexcept { m_customHandler = std::move(handler); }

        /// デフォルトハンドラに戻す
        void ResetHandler() noexcept { m_customHandler = nullptr; }

    private:
        ResultLogger() = default;

        void LogImpl(::NS::Result result, std::string_view context, const SourceLocation& location) noexcept;

        ResultLogConfig m_config;
        LogHandler m_customHandler;
    };

    //=========================================================================
    // グローバル関数
    //=========================================================================

#if NS_RESULT_ENABLE_LOGGING

    /// Resultをログ出力
    inline void LogResult(::NS::Result result,
                          std::string_view context = {},
                          SourceLocation location = SourceLocation::FromStd()) noexcept
    {
        ResultLogger::Instance().Log(result, context, location);
    }

    /// 失敗時のみログ出力
    inline void LogResultIfFailed(::NS::Result result,
                                  std::string_view context = {},
                                  SourceLocation location = SourceLocation::FromStd()) noexcept
    {
        ResultLogger::Instance().LogIfFailed(result, context, location);
    }

#else

    // リリースビルド: 何もしない
    inline void LogResult(::NS::Result /*result*/,
                          std::string_view /*context*/ = {},
                          SourceLocation /*location*/ = {}) noexcept
    {}

    inline void LogResultIfFailed(::NS::Result /*result*/,
                                  std::string_view /*context*/ = {},
                                  SourceLocation /*location*/ = {}) noexcept
    {}

#endif

    //=========================================================================
    // マクロ
    //=========================================================================

#if NS_RESULT_ENABLE_LOGGING

/// 失敗時にログ出力して早期リターン
#define NS_LOG_AND_RETURN_IF_FAILED(expr, ...)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure())                                                        \
        {                                                                                                              \
            ::NS::result::LogResult(_ns_result_, __VA_ARGS__);                                                         \
            return _ns_result_;                                                                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// 診断情報を含めてログ出力
#define NS_LOG_RESULT_DIAGNOSTIC(result) ::NS::result::LogResult(result, {}, NS_CURRENT_SOURCE_LOCATION())

#else

// リリースビルド: ログなし、単純なリターン
#define NS_LOG_AND_RETURN_IF_FAILED(expr, ...) NS_RETURN_IF_FAILED(expr)
#define NS_LOG_RESULT_DIAGNOSTIC(result) ((void)(result))

#endif

}} // namespace NS::result
