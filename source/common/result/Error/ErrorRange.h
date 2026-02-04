/// @file ErrorRange.h
/// @brief エラー範囲を表すテンプレート（階層的マッチング用）
#pragma once

#include "common/result/Core/Result.h"

namespace NS::result::detail
{

    /// エラー範囲を表すテンプレート
    /// @tparam Module_ モジュールID
    /// @tparam DescriptionBegin_ 範囲開始（含む）
    /// @tparam DescriptionEnd_ 範囲終了（含まない）
    template <int Module_, int DescriptionBegin_, int DescriptionEnd_> class ErrorRange
    {
    public:
        static constexpr int kModule = Module_;
        static constexpr int kDescriptionBegin = DescriptionBegin_;
        static constexpr int kDescriptionEnd = DescriptionEnd_;

        static_assert(DescriptionBegin_ < DescriptionEnd_, "Invalid range: Begin must be < End");
        static_assert(ResultTraits::IsValidDescription(DescriptionBegin_), "DescriptionBegin out of range");
        static_assert(ResultTraits::IsValidDescription(DescriptionEnd_ - 1), "DescriptionEnd out of range");

        /// この範囲内かどうかを判定
        [[nodiscard]] static constexpr bool Includes(::NS::Result result) noexcept
        {
            if (result.GetModule() != kModule)
            {
                return false;
            }
            const int desc = result.GetDescription();
            return kDescriptionBegin <= desc && desc < kDescriptionEnd;
        }

        /// Result型との比較（範囲マッチング）
        [[nodiscard]] friend constexpr bool operator==(::NS::Result lhs, ErrorRange) noexcept { return Includes(lhs); }

        [[nodiscard]] friend constexpr bool operator==(ErrorRange, ::NS::Result rhs) noexcept { return Includes(rhs); }

        [[nodiscard]] friend constexpr bool operator!=(::NS::Result lhs, ErrorRange rhs) noexcept
        {
            return !(lhs == rhs);
        }

        [[nodiscard]] friend constexpr bool operator!=(ErrorRange lhs, ::NS::Result rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

} // namespace NS::result::detail
