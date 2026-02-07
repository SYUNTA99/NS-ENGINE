/// @file SourceLocation.h
/// @brief エラー発生箇所情報
#pragma once


#include <cstdint>
#include <source_location>
#include <string_view>

namespace NS::result
{

    /// エラー発生箇所情報
    struct SourceLocation
    {
        std::string_view file;
        std::string_view function;
        std::uint32_t line = 0;
        std::uint32_t column = 0;

        constexpr SourceLocation() noexcept = default;

        constexpr SourceLocation(std::string_view file_,
                                 std::string_view function_,
                                 std::uint32_t line_,
                                 std::uint32_t column_ = 0) noexcept
            : file(file_), function(function_), line(line_), column(column_)
        {}

        /// std::source_locationから構築
        static SourceLocation FromStd(const std::source_location& loc = std::source_location::current()) noexcept
        {
            return SourceLocation{loc.file_name(),
                                  loc.function_name(),
                                  static_cast<std::uint32_t>(loc.line()),
                                  static_cast<std::uint32_t>(loc.column())};
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept { return !file.empty() && line > 0; }
    };

/// 現在位置を取得するマクロ
#define NS_CURRENT_SOURCE_LOCATION() ::NS::result::SourceLocation::FromStd(std::source_location::current())

} // namespace NS::result
