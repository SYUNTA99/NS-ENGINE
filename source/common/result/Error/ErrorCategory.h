/// @file ErrorCategory.h
/// @brief エラーの性質分類（時間的性質、重大度）
#pragma once


#include "common/result/Core/Result.h"
#include "common/result/Core/ResultTraits.h"
#include <cstdint>

namespace NS::result
{

    /// エラーの時間的性質（2bit: Reserved領域 bit22-23に格納）
    enum class ErrorPersistence : std::uint8_t
    {
        Unknown = 0,   // 不明
        Transient = 1, // 一時的（リトライで解決可能）
        Permanent = 2, // 永続的（リトライしても解決しない）
    };

    /// エラーの重大度（2bit: Reserved領域 bit24-25に格納）
    enum class ErrorSeverity : std::uint8_t
    {
        Unknown = 0,     // 不明
        Recoverable = 1, // 回復可能（処理続行可）
        Fatal = 2,       // 致命的（処理続行不可）
    };

    /// エラーカテゴリ（組み合わせ）
    /// Result値のReserved領域に直接エンコードされる
    struct ErrorCategory
    {
        ErrorPersistence persistence = ErrorPersistence::Unknown;
        ErrorSeverity severity = ErrorSeverity::Unknown;

        /// リトライ可能か
        [[nodiscard]] constexpr bool IsRetriable() const noexcept { return persistence == ErrorPersistence::Transient; }

        /// 致命的か
        [[nodiscard]] constexpr bool IsFatal() const noexcept { return severity == ErrorSeverity::Fatal; }

        /// 回復可能か
        [[nodiscard]] constexpr bool IsRecoverable() const noexcept { return severity == ErrorSeverity::Recoverable; }

        /// カテゴリが判明しているか
        [[nodiscard]] constexpr bool IsKnown() const noexcept
        {
            return persistence != ErrorPersistence::Unknown || severity != ErrorSeverity::Unknown;
        }
    };

    /// エラーカテゴリを取得（Result値から直接デコード）
    [[nodiscard]] inline constexpr ErrorCategory GetErrorCategory(::NS::Result result) noexcept
    {
        if (result.IsSuccess())
        {
            return ErrorCategory{};
        }
        const auto raw = result.GetRawValue();
        return ErrorCategory{.persistence =
                                 static_cast<ErrorPersistence>(detail::ResultTraits::GetPersistenceFromValue(raw)),
                             .severity = static_cast<ErrorSeverity>(detail::ResultTraits::GetSeverityFromValue(raw))};
    }

    /// リトライ可能か判定
    [[nodiscard]] inline constexpr bool IsRetriable(::NS::Result result) noexcept
    {
        return GetErrorCategory(result).IsRetriable();
    }

    /// 致命的エラーか判定
    [[nodiscard]] inline constexpr bool IsFatal(::NS::Result result) noexcept
    {
        return GetErrorCategory(result).IsFatal();
    }

} // namespace NS::result
