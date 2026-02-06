# 01-02: プラットフォームマクロ

## 目的

HAL用のプラットフォーム識別マクロエイリアスとグループマクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part1.md](docs/Platform_Abstraction_Layer_Part1.md) - セクション5「プラットフォーム識別マクロ一覧」

## 依存（commonモジュール）

- `common/utility/macros.h` - 基盤マクロ（`NS_PLATFORM_*`, `NS_ARCH_*`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/Platform.h`

## TODO

- [ ] `Platform.h` 作成
- [ ] commonマクロのインクルード
- [ ] HAL固有のグループマクロ定義（PLATFORM_DESKTOP, PLATFORM_UNIX等）
- [ ] CPU ファミリーマクロ定義

## 実装内容

```cpp
// Platform.h

#pragma once

// common モジュールの基盤マクロを使用
#include "common/utility/macros.h"

// commonマクロからHALエイリアスを定義（互換性のため）
#define PLATFORM_WINDOWS NS_PLATFORM_WINDOWS
#define PLATFORM_MAC NS_PLATFORM_APPLE
#define PLATFORM_LINUX NS_PLATFORM_LINUX

// プラットフォームグループ（HAL固有）
#define PLATFORM_DESKTOP (PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)
#define PLATFORM_UNIX (PLATFORM_MAC || PLATFORM_LINUX)
#define PLATFORM_APPLE NS_PLATFORM_APPLE

// アーキテクチャ（commonマクロを使用）
#define PLATFORM_64BITS (NS_POINTER_SIZE == 8)
#define PLATFORM_CPU_X86_FAMILY (NS_ARCH_X64 || NS_ARCH_X86)
#define PLATFORM_CPU_ARM_FAMILY NS_ARCH_ARM64

#define PLATFORM_LITTLE_ENDIAN 1
```

## 検証

- PLATFORM_WINDOWS が Windows ビルドで 1
- PLATFORM_64BITS が 64bit ビルドで 1
- PLATFORM_CPU_X86_FAMILY が x64/x86 で 1

## 注意事項

- commonの `NS_PLATFORM_*` を直接使っても良いが、HALは互換性のため `PLATFORM_*` エイリアスを提供
- このファイルはHALの他すべてのヘッダから間接的にインクルードされる

## 次のサブ計画

→ 01-03: COMPILED_PLATFORM_HEADER（PreprocessorHelpers.h作成）
