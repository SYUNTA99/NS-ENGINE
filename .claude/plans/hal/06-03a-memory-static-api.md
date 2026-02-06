# 06-03a: Memory静的API

## 目的

統一メモリAPIの静的インターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション1「統一API層」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 06-01a Malloc基底クラス（`kDefaultAlignment`）
- 06-01b GMallocグローバル定義（`GMalloc`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/UnrealMemory.h`

## TODO

- [ ] `Memory` 構造体定義
- [ ] GMallocラッパー関数宣言
- [ ] SystemMalloc/SystemFree宣言（GMallocバイパス）
- [ ] ユーティリティ関数宣言

## 実装内容

```cpp
// UnrealMemory.h
#pragma once

#include "HAL/MemoryBase.h"

namespace NS
{
    /// 統一メモリAPI
    ///
    /// GMalloc経由のメモリ操作と、GMallocをバイパスする
    /// システムメモリ操作の両方を提供する。
    struct Memory
    {
        // GMalloc経由のメモリ操作

        /// メモリ割り当て
        static void* Malloc(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// メモリ再割り当て
        static void* Realloc(void* ptr, SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// メモリ解放
        static void Free(void* ptr);

        /// 割り当てサイズ取得
        static SIZE_T GetAllocSize(void* ptr);

        /// ゼロ初期化付き割り当て
        static void* MallocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// サイズ量子化
        static SIZE_T QuantizeSize(SIZE_T count, uint32 alignment = kDefaultAlignment);

        // GMallocバイパス（起動初期段階用）

        /// システムメモリ割り当て（GMalloc不使用）
        static void* SystemMalloc(SIZE_T size);

        /// システムメモリ解放（GMalloc不使用）
        static void SystemFree(void* ptr);

        // ユーティリティ

        /// メモリトリム
        static void Trim(bool trimThreadCaches = true);

        /// ヒープ検証
        static bool TestMemory();

        /// GMalloc初期化済みかどうか
        static bool IsGMallocReady();
    };
}
```

## 検証

- `Memory` 構造体がコンパイル可能
- 全メソッドが宣言されている

## 次のサブ計画

→ 06-03b: Memory API実装
