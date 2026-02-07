/// @file ErrorResultBase.h
/// @brief 特定エラーを表すコンパイル時型テンプレート
#pragma once


#include "common/result/Core/InternalAccessor.h"
#include "common/result/Core/ResultBase.h"

namespace NS::result::detail
{

    /// 特定エラーを表すコンパイル時型
    /// @tparam Module_ モジュールID
    /// @tparam Description_ エラー詳細ID
    /// @tparam Persistence_ 時間的性質 (0=Unknown, 1=Transient, 2=Permanent)
    /// @tparam Severity_ 重大度 (0=Unknown, 1=Recoverable, 2=Fatal)
    template <int Module_, int Description_, int Persistence_ = 0, int Severity_ = 0>
    class ErrorResultBase : public ResultBase<ErrorResultBase<Module_, Description_, Persistence_, Severity_>>
    {
    public:
        using InnerType = ResultTraits::InnerType;

        static constexpr int kModule = Module_;
        static constexpr int kDescription = Description_;
        static constexpr int kPersistence = Persistence_;
        static constexpr int kSeverity = Severity_;

        // カテゴリ情報を含む完全な内部値
        static constexpr InnerType kInnerValueWithCategory = ResultTraits::SetCategory(
            ResultTraits::MakeInnerValueStatic<Module_, Description_>::value, Persistence_, Severity_);

        // 比較用（カテゴリを除外した値）
        static constexpr InnerType kInnerValue = ResultTraits::MakeInnerValueStatic<Module_, Description_>::value;

        constexpr ErrorResultBase() noexcept = default;

        /// Resultへの暗黙変換
        [[nodiscard]] constexpr operator ::NS::Result() const noexcept
        {
            return ConstructResult(kInnerValueWithCategory);
        }

        /// デバッグ用内部値（カテゴリ除外）
        [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept { return kInnerValue; }

        /// このエラー型を受け入れ可能か（カテゴリを無視して比較）
        [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept
        {
            return result.GetInnerValueForDebug() == kInnerValue;
        }

        /// Result型との比較
        [[nodiscard]] friend constexpr bool operator==(::NS::Result lhs, ErrorResultBase) noexcept
        {
            return CanAccept(lhs);
        }

        [[nodiscard]] friend constexpr bool operator==(ErrorResultBase, ::NS::Result rhs) noexcept
        {
            return CanAccept(rhs);
        }

        [[nodiscard]] friend constexpr bool operator!=(::NS::Result lhs, ErrorResultBase rhs) noexcept
        {
            return !(lhs == rhs);
        }

        [[nodiscard]] friend constexpr bool operator!=(ErrorResultBase lhs, ::NS::Result rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

} // namespace NS::result::detail
