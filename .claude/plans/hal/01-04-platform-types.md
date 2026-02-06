# 01-04: プラットフォーム型

## 目的

HAL固有の型定義を追加し、commonモジュールの型を再利用する。

## 参考

[Platform_Abstraction_Layer_Part1.md](docs/Platform_Abstraction_Layer_Part1.md) - セクション3「3層抽象化モデル」
[Platform_Abstraction_Layer_Part2.md](docs/Platform_Abstraction_Layer_Part2.md) - セクション4「型システム」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint8`〜`uint64`, `int8`〜`int64`, `size`, `ptrdiff`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER（`COMPILED_PLATFORM_HEADER`マクロ）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformTypes.h`
- `source/engine/hal/Public/Windows/WindowsPlatformTypes.h`
- `source/engine/hal/Public/HAL/PlatformTypes.h`

## TODO

- [ ] `GenericPlatformTypes.h` 作成（HAL固有型定義）
- [ ] `WindowsPlatformTypes.h` 作成（Windows固有型）
- [ ] `PlatformTypes.h` 作成（エントリポイント）
- [ ] `TEXT()` マクロ定義（文字列リテラルのワイド化）

## 実装内容

### GenericPlatformTypes.h
```cpp
#pragma once

// commonモジュールの基本型を使用
#include "common/utility/types.h"
#include <cstdint>

namespace NS
{
    struct GenericPlatformTypes
    {
        // commonモジュールの型を再エクスポート
        using uint8 = ::uint8;
        using uint16 = ::uint16;
        using uint32 = ::uint32;
        using uint64 = ::uint64;
        using int8 = ::int8;
        using int16 = ::int16;
        using int32 = ::int32;
        using int64 = ::int64;

        // HAL固有の文字型
        using ANSICHAR = char;
        using WIDECHAR = wchar_t;
        using TCHAR = WIDECHAR;

        // HAL固有のサイズ型（commonの`size`と互換）
        using SIZE_T = ::size;
        using SSIZE_T = ::ptrdiff;
        using UPTRINT = std::uintptr_t;
        using PTRINT = std::intptr_t;
    };
}
```

### WindowsPlatformTypes.h
```cpp
#pragma once
#include "GenericPlatform/GenericPlatformTypes.h"

namespace NS
{
    struct WindowsPlatformTypes : public GenericPlatformTypes
    {
        // Windows固有の型オーバーライド（必要に応じて）
    };

    typedef WindowsPlatformTypes PlatformTypes;
}
```

### PlatformTypes.h
```cpp
#pragma once
#include "GenericPlatform/GenericPlatformTypes.h"
#include COMPILED_PLATFORM_HEADER(PlatformTypes.h)

namespace NS
{
    // HAL固有型のエクスポート
    using SIZE_T = PlatformTypes::SIZE_T;
    using SSIZE_T = PlatformTypes::SSIZE_T;
    using UPTRINT = PlatformTypes::UPTRINT;
    using PTRINT = PlatformTypes::PTRINT;
    using TCHAR = PlatformTypes::TCHAR;
    using ANSICHAR = PlatformTypes::ANSICHAR;
    using WIDECHAR = PlatformTypes::WIDECHAR;
}

// 文字列リテラルマクロ
#define TEXT(x) L##x
```

## 検証

- `NS::SIZE_T` が `::size` と同等
- `NS::PlatformTypes` が Windows では `WindowsPlatformTypes` に解決
- `TEXT("hello")` が `L"hello"` に展開される

## 注意事項

- `uint8`〜`uint64` は common/utility/types.h のグローバルエイリアス
- HALでは `NS::PlatformTypes::uint32` 等としてもアクセス可能にする
- `TCHAR` はWindowsでは `wchar_t`（UTF-16）

## 次のサブ計画

→ 02-01: 機能フラグマクロ（Platform.hに追加）
