# 02-01: 機能フラグマクロ

## 目的

プラットフォーム機能の有無を示すフラグマクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part2.md](docs/Platform_Abstraction_Layer_Part2.md) - セクション2「機能フラグマクロ」

## 依存（commonモジュール）

- `common/utility/macros.h` - アーキテクチャ検出（`NS_ARCH_X64`, `NS_ARCH_ARM64`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`, `PLATFORM_CPU_X86_FAMILY` ← common由来）

## 変更対象

変更:
- `source/engine/hal/Public/HAL/Platform.h`

## TODO

- [ ] メモリ関連フラグ（PLATFORM_SUPPORTS_MIMALLOC等）
- [ ] SIMD/ベクトル命令フラグ（PLATFORM_ENABLE_VECTORINTRINSICS等）
- [ ] ソケット機能フラグ（PLATFORM_HAS_BSD_SOCKETS等）
- [ ] その他機能フラグ（PLATFORM_SUPPORTS_STACK_SYMBOLS等）

## 実装内容

```cpp
// Platform.h に追加

// メモリ関連
#ifndef PLATFORM_SUPPORTS_MIMALLOC
    #define PLATFORM_SUPPORTS_MIMALLOC (PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)
#endif

// SIMD
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
    #define PLATFORM_ENABLE_VECTORINTRINSICS 1
#endif

#if PLATFORM_CPU_X86_FAMILY
    #define PLATFORM_ALWAYS_HAS_SSE4_2 1
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 0
#elif PLATFORM_CPU_ARM_FAMILY
    #define PLATFORM_ALWAYS_HAS_SSE4_2 0
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 1
#endif

// ソケット
#define PLATFORM_HAS_BSD_SOCKETS 1
#if PLATFORM_WINDOWS
    #define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS 1
#else
    #define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS 0
#endif

// デバッグ
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1
```

## 検証

- 各フラグがプラットフォームに応じた値を持つこと
- `PLATFORM_ALWAYS_HAS_SSE4_2` が x64 で 1
- `PLATFORM_SUPPORTS_MIMALLOC` がデスクトップで 1

## 注意事項

- フラグは `#ifndef` ガードで外部オーバーライド可能にする
- ビルド時に `-DPLATFORM_ENABLE_VECTORINTRINSICS=0` 等で無効化可能

## 次のサブ計画

→ 02-02: コンパイラマクロ（Platform.hに追加）
