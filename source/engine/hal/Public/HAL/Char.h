/// @file Char.h
/// @brief 文字ユーティリティテンプレート
#pragma once

#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// 文字ユーティリティテンプレート
    ///
    /// ASCII範囲の文字分類・変換を提供する。
    /// Unicode正規化は上位層で行う。
    template <typename CharType> struct TChar
    {
        // 文字定数
        static constexpr CharType kLineFeed = CharType(0x0A);       ///< LF (\n)
        static constexpr CharType kCarriageReturn = CharType(0x0D); ///< CR (\r)
        static constexpr CharType kTab = CharType(0x09);            ///< Tab (\t)
        static constexpr CharType kSpace = CharType(0x20);          ///< Space
        static constexpr CharType kNullChar = CharType(0x00);       ///< Null終端

        /// 大文字に変換
        static FORCEINLINE CharType ToUpper(CharType c)
        {
            return (c >= CharType('a') && c <= CharType('z')) ? (c - CharType('a') + CharType('A')) : c;
        }

        /// 小文字に変換
        static FORCEINLINE CharType ToLower(CharType c)
        {
            return (c >= CharType('A') && c <= CharType('Z')) ? (c - CharType('A') + CharType('a')) : c;
        }

        /// 大文字かどうか
        static FORCEINLINE bool IsUpper(CharType c) { return c >= CharType('A') && c <= CharType('Z'); }

        /// 小文字かどうか
        static FORCEINLINE bool IsLower(CharType c) { return c >= CharType('a') && c <= CharType('z'); }

        /// アルファベットかどうか
        static FORCEINLINE bool IsAlpha(CharType c) { return IsUpper(c) || IsLower(c); }

        /// 数字かどうか
        static FORCEINLINE bool IsDigit(CharType c) { return c >= CharType('0') && c <= CharType('9'); }

        /// 英数字かどうか
        static FORCEINLINE bool IsAlnum(CharType c) { return IsAlpha(c) || IsDigit(c); }

        /// 空白文字かどうか
        static FORCEINLINE bool IsWhitespace(CharType c)
        {
            return c == kSpace || c == kTab || c == kLineFeed || c == kCarriageReturn;
        }

        /// 16進数字かどうか
        static FORCEINLINE bool IsHexDigit(CharType c)
        {
            return IsDigit(c) || (c >= CharType('A') && c <= CharType('F')) ||
                   (c >= CharType('a') && c <= CharType('f'));
        }

        /// 印字可能文字かどうか
        static FORCEINLINE bool IsPrint(CharType c) { return c >= CharType(0x20) && c <= CharType(0x7E); }

        /// 制御文字かどうか
        static FORCEINLINE bool IsControl(CharType c) { return c < CharType(0x20) || c == CharType(0x7F); }

        /// 数字文字を整数に変換
        static FORCEINLINE int32 ConvertCharDigitToInt(CharType c) { return static_cast<int32>(c - CharType('0')); }

        /// 16進数字を整数に変換
        static FORCEINLINE int32 ConvertHexDigitToInt(CharType c)
        {
            if (c >= CharType('0') && c <= CharType('9'))
            {
                return c - CharType('0');
            }
            if (c >= CharType('A') && c <= CharType('F'))
            {
                return c - CharType('A') + 10;
            }
            if (c >= CharType('a') && c <= CharType('f'))
            {
                return c - CharType('a') + 10;
            }
            return 0;
        }
    };

    /// ANSI文字ユーティリティ
    using CharAnsi = TChar<ANSICHAR>;

    /// ワイド文字ユーティリティ
    using CharWide = TChar<WIDECHAR>;
} // namespace NS
