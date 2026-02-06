> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [10-01a: TCharテンプレート](10-01a-tchar-template.md)
> - [10-01b: PlatformString文字列操作](10-01b-platform-string.md)

# 10-01: 文字列ユーティリティ（旧版）

## 目的

プラットフォーム共通の文字列ユーティリティを実装する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション1「文字列・テキストユーティリティ」

## ドキュメントとの差異

| UE5 (ドキュメント) | NS-ENGINE | 理由 |
|-------------------|-----------|------|
| `FChar` | `TChar<TCHAR>` | NS命名規則（`F`プレフィックス不使用、テンプレートは`T`） |
| `FCharAnsi` | `TChar<ANSICHAR>` | 同上 |
| `FCharWide` | `TChar<WIDECHAR>` | 同上 |

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`, `ANSICHAR`, `WIDECHAR`, `int32`, `SIZE_T`）
- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/Char.h`
- `source/engine/hal/Public/HAL/PlatformString.h`

## TODO

- [ ] `TChar` テンプレートクラス（文字分類・変換）
- [ ] 文字定数定義（LineFeed等）
- [ ] `PlatformString` 構造体（strlen, strcmp等）
- [ ] エンコーディング変換

## 実装内容

```cpp
// Char.h
#pragma once

namespace NS
{
    template<typename CharType>
    struct TChar
    {
        static constexpr CharType LineFeed = 0x0A;
        static constexpr CharType CarriageReturn = 0x0D;

        static FORCEINLINE CharType ToUpper(CharType c)
        {
            return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
        }

        static FORCEINLINE CharType ToLower(CharType c)
        {
            return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
        }

        static FORCEINLINE bool IsUpper(CharType c) { return c >= 'A' && c <= 'Z'; }
        static FORCEINLINE bool IsLower(CharType c) { return c >= 'a' && c <= 'z'; }
        static FORCEINLINE bool IsAlpha(CharType c) { return IsUpper(c) || IsLower(c); }
        static FORCEINLINE bool IsDigit(CharType c) { return c >= '0' && c <= '9'; }
        static FORCEINLINE bool IsAlnum(CharType c) { return IsAlpha(c) || IsDigit(c); }
        static FORCEINLINE bool IsWhitespace(CharType c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }
        static FORCEINLINE bool IsHexDigit(CharType c)
        {
            return IsDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
        }

        static FORCEINLINE int32 ConvertCharDigitToInt(CharType c)
        {
            return c - '0';
        }
    };

    // 特殊化エイリアス（直接 TChar<CharType> を使用することを推奨）
    using CharUtf16 = TChar<WIDECHAR>;
    using CharAnsi = TChar<ANSICHAR>;
}

// PlatformString.h
namespace NS
{
    struct GenericPlatformString
    {
        static int32 Strlen(const ANSICHAR* str);
        static int32 Strlen(const WIDECHAR* str);

        static int32 Strcmp(const ANSICHAR* a, const ANSICHAR* b);
        static int32 Strcmp(const WIDECHAR* a, const WIDECHAR* b);

        static int32 Stricmp(const ANSICHAR* a, const ANSICHAR* b);
        static int32 Stricmp(const WIDECHAR* a, const WIDECHAR* b);

        static ANSICHAR* Strcpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src);
        static WIDECHAR* Strcpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src);
    };

    typedef GenericPlatformString PlatformString;
}
```

## 検証

- 文字分類関数が正しく動作
- 文字列比較が期待通り動作
- `TChar<char>::ToUpper('a')` が `'A'` を返す

## 注意事項

- ASCII範囲のみ対応（Unicode正規化は上位層）
- `Stricmp` は大文字小文字無視比較
- `Strcpy` はバッファサイズを受け取る安全版

## 次のサブ計画

→ 10-02: アライメントユーティリティ
