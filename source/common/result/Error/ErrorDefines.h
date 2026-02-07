/// @file ErrorDefines.h
/// @brief エラー定義用マクロ
#pragma once


#include "common/result/Error/ErrorCategory.h"
#include "common/result/Error/ErrorRange.h"
#include "common/result/Error/ErrorResultBase.h"

// カテゴリ定数（マクロ引数用）
#define NS_PERSISTENCE_UNKNOWN 0
#define NS_PERSISTENCE_TRANSIENT 1
#define NS_PERSISTENCE_PERMANENT 2

#define NS_SEVERITY_UNKNOWN 0
#define NS_SEVERITY_RECOVERABLE 1
#define NS_SEVERITY_FATAL 2

/// 単一エラー定義（カテゴリなし）
#define NS_DEFINE_ERROR_RESULT(name, module, description)                                                              \
    class name : public ::NS::result::detail::ErrorResultBase<module, description, 0, 0>                               \
    {}

/// 単一エラー定義（カテゴリ付き）
#define NS_DEFINE_ERROR_RESULT_CAT(name, module, description, persistence, severity)                                   \
    class name : public ::NS::result::detail::ErrorResultBase<module, description, persistence, severity>              \
    {}

/// 範囲付きエラー定義（カテゴリなし）
#define NS_DEFINE_ERROR_RANGE_RESULT(name, module, descriptionBegin, descriptionEnd)                                   \
    class name : public ::NS::result::detail::ErrorResultBase<module, descriptionBegin, 0, 0>,                         \
                 public ::NS::result::detail::ErrorRange<module, descriptionBegin, descriptionEnd>                     \
    {                                                                                                                  \
    public:                                                                                                            \
        using RangeType = ::NS::result::detail::ErrorRange<module, descriptionBegin, descriptionEnd>;                  \
        using RangeType::Includes;                                                                                     \
        [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept                                    \
        {                                                                                                              \
            return RangeType::Includes(result);                                                                        \
        }                                                                                                              \
    }

/// 範囲付きエラー定義（カテゴリ付き）
#define NS_DEFINE_ERROR_RANGE_RESULT_CAT(name, module, descriptionBegin, descriptionEnd, persistence, severity)        \
    class name : public ::NS::result::detail::ErrorResultBase<module, descriptionBegin, persistence, severity>,        \
                 public ::NS::result::detail::ErrorRange<module, descriptionBegin, descriptionEnd>                     \
    {                                                                                                                  \
    public:                                                                                                            \
        using RangeType = ::NS::result::detail::ErrorRange<module, descriptionBegin, descriptionEnd>;                  \
        using RangeType::Includes;                                                                                     \
        [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept                                    \
        {                                                                                                              \
            return RangeType::Includes(result);                                                                        \
        }                                                                                                              \
    }

/// 抽象エラー範囲（具体的な値を持たない、範囲マッチングのみ）
#define NS_DEFINE_ABSTRACT_ERROR_RANGE(name, module, descriptionBegin, descriptionEnd)                                 \
    class name : public ::NS::result::detail::ErrorRange<module, descriptionBegin, descriptionEnd>                     \
    {}
