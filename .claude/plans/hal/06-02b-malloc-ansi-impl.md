# 06-02b: MallocAnsi実装

## 目的

MallocAnsiクラスのプラットフォーム固有実装を行う。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション2「標準アロケータ」

## 依存（commonモジュール）

- `common/utility/macros.h` - プラットフォーム検出（`NS_PLATFORM_WINDOWS`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS`）
- 06-02a MallocAnsiクラス定義

## 必要なヘッダ

- `<cstdlib>` - `aligned_alloc`, `free`（非Windows）
- `<malloc.h>` - `_aligned_malloc`, `_aligned_free`, `_aligned_msize`（Windows）

## 変更対象

新規作成:
- `source/engine/hal/Private/HAL/MallocAnsi.cpp`

## TODO

- [ ] `Alloc` 実装（プラットフォーム分岐）
- [ ] `TryAlloc` 実装
- [ ] `Realloc` 実装（プラットフォーム分岐）
- [ ] `Free` 実装（プラットフォーム分岐）
- [ ] `GetAllocationSize` 実装（Windows: `_aligned_msize`）
- [ ] `GetActualAlignment` 実装

## 実装内容

```cpp
// MallocAnsi.cpp
#include "HAL/MallocAnsi.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace NS
{
    void* MallocAnsi::Alloc(SIZE_T count, uint32 alignment)
    {
        if (count == 0)
        {
            return nullptr;
        }

        uint32 actualAlignment = GetActualAlignment(count, alignment);

#if PLATFORM_WINDOWS
        return _aligned_malloc(count, actualAlignment);
#else
        // C11 aligned_alloc はサイズがアライメントの倍数である必要がある
        SIZE_T alignedCount = (count + actualAlignment - 1) & ~(actualAlignment - 1);
        return aligned_alloc(actualAlignment, alignedCount);
#endif
    }

    void* MallocAnsi::TryAlloc(SIZE_T count, uint32 alignment)
    {
        // 標準Cライブラリは失敗時にnullptrを返すので同じ実装
        return Alloc(count, alignment);
    }

    void* MallocAnsi::Realloc(void* ptr, SIZE_T newCount, uint32 alignment)
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

        uint32 actualAlignment = GetActualAlignment(newCount, alignment);

#if PLATFORM_WINDOWS
        return _aligned_realloc(ptr, newCount, actualAlignment);
#else
        // 非Windowsでは新規確保 + コピー + 解放
        void* newPtr = Alloc(newCount, alignment);
        if (newPtr)
        {
            // 古いサイズが不明なので、新しいサイズ分だけコピー
            // （実際には古いサイズが小さい場合に問題になる可能性）
            std::memcpy(newPtr, ptr, newCount);
            Free(ptr);
        }
        return newPtr;
#endif
    }

    void MallocAnsi::Free(void* ptr)
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

    bool MallocAnsi::GetAllocationSize(void* ptr, SIZE_T& outSize)
    {
        if (ptr == nullptr)
        {
            outSize = 0;
            return true;
        }

#if PLATFORM_WINDOWS
        outSize = _aligned_msize(ptr, kMinAlignment, 0);
        return true;
#else
        // 非Windowsではサイズ取得不可
        outSize = 0;
        return false;
#endif
    }

    uint32 MallocAnsi::GetActualAlignment(SIZE_T count, uint32 alignment) const
    {
        if (alignment == kDefaultAlignment)
        {
            // サイズに応じてアライメントを決定
            // 16バイト以上は16バイトアライメント（SIMD対応）
            if (count >= 16)
            {
                return 16;
            }
            return kMinAlignment;
        }

        // 最低でも kMinAlignment
        return (alignment < kMinAlignment) ? kMinAlignment : alignment;
    }
}
```

## 検証

- `MallocAnsi::Alloc(100, 0)` が非nullを返す
- `MallocAnsi::Free()` がクラッシュしない
- `MallocAnsi::GetAllocationSize()` が正しいサイズを返す（Windows）
- アライメントが正しく適用される（`Alloc(100, 64)` が64バイト境界）

## 注意事項

- `_aligned_realloc` はWindowsのみ
- 非Windowsの `Realloc` は古いサイズが不明なため注意が必要
- `aligned_alloc` はサイズがアライメントの倍数である必要がある（C11仕様）

## 次のサブ計画

→ 06-03a: Memory静的API
