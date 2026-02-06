# 01-03: COMPILED_PLATFORM_HEADER マクロ

## 目的

プラットフォーム固有ヘッダを自動選択するマクロシステムを実装する。

## 参考

[Platform_Abstraction_Layer_Part1.md](docs/Platform_Abstraction_Layer_Part1.md) - セクション4「ヘッダインクルードシステム」

## 依存（commonモジュール）

- `common/utility/macros.h` - 文字列化・連結マクロ（`NS_MACRO_STRINGIZE`, `NS_MACRO_CONCATENATE`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS` 等）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/PreprocessorHelpers.h`

## TODO

- [ ] `PreprocessorHelpers.h` 作成
- [ ] PLATFORM_HEADER_NAME マクロ定義
- [ ] COMPILED_PLATFORM_HEADER マクロ定義
- [ ] Platform.h から PreprocessorHelpers.h をインクルード

## 実装内容

```cpp
// PreprocessorHelpers.h

#pragma once

// commonモジュールの基盤マクロを使用
#include "common/utility/macros.h"

// HALエイリアス（互換性のため）
#define NS_STRINGIFY NS_MACRO_STRINGIZE
#define NS_STRINGIFY_MACRO(x) NS_MACRO_STRINGIZE(x)
#define NS_CONCAT NS_MACRO_CONCATENATE
#define NS_CONCAT_MACRO(a, b) NS_MACRO_CONCATENATE(a, b)

// プラットフォームヘッダ名（ビルドシステムから定義）
// 例: -DNS_COMPILED_PLATFORM=Windows
#if !defined(NS_COMPILED_PLATFORM)
    #if PLATFORM_WINDOWS
        #define NS_COMPILED_PLATFORM Windows
    #elif PLATFORM_MAC
        #define NS_COMPILED_PLATFORM Mac
    #elif PLATFORM_LINUX
        #define NS_COMPILED_PLATFORM Linux
    #else
        #error "NS_COMPILED_PLATFORM not defined"
    #endif
#endif

#define PLATFORM_HEADER_NAME NS_COMPILED_PLATFORM

// プラットフォーム固有ヘッダをインクルード
// 使用例: #include COMPILED_PLATFORM_HEADER(PlatformMemory.h)
// Windows時: "Windows/WindowsPlatformMemory.h" に展開
#define COMPILED_PLATFORM_HEADER(Suffix) \
    NS_STRINGIFY_MACRO(NS_CONCAT_MACRO(PLATFORM_HEADER_NAME, /NS_CONCAT_MACRO(PLATFORM_HEADER_NAME, Suffix)))
```

## 検証

```cpp
// テスト
#include COMPILED_PLATFORM_HEADER(PlatformMemory.h)
// Windows: "Windows/WindowsPlatformMemory.h" に展開されること
```

## 注意事項

- マクロ展開は2段階必要（STRINGIZE_MACRO, CONCAT_MACRO）
- `NS_COMPILED_PLATFORM` はpremakeで定義するか、ヘッダ内でフォールバック定義
- このマクロは Part 3 以降の全コンポーネントで使用される

## 次のサブ計画

→ 01-04: プラットフォーム型（GenericPlatformTypes.h, WindowsPlatformTypes.h, PlatformTypes.h作成）
