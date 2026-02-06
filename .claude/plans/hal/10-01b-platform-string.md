# 10-01b: PlatformString文字列操作

## 目的

プラットフォーム共通の文字列操作関数を実装する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション1「文字列ユーティリティ」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`int32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`ANSICHAR`, `WIDECHAR`, `SIZE_T`）
- 10-01a TCharテンプレート

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/PlatformString.h`
- `source/engine/hal/Private/HAL/PlatformString.cpp`

## TODO

- [ ] `GenericPlatformString` 構造体定義
- [ ] `Strlen` 実装
- [ ] `Strcmp` / `Stricmp` 実装
- [ ] `Strcpy` / `Strncpy` 実装
- [ ] `Strcat` 実装
- [ ] 文字列検索関数

## 実装内容

### PlatformString.h

```cpp
// PlatformString.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// プラットフォーム共通の文字列操作
    struct GenericPlatformString
    {
        // 長さ取得
        static int32 Strlen(const ANSICHAR* str);
        static int32 Strlen(const WIDECHAR* str);

        // 比較（大文字小文字区別）
        static int32 Strcmp(const ANSICHAR* a, const ANSICHAR* b);
        static int32 Strcmp(const WIDECHAR* a, const WIDECHAR* b);

        // 比較（大文字小文字無視）
        static int32 Stricmp(const ANSICHAR* a, const ANSICHAR* b);
        static int32 Stricmp(const WIDECHAR* a, const WIDECHAR* b);

        // 比較（長さ制限、大文字小文字区別）
        static int32 Strncmp(const ANSICHAR* a, const ANSICHAR* b, SIZE_T count);
        static int32 Strncmp(const WIDECHAR* a, const WIDECHAR* b, SIZE_T count);

        // コピー（安全版）
        static ANSICHAR* Strcpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src);
        static WIDECHAR* Strcpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src);

        // コピー（長さ制限、安全版）
        static ANSICHAR* Strncpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src, SIZE_T count);
        static WIDECHAR* Strncpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src, SIZE_T count);

        // 連結（安全版）
        static ANSICHAR* Strcat(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src);
        static WIDECHAR* Strcat(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src);

        // 検索
        static const ANSICHAR* Strstr(const ANSICHAR* str, const ANSICHAR* find);
        static const WIDECHAR* Strstr(const WIDECHAR* str, const WIDECHAR* find);

        // 文字検索
        static const ANSICHAR* Strchr(const ANSICHAR* str, ANSICHAR c);
        static const WIDECHAR* Strchr(const WIDECHAR* str, WIDECHAR c);

        // 逆方向文字検索
        static const ANSICHAR* Strrchr(const ANSICHAR* str, ANSICHAR c);
        static const WIDECHAR* Strrchr(const WIDECHAR* str, WIDECHAR c);
    };

    typedef GenericPlatformString PlatformString;
}
```

### PlatformString.cpp

```cpp
#include "HAL/PlatformString.h"
#include "HAL/Char.h"
#include <cstring>
#include <cwchar>

namespace NS
{
    int32 GenericPlatformString::Strlen(const ANSICHAR* str)
    {
        return str ? static_cast<int32>(std::strlen(str)) : 0;
    }

    int32 GenericPlatformString::Strlen(const WIDECHAR* str)
    {
        return str ? static_cast<int32>(std::wcslen(str)) : 0;
    }

    int32 GenericPlatformString::Strcmp(const ANSICHAR* a, const ANSICHAR* b)
    {
        return std::strcmp(a, b);
    }

    int32 GenericPlatformString::Strcmp(const WIDECHAR* a, const WIDECHAR* b)
    {
        return std::wcscmp(a, b);
    }

    int32 GenericPlatformString::Stricmp(const ANSICHAR* a, const ANSICHAR* b)
    {
        while (*a && *b)
        {
            ANSICHAR ca = CharAnsi::ToLower(*a);
            ANSICHAR cb = CharAnsi::ToLower(*b);
            if (ca != cb)
            {
                return ca - cb;
            }
            ++a;
            ++b;
        }
        return CharAnsi::ToLower(*a) - CharAnsi::ToLower(*b);
    }

    int32 GenericPlatformString::Stricmp(const WIDECHAR* a, const WIDECHAR* b)
    {
        while (*a && *b)
        {
            WIDECHAR ca = CharWide::ToLower(*a);
            WIDECHAR cb = CharWide::ToLower(*b);
            if (ca != cb)
            {
                return ca - cb;
            }
            ++a;
            ++b;
        }
        return CharWide::ToLower(*a) - CharWide::ToLower(*b);
    }

    ANSICHAR* GenericPlatformString::Strcpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src)
    {
        if (dest && src && destSize > 0)
        {
#if PLATFORM_WINDOWS
            strcpy_s(dest, destSize, src);
#else
            std::strncpy(dest, src, destSize - 1);
            dest[destSize - 1] = '\0';
#endif
        }
        return dest;
    }

    WIDECHAR* GenericPlatformString::Strcpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src)
    {
        if (dest && src && destSize > 0)
        {
#if PLATFORM_WINDOWS
            wcscpy_s(dest, destSize, src);
#else
            std::wcsncpy(dest, src, destSize - 1);
            dest[destSize - 1] = L'\0';
#endif
        }
        return dest;
    }

    const ANSICHAR* GenericPlatformString::Strstr(const ANSICHAR* str, const ANSICHAR* find)
    {
        return std::strstr(str, find);
    }

    const WIDECHAR* GenericPlatformString::Strstr(const WIDECHAR* str, const WIDECHAR* find)
    {
        return std::wcsstr(str, find);
    }

    // ... 他の関数も同様に実装
}
```

## 検証

- `Strlen("hello")` が 5
- `Stricmp("Hello", "HELLO")` が 0
- `Strcpy` がバッファオーバーフローしない

## 注意事項

- 安全版（バッファサイズ指定）を使用
- Windowsでは `_s` サフィックス版を使用
- `Stricmp` はASCII範囲のみ対応

## 次のサブ計画

→ 10-02a: アライメントテンプレート
