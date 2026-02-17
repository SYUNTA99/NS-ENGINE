/// @file macros.h
/// @brief 統一マクロ群
/// @details コンパイラ差異を吸収し、コード品質を統一するためのマクロ群
#pragma once

#include <cstddef>

// =============================================================================
// コンパイラ検出
// =============================================================================

#if defined(_MSC_VER)
#define NS_COMPILER_MSVC 1
#define NS_COMPILER_GCC 0
#define NS_COMPILER_CLANG 0
#elif defined(__clang__)
#define NS_COMPILER_MSVC 0
#define NS_COMPILER_GCC 0
#define NS_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define NS_COMPILER_MSVC 0
#define NS_COMPILER_GCC 1
#define NS_COMPILER_CLANG 0
#else
#define NS_COMPILER_MSVC 0
#define NS_COMPILER_GCC 0
#define NS_COMPILER_CLANG 0
#endif

// =============================================================================
// 1. 言語サポートマクロ
// =============================================================================

/// 定数初期化
#if __cplusplus >= 202002L
#define NS_CONSTINIT constinit
#else
#define NS_CONSTINIT
#endif

// =============================================================================
// 2. インライン制御マクロ
// =============================================================================

/// インライン化禁止
#if NS_COMPILER_MSVC
#define NS_NOINLINE __declspec(noinline)
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_NOINLINE __attribute__((noinline))
#else
#define NS_NOINLINE
#endif

/// インライン化強制
#if NS_COMPILER_MSVC
#define NS_FORCEINLINE __forceinline
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_FORCEINLINE inline __attribute__((always_inline))
#else
#define NS_FORCEINLINE inline
#endif

// =============================================================================
// 3. 警告抑制マクロ
// =============================================================================

/// 非推奨マーク
#if NS_COMPILER_MSVC
#define NS_DEPRECATED(msg) __declspec(deprecated(msg))
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define NS_DEPRECATED(msg)
#endif

// =============================================================================
// 4. メモリレイアウト制御
// =============================================================================

/// パディング抑止（構造体定義に付加）
#if NS_COMPILER_MSVC
#define NS_PACK
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_PACK __attribute__((packed))
#else
#define NS_PACK
#endif

/// パディング抑止開始（MSVC用）
#if NS_COMPILER_MSVC
#define NS_PACK_BEGIN __pragma(pack(push, 1))
#else
#define NS_PACK_BEGIN
#endif

/// パディング抑止終了（MSVC用）
#if NS_COMPILER_MSVC
#define NS_PACK_END __pragma(pack(pop))
#else
#define NS_PACK_END
#endif

/// 明示的パディング挿入
#define NS_PADDING1 char _padding1_[1]
#define NS_PADDING2 char _padding2_[2]
#define NS_PADDING3 char _padding3_[3]
#define NS_PADDING4 char _padding4_[4]
#define NS_PADDING5 char _padding5_[5]
#define NS_PADDING6 char _padding6_[6]
#define NS_PADDING7 char _padding7_[7]
#define NS_PADDING8 char _padding8_[8]

/// 名前付きパディング（複数使用時の衝突回避）
#define NS_PADDING(name, size) char name[size]

// =============================================================================
// 5. ユーティリティマクロ
// =============================================================================

/// コピー禁止
/// @note 使用後は private スコープになる
#define NS_DISALLOW_COPY(TypeName)                                                                                     \
private:                                                                                                               \
    TypeName(const TypeName&) = delete;                                                                                \
    TypeName& operator=(const TypeName&) = delete

/// ムーブ禁止
/// @note 使用後は private スコープになる
#define NS_DISALLOW_MOVE(TypeName)                                                                                     \
private:                                                                                                               \
    TypeName(TypeName&&) = delete;                                                                                     \
    TypeName& operator=(TypeName&&) = delete

/// コピー・ムーブ両方禁止
/// @note 使用後は private スコープになる
#define NS_DISALLOW_COPY_AND_MOVE(TypeName)                                                                            \
    NS_DISALLOW_COPY(TypeName);                                                                                        \
    NS_DISALLOW_MOVE(TypeName)

/// 未使用変数の警告抑制
#define NS_UNUSED(x) (void)(x)

/// ビットサイズ取得
#define NS_BITSIZEOF(T) (sizeof(T) * 8)

/// 配列要素数取得
template <typename T, std::size_t N> constexpr std::size_t NsArraySizeImpl(const T (&)[N]) noexcept
{
    return N;
}
#define NS_ARRAY_SIZE(arr) NsArraySizeImpl(arr)

/// 文字列化
#define NS_MACRO_STRINGIZE_IMPL(x) #x
#define NS_MACRO_STRINGIZE(x) NS_MACRO_STRINGIZE_IMPL(x)

