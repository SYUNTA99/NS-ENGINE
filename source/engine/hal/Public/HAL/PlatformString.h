/// @file PlatformString.h
/// @brief プラットフォーム共通の文字列操作
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// プラットフォーム共通の文字列操作
    ///
    /// 安全版（バッファサイズ指定）を使用。
    /// 大文字小文字無視の比較はASCII範囲のみ対応。
    struct GenericPlatformString
    {
        // =========================================================================
        // 長さ取得
        // =========================================================================

        /// ANSI文字列の長さを取得
        static int32 Strlen(const ANSICHAR* str);

        /// ワイド文字列の長さを取得
        static int32 Strlen(const WIDECHAR* str);

        // =========================================================================
        // 比較（大文字小文字区別）
        // =========================================================================

        /// ANSI文字列を比較
        static int32 Strcmp(const ANSICHAR* a, const ANSICHAR* b);

        /// ワイド文字列を比較
        static int32 Strcmp(const WIDECHAR* a, const WIDECHAR* b);

        // =========================================================================
        // 比較（大文字小文字無視）
        // =========================================================================

        /// ANSI文字列を比較（大文字小文字無視）
        static int32 Stricmp(const ANSICHAR* a, const ANSICHAR* b);

        /// ワイド文字列を比較（大文字小文字無視）
        static int32 Stricmp(const WIDECHAR* a, const WIDECHAR* b);

        // =========================================================================
        // 比較（長さ制限）
        // =========================================================================

        /// ANSI文字列を長さ制限付きで比較
        static int32 Strncmp(const ANSICHAR* a, const ANSICHAR* b, SIZE_T count);

        /// ワイド文字列を長さ制限付きで比較
        static int32 Strncmp(const WIDECHAR* a, const WIDECHAR* b, SIZE_T count);

        // =========================================================================
        // コピー
        // =========================================================================

        /// ANSI文字列をコピー
        /// @param dest コピー先バッファ
        /// @param destSize コピー先バッファサイズ（文字数）
        /// @param src コピー元文字列
        /// @return コピー先ポインタ
        static ANSICHAR* Strcpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src);

        /// ワイド文字列をコピー
        static WIDECHAR* Strcpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src);

        /// ANSI文字列を長さ制限付きでコピー
        static ANSICHAR* Strncpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src, SIZE_T count);

        /// ワイド文字列を長さ制限付きでコピー
        static WIDECHAR* Strncpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src, SIZE_T count);

        // =========================================================================
        // 連結
        // =========================================================================

        /// ANSI文字列を連結
        static ANSICHAR* Strcat(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src);

        /// ワイド文字列を連結
        static WIDECHAR* Strcat(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src);

        // =========================================================================
        // 検索
        // =========================================================================

        /// ANSI文字列内を検索
        static const ANSICHAR* Strstr(const ANSICHAR* str, const ANSICHAR* find);

        /// ワイド文字列内を検索
        static const WIDECHAR* Strstr(const WIDECHAR* str, const WIDECHAR* find);

        /// ANSI文字列内で文字を検索
        static const ANSICHAR* Strchr(const ANSICHAR* str, ANSICHAR c);

        /// ワイド文字列内で文字を検索
        static const WIDECHAR* Strchr(const WIDECHAR* str, WIDECHAR c);

        /// ANSI文字列内で文字を逆方向検索
        static const ANSICHAR* Strrchr(const ANSICHAR* str, ANSICHAR c);

        /// ワイド文字列内で文字を逆方向検索
        static const WIDECHAR* Strrchr(const WIDECHAR* str, WIDECHAR c);
    };

    typedef GenericPlatformString PlatformString;
} // namespace NS
