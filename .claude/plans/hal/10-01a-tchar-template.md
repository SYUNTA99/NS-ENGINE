# 10-01a: TCharテンプレート

## 目的

文字分類・変換のテンプレートクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション1「文字列ユーティリティ」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`ANSICHAR`, `WIDECHAR`）
- 02-03 関数呼び出し規約（`FORCEINLINE`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/Char.h`

## TODO

- [ ] `TChar` テンプレートクラス定義
- [ ] 文字定数定義（LineFeed等）
- [ ] 文字分類関数（IsUpper, IsLower等）
- [ ] 文字変換関数（ToUpper, ToLower）
- [ ] 16進数変換

## 実装内容

```cpp
// Char.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PlatformMacros.h"

namespace NS
{
    /// 文字ユーティリティテンプレート
    ///
    /// ASCII範囲の文字分類・変換を提供する。
    /// Unicode正規化は上位層で行う。
    template<typename CharType>
    struct TChar
    {
        // 文字定数
        static constexpr CharType LineFeed = CharType(0x0A);        ///< LF (\n)
        static constexpr CharType CarriageReturn = CharType(0x0D); ///< CR (\r)
        static constexpr CharType Tab = CharType(0x09);            ///< Tab (\t)
        static constexpr CharType Space = CharType(0x20);          ///< Space
        static constexpr CharType NullChar = CharType(0x00);       ///< Null終端

        /// 大文字に変換
        static FORCEINLINE CharType ToUpper(CharType c)
        {
            return (c >= CharType('a') && c <= CharType('z'))
                ? (c - CharType('a') + CharType('A'))
                : c;
        }

        /// 小文字に変換
        static FORCEINLINE CharType ToLower(CharType c)
        {
            return (c >= CharType('A') && c <= CharType('Z'))
                ? (c - CharType('A') + CharType('a'))
                : c;
        }

        /// 大文字かどうか
        static FORCEINLINE bool IsUpper(CharType c)
        {
            return c >= CharType('A') && c <= CharType('Z');
        }

        /// 小文字かどうか
        static FORCEINLINE bool IsLower(CharType c)
        {
            return c >= CharType('a') && c <= CharType('z');
        }

        /// アルファベットかどうか
        static FORCEINLINE bool IsAlpha(CharType c)
        {
            return IsUpper(c) || IsLower(c);
        }

        /// 数字かどうか
        static FORCEINLINE bool IsDigit(CharType c)
        {
            return c >= CharType('0') && c <= CharType('9');
        }

        /// 英数字かどうか
        static FORCEINLINE bool IsAlnum(CharType c)
        {
            return IsAlpha(c) || IsDigit(c);
        }

        /// 空白文字かどうか
        static FORCEINLINE bool IsWhitespace(CharType c)
        {
            return c == Space || c == Tab || c == LineFeed || c == CarriageReturn;
        }

        /// 16進数字かどうか
        static FORCEINLINE bool IsHexDigit(CharType c)
        {
            return IsDigit(c)
                || (c >= CharType('A') && c <= CharType('F'))
                || (c >= CharType('a') && c <= CharType('f'));
        }

        /// 印字可能文字かどうか
        static FORCEINLINE bool IsPrint(CharType c)
        {
            return c >= CharType(0x20) && c <= CharType(0x7E);
        }

        /// 制御文字かどうか
        static FORCEINLINE bool IsControl(CharType c)
        {
            return c < CharType(0x20) || c == CharType(0x7F);
        }

        /// 数字文字を整数に変換
        static FORCEINLINE int32 ConvertCharDigitToInt(CharType c)
        {
            return static_cast<int32>(c - CharType('0'));
        }

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

    // よく使う特殊化
    using CharAnsi = TChar<ANSICHAR>;
    using CharWide = TChar<WIDECHAR>;
}
```

## 検証

- `TChar<char>::ToUpper('a')` が `'A'` を返す
- `TChar<char>::IsDigit('5')` が true
- `TChar<char>::ConvertHexDigitToInt('F')` が 15

## 注意事項

- ASCII範囲のみ対応（Unicode大文字変換は上位層）
- FORCEINLINE でゼロオーバーヘッド
- テンプレートなのでヘッダのみ

## 次のサブ計画

→ 10-01b: PlatformString文字列操作
