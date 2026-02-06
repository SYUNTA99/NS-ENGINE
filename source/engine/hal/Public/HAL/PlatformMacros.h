/// @file PlatformMacros.h
/// @brief HAL関数属性・呼び出し規約マクロ
/// @details commonモジュールのマクロをHAL用にエイリアス定義し、
///          追加の関数属性マクロを提供する。
#pragma once

#include "HAL/Platform.h"
// common/utility/macros.h は Platform.h 経由でインクルード済み

// =============================================================================
// 呼び出し規約マクロ（commonからのエイリアス）
// windows.hで定義済みの場合はスキップ
// =============================================================================

/// 可変引数呼び出し規約（CDECLと同じ）
#ifndef VARARGS
#define VARARGS NS_CDECL
#endif

/// C言語デフォルト呼び出し規約
#ifndef CDECL
#define CDECL NS_CDECL
#endif

/// Windows標準呼び出し規約
#ifndef STDCALL
#define STDCALL NS_STDCALL
#endif

// =============================================================================
// インライン制御マクロ（commonからのエイリアス）
// windows.hで定義済みの場合はスキップ
// =============================================================================

/// インライン化強制
#ifndef FORCEINLINE
#define FORCEINLINE NS_FORCEINLINE
#endif

/// インライン化禁止
#ifndef FORCENOINLINE
#define FORCENOINLINE NS_NOINLINE
#endif

// =============================================================================
// DLLエクスポート/インポートマクロ（commonからのエイリアス）
// =============================================================================

/// DLLエクスポート
#define DLLEXPORT NS_EXPORT

/// DLLインポート
#define DLLIMPORT NS_IMPORT

// =============================================================================
// ポインタ修飾マクロ（commonからのエイリアス）
// =============================================================================

/// 制限付きポインタ（エイリアシングなし保証）
#define RESTRICT NS_RESTRICT

// =============================================================================
// HAL固有の追加マクロ
// =============================================================================

/// 空の基底クラス最適化（EBO）
///
/// 空のメンバにサイズを消費しない。
/// @code
/// struct Empty {};
/// struct Holder { NS_NO_UNIQUE_ADDRESS Empty e; int value; }; // sizeof == 4
/// @endcode
#if PLATFORM_COMPILER_MSVC
#define NS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

/// コールドパス関数（頻繁に実行されない）
///
/// エラーハンドリング等、通常実行されない関数に使用。
/// 分岐予測とコードレイアウトを最適化。
#if PLATFORM_COMPILER_MSVC
#define NS_COLD FORCENOINLINE
#elif PLATFORM_COMPILER_GCC || PLATFORM_COMPILER_CLANG
#define NS_COLD __attribute__((cold))
#else
#define NS_COLD
#endif

/// ホットパス関数（頻繁に実行される）
///
/// パフォーマンスクリティカルな関数に使用。
#if PLATFORM_COMPILER_GCC || PLATFORM_COMPILER_CLANG
#define NS_HOT __attribute__((hot))
#else
#define NS_HOT
#endif

/// 純粋関数（副作用なし、同じ入力で同じ出力）
#if PLATFORM_COMPILER_GCC || PLATFORM_COMPILER_CLANG
#define NS_PURE __attribute__((pure))
#else
#define NS_PURE
#endif

/// 定数関数（グローバル状態を読まない純粋関数）
#if PLATFORM_COMPILER_GCC || PLATFORM_COMPILER_CLANG
#define NS_CONST __attribute__((const))
#else
#define NS_CONST
#endif

/// printf形式チェック
/// @param fmtIdx printfフォーマット文字列の引数番号（1始まり）
/// @param argIdx 可変引数開始の引数番号（1始まり）
#if PLATFORM_COMPILER_GCC || PLATFORM_COMPILER_CLANG
#define NS_PRINTF_FORMAT(fmtIdx, argIdx) __attribute__((format(printf, fmtIdx, argIdx)))
#else
#define NS_PRINTF_FORMAT(fmtIdx, argIdx)
#endif

// =============================================================================
// アサート・チェックマクロ
// =============================================================================

/// 軽量チェック（常に有効）
#ifndef NS_CHECK
#define NS_CHECK(expr)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            NS_UNREACHABLE();                                                                                          \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)
#endif

/// チェック + メッセージ
#ifndef NS_CHECK_MSG
#define NS_CHECK_MSG(expr, msg)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            NS_UNREACHABLE();                                                                                          \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)
#endif

/// 低速チェック（Debugビルドのみ）
#if NS_DEBUG
#define NS_CHECK_SLOW(expr) NS_CHECK(expr)
#else
#define NS_CHECK_SLOW(expr)                                                                                            \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#endif

// =============================================================================
// HALモジュールエクスポートマクロ
// =============================================================================

/// HALモジュールのエクスポート/インポート
#ifdef HAL_EXPORTS
#define HAL_API DLLEXPORT
#else
#define HAL_API DLLIMPORT
#endif
