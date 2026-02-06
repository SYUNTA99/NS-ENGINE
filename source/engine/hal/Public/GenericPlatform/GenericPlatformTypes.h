/// @file GenericPlatformTypes.h
/// @brief プラットフォーム非依存の型定義
/// @details HAL固有の型を定義し、commonモジュールの型を再エクスポートする。
#pragma once

#include "common/utility/types.h"
#include <cstdint>

namespace NS
{
    /// プラットフォーム非依存の型定義
    ///
    /// 各プラットフォーム固有の型は継承してオーバーライド可能。
    struct GenericPlatformTypes
    {
        // =====================================================================
        // 基本整数型（commonモジュールの型を再エクスポート）
        // =====================================================================
        using uint8 = ::uint8;
        using uint16 = ::uint16;
        using uint32 = ::uint32;
        using uint64 = ::uint64;
        using int8 = ::int8;
        using int16 = ::int16;
        using int32 = ::int32;
        using int64 = ::int64;

        // =====================================================================
        // 文字型（HAL固有）
        // =====================================================================

        /// ANSI文字型（8ビット）
        using ANSICHAR = char;

        /// ワイド文字型（UTF-16/UTF-32、プラットフォーム依存）
        using WIDECHAR = wchar_t;

        /// テキスト文字型（UnicodeビルドではWIDECHAR）
        using TCHAR = WIDECHAR;

        /// UTF-8文字型
        using UTF8CHAR = char;

        /// UTF-16文字型
        using UTF16CHAR = char16_t;

        /// UTF-32文字型
        using UTF32CHAR = char32_t;

        // =====================================================================
        // サイズ・ポインタ型（HAL固有）
        // =====================================================================

        /// サイズ型（符号なし）
        using SIZE_T = ::size;

        /// サイズ差分型（符号付き）
        using SSIZE_T = ::ptrdiff;

        /// ポインタ値を格納できる符号なし整数型
        using UPTRINT = std::uintptr_t;

        /// ポインタ値を格納できる符号付き整数型
        using PTRINT = std::intptr_t;
    };
} // namespace NS
