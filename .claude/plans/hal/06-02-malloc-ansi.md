> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [06-02a: MallocAnsiクラス定義](06-02a-malloc-ansi-class.md)
> - [06-02b: MallocAnsi実装](06-02b-malloc-ansi-impl.md)

# 06-02: MallocAnsi 実装（旧版）

## 目的

標準Cライブラリを使用したフォールバックアロケータを実装する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション2「標準アロケータ」

## 依存（commonモジュール）

- `common/utility/macros.h` - プラットフォーム検出（`NS_PLATFORM_WINDOWS`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_WINDOWS` ← `NS_PLATFORM_WINDOWS`のエイリアス）
- 01-04 プラットフォーム型（`SIZE_T`, `uint32`, `TCHAR`, `TEXT()`）
- 06-01 Mallocインターフェース（`Malloc`, `kDefaultAlignment`, `kMinAlignment`）

## 必要なヘッダ

- `<cstdlib>` - `aligned_alloc`, `free`（非Windows）
- `<malloc.h>` - `_aligned_malloc`, `_aligned_free`（Windows）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/MallocAnsi.h`
- `source/engine/hal/Private/HAL/MallocAnsi.cpp`

## TODO

- [ ] `MallocAnsi` クラス定義
- [ ] aligned_alloc / _aligned_malloc 使用
- [ ] アライメント対応実装
- [ ] サイズ追跡（オプション）

## 実装内容

```cpp
// MallocAnsi.h
#pragma once
#include "HAL/MemoryBase.h"

namespace NS
{
    class MallocAnsi : public Malloc
    {
    public:
        void* Alloc(SIZE_T count, uint32 alignment = kDefaultAlignment) override;
        void* Realloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment) override;
        void Free(void* ptr) override;

        bool GetAllocationSize(void* ptr, SIZE_T& outSize) override;

        const TCHAR* GetDescriptiveName() override { return TEXT("MallocAnsi"); }
        bool IsInternallyThreadSafe() const override { return true; }

    private:
        uint32 GetActualAlignment(SIZE_T count, uint32 alignment);
    };
}

// MallocAnsi.cpp
namespace NS
{
    void* MallocAnsi::Alloc(SIZE_T count, uint32 alignment)
    {
        uint32 actualAlignment = GetActualAlignment(count, alignment);

#if PLATFORM_WINDOWS
        return _aligned_malloc(count, actualAlignment);
#else
        return aligned_alloc(actualAlignment, count);
#endif
    }

    void MallocAnsi::Free(void* ptr)
    {
        if (ptr)
        {
#if PLATFORM_WINDOWS
            _aligned_free(ptr);
#else
            free(ptr);
#endif
        }
    }

    uint32 MallocAnsi::GetActualAlignment(SIZE_T count, uint32 alignment)
    {
        if (alignment == kDefaultAlignment)
        {
            return (count >= 16) ? 16 : kMinAlignment;
        }
        return alignment;
    }
}
```

## 検証

- 様々なサイズでの割り当て/解放が正常動作
- アライメントが正しく適用される
- `GetDescriptiveName()` が "MallocAnsi" を返す

## 注意事項

- Windowsでは `_aligned_malloc`/`_aligned_free` を使用
- 非Windowsでは `aligned_alloc`/`free` を使用
- `Realloc` は `_aligned_realloc` ではなく新規確保+コピー+解放が安全

## 次のサブ計画

→ 06-03: Memory API（統一メモリAPI）
