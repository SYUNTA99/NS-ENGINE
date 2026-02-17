/// @file PlatformTypes.h
/// @brief HAL型定義のエントリポイント
/// @details プラットフォーム固有の型を自動選択してインクルードし、
///          HAL固有型をNS名前空間にエクスポートする。
#pragma once

#include "GenericPlatform/GenericPlatformTypes.h"
#include "HAL/Platform.h"

// プラットフォーム固有の型定義をインクルード
#if PLATFORM_WINDOWS
    #include "Windows/WindowsPlatformTypes.h"
#elif PLATFORM_MAC
    #include "Mac/MacPlatformTypes.h"
#elif PLATFORM_LINUX
    #include "Linux/LinuxPlatformTypes.h"
#else
    #error "No platform-specific types header available"
#endif

namespace NS
{
    // =========================================================================
    // HAL固有型のエクスポート
    // =========================================================================

    /// サイズ型（符号なし）
    using SIZE_T = PlatformTypes::SIZE_T;

    /// サイズ差分型（符号付き）
    using SSIZE_T = PlatformTypes::SSIZE_T;

    /// ポインタ値を格納できる符号なし整数型
    using UPTRINT = PlatformTypes::UPTRINT;

    /// ポインタ値を格納できる符号付き整数型
    using PTRINT = PlatformTypes::PTRINT;

    /// テキスト文字型
    using TCHAR = PlatformTypes::TCHAR;

    /// ANSI文字型
    using ANSICHAR = PlatformTypes::ANSICHAR;

    /// ワイド文字型
    using WIDECHAR = PlatformTypes::WIDECHAR;

    /// UTF-8文字型
    using UTF8CHAR = PlatformTypes::UTF8CHAR;

    /// UTF-16文字型
    using UTF16CHAR = PlatformTypes::UTF16CHAR;

    /// UTF-32文字型
    using UTF32CHAR = PlatformTypes::UTF32CHAR;
} // namespace NS

// =============================================================================
// 文字列リテラルマクロ
// =============================================================================

/// テキスト文字列リテラルマクロ
///
/// TCHAR型の文字列リテラルを生成する。
/// Unicodeビルドではワイド文字列、ANSIビルドでは通常文字列。
///
/// @code
/// const NS::TCHAR* str = TEXT("Hello, World!");
/// @endcode
#ifndef TEXT
#define TEXT(x) L##x
#endif

/// ANSI文字列リテラルマクロ（明示的にANSIを使用）
#ifndef ANSITEXT
#define ANSITEXT(x) x
#endif

/// ワイド文字列リテラルマクロ（明示的にワイドを使用）
#ifndef WIDETEXT
#define WIDETEXT(x) L##x
#endif
