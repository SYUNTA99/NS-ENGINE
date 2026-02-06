# 06-01b: GMallocグローバル定義

## 目的

グローバルアロケータポインタとデフォルト実装を定義する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション1「アロケータアーキテクチャ」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型

## 依存（HAL）

- 06-01a Malloc基底クラス

## 変更対象

変更:
- `source/engine/hal/Public/HAL/MemoryBase.h`（GMalloc宣言追加）

新規作成:
- `source/engine/hal/Private/HAL/MemoryBase.cpp`

## TODO

- [ ] `GMalloc` 外部変数宣言
- [ ] `Malloc` 基底クラスのデフォルト実装（cpp）
- [ ] `TryAlloc` デフォルト実装
- [ ] `Realloc` デフォルト実装
- [ ] `AllocZeroed` デフォルト実装

## 実装内容

### MemoryBase.h（追加部分）

```cpp
// MemoryBase.h 末尾に追加

namespace NS
{
    /// グローバルアロケータ（起動時に設定）
    extern Malloc* GMalloc;
}
```

### MemoryBase.cpp

```cpp
#include "HAL/MemoryBase.h"
#include <cstring>  // memset, memcpy

namespace NS
{
    // グローバルアロケータ（初期はnullptr、起動時に設定）
    Malloc* GMalloc = nullptr;

    void* Malloc::TryAlloc(SIZE_T count, uint32 alignment)
    {
        // デフォルト実装：Allocをそのまま呼ぶ（派生クラスでオーバーライド推奨）
        return Alloc(count, alignment);
    }

    void* Malloc::Realloc(void* ptr, SIZE_T newCount, uint32 alignment)
    {
        if (ptr == nullptr)
        {
            return Alloc(newCount, alignment);
        }

        if (newCount == 0)
        {
            Free(ptr);
            return nullptr;
        }

        // デフォルト実装：新規確保 + コピー + 解放
        void* newPtr = Alloc(newCount, alignment);
        if (newPtr)
        {
            SIZE_T oldSize = 0;
            if (GetAllocationSize(ptr, oldSize))
            {
                SIZE_T copySize = (oldSize < newCount) ? oldSize : newCount;
                std::memcpy(newPtr, ptr, copySize);
            }
            Free(ptr);
        }
        return newPtr;
    }

    void* Malloc::AllocZeroed(SIZE_T count, uint32 alignment)
    {
        void* ptr = Alloc(count, alignment);
        if (ptr)
        {
            std::memset(ptr, 0, count);
        }
        return ptr;
    }

    SIZE_T Malloc::QuantizeSize(SIZE_T count, uint32 alignment)
    {
        // デフォルト：量子化なし
        return count;
    }

    bool Malloc::GetAllocationSize(void* ptr, SIZE_T& outSize)
    {
        // デフォルト：サイズ取得不可
        outSize = 0;
        return false;
    }

    bool Malloc::ValidateHeap()
    {
        // デフォルト：常に有効
        return true;
    }

    void Malloc::Trim(bool trimThreadCaches)
    {
        // デフォルト：何もしない
    }
}
```

## 検証

- `GMalloc` がnullptrで初期化される
- `Realloc(nullptr, size)` が `Alloc` と同等
- `Realloc(ptr, 0)` が `Free` と同等

## 注意事項

- `GMalloc` は起動時に適切なアロケータを設定する必要がある
- 設定前にアクセスするとnullptrデリファレンス

## 次のサブ計画

→ 06-02a: MallocAnsiクラス定義
