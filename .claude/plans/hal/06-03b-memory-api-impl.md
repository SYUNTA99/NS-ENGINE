# 06-03b: Memory API実装

## 目的

統一メモリAPIの実装を行う。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション1「統一API層」

## 依存（commonモジュール）

- `common/utility/macros.h` - プラットフォーム検出（`NS_PLATFORM_WINDOWS`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`）
- 06-01b GMallocグローバル定義（`GMalloc`）
- 06-03a Memory静的API

## 必要なヘッダ

- `<cstdlib>` - `aligned_alloc`, `free`（非Windows）
- `<malloc.h>` - `_aligned_malloc`, `_aligned_free`（Windows）
- `<cstring>` - `memset`

## 変更対象

新規作成:
- `source/engine/hal/Private/HAL/UnrealMemory.cpp`

## TODO

- [ ] `Memory::Malloc` 実装
- [ ] `Memory::Realloc` 実装
- [ ] `Memory::Free` 実装
- [ ] `Memory::MallocZeroed` 実装
- [ ] `Memory::SystemMalloc` 実装
- [ ] `Memory::SystemFree` 実装
- [ ] `Memory::GetAllocSize` 実装
- [ ] `Memory::IsGMallocReady` 実装

## 実装内容

```cpp
// UnrealMemory.cpp
#include "HAL/UnrealMemory.h"
#include "HAL/Platform.h"
#include <cstring>

#if PLATFORM_WINDOWS
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace NS
{
    void* Memory::Malloc(SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            // GMalloc未初期化時はSystemMallocにフォールバック
            return SystemMalloc(count);
        }
        return GMalloc->Alloc(count, alignment);
    }

    void* Memory::Realloc(void* ptr, SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            // GMalloc未初期化時は未サポート
            return nullptr;
        }
        return GMalloc->Realloc(ptr, count, alignment);
    }

    void Memory::Free(void* ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        if (!GMalloc)
        {
            // GMalloc未初期化時はSystemFree
            SystemFree(ptr);
            return;
        }
        GMalloc->Free(ptr);
    }

    SIZE_T Memory::GetAllocSize(void* ptr)
    {
        if (ptr == nullptr || !GMalloc)
        {
            return 0;
        }

        SIZE_T size = 0;
        GMalloc->GetAllocationSize(ptr, size);
        return size;
    }

    void* Memory::MallocZeroed(SIZE_T count, uint32 alignment)
    {
        void* ptr = Malloc(count, alignment);
        if (ptr)
        {
            std::memset(ptr, 0, count);
        }
        return ptr;
    }

    SIZE_T Memory::QuantizeSize(SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            return count;
        }
        return GMalloc->QuantizeSize(count, alignment);
    }

    void* Memory::SystemMalloc(SIZE_T size)
    {
#if PLATFORM_WINDOWS
        return _aligned_malloc(size, kMinAlignment);
#else
        SIZE_T alignedSize = (size + kMinAlignment - 1) & ~(kMinAlignment - 1);
        return aligned_alloc(kMinAlignment, alignedSize);
#endif
    }

    void Memory::SystemFree(void* ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

#if PLATFORM_WINDOWS
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    void Memory::Trim(bool trimThreadCaches)
    {
        if (GMalloc)
        {
            GMalloc->Trim(trimThreadCaches);
        }
    }

    bool Memory::TestMemory()
    {
        if (!GMalloc)
        {
            return false;
        }
        return GMalloc->ValidateHeap();
    }

    bool Memory::IsGMallocReady()
    {
        return GMalloc != nullptr;
    }
}
```

## 検証

- `Memory::Malloc(100)` が非nullを返す（GMalloc設定後）
- `Memory::SystemMalloc(100)` が非nullを返す（GMalloc未設定でも）
- `Memory::Free(nullptr)` がクラッシュしない
- `Memory::IsGMallocReady()` が正しい状態を返す

## 注意事項

- `SystemMalloc`/`SystemFree` は `GMalloc` 初期化前に使用可能
- `Memory::Malloc` で確保したメモリを `Memory::SystemFree` で解放しないこと
- エンジン起動時は `GMalloc` を設定してから通常のメモリ操作を行う

## 次のサブ計画

→ 07-01a: LLMタグ列挙型
