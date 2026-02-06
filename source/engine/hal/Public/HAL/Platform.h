/// @file Platform.h
/// @brief HALプラットフォーム識別マクロ
/// @details commonモジュールのマクロをHAL用にエイリアス定義し、
///          プラットフォームグループマクロを追加する。
#pragma once

// =============================================================================
// common モジュールの基盤マクロを使用
// =============================================================================
#include "common/utility/macros.h"

// =============================================================================
// プラットフォーム識別マクロ（commonからのエイリアス）
// =============================================================================

/// Windows プラットフォーム
#define PLATFORM_WINDOWS NS_PLATFORM_WINDOWS

/// macOS / iOS プラットフォーム
#define PLATFORM_MAC NS_PLATFORM_APPLE
#define PLATFORM_APPLE NS_PLATFORM_APPLE

/// Linux プラットフォーム
#define PLATFORM_LINUX NS_PLATFORM_LINUX

// =============================================================================
// プラットフォームグループマクロ（HAL固有）
// =============================================================================

/// デスクトッププラットフォーム（Windows, Mac, Linux）
#define PLATFORM_DESKTOP (PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)

/// Unix系プラットフォーム（Mac, Linux）
#define PLATFORM_UNIX (PLATFORM_MAC || PLATFORM_LINUX)

// =============================================================================
// アーキテクチャマクロ
// =============================================================================

/// 64ビットプラットフォーム
#define PLATFORM_64BITS (NS_POINTER_SIZE == 8)

/// 32ビットプラットフォーム
#define PLATFORM_32BITS (NS_POINTER_SIZE == 4)

/// x86ファミリー（x64またはx86）
#define PLATFORM_CPU_X86_FAMILY (NS_ARCH_X64 || NS_ARCH_X86)

/// ARMファミリー（ARM64）
#define PLATFORM_CPU_ARM_FAMILY NS_ARCH_ARM64

// =============================================================================
// エンディアン
// =============================================================================

/// リトルエンディアン（x86/x64/ARM64はすべてリトルエンディアン）
#define PLATFORM_LITTLE_ENDIAN 1

/// ビッグエンディアン（現在サポートなし）
#define PLATFORM_BIG_ENDIAN 0

// =============================================================================
// コンパイラ識別マクロ（commonからのエイリアス）
// =============================================================================

/// MSVCコンパイラ
#define PLATFORM_COMPILER_MSVC NS_COMPILER_MSVC

/// GCCコンパイラ
#define PLATFORM_COMPILER_GCC NS_COMPILER_GCC

/// Clangコンパイラ
#define PLATFORM_COMPILER_CLANG NS_COMPILER_CLANG

// =============================================================================
// ビルド構成マクロ
// =============================================================================

/// Debugビルド
#ifndef NS_DEBUG
#if NS_BUILD_DEBUG
#define NS_DEBUG 1
#else
#define NS_DEBUG 0
#endif
#endif

/// Developmentビルド（Debugと同等、将来的に分離）
#ifndef NS_DEVELOPMENT
#define NS_DEVELOPMENT NS_DEBUG
#endif

/// Releaseビルド
#ifndef NS_RELEASE
#if NS_BUILD_RELEASE
#define NS_RELEASE 1
#else
#define NS_RELEASE 0
#endif
#endif

// =============================================================================
// プラットフォーム検証
// =============================================================================

// 少なくとも1つのプラットフォームが有効であること
#if !(PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)
#error "No supported platform detected"
#endif

// 少なくとも1つのアーキテクチャが有効であること
#if !(NS_ARCH_X64 || NS_ARCH_X86 || NS_ARCH_ARM64)
#error "No supported architecture detected"
#endif

// =============================================================================
// 機能フラグマクロ（02-01）
// =============================================================================

// メモリ関連
#ifndef PLATFORM_SUPPORTS_MIMALLOC
    #define PLATFORM_SUPPORTS_MIMALLOC (PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)
#endif

// SIMD / ベクトル命令
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
    #define PLATFORM_ENABLE_VECTORINTRINSICS 1
#endif

#if PLATFORM_CPU_X86_FAMILY
    #define PLATFORM_ALWAYS_HAS_SSE4_2 1
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 0
#elif PLATFORM_CPU_ARM_FAMILY
    #define PLATFORM_ALWAYS_HAS_SSE4_2 0
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 1
#else
    #define PLATFORM_ALWAYS_HAS_SSE4_2 0
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 0
#endif

// ソケット機能
#define PLATFORM_HAS_BSD_SOCKETS 1

#if PLATFORM_WINDOWS
    #define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS 1
#else
    #define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS 0
#endif

// デバッグ・スタックシンボル
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1

// =============================================================================
// コンパイラ機能マクロ（02-02）
// =============================================================================

// TCHAR wmain サポート（Windowsのみ）
#if PLATFORM_COMPILER_MSVC
    #define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 1
#else
    #define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 0
#endif

// int と long の区別（LP64 vs LLP64）
// Windows (LLP64): int=32bit, long=32bit, long long=64bit
// Unix (LP64): int=32bit, long=64bit, long long=64bit
#if PLATFORM_WINDOWS
    #define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG 0
#else
    #define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG 1
#endif

// C++20 三方比較演算子
#if __cplusplus >= 202002L
    #define PLATFORM_COMPILER_HAS_GENERATED_COMPARISON_OPERATORS 1
#else
    #define PLATFORM_COMPILER_HAS_GENERATED_COMPARISON_OPERATORS 0
#endif

// C++20 コンセプト
#if __cplusplus >= 202002L
    #define PLATFORM_COMPILER_HAS_CONCEPTS 1
#else
    #define PLATFORM_COMPILER_HAS_CONCEPTS 0
#endif
