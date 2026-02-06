> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [10-02a: アライメントテンプレート](10-02a-alignment-templates.md)

# 10-02: アライメントユーティリティ（旧版）

## 目的

メモリアライメント関連のユーティリティを実装する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション2「メモリアライメントユーティリティ」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`, `uint8`, `uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`, `UPTRINT`）
- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/AlignmentTemplates.h`

## TODO

- [ ] `TTypeCompatibleBytes` テンプレート
- [ ] `TAlignedBytes` テンプレート
- [ ] `Align` / `AlignDown` 関数
- [ ] `IsAligned` 関数

## 実装内容

```cpp
// AlignmentTemplates.h
#pragma once

namespace NS
{
    template<typename T>
    struct TTypeCompatibleBytes
    {
        alignas(T) uint8 pad[sizeof(T)];

        T* GetTypedPtr() { return reinterpret_cast<T*>(pad); }
        const T* GetTypedPtr() const { return reinterpret_cast<const T*>(pad); }

        T& GetUnchecked() { return *GetTypedPtr(); }
        const T& GetUnchecked() const { return *GetTypedPtr(); }

        template<typename... Args>
        void EmplaceUnchecked(Args&&... args)
        {
            new (pad) T(std::forward<Args>(args)...);
        }

        void DestroyUnchecked()
        {
            GetTypedPtr()->~T();
        }
    };

    template<int32 Size, uint32 Alignment>
    struct TAlignedBytes
    {
        alignas(Alignment) uint8 pad[Size];
    };

    template<typename T>
    FORCEINLINE constexpr T Align(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>);
        return (T)(((uint64)value + alignment - 1) & ~(alignment - 1));
    }

    template<typename T>
    FORCEINLINE constexpr T AlignDown(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>);
        return (T)((uint64)value & ~(alignment - 1));
    }

    template<typename T>
    FORCEINLINE constexpr bool IsAligned(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>);
        return ((uint64)value & (alignment - 1)) == 0;
    }

    FORCEINLINE void* AlignPtr(void* ptr, SIZE_T alignment)
    {
        return (void*)Align((UPTRINT)ptr, alignment);
    }
}
```

## 検証

- Align(15, 16) == 16
- AlignDown(17, 16) == 16
- IsAligned(16, 16) == true
- `TTypeCompatibleBytes<T>` が正しいサイズとアライメント

## 注意事項

- アライメントは2の累乗のみ有効
- `TTypeCompatibleBytes` は遅延初期化に使用
- `constexpr` なのでコンパイル時計算可能

## 次のサブ計画

→ 10-03: ハッシュ関数
