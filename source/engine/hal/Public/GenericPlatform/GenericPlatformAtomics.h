/// @file GenericPlatformAtomics.h
/// @brief プラットフォーム非依存のアトミック操作インターフェース
#pragma once

#include "HAL/PlatformMacros.h"
#include "common/utility/types.h"

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
    /// - **InterlockedIncrement/Decrement**: 操作**後**の値を返す
    /// - **その他**: 操作**前**の値を返す
    ///
    /// ## 使用方法
    ///
    /// このインターフェースは直接使用せず、PlatformAtomicsを使用する。
    /// 子クラス（WindowsPlatformAtomics等）でインライン実装される。
    struct GenericPlatformAtomics
    {
        // =====================================================================
        // メモリバリア
        // =====================================================================

        /// 読み取りバリア（acquire fence）
        ///
        /// このバリア以降の読み書きが、バリア以前に移動しない。
        static void ReadBarrier();

        /// 書き込みバリア（release fence）
        ///
        /// このバリア以前の読み書きが、バリア以降に移動しない。
        static void WriteBarrier();

        /// 完全メモリバリア（full fence）
        ///
        /// 全ての読み書きがバリアを超えて移動しない。
        static void MemoryBarrier();

        // =====================================================================
        // 32bit 操作
        // =====================================================================

        /// アトミックインクリメント
        /// @param value 対象変数へのポインタ（非null）
        /// @return 操作**後**の値
        static int32 InterlockedIncrement(volatile int32* value);

        /// アトミックデクリメント
        /// @param value 対象変数へのポインタ（非null）
        /// @return 操作**後**の値
        static int32 InterlockedDecrement(volatile int32* value);

        /// アトミック加算
        /// @param value 対象変数へのポインタ（非null）
        /// @param amount 加算する値
        /// @return 操作**前**の値
        static int32 InterlockedAdd(volatile int32* value, int32 amount);

        /// アトミック交換
        /// @param value 対象変数へのポインタ（非null）
        /// @param exchange 新しい値
        /// @return 操作**前**の値
        static int32 InterlockedExchange(volatile int32* value, int32 exchange);

        /// アトミック比較交換
        /// @param dest 対象変数へのポインタ（非null）
        /// @param exchange *dest == comparand の場合に設定する値
        /// @param comparand 比較する値
        /// @return 操作**前**の値（comparandと比較して成否判定）
        static int32 InterlockedCompareExchange(volatile int32* dest, int32 exchange, int32 comparand);

        /// アトミックAND
        /// @return 操作**前**の値
        static int32 InterlockedAnd(volatile int32* value, int32 andValue);

        /// アトミックOR
        /// @return 操作**前**の値
        static int32 InterlockedOr(volatile int32* value, int32 orValue);

        // =====================================================================
        // 64bit 操作
        // =====================================================================

        /// @copydoc InterlockedIncrement(volatile int32*)
        static int64 InterlockedIncrement(volatile int64* value);

        /// @copydoc InterlockedDecrement(volatile int32*)
        static int64 InterlockedDecrement(volatile int64* value);

        /// @copydoc InterlockedAdd(volatile int32*, int32)
        static int64 InterlockedAdd(volatile int64* value, int64 amount);

        /// @copydoc InterlockedExchange(volatile int32*, int32)
        static int64 InterlockedExchange(volatile int64* value, int64 exchange);

        /// @copydoc InterlockedCompareExchange(volatile int32*, int32, int32)
        static int64 InterlockedCompareExchange(volatile int64* dest, int64 exchange, int64 comparand);

        /// @copydoc InterlockedAnd(volatile int32*, int32)
        static int64 InterlockedAnd(volatile int64* value, int64 andValue);

        /// @copydoc InterlockedOr(volatile int32*, int32)
        static int64 InterlockedOr(volatile int64* value, int64 orValue);

        // =====================================================================
        // ポインタ操作
        // =====================================================================

        /// アトミックポインタ交換
        /// @param dest 対象変数へのポインタ（非null）
        /// @param exchange 新しいポインタ値
        /// @return 操作**前**のポインタ値
        static void* InterlockedExchangePtr(void** dest, void* exchange);

        /// アトミックポインタ比較交換
        /// @param dest 対象変数へのポインタ（非null）
        /// @param exchange *dest == comparand の場合に設定する値
        /// @param comparand 比較する値
        /// @return 操作**前**のポインタ値
        static void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand);

        // =====================================================================
        // ユーティリティ
        // =====================================================================

        /// アトミック読み取り（Acquire）
        ///
        /// 後続の読み書きがこの読み取りより前に移動しない。
        /// @tparam T 32bit または 64bit 整数型
        template <typename T>
        static T AtomicRead(const volatile T* src);

        /// アトミック書き込み（Release）
        ///
        /// 先行の読み書きがこの書き込みより後に移動しない。
        /// @tparam T 32bit または 64bit 整数型
        template <typename T>
        static void AtomicStore(volatile T* dest, T value);
    };
} // namespace NS
