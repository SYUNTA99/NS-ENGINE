# 10-02a: アライメントテンプレート

## 目的

メモリアライメント用のテンプレートクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション2「アライメントユーティリティ」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`, `uint8`, `uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`, `UPTRINT`）
- 02-03 関数呼び出し規約（`FORCEINLINE`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/AlignmentTemplates.h`

## TODO

- [ ] `TTypeCompatibleBytes` テンプレート定義
- [ ] `TAlignedBytes` テンプレート定義
- [ ] `Align` / `AlignDown` 関数
- [ ] `IsAligned` 関数
- [ ] `AlignPtr` 関数

## 実装内容

```cpp
// AlignmentTemplates.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PlatformMacros.h"
#include <type_traits>
#include <new>

namespace NS
{
    /// 型Tと同じサイズ・アライメントのバイト配列
    ///
    /// 遅延初期化やunion代替に使用。
    template<typename T>
    struct TTypeCompatibleBytes
    {
        alignas(T) uint8 pad[sizeof(T)];

        /// 型付きポインタ取得
        T* GetTypedPtr() { return reinterpret_cast<T*>(pad); }
        const T* GetTypedPtr() const { return reinterpret_cast<const T*>(pad); }

        /// 値への参照取得（未チェック）
        T& GetUnchecked() { return *GetTypedPtr(); }
        const T& GetUnchecked() const { return *GetTypedPtr(); }

        /// 配置new（未チェック）
        template<typename... Args>
        void EmplaceUnchecked(Args&&... args)
        {
            new (pad) T(std::forward<Args>(args)...);
        }

        /// デストラクタ呼び出し（未チェック）
        void DestroyUnchecked()
        {
            GetTypedPtr()->~T();
        }
    };

    /// 指定サイズ・アライメントのバイト配列
    ///
    /// 固定サイズバッファに使用。
    template<int32 Size, uint32 Alignment>
    struct TAlignedBytes
    {
        alignas(Alignment) uint8 pad[Size];

        void* GetData() { return pad; }
        const void* GetData() const { return pad; }
    };

    /// 2の累乗かどうか
    template<typename T>
    FORCEINLINE constexpr bool IsPowerOfTwo(T value)
    {
        static_assert(std::is_integral_v<T>, "IsPowerOfTwo only supports integral types");
        return value > 0 && (value & (value - 1)) == 0;
    }

    /// 値をアライメント境界に切り上げ
    ///
    /// @param value 切り上げる値
    /// @param alignment アライメント（2の累乗でなければならない）
    /// @return アライメント境界に切り上げた値
    template<typename T>
    FORCEINLINE constexpr T Align(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                      "Align only supports integral and pointer types");
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return static_cast<T>(((static_cast<uint64>(value) + alignment - 1) & ~(alignment - 1)));
    }

    /// コンパイル時アライメント切り上げ（アライメント検証付き）
    template<typename T, uint64 Alignment>
    FORCEINLINE constexpr T AlignConstexpr(T value)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                      "AlignConstexpr only supports integral and pointer types");
        static_assert(Alignment > 0 && (Alignment & (Alignment - 1)) == 0,
                      "Alignment must be a power of two");
        return static_cast<T>(((static_cast<uint64>(value) + Alignment - 1) & ~(Alignment - 1)));
    }

    /// 値をアライメント境界に切り下げ
    ///
    /// @param value 切り下げる値
    /// @param alignment アライメント（2の累乗でなければならない）
    /// @return アライメント境界に切り下げた値
    template<typename T>
    FORCEINLINE constexpr T AlignDown(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                      "AlignDown only supports integral and pointer types");
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return static_cast<T>(static_cast<uint64>(value) & ~(alignment - 1));
    }

    /// 値がアライメント境界にあるか
    ///
    /// @param value 検査する値
    /// @param alignment アライメント（2の累乗でなければならない）
    /// @return アライメント境界にあればtrue
    template<typename T>
    FORCEINLINE constexpr bool IsAligned(T value, uint64 alignment)
    {
        static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                      "IsAligned only supports integral and pointer types");
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return (static_cast<uint64>(value) & (alignment - 1)) == 0;
    }

    /// ポインタをアライメント境界に切り上げ
    ///
    /// @param ptr 切り上げるポインタ（nullptrも許容）
    /// @param alignment アライメント（2の累乗でなければならない）
    /// @return アライメント境界に切り上げたポインタ
    FORCEINLINE void* AlignPtr(void* ptr, SIZE_T alignment)
    {
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return reinterpret_cast<void*>(Align(reinterpret_cast<UPTRINT>(ptr), alignment));
    }

    /// ポインタをアライメント境界に切り下げ
    FORCEINLINE void* AlignPtrDown(void* ptr, SIZE_T alignment)
    {
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return reinterpret_cast<void*>(AlignDown(reinterpret_cast<UPTRINT>(ptr), alignment));
    }

    /// ポインタがアライメント境界にあるか
    FORCEINLINE bool IsPtrAligned(const void* ptr, SIZE_T alignment)
    {
        NS_CHECK_MSG(IsPowerOfTwo(alignment), "Alignment must be a power of two");
        return (reinterpret_cast<UPTRINT>(ptr) & (alignment - 1)) == 0;
    }
}
```

## 検証

- `Align(15, 16)` が 16
- `AlignDown(17, 16)` が 16
- `IsAligned(16, 16)` が true
- `IsAligned(17, 16)` が false
- `IsPowerOfTwo(64)` が true
- `TTypeCompatibleBytes<int>` が `sizeof(int)` と同サイズ

## 注意事項

- アライメントは2の累乗のみ有効
- `constexpr` なのでコンパイル時計算可能
- `TTypeCompatibleBytes` の `Emplace`/`Destroy` は手動管理が必要

## 次のサブ計画

→ 10-03a: GetTypeHashテンプレート
