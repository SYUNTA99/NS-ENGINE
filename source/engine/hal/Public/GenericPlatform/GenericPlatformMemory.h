/// @file GenericPlatformMemory.h
/// @brief プラットフォーム非依存のメモリ管理インターフェース
/// @details メモリ統計、定数、OS直接割り当てAPIを提供する。
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// メモリ使用状況の統計情報
    ///
    /// GetStats()から返される値はスナップショット。
    /// マルチスレッド環境では値が即座に古くなる可能性がある。
    struct PlatformMemoryStats
    {
        uint64 availablePhysical; ///< 利用可能な物理メモリ（バイト）
        uint64 availableVirtual;  ///< 利用可能な仮想メモリ（バイト）
        uint64 usedPhysical;      ///< 使用中の物理メモリ（バイト）
        uint64 usedVirtual;       ///< 使用中の仮想メモリ（バイト）
        uint64 peakUsedPhysical;  ///< ピーク物理メモリ使用量（バイト）
        uint64 peakUsedVirtual;   ///< ピーク仮想メモリ使用量（バイト）

        /// デフォルト初期化
        PlatformMemoryStats()
            : availablePhysical(0), availableVirtual(0), usedPhysical(0), usedVirtual(0), peakUsedPhysical(0),
              peakUsedVirtual(0)
        {}
    };

    /// システムメモリの定数情報
    ///
    /// Init()後は不変。スレッドセーフに読み取り可能。
    struct PlatformMemoryConstants
    {
        uint64 totalPhysical;         ///< 総物理メモリ（バイト）
        uint64 totalVirtual;          ///< 総仮想メモリ（バイト）
        SIZE_T pageSize;              ///< ページサイズ（通常4096）
        SIZE_T allocationGranularity; ///< 割り当て粒度（Windowsは64KB）
        SIZE_T cacheLineSize;         ///< CPUキャッシュラインサイズ（通常64）
        uint32 numberOfCores;         ///< 物理コア数
        uint32 numberOfThreads;       ///< 論理スレッド数（ハイパースレッディング含む）

        /// デフォルト初期化
        PlatformMemoryConstants()
            : totalPhysical(0), totalVirtual(0), pageSize(0), allocationGranularity(0), cacheLineSize(0),
              numberOfCores(0), numberOfThreads(0)
        {}
    };

    /// プラットフォーム非依存のメモリ管理インターフェース
    ///
    /// ## スレッドセーフティ
    ///
    /// - **Init()**: メインスレッドから一度だけ呼び出す（起動時）
    /// - **GetStats()**: スレッドセーフ（内部で同期）
    /// - **GetConstants()**: スレッドセーフ（Init後は読み取り専用）
    /// - **BinnedAllocFromOS/BinnedFreeToOS**: スレッドセーフ（OS APIが保証）
    /// - **VirtualReserve/Commit/Decommit/Free**: スレッドセーフ（OS APIが保証）
    ///
    /// ## 初期化順序
    ///
    /// 1. エンジン起動時に `Init()` を呼び出す
    /// 2. `GetConstants()` は `Init()` 後のみ有効な値を返す
    /// 3. `Init()` 前に `GetConstants()` を呼ぶとゼロ初期化された値が返る
    struct GenericPlatformMemory
    {
        // =====================================================================
        // 初期化
        // =====================================================================

        /// 初期化（起動時に一度だけ呼び出す）
        ///
        /// @note メインスレッドから呼び出すこと
        /// @note 二重呼び出しは安全（何もしない）
        static void Init();

        /// 初期化済みかどうか
        ///
        /// @return 初期化済みならtrue
        static bool IsInitialized();

        // =====================================================================
        // 統計・定数取得
        // =====================================================================

        /// メモリ統計取得
        ///
        /// @return 現在のメモリ使用状況（スナップショット）
        /// @note スレッドセーフ
        /// @note OSに問い合わせるため、頻繁な呼び出しは避ける
        static PlatformMemoryStats GetStats();

        /// メモリ定数取得
        ///
        /// @return システムメモリ定数への参照
        /// @note Init()後は値が不変、スレッドセーフ
        /// @warning Init()前はゼロ初期化された値
        static const PlatformMemoryConstants& GetConstants();

        // =====================================================================
        // OS直接割り当て（Binned Allocator向け）
        // =====================================================================

        /// OS直接割り当て（大きなブロック用）
        ///
        /// @param size 割り当てサイズ（pageSize単位に切り上げられる）
        /// @return 割り当てたメモリ（pageSize境界にアライン）、失敗時nullptr
        /// @note スレッドセーフ
        static void* BinnedAllocFromOS(SIZE_T size);

        /// OS直接解放
        ///
        /// @param ptr BinnedAllocFromOSで取得したポインタ
        /// @param size 割り当て時のサイズ（0でも可、Windowsでは無視される）
        /// @note スレッドセーフ
        /// @note ptr==nullptrは安全（何もしない）
        static void BinnedFreeToOS(void* ptr, SIZE_T size);

        // =====================================================================
        // 仮想メモリ管理
        // =====================================================================

        /// 仮想メモリ予約（コミットなし）
        ///
        /// アドレス空間を予約するが、物理メモリは割り当てない。
        /// 大きなメモリプールの事前確保に使用。
        ///
        /// @param size 予約サイズ（allocationGranularity単位に切り上げ）
        /// @return 予約したアドレス、失敗時nullptr
        /// @note スレッドセーフ
        static void* VirtualReserve(SIZE_T size);

        /// 仮想メモリコミット
        ///
        /// 予約済みアドレス範囲に物理メモリを割り当てる。
        ///
        /// @param ptr VirtualReserveで取得したアドレス（またはその範囲内）
        /// @param size コミットサイズ（pageSize単位に切り上げ）
        /// @return 成功時true
        /// @note スレッドセーフ
        static bool VirtualCommit(void* ptr, SIZE_T size);

        /// 仮想メモリデコミット（予約は維持）
        ///
        /// 物理メモリを解放するが、アドレス空間の予約は維持する。
        /// 後で再度VirtualCommitで使用可能。
        ///
        /// @param ptr コミット済みアドレス
        /// @param size デコミットサイズ
        /// @return 成功時true
        /// @note スレッドセーフ
        static bool VirtualDecommit(void* ptr, SIZE_T size);

        /// 仮想メモリ解放（予約も解放）
        ///
        /// アドレス空間の予約を含めて完全に解放する。
        ///
        /// @param ptr VirtualReserveで取得したポインタ
        /// @param size 予約時のサイズ（Windowsでは無視される）
        /// @note スレッドセーフ
        /// @note ptr==nullptrは安全（何もしない）
        static void VirtualFree(void* ptr, SIZE_T size);

    protected:
        static bool s_initialized;
        static PlatformMemoryConstants s_constants;
    };
} // namespace NS
