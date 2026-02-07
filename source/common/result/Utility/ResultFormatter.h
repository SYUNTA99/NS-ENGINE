/// @file ResultFormatter.h
/// @brief Resultのフォーマットユーティリティ
#pragma once


#include "common/result/Context/ErrorChain.h"
#include "common/result/Context/ResultContext.h"
#include "common/result/Core/Result.h"
#include "common/result/Utility/ErrorInfo.h"

#include <format>
#include <string>
#include <string_view>

namespace NS::result
{

    /// フォーマットオプション
    struct FormatOptions
    {
        bool includeModuleName = true;
        bool includeErrorName = true;
        bool includeNumericValues = true;
        bool includeMessage = true;
        bool includeContext = false;
        bool includeChain = false;
        bool verbose = false;
        std::string_view separator = ": ";
        std::string_view chainIndent = "  -> ";
    };

    /// Resultをフォーマット
    [[nodiscard]] std::string FormatResult(::NS::Result result, const FormatOptions& options = {}) noexcept;

    /// コンパクトフォーマット
    /// 例: "FileSystem::PathNotFound (2:1)"
    [[nodiscard]] std::string FormatResultCompact(::NS::Result result) noexcept;

    /// 詳細フォーマット
    /// 例: "FileSystem::PathNotFound (Module=2, Desc=1): 指定されたパスが見つかりません"
    [[nodiscard]] std::string FormatResultVerbose(::NS::Result result) noexcept;

    /// チェーン付きフォーマット
    [[nodiscard]] std::string FormatResultWithChain(::NS::Result result) noexcept;

    /// コンテキスト付きフォーマット
    [[nodiscard]] std::string FormatResultWithContext(::NS::Result result) noexcept;

    /// デバッグ用完全フォーマット
    [[nodiscard]] std::string FormatResultFull(::NS::Result result) noexcept;

    /// 16進数でRaw値を取得
    [[nodiscard]] std::string FormatResultRaw(::NS::Result result) noexcept;

    /// std::formatサポート用アダプター
    class ResultFormatAdapter
    {
    public:
        explicit ResultFormatAdapter(::NS::Result result, FormatOptions options = {}) noexcept
            : m_result(result), m_options(options)
        {}

        [[nodiscard]] std::string ToString() const noexcept { return FormatResult(m_result, m_options); }

    private:
        ::NS::Result m_result;
        FormatOptions m_options;
    };

} // namespace NS::result

// std::format specialization
template <> struct std::formatter<::NS::Result>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(::NS::Result result, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ::NS::result::FormatResultCompact(result));
    }
};
