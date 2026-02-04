/// @file InternalAccessor.h
/// @brief Result型の内部アクセス用クラス
#pragma once

#include "common/result/Core/Result.h"

namespace NS::result::detail
{

    /// 内部アクセス用クラス
    /// Resultのprivateコンストラクタにアクセスするためのfriendクラス
    class InternalAccessor
    {
    public:
        /// Result値を構築
        [[nodiscard]] static constexpr ::NS::Result ConstructResult(ResultTraits::InnerType value) noexcept
        {
            return ::NS::Result(value);
        }

        /// 内部値を取得
        [[nodiscard]] static constexpr ResultTraits::InnerType GetInnerValue(::NS::Result result) noexcept
        {
            return result.m_value;
        }

        /// Reserved領域を設定
        [[nodiscard]] static constexpr ::NS::Result SetReserved(::NS::Result result, int reserved) noexcept
        {
            return ::NS::Result(ResultTraits::SetReserved(result.m_value, reserved));
        }

        /// Persistence/Severityを設定
        [[nodiscard]] static constexpr ::NS::Result SetCategory(::NS::Result result,
                                                                int persistence,
                                                                int severity) noexcept
        {
            return ::NS::Result(ResultTraits::SetCategory(result.m_value, persistence, severity));
        }
    };

    /// Result構築用ヘルパー関数
    [[nodiscard]] inline constexpr ::NS::Result ConstructResult(ResultTraits::InnerType value) noexcept
    {
        return InternalAccessor::ConstructResult(value);
    }

} // namespace NS::result::detail

// ResultSuccess::operator Result の実装
inline constexpr NS::ResultSuccess::operator NS::Result() const noexcept
{
    return result::detail::ConstructResult(kInnerSuccessValue);
}

// Result::operator ResultSuccess の実装
inline NS::Result::operator NS::ResultSuccess() const noexcept
{
    if (!NS::ResultSuccess::CanAccept(*this))
    {
        result::detail::OnUnhandledResult(*this);
    }
    return NS::ResultSuccess();
}
