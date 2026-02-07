/// @file Result.h
/// @brief 処理結果を表す型
#pragma once


#include "common/result/Core/ResultBase.h"
#include <type_traits>

namespace NS
{

    class Result;
    class ResultSuccess;

} // namespace NS

namespace NS::result::detail
{
    class InternalAccessor;
    [[noreturn]] void OnUnhandledResult(::NS::Result result) noexcept;
} // namespace NS::result::detail

namespace NS
{

    /// 処理結果を表す型
    /// 成功(0)または失敗(非0)を表す32bit値
    class Result : public result::detail::ResultBase<Result>
    {
        friend result::detail::InternalAccessor;

    public:
        Result() noexcept = default;

        /// 成功かどうか
        [[nodiscard]] constexpr bool IsSuccess() const noexcept { return m_value == kInnerSuccessValue; }

        /// 失敗かどうか
        [[nodiscard]] constexpr bool IsFailure() const noexcept { return !IsSuccess(); }

        /// デバッグ用内部値取得（Reserved領域をマスク）
        [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept
        {
            return result::detail::ResultTraits::MaskReservedFromValue(m_value);
        }

        /// 生の内部値取得
        [[nodiscard]] constexpr InnerType GetRawValue() const noexcept { return m_value; }

        using ResultBase::GetModule;
        using ResultBase::GetDescription;
        using ResultBase::GetReserved;

        /// ResultSuccessへの変換（失敗時はアボート）
        operator ResultSuccess() const noexcept;

        /// 任意のResultを受け入れ可能
        [[nodiscard]] static constexpr bool CanAccept(Result) noexcept { return true; }

        /// 比較演算子
        [[nodiscard]] constexpr bool operator==(Result other) const noexcept
        {
            return GetInnerValueForDebug() == other.GetInnerValueForDebug();
        }

        [[nodiscard]] constexpr bool operator!=(Result other) const noexcept { return !(*this == other); }

    private:
        InnerType m_value = kInnerSuccessValue;

        explicit constexpr Result(InnerType value) noexcept : m_value(value) {}
    };

    static_assert(std::is_trivially_copyable_v<Result>);
    static_assert(sizeof(Result) == sizeof(Result::InnerType));

    /// 成功を表す型
    class ResultSuccess : public result::detail::ResultBase<ResultSuccess>
    {
    public:
        /// Resultへの変換
        [[nodiscard]] constexpr operator Result() const noexcept;

        /// 常に成功
        [[nodiscard]] constexpr bool IsSuccess() const noexcept { return true; }

        /// デバッグ用内部値
        [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept { return kInnerSuccessValue; }

        /// 成功のResultのみ受け入れ可能
        [[nodiscard]] static constexpr bool CanAccept(Result result) noexcept { return result.IsSuccess(); }
    };

    static_assert(std::is_trivially_copyable_v<ResultSuccess>);

} // namespace NS
