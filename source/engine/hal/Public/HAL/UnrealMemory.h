/// @file UnrealMemory.h
/// @brief 統一メモリAPI
#pragma once

#include "HAL/MemoryBase.h"

namespace NS
{
    /// 統一メモリAPI
    ///
    /// GMalloc経由のメモリ操作と、GMallocをバイパスする
    /// システムメモリ操作の両方を提供する。
    ///
    /// ## 使用パターン
    ///
    /// @code
    /// // 通常のメモリ操作（GMalloc経由）
    /// void* ptr = Memory::Malloc(1024);
    /// Memory::Free(ptr);
    ///
    /// // ゼロ初期化付き
    /// void* zeroed = Memory::MallocZeroed(1024);
    ///
    /// // GMalloc初期化前（起動初期段階）
    /// void* early = Memory::SystemMalloc(1024);
    /// Memory::SystemFree(early);
    /// @endcode
    struct Memory
    {
        // =====================================================================
        // GMalloc経由のメモリ操作
        // =====================================================================

        /// メモリ割り当て
        ///
        /// @param count 割り当てバイト数
        /// @param alignment アライメント（0=自動）
        /// @return 割り当てたメモリ、失敗時nullptr
        static void* Malloc(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// メモリ再割り当て
        ///
        /// @param ptr 既存のポインタ（nullptrの場合はMallocと同等）
        /// @param count 新しいサイズ
        /// @param alignment アライメント
        /// @return 再割り当てしたメモリ
        static void* Realloc(void* ptr, SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// メモリ解放
        ///
        /// @param ptr 解放するポインタ（nullptrは無視）
        static void Free(void* ptr);

        /// 割り当てサイズ取得
        ///
        /// @param ptr 割り当て済みポインタ
        /// @return 割り当てサイズ（取得不可の場合0）
        static SIZE_T GetAllocSize(void* ptr);

        /// ゼロ初期化付き割り当て
        ///
        /// @param count 割り当てバイト数
        /// @param alignment アライメント
        /// @return ゼロ初期化されたメモリ
        static void* MallocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// サイズ量子化
        ///
        /// @param count 要求サイズ
        /// @param alignment アライメント
        /// @return 実際に割り当てられるサイズ
        static SIZE_T QuantizeSize(SIZE_T count, uint32 alignment = kDefaultAlignment);

        // =====================================================================
        // GMallocバイパス（起動初期段階用）
        // =====================================================================

        /// システムメモリ割り当て（GMalloc不使用）
        ///
        /// GMalloc初期化前に使用可能。
        /// @param size 割り当てバイト数
        /// @return 割り当てたメモリ
        static void* SystemMalloc(SIZE_T size);

        /// システムメモリ解放（GMalloc不使用）
        ///
        /// SystemMallocで確保したメモリを解放。
        /// @param ptr 解放するポインタ
        static void SystemFree(void* ptr);

        // =====================================================================
        // ユーティリティ
        // =====================================================================

        /// メモリトリム
        ///
        /// 未使用メモリをOSに返却。
        /// @param trimThreadCaches TLSキャッシュもトリムするか
        static void Trim(bool trimThreadCaches = true);

        /// ヒープ検証
        ///
        /// @return ヒープが正常ならtrue
        static bool TestMemory();

        /// GMalloc初期化済みかどうか
        static bool IsGMallocReady();
    };
} // namespace NS
