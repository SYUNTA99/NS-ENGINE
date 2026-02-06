> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [06-01a: Malloc基底クラス](06-01a-malloc-base.md)
> - [06-01b: GMallocグローバル](06-01b-gmalloc-global.md)

# 06-01: Malloc インターフェース（旧版）

## 目的

メモリアロケータの基底インターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション1-2「アロケータアーキテクチャ」

## ドキュメントとの差異

| UE5 (ドキュメント) | NS-ENGINE | 理由 |
|-------------------|-----------|------|
| `FMalloc` | `Malloc` | NS命名規則（`F`プレフィックス不使用） |
| `Malloc()` | `Alloc()` | クラス名との衝突回避、C++標準的命名 |
| `TryMalloc()` | `TryAlloc()` | 同上 |

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`, `TCHAR`, `TEXT()`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/MemoryBase.h`

## TODO

- [ ] `Malloc` 基底クラス定義
- [ ] アライメント定数定義
- [ ] AllocationHints 列挙型
- [ ] 仮想メソッド宣言

## 実装内容

```cpp
// MemoryBase.h
#pragma once

namespace NS
{
    constexpr uint32 kDefaultAlignment = 0;
    constexpr uint32 kMinAlignment = 8;

    enum class AllocationHints
    {
        None = -1,
        Default = 0,
        Temporary = 1,
        SmallPool = 2
    };

    class Malloc
    {
    public:
        virtual ~Malloc() = default;

        virtual void* Alloc(SIZE_T count, uint32 alignment = kDefaultAlignment) = 0;
        virtual void* TryAlloc(SIZE_T count, uint32 alignment = kDefaultAlignment);
        virtual void* Realloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment);
        virtual void Free(void* ptr) = 0;

        virtual void* AllocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);

        virtual SIZE_T QuantizeSize(SIZE_T count, uint32 alignment);
        virtual bool GetAllocationSize(void* ptr, SIZE_T& outSize);

        virtual bool ValidateHeap();
        virtual void Trim(bool trimThreadCaches);

        virtual const TCHAR* GetDescriptiveName() = 0;
        virtual bool IsInternallyThreadSafe() const { return false; }
    };

    extern Malloc* GMalloc;
}
```

## 検証

- Malloc インターフェースが正しく定義される
- 仮想メソッドが適切に宣言される
- `GMalloc` ポインタが宣言される

## 注意事項

- `Alloc()` は失敗時にnullptrではなくクラッシュすることがある（実装依存）
- `TryAlloc()` は失敗時にnullptrを返すことが保証される
- `GMalloc` はグローバルアロケータへのポインタ（起動時に設定）

## 次のサブ計画

→ 06-02: MallocAnsi（標準Cアロケータ実装）
