/// @file ResultBase.h
/// @brief Result型のCRTP基底クラス
#pragma once

#include "common/result/Core/ResultTraits.h"

namespace NS::result::detail
{

    /// CRTP基底クラス - GetModule/GetDescriptionを共有
    template <typename Self> class ResultBase
    {
    public:
        using InnerType = ResultTraits::InnerType;
        static constexpr InnerType kInnerSuccessValue = ResultTraits::kInnerSuccessValue;

        [[nodiscard]] constexpr int GetModule() const noexcept
        {
            return ResultTraits::GetModuleFromValue(static_cast<const Self&>(*this).GetInnerValueForDebug());
        }

        [[nodiscard]] constexpr int GetDescription() const noexcept
        {
            return ResultTraits::GetDescriptionFromValue(static_cast<const Self&>(*this).GetInnerValueForDebug());
        }

        [[nodiscard]] constexpr int GetReserved() const noexcept
        {
            return ResultTraits::GetReservedFromValue(static_cast<const Self&>(*this).GetInnerValueForDebug());
        }

    protected:
        ResultBase() noexcept = default;
        ~ResultBase() noexcept = default;
        ResultBase(const ResultBase&) noexcept = default;
        ResultBase& operator=(const ResultBase&) noexcept = default;
    };

} // namespace NS::result::detail
