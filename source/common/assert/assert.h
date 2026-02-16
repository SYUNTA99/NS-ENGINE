//----------------------------------------------------------------------------
//! @file   assert.h
//! @brief  汎用アサートマクロ（詳細ログ出力付き）
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_common.h"
#include "common/logging/logging.h"

namespace NS { namespace detail {

//----------------------------------------------------------------------------
//! @brief アサート失敗時のログ出力
//! @param condition 条件式の文字列表現
//! @param message ユーザーメッセージ
//! @param loc ソース位置情報
//----------------------------------------------------------------------------
inline void LogAssertFailure(
    const char* condition,
    const char* message,
    const std::source_location& loc)
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "NS_ASSERT FAILED: %s\n  Condition: %s\n  File: %s(%d)\n  Function: %s",
        message, condition,
        loc.file_name(), static_cast<int>(loc.line()),
        loc.function_name());
    LOG_ERROR(buffer);
}

//----------------------------------------------------------------------------
//! @brief フォーマット付きアサート失敗ログ
//! @param condition 条件式の文字列表現
//! @param formatted_message フォーマット済みメッセージ
//! @param loc ソース位置情報
//----------------------------------------------------------------------------
inline void LogAssertFailureFormatted(
    const char* condition,
    const std::string& formatted_message,
    const std::source_location& loc)
{
    char buffer[768];
    snprintf(buffer, sizeof(buffer),
        "NS_ASSERT FAILED: %s\n  Condition: %s\n  File: %s(%d)\n  Function: %s",
        formatted_message.c_str(), condition,
        loc.file_name(), static_cast<int>(loc.line()),
        loc.function_name());
    LOG_ERROR(buffer);
}

}} // namespace NS::detail

//============================================================================
// 汎用アサートマクロ
//============================================================================

#ifdef _DEBUG

//----------------------------------------------------------------------------
//! @brief 条件付きアサート（メッセージ付き）
//! @param condition 条件式
//! @param message エラーメッセージ
//!
//! @code
//! NS_ASSERT(ptr != nullptr, "Pointer must not be null");
//! @endcode
//----------------------------------------------------------------------------
#define NS_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            NS::detail::LogAssertFailure(#condition, message, std::source_location::current()); \
            assert(false && message); \
        } \
    } while(0)

//----------------------------------------------------------------------------
//! @brief nullptrチェック用アサート
//! @param ptr ポインタ
//! @param message エラーメッセージ
//!
//! @code
//! NS_ASSERT_NOT_NULL(texture, "Texture must be loaded");
//! @endcode
//----------------------------------------------------------------------------
#define NS_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == nullptr) { \
            NS::detail::LogAssertFailure(#ptr " != nullptr", message, std::source_location::current()); \
            assert(false && message); \
        } \
    } while(0)

//----------------------------------------------------------------------------
//! @brief フォーマット付きアサート
//! @param condition 条件式
//! @param fmt フォーマット文字列
//! @param ... フォーマット引数
//!
//! @code
//! NS_ASSERT_MSG(index < size, "Index {} out of range [0, {})", index, size);
//! @endcode
//----------------------------------------------------------------------------
#define NS_ASSERT_MSG(condition, fmt, ...) \
    do { \
        if (!(condition)) { \
            NS::detail::LogAssertFailureFormatted(#condition, \
                std::format(fmt, ##__VA_ARGS__), std::source_location::current()); \
            assert(false && "Assertion failed"); \
        } \
    } while(0)

//----------------------------------------------------------------------------
//! @brief 範囲チェック用アサート
//! @param value チェックする値
//! @param min 最小値（含む）
//! @param max 最大値（含まない）
//!
//! @code
//! NS_ASSERT_RANGE(index, 0, array.size());
//! @endcode
//----------------------------------------------------------------------------
#define NS_ASSERT_RANGE(value, min, max) \
    do { \
        auto _v = (value); auto _min = (min); auto _max = (max); \
        if (_v < _min || _v >= _max) { \
            NS::detail::LogAssertFailureFormatted(#value " in [" #min ", " #max ")", \
                std::format("Value {} out of range [{}, {})", _v, _min, _max), \
                std::source_location::current()); \
            assert(false && "Value out of range"); \
        } \
    } while(0)

#else
// Releaseビルドでは無効化
#define NS_ASSERT(condition, message) ((void)0)
#define NS_ASSERT_NOT_NULL(ptr, message) ((void)0)
#define NS_ASSERT_MSG(condition, fmt, ...) ((void)0)
#define NS_ASSERT_RANGE(value, min, max) ((void)0)
#endif