/// トークン連結
#define NS_MACRO_CONCATENATE_IMPL(a, b) a##b
#define NS_MACRO_CONCATENATE(a, b) NS_MACRO_CONCATENATE_IMPL(a, b)

/// 到達不能 default
#if NS_COMPILER_MSVC
#define NS_UNEXPECTED_DEFAULT                                                                                          \
    default:                                                                                                           \
        __assume(0)
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_UNEXPECTED_DEFAULT                                                                                          \
    default:                                                                                                           \
        __builtin_unreachable()
#else
#define NS_UNEXPECTED_DEFAULT                                                                                          \
    default:                                                                                                           \
        break
#endif

/// 到達不能コード
#if NS_COMPILER_MSVC
#define NS_UNREACHABLE() __assume(0)
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_UNREACHABLE() __builtin_unreachable()
#else
#define NS_UNREACHABLE() ((void)0)
#endif

// =============================================================================
// 6. 警告プラグマ制御
// =============================================================================

#if NS_COMPILER_MSVC

#define NS_PRAGMA_PUSH_WARNINGS __pragma(warning(push))
#define NS_PRAGMA_POP_WARNINGS __pragma(warning(pop))

#define NS_DISABLE_WARNING_OVERFLOW __pragma(warning(disable : 4307))
#define NS_DISABLE_WARNING_LOCAL_STATIC __pragma(warning(disable : 4640))
#define NS_DISABLE_WARNING_UNUSED_PARAMETER __pragma(warning(disable : 4100))
#define NS_DISABLE_WARNING_UNUSED_VAR __pragma(warning(disable : 4189))
#define NS_DISABLE_WARNING_UNUSED_FUNCTION __pragma(warning(disable : 4505))
#define NS_DISABLE_WARNING_SHADOW __pragma(warning(disable : 4456 4457 4458 4459))
#define NS_DISABLE_WARNING_DEPRECATED_DECLARATIONS __pragma(warning(disable : 4996))
#define NS_DISABLE_WARNING_UNINITIALIZED __pragma(warning(disable : 4701 4703))
#define NS_DISABLE_WARNING_CONVERSION __pragma(warning(disable : 4244 4267))
#define NS_DISABLE_WARNING_SIGN_COMPARE __pragma(warning(disable : 4018 4389))
#define NS_DISABLE_WARNING_UNREACHABLE_CODE __pragma(warning(disable : 4702))
#define NS_DISABLE_WARNING_SWITCH __pragma(warning(disable : 4061 4062))

#elif NS_COMPILER_CLANG

#define NS_PRAGMA_PUSH_WARNINGS _Pragma("clang diagnostic push")
#define NS_PRAGMA_POP_WARNINGS _Pragma("clang diagnostic pop")

#define NS_DISABLE_WARNING_OVERFLOW _Pragma("clang diagnostic ignored \"-Winteger-overflow\"")
#define NS_DISABLE_WARNING_LOCAL_STATIC
#define NS_DISABLE_WARNING_UNUSED_PARAMETER _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")
#define NS_DISABLE_WARNING_UNUSED_VAR _Pragma("clang diagnostic ignored \"-Wunused-variable\"")
#define NS_DISABLE_WARNING_UNUSED_FUNCTION _Pragma("clang diagnostic ignored \"-Wunused-function\"")
#define NS_DISABLE_WARNING_SHADOW _Pragma("clang diagnostic ignored \"-Wshadow\"")
#define NS_DISABLE_WARNING_DEPRECATED_DECLARATIONS _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define NS_DISABLE_WARNING_UNINITIALIZED _Pragma("clang diagnostic ignored \"-Wuninitialized\"")
#define NS_DISABLE_WARNING_CONVERSION _Pragma("clang diagnostic ignored \"-Wconversion\"")
#define NS_DISABLE_WARNING_SIGN_COMPARE _Pragma("clang diagnostic ignored \"-Wsign-compare\"")
#define NS_DISABLE_WARNING_UNREACHABLE_CODE _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
#define NS_DISABLE_WARNING_SWITCH _Pragma("clang diagnostic ignored \"-Wswitch\"")

#elif NS_COMPILER_GCC

#define NS_PRAGMA_PUSH_WARNINGS _Pragma("GCC diagnostic push")
#define NS_PRAGMA_POP_WARNINGS _Pragma("GCC diagnostic pop")

