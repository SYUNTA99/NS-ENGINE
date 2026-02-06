# 06-02a: MallocAnsiクラス定義

## 目的

標準Cライブラリベースのアロケータクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part6.md](docs/Platform_Abstraction_Layer_Part6.md) - セクション2「標準アロケータ」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 06-01a Malloc基底クラス
- 06-01b GMallocグローバル定義

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/MallocAnsi.h`

## TODO

- [ ] `MallocAnsi` クラス宣言
- [ ] 必要なメソッドオーバーライド宣言
- [ ] プライベートヘルパー関数宣言

## 実装内容

```cpp
// MallocAnsi.h
#pragma once

#include "HAL/MemoryBase.h"

namespace NS
{
    /// 標準Cライブラリベースのアロケータ
    ///
    /// aligned_alloc / _aligned_malloc を使用した基本的なアロケータ。
    /// 他のアロケータが使えない場合のフォールバックとして使用。
    class MallocAnsi : public Malloc
    {
    public:
        MallocAnsi() = default;
        ~MallocAnsi() override = default;

        // Malloc インターフェース実装
        void* Alloc(SIZE_T count, uint32 alignment = kDefaultAlignment) override;
        void* TryAlloc(SIZE_T count, uint32 alignment = kDefaultAlignment) override;
        void* Realloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment) override;
        void Free(void* ptr) override;

        bool GetAllocationSize(void* ptr, SIZE_T& outSize) override;

        const TCHAR* GetDescriptiveName() override { return TEXT("MallocAnsi"); }
        bool IsInternallyThreadSafe() const override { return true; }

    private:
        /// 実際のアライメントを計算
        /// @param count 要求サイズ
        /// @param alignment 要求アライメント（0の場合は自動決定）
        /// @return 使用するアライメント
        uint32 GetActualAlignment(SIZE_T count, uint32 alignment) const;
    };
}
```

## 検証

- `MallocAnsi` クラスがコンパイル可能
- `Malloc` を正しく継承

## 次のサブ計画

→ 06-02b: MallocAnsi実装
