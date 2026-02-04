/// @file ResultLogging.cpp
/// @brief Result診断ログの実装
#include "common/result/Diagnostics/ResultLogging.h"

#include "common/result/Context/ErrorChain.h"
#include "common/result/Context/ResultContext.h"
#include "common/result/Diagnostics/ResultStatistics.h"
#include "common/result/Error/ErrorCategory.h"
#include "common/result/Utility/ErrorInfo.h"

#include <cstdio>

namespace NS::result
{

    ResultLogger& ResultLogger::Instance() noexcept
    {
        static ResultLogger instance;
        return instance;
    }

    void ResultLogger::Log(::NS::Result result, std::string_view context, SourceLocation location) noexcept
    {
        // 成功で、成功ログが無効なら何もしない
        if (result.IsSuccess() && !m_config.logSuccess)
        {
            return;
        }

        // 統計に記録
        if (m_config.recordStatistics)
        {
            RecordResult(result);
        }

        LogImpl(result, context, location);
    }

    void ResultLogger::LogIfFailed(::NS::Result result, std::string_view context, SourceLocation location) noexcept
    {
        if (result.IsFailure())
        {
            Log(result, context, location);
        }
    }

    void ResultLogger::LogImpl(::NS::Result result, std::string_view context, const SourceLocation& location) noexcept
    {
        if (m_config.level == ResultLogLevel::None)
        {
            return;
        }

        // フォーマットオプションを設定
        FormatOptions options = m_config.formatOptions;
        options.includeContext = m_config.logContext;
        options.includeChain = m_config.logChain;

        std::string message = FormatResult(result, options);

        // コンテキスト情報を追加
        if (!context.empty())
        {
            message = std::string(context) + ": " + message;
        }

        // ログレベル決定
        ResultLogLevel logLevel = ResultLogLevel::Info;
        if (result.IsFailure())
        {
            ErrorCategory category = GetErrorCategory(result);
            if (category.IsFatal())
            {
                logLevel = ResultLogLevel::Error;
            }
            else
            {
                logLevel = ResultLogLevel::Warning;
            }
        }

        // カスタムハンドラがあれば使用
        if (m_customHandler)
        {
            m_customHandler(logLevel, result, message, context, location);
            return;
        }

        // デフォルト: stderrに出力
        const char* levelStr = "INFO";
        switch (logLevel)
        {
        case ResultLogLevel::Error:
            levelStr = "ERROR";
            break;
        case ResultLogLevel::Warning:
            levelStr = "WARN";
            break;
        case ResultLogLevel::Info:
            levelStr = "INFO";
            break;
        case ResultLogLevel::Debug:
            levelStr = "DEBUG";
            break;
        case ResultLogLevel::Verbose:
            levelStr = "VERBOSE";
            break;
        default:
            break;
        }

        if (location.IsValid())
        {
            std::fprintf(stderr,
                         "[%s] [Result] %s (at %.*s:%u)\n",
                         levelStr,
                         message.c_str(),
                         static_cast<int>(location.file.size()),
                         location.file.data(),
                         location.line);
        }
        else
        {
            std::fprintf(stderr, "[%s] [Result] %s\n", levelStr, message.c_str());
        }
    }

} // namespace NS::result
