> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [06-03a: Memory静的API](06-03a-memory-static-api.md)
> - [06-03b: Memory API実装](06-03b-memory-api-impl.md)

# 06-03: Memory API（旧版）

## 目的

統一メモリAPIを提供する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション1「アロケータアーキテクチャ概要」（統一API層）

## 依存（commonモジュール）

- `common/utility/macros.h` - プラットフォーム検出（`NS_PLATFORM_WINDOWS`）
- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS` ← `NS_PLATFORM_WINDOWS`のエイリアス）
- 01-04 プラットフォーム型（`SIZE_T`）
- 06-01 Mallocインターフェース（`Malloc`, `GMalloc`, `kDefaultAlignment`, `kMinAlignment`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/UnrealMemory.h`
- `source/engine/hal/Private/HAL/UnrealMemory.cpp`

## TODO

- [ ] `Memory` 名前空間/クラス定義
- [ ] GMalloc ラッパー関数
- [ ] SystemMalloc/SystemFree（GMallocバイパス）
- [ ] 初期化関数

## 実装内容

```cpp
// UnrealMemory.h
#pragma once
#include "HAL/MemoryBase.h"

namespace NS
{
    struct Memory
    {
        static void* Malloc(SIZE_T count, uint32 alignment = kDefaultAlignment);
        static void* Realloc(void* ptr, SIZE_T count, uint32 alignment = kDefaultAlignment);
        static void Free(void* ptr);
        static SIZE_T GetAllocSize(void* ptr);

        static void* MallocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);
        static SIZE_T QuantizeSize(SIZE_T count, uint32 alignment = kDefaultAlignment);

        static void* SystemMalloc(SIZE_T size);
        static void SystemFree(void* ptr);

        static void Trim(bool trimThreadCaches = true);

        static bool TestMemory();
    };
}

// UnrealMemory.cpp
namespace NS
{
    void* Memory::Malloc(SIZE_T count, uint32 alignment)
    {
        return GMalloc->Alloc(count, alignment);
    }

    void Memory::Free(void* ptr)
    {
        if (ptr)
        {
            GMalloc->Free(ptr);
        }
    }

    void* Memory::SystemMalloc(SIZE_T size)
    {
#if PLATFORM_WINDOWS
        return _aligned_malloc(size, kMinAlignment);
#else
        return aligned_alloc(kMinAlignment, size);
#endif
    }

    void Memory::SystemFree(void* ptr)
    {
#if PLATFORM_WINDOWS
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}
```

## 検証

- Memory::Malloc/Free が GMalloc 経由で動作
- SystemMalloc/SystemFree が直接OS経由で動作
- `Memory::MallocZeroed()` がゼロ初期化されたメモリを返す

## 注意事項

- `SystemMalloc`/`SystemFree` は `GMalloc` 初期化前に使用可能
- エンジン起動初期段階での使用を想定
- `Trim()` はメモリプレッシャー時に呼び出す

## 次のサブ計画

→ 07-01: LLMタグシステム
