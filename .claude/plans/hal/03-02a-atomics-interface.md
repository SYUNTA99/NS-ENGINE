# 03-02a: アトミックインターフェース

## 目的

アトミック操作のインターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション4「アトミック操作」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`int32`, `int64`）

## 依存（HAL）

- 02-03 関数呼び出し規約（`FORCEINLINE`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformAtomics.h`

## TODO

- [ ] `MemoryOrder` 列挙型定義
- [ ] `GenericPlatformAtomics` インターフェース宣言
- [ ] 32bit アトミック操作宣言
- [ ] 64bit アトミック操作宣言
- [ ] ポインタ アトミック操作宣言
- [ ] メモリバリア関数宣言

## 実装内容

```cpp
// GenericPlatformAtomics.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformMacros.h"

namespace NS
{
    /// メモリオーダリング
    ///
    /// アトミック操作のメモリ順序制約を指定。
    /// C++11 memory_order に対応。
    enum class MemoryOrder
    {
        /// 順序制約なし（最高性能、データ競合に注意）
        Relaxed,

        /// 読み取り操作: この操作より後の読み書きが前に移動しない
        Acquire,

        /// 書き込み操作: この操作より前の読み書きが後に移動しない
        Release,

        /// Acquire + Release の両方
        AcquireRelease,

        /// 完全な逐次一貫性（最も安全、最も遅い）
        SequentiallyConsistent
    };

    /// プラットフォーム非依存のアトミック操作インターフェース
    ///
    /// ## メモリオーダリング規約
    ///
    /// - **InterlockedIncrement/Decrement**: AcquireRelease相当
    /// - **InterlockedAdd**: AcquireRelease相当
    /// - **InterlockedExchange**: AcquireRelease相当
    /// - **InterlockedCompareExchange**: 成功時AcquireRelease、失敗時Acquire
    ///
    /// ## スレッドセーフティ
    ///
    /// 全関数はスレッドセーフ。nullptrを渡した場合の動作は未定義。
    ///
    /// ## 戻り値規約
    ///
    /// 特に明記しない限り、**操作前の値**を返す。
    struct GenericPlatformAtomics
    {
        // ========== メモリバリア ==========

        /// 読み取りバリア（acquire fence）
        static FORCEINLINE void ReadBarrier();

        /// 書き込みバリア（release fence）
        static FORCEINLINE void WriteBarrier();

        /// 完全メモリバリア（full fence）
        static FORCEINLINE void MemoryBarrier();

        // ========== 32bit 操作 ==========

        /// アトミックインクリメント
        /// @param value 対象変数へのポインタ（非null）
        /// @return 操作**後**の値
        static FORCEINLINE int32 InterlockedIncrement(volatile int32* value);

        /// アトミックデクリメント
        /// @param value 対象変数へのポインタ（非null）
        /// @return 操作**後**の値
        static FORCEINLINE int32 InterlockedDecrement(volatile int32* value);

        /// アトミック加算
        /// @param value 対象変数へのポインタ（非null）
        /// @param amount 加算する値
        /// @return 操作**前**の値
        static FORCEINLINE int32 InterlockedAdd(volatile int32* value, int32 amount);

        /// アトミック交換
        /// @param value 対象変数へのポインタ（非null）
        /// @param exchange 新しい値
        /// @return 操作**前**の値
        static FORCEINLINE int32 InterlockedExchange(volatile int32* value, int32 exchange);

        /// アトミック比較交換
        /// @param dest 対象変数へのポインタ（非null）
        /// @param exchange *dest == comparand の場合に設定する値
        /// @param comparand 比較する値
        /// @return 操作**前**の値（comparandと比較して成否判定）
        static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* dest, int32 exchange, int32 comparand);

        /// アトミックAND
        /// @return 操作**前**の値
        static FORCEINLINE int32 InterlockedAnd(volatile int32* value, int32 andValue);

        /// アトミックOR
        /// @return 操作**前**の値
        static FORCEINLINE int32 InterlockedOr(volatile int32* value, int32 orValue);

        // ========== 64bit 操作 ==========

        static FORCEINLINE int64 InterlockedIncrement(volatile int64* value);
        static FORCEINLINE int64 InterlockedDecrement(volatile int64* value);
        static FORCEINLINE int64 InterlockedAdd(volatile int64* value, int64 amount);
        static FORCEINLINE int64 InterlockedExchange(volatile int64* value, int64 exchange);
        static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* dest, int64 exchange, int64 comparand);
        static FORCEINLINE int64 InterlockedAnd(volatile int64* value, int64 andValue);
        static FORCEINLINE int64 InterlockedOr(volatile int64* value, int64 orValue);

        // ========== ポインタ操作 ==========

        static FORCEINLINE void* InterlockedExchangePtr(void** dest, void* exchange);
        static FORCEINLINE void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand);

        // ========== ユーティリティ ==========

        /// アトミック読み取り（Acquire）
        template<typename T>
        static FORCEINLINE T AtomicRead(const volatile T* src);

        /// アトミック書き込み（Release）
        template<typename T>
        static FORCEINLINE void AtomicStore(volatile T* dest, T value);
    };
}
```

## 検証

- ヘッダがコンパイル可能（実装は03-02bで提供）
- Increment/Decrementが操作**後**の値を返す
- その他の操作が操作**前**の値を返す

## 注意事項

- **nullptrを渡した場合の動作は未定義**（クラッシュの可能性）
- Relaxedオーダーはロックフリーデータ構造の専門家以外は使用を避ける
- 通常はAcquireRelease相当のデフォルト関数で十分

## 次のサブ計画

→ 03-02b: Windowsアトミック実装
