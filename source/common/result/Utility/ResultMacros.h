/// @file ResultMacros.h
/// @brief Result操作用ヘルパーマクロ
#pragma once

#include "common/result/Core/Result.h"

//=============================================================================
// 基本マクロ
//=============================================================================

/// 失敗時に早期リターン
#define NS_RETURN_IF_FAILED(expr)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure())                                                        \
        {                                                                                                              \
            return _ns_result_;                                                                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// 成功時に早期リターン
#define NS_RETURN_IF_SUCCESS(expr)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsSuccess())                                                        \
        {                                                                                                              \
            return _ns_result_;                                                                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// 失敗時にgotoでジャンプ
#define NS_GOTO_IF_FAILED(expr, label)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure())                                                        \
        {                                                                                                              \
            goto label;                                                                                                \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

//=============================================================================
// 条件付きリターンマクロ
//=============================================================================

/// 条件が真の場合に指定したResultを返す
#define NS_RETURN_IF(cond, result)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (cond)                                                                                                      \
        {                                                                                                              \
            return (result);                                                                                           \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// nullptrの場合にResultを返す
#define NS_RETURN_IF_NULL(ptr, result) NS_RETURN_IF((ptr) == nullptr, result)

/// 範囲外の場合にResultを返す
#define NS_RETURN_IF_OUT_OF_RANGE(val, min, max, result) NS_RETURN_IF((val) < (min) || (val) >= (max), result)

//=============================================================================
// 変換マクロ
//=============================================================================

/// HRESULTからResultへ変換
#define NS_FROM_HRESULT(hr, successResult, failResult) (SUCCEEDED(hr) ? (successResult) : (failResult))

/// Win32エラーコードからResultへ変換
#define NS_FROM_WIN32(err, successResult, failResult) ((err) == ERROR_SUCCESS ? (successResult) : (failResult))

/// boolからResultへ変換
#define NS_FROM_BOOL(b, successResult, failResult) ((b) ? (successResult) : (failResult))

/// nullptrチェック付きResult変換
#define NS_FROM_POINTER(ptr, successResult, failResult) ((ptr) != nullptr ? (successResult) : (failResult))

//=============================================================================
// デバッグ用マクロ
//=============================================================================

#ifdef _DEBUG

/// デバッグビルドでのみResultを検証（成功を期待）
#define NS_DEBUG_EXPECT_SUCCESS(expr)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure())                                                        \
        {                                                                                                              \
            std::fprintf(stderr, "[DEBUG] Expected success but got failure: %s\n", #expr);                             \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// デバッグビルドでのみResultを検証（失敗を期待）
#define NS_DEBUG_EXPECT_FAILED(expr)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _ns_result_ = (expr); _ns_result_.IsSuccess())                                                        \
        {                                                                                                              \
            std::fprintf(stderr, "[DEBUG] Expected failure but got success: %s\n", #expr);                             \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

#else

#define NS_DEBUG_EXPECT_SUCCESS(expr) ((void)(expr))
#define NS_DEBUG_EXPECT_FAILED(expr) ((void)(expr))

#endif // _DEBUG

//=============================================================================
// 未使用警告抑制
//=============================================================================

/// Resultを明示的に無視（警告抑制用）
#define NS_IGNORE_RESULT(expr)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        [[maybe_unused]] auto _ns_ignored_ = (expr);                                                                   \
    }                                                                                                                  \
    while (false)