#define NS_DISABLE_WARNING_OVERFLOW _Pragma("GCC diagnostic ignored \"-Woverflow\"")
#define NS_DISABLE_WARNING_LOCAL_STATIC
#define NS_DISABLE_WARNING_UNUSED_PARAMETER _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
#define NS_DISABLE_WARNING_UNUSED_VAR _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")
#define NS_DISABLE_WARNING_UNUSED_FUNCTION _Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#define NS_DISABLE_WARNING_SHADOW _Pragma("GCC diagnostic ignored \"-Wshadow\"")
#define NS_DISABLE_WARNING_DEPRECATED_DECLARATIONS _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define NS_DISABLE_WARNING_UNINITIALIZED _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")
#define NS_DISABLE_WARNING_CONVERSION _Pragma("GCC diagnostic ignored \"-Wconversion\"")
#define NS_DISABLE_WARNING_SIGN_COMPARE _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
#define NS_DISABLE_WARNING_UNREACHABLE_CODE _Pragma("GCC diagnostic ignored \"-Wunreachable-code\"")
#define NS_DISABLE_WARNING_SWITCH _Pragma("GCC diagnostic ignored \"-Wswitch\"")

#else

#define NS_PRAGMA_PUSH_WARNINGS
#define NS_PRAGMA_POP_WARNINGS
#define NS_DISABLE_WARNING_OVERFLOW
#define NS_DISABLE_WARNING_LOCAL_STATIC
#define NS_DISABLE_WARNING_UNUSED_PARAMETER
#define NS_DISABLE_WARNING_UNUSED_VAR
#define NS_DISABLE_WARNING_UNUSED_FUNCTION
#define NS_DISABLE_WARNING_SHADOW
#define NS_DISABLE_WARNING_DEPRECATED_DECLARATIONS
#define NS_DISABLE_WARNING_UNINITIALIZED
#define NS_DISABLE_WARNING_CONVERSION
#define NS_DISABLE_WARNING_SIGN_COMPARE
#define NS_DISABLE_WARNING_UNREACHABLE_CODE
#define NS_DISABLE_WARNING_SWITCH

#endif

// =============================================================================
// 7. プラットフォーム検出
// =============================================================================

#if defined(_WIN32) || defined(_WIN64)
#define NS_PLATFORM_WINDOWS 1
#else
#define NS_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
#define NS_PLATFORM_LINUX 1
#else
#define NS_PLATFORM_LINUX 0
#endif

#if defined(__APPLE__)
#define NS_PLATFORM_APPLE 1
#else
#define NS_PLATFORM_APPLE 0
#endif

// =============================================================================
// 8. アーキテクチャ検出
// =============================================================================

#if defined(_M_X64) || defined(__x86_64__)
#define NS_ARCH_X64 1
#define NS_ARCH_X86 0
#define NS_ARCH_ARM64 0
#elif defined(_M_IX86) || defined(__i386__)
#define NS_ARCH_X64 0
#define NS_ARCH_X86 1
#define NS_ARCH_ARM64 0
#elif defined(_M_ARM64) || defined(__aarch64__)
#define NS_ARCH_X64 0
#define NS_ARCH_X86 0
#define NS_ARCH_ARM64 1
#else
#define NS_ARCH_X64 0
#define NS_ARCH_X86 0
#define NS_ARCH_ARM64 0
#endif

// ポインタサイズ
#if NS_ARCH_X64 || NS_ARCH_ARM64
#define NS_POINTER_SIZE 8
#else
#define NS_POINTER_SIZE 4
#endif

// =============================================================================
// 9. ビルド構成検出
// =============================================================================

#if defined(_DEBUG) || defined(DEBUG)
#define NS_BUILD_DEBUG 1
#else
#define NS_BUILD_DEBUG 0
#endif

#if defined(NDEBUG)
#define NS_BUILD_RELEASE 1
#else
#define NS_BUILD_RELEASE 0
#endif

#if defined(NS_SHIPPING)
#define NS_BUILD_SHIPPING 1
#else
#define NS_BUILD_SHIPPING 0
#endif

// =============================================================================
// 10. DLL エクスポート/インポート
// =============================================================================

#if NS_PLATFORM_WINDOWS
#define NS_EXPORT __declspec(dllexport)
#define NS_IMPORT __declspec(dllimport)
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_EXPORT __attribute__((visibility("default")))
#define NS_IMPORT
#else
#define NS_EXPORT
#define NS_IMPORT
#endif

// =============================================================================
// 11. 呼び出し規約
// =============================================================================

#if NS_PLATFORM_WINDOWS && NS_ARCH_X86
#define NS_CDECL __cdecl
#define NS_STDCALL __stdcall
#define NS_FASTCALL __fastcall
#else
#define NS_CDECL
#define NS_STDCALL
#define NS_FASTCALL
#endif

// =============================================================================
// 12. 制限付きポインタ
// =============================================================================

#if NS_COMPILER_MSVC
#define NS_RESTRICT __restrict
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
#define NS_RESTRICT __restrict__
#else
#define NS_RESTRICT
#endif
