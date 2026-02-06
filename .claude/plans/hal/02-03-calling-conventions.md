# 02-03: 関数呼び出し規約

## 目的

HAL用の関数呼び出し規約エイリアスと追加属性マクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part2.md](docs/Platform_Abstraction_Layer_Part2.md) - セクション5「関数呼び出し規約」

## 依存（commonモジュール）

- `common/utility/macros.h` - 呼び出し規約・インライン・DLL（`NS_CDECL`, `NS_STDCALL`, `NS_FORCEINLINE`, `NS_NOINLINE`, `NS_EXPORT`, `NS_IMPORT`, `NS_RESTRICT`, `NS_NODISCARD`, `NS_NORETURN`, `NS_MAYBE_UNUSED`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`）
- 02-02 コンパイラマクロ（`PLATFORM_COMPILER_MSVC`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/PlatformMacros.h`

## TODO

- [ ] `PlatformMacros.h` 作成
- [ ] commonマクロからHALエイリアス定義
- [ ] HAL固有の追加マクロ

## 実装内容

```cpp
// PlatformMacros.h

#pragma once

#include "HAL/Platform.h"
// common/utility/macros.h は Platform.h 経由でインクルード済み

// commonマクロからHALエイリアスを定義（互換性のため）
#define VARARGS NS_CDECL
#define CDECL NS_CDECL
#define STDCALL NS_STDCALL
#define FORCEINLINE NS_FORCEINLINE
#define FORCENOINLINE NS_NOINLINE
#define DLLEXPORT NS_EXPORT
#define DLLIMPORT NS_IMPORT
#define RESTRICT NS_RESTRICT

// NS_* マクロはすでに common/utility/macros.h で定義済み
// NS_NODISCARD, NS_NORETURN, NS_MAYBE_UNUSED を直接使用

// HAL固有の追加マクロ
#if PLATFORM_COMPILER_MSVC
    #define NS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define NS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

// コールドパス（頻繁に実行されない関数）
#if PLATFORM_COMPILER_MSVC
    #define NS_COLD FORCENOINLINE
#else
    #define NS_COLD __attribute__((cold))
#endif
```

## 検証

- FORCEINLINE が NS_FORCEINLINE と同値
- DLLEXPORT が NS_EXPORT と同値
- NS_NO_UNIQUE_ADDRESS が正しいコンパイラ属性に展開

## 注意事項

- `NS_NO_UNIQUE_ADDRESS` はC++20の `[[no_unique_address]]` のMSVC対応版
- `NS_COLD` はコールドパス（エラーハンドリング等）に使用
- これらはすべてcommonマクロの薄いラッパー

## 次のサブ計画

→ 04-01: premake HALプロジェクト（ビルド可能にする）
