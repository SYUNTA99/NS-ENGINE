/// @file ResultFormatter.cpp
/// @brief Resultのフォーマット実装
#include "common/result/Utility/ResultFormatter.h"

#include <charconv>
#include <cstring>
#include <format>

namespace NS::result
{

    namespace
    {

        // スレッドローカルバッファで動的メモリ確保を最小化
        thread_local char t_formatBuffer[2048];

        // 高速フォーマット用ヘルパー
        class FastStringBuilder
        {
        public:
            FastStringBuilder() noexcept : m_pos(0) {}

            void Append(std::string_view str) noexcept
            {
                const std::size_t remaining = sizeof(t_formatBuffer) - m_pos - 1;
                const std::size_t copyLen = (std::min)(str.size(), remaining);
                std::memcpy(t_formatBuffer + m_pos, str.data(), copyLen);
                m_pos += copyLen;
            }

            void Append(char c) noexcept
            {
                if (m_pos < sizeof(t_formatBuffer) - 1)
                {
                    t_formatBuffer[m_pos++] = c;
                }
            }

            void AppendNumber(int value) noexcept
            {
                char buf[16];
                auto result = std::to_chars(buf, buf + sizeof(buf), value);
                Append(std::string_view(buf, result.ptr - buf));
            }

            [[nodiscard]] std::string ToString() const noexcept { return std::string(t_formatBuffer, m_pos); }

        private:
            std::size_t m_pos;
        };

    } // namespace

    std::string FormatResult(::NS::Result result, const FormatOptions& options) noexcept
    {
        if (result.IsSuccess())
        {
            return "Success";
        }

        FastStringBuilder builder;
        ErrorInfo info = GetErrorInfo(result);

        // モジュール名::エラー名
        if (options.includeModuleName && options.includeErrorName)
        {
            builder.Append(info.moduleName);
            builder.Append("::");
            builder.Append(info.errorName);
        }
        else if (options.includeModuleName)
        {
            builder.Append(info.moduleName);
        }
        else if (options.includeErrorName)
        {
            builder.Append(info.errorName);
        }

        // 数値ID
        if (options.includeNumericValues)
        {
            if (options.verbose)
            {
                builder.Append(" (Module=");
                builder.AppendNumber(info.module);
                builder.Append(", Desc=");
                builder.AppendNumber(info.description);
                builder.Append(')');
            }
            else
            {
                builder.Append(" (");
                builder.AppendNumber(info.module);
                builder.Append(':');
                builder.AppendNumber(info.description);
                builder.Append(')');
            }
        }

        // メッセージ
        if (options.includeMessage && !info.message.empty())
        {
            builder.Append(options.separator);
            builder.Append(info.message);
        }

        // コンテキスト
        if (options.includeContext)
        {
            if (auto ctx = GetResultContext(result))
            {
                builder.Append("\n  at ");
                builder.Append(ctx->location.file);
                builder.Append(':');
                builder.AppendNumber(static_cast<int>(ctx->location.line));
                if (!ctx->location.function.empty())
                {
                    builder.Append(" in ");
                    builder.Append(ctx->location.function);
                }
                if (!ctx->message.empty())
                {
                    builder.Append("\n  Message: ");
                    builder.Append(ctx->message);
                }
            }
        }

        // チェーン
        if (options.includeChain)
        {
            if (auto chain = GetErrorChain(result))
            {
                for (std::size_t i = 1; i < chain->GetDepth(); ++i)
                {
                    const auto& entry = (*chain)[i];
                    ErrorInfo entryInfo = GetErrorInfo(entry.result);
                    builder.Append('\n');
                    builder.Append(options.chainIndent);
                    builder.Append(entryInfo.moduleName);
                    builder.Append("::");
                    builder.Append(entryInfo.errorName);
                    if (options.includeNumericValues)
                    {
                        builder.Append(" (");
                        builder.AppendNumber(entryInfo.module);
                        builder.Append(':');
                        builder.AppendNumber(entryInfo.description);
                        builder.Append(')');
                    }
                    if (!entryInfo.message.empty())
                    {
                        builder.Append(options.separator);
                        builder.Append(entryInfo.message);
                    }
                }
            }
        }

        return builder.ToString();
    }

    std::string FormatResultCompact(::NS::Result result) noexcept
    {
        return FormatResult(result, FormatOptions{.includeMessage = false});
    }

    std::string FormatResultVerbose(::NS::Result result) noexcept
    {
        return FormatResult(result, FormatOptions{.verbose = true});
    }

    std::string FormatResultWithChain(::NS::Result result) noexcept
    {
        return FormatResult(result, FormatOptions{.includeChain = true});
    }

    std::string FormatResultWithContext(::NS::Result result) noexcept
    {
        return FormatResult(result, FormatOptions{.includeContext = true});
    }

    std::string FormatResultFull(::NS::Result result) noexcept
    {
        return FormatResult(result, FormatOptions{.includeContext = true, .includeChain = true, .verbose = true});
    }

    std::string FormatResultRaw(::NS::Result result) noexcept
    {
        return std::format("0x{:08X}", result.GetRawValue());
    }

} // namespace NS::result
