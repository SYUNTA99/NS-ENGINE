/// @file RHIMacros.h
/// @brief RHI共通マクロ定義
/// @details DLLエクスポート、インライン制御、enum classビットフラグ演算子等を提供。
/// @see 01-01-fwd-macros.md
#pragma once

#include "Common/Utility/Macros.h"
#include <type_traits>

//=============================================================================
// DLLエクスポート/インポート
//=============================================================================

#ifdef RHI_EXPORTS
#define RHI_API NS_EXPORT
#else
#define RHI_API NS_IMPORT
#endif

//=============================================================================
// インライン制御
//=============================================================================

#define RHI_FORCEINLINE NS_FORCEINLINE
#define RHI_NOINLINE NS_NOINLINE

//=============================================================================
// デバッグ・検証
// チェックマクロ本体は RHICheck.h (17-01-rhi-result.md) で定義。
// 17-01 実装後にインクルードを有効化する。
//=============================================================================

#include "RHICheck.h" // RHI_CHECK, RHI_CHECKF, RHI_ENSURE, RHI_VERIFY

// GPU検証レイヤー用
#if NS_BUILD_DEBUG
#define RHI_GPU_VALIDATION 1
#else
#define RHI_GPU_VALIDATION 0
#endif

//=============================================================================
// enum class ビットフラグ演算子
//=============================================================================

/// @brief enum class にビット演算子を追加するマクロ
/// @param EnumType 対象のenum class型（基底型はuint32にキャスト）
/// @details |, &, ^, ~, |=, &=, ^= とヘルパー関数を生成する。
///          EnumHasAnyFlags(a, b): aにbのいずれかのフラグが含まれるか
///          EnumHasAllFlags(a, b): aにbの全てのフラグが含まれるか
#define RHI_ENUM_CLASS_FLAGS(EnumType)                                                                                 \
    inline constexpr EnumType operator|(EnumType a, EnumType b)                                                        \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return static_cast<EnumType>(static_cast<T>(a) | static_cast<T>(b));                                           \
    }                                                                                                                  \
    inline constexpr EnumType operator&(EnumType a, EnumType b)                                                        \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return static_cast<EnumType>(static_cast<T>(a) & static_cast<T>(b));                                           \
    }                                                                                                                  \
    inline constexpr EnumType operator^(EnumType a, EnumType b)                                                        \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return static_cast<EnumType>(static_cast<T>(a) ^ static_cast<T>(b));                                           \
    }                                                                                                                  \
    inline constexpr EnumType operator~(EnumType a)                                                                    \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return static_cast<EnumType>(static_cast<T>(~static_cast<T>(a)));                                              \
    }                                                                                                                  \
    inline EnumType& operator|=(EnumType& a, EnumType b)                                                               \
    {                                                                                                                  \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    inline EnumType& operator&=(EnumType& a, EnumType b)                                                               \
    {                                                                                                                  \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    inline EnumType& operator^=(EnumType& a, EnumType b)                                                               \
    {                                                                                                                  \
        return a = a ^ b;                                                                                              \
    }                                                                                                                  \
    inline constexpr bool EnumHasAnyFlags(EnumType a, EnumType b)                                                      \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return (static_cast<T>(a) & static_cast<T>(b)) != 0;                                                           \
    }                                                                                                                  \
    inline constexpr bool EnumHasAllFlags(EnumType a, EnumType b)                                                      \
    {                                                                                                                  \
        using T = std::underlying_type_t<EnumType>;                                                                    \
        return (static_cast<T>(a) & static_cast<T>(b)) == static_cast<T>(b);                                           \
    }
