# 02-02: コンパイラマクロ

## 目的

HAL用のコンパイラ識別エイリアスと機能マクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part2.md](docs/Platform_Abstraction_Layer_Part2.md) - セクション3「コンパイラマクロ」

## 依存（commonモジュール）

- `common/utility/macros.h` - コンパイラ検出（`NS_COMPILER_MSVC`, `NS_COMPILER_CLANG`, `NS_COMPILER_GCC`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`）

## 変更対象

変更:
- `source/engine/hal/Public/HAL/Platform.h`

## TODO

- [ ] commonマクロからHALエイリアス定義
- [ ] HAL固有のコンパイラ機能マクロ
- [ ] C++バージョン検出

## 実装内容

```cpp
// Platform.h に追加

// commonマクロからHALエイリアスを定義（互換性のため）
#define PLATFORM_COMPILER_CLANG NS_COMPILER_CLANG
#define PLATFORM_COMPILER_MSVC NS_COMPILER_MSVC
#define PLATFORM_COMPILER_GCC NS_COMPILER_GCC

// HAL固有のコンパイラ機能マクロ
#if PLATFORM_COMPILER_MSVC
    #define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 1
#else
    #define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 0
#endif

// int と long の区別（LP64 vs LLP64）
#if PLATFORM_WINDOWS
    #define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG 0
#else
    #define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG 1
#endif

// C++20 機能
#if __cplusplus >= 202002L
    #define PLATFORM_COMPILER_HAS_GENERATED_COMPARISON_OPERATORS 1
#else
    #define PLATFORM_COMPILER_HAS_GENERATED_COMPARISON_OPERATORS 0
#endif
```

## 検証

- PLATFORM_COMPILER_MSVC が NS_COMPILER_MSVC と同値
- Clang で PLATFORM_COMPILER_CLANG が 1
- C++20環境で PLATFORM_COMPILER_HAS_GENERATED_COMPARISON_OPERATORS が 1

## 注意事項

- LP64（Unix系）とLLP64（Windows）の違いを `PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG` で表現
- Windowsでは `int` と `long` が同じサイズ（32bit）

## 次のサブ計画

→ 02-03: 関数呼び出し規約（PlatformMacros.h作成）
