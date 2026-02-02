//----------------------------------------------------------------------------
//! @file   allocator.h
//! @brief  メモリアロケータ基底インターフェース
//----------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

namespace Memory {

//============================================================================
//! @brief アロケータ統計情報
//!
//! 各アロケータのメモリ使用状況を追跡する。
//============================================================================
struct AllocatorStats {
    size_t totalAllocated = 0;      //!< 累計確保サイズ（バイト）
    size_t currentUsed = 0;         //!< 現在使用中サイズ（バイト）
    size_t peakUsed = 0;            //!< ピーク使用量（バイト）
    size_t allocationCount = 0;     //!< 確保回数
    size_t deallocationCount = 0;   //!< 解放回数

    //! @brief 確保を記録
    void RecordAllocation(size_t size) noexcept {
        totalAllocated += size;
        currentUsed += size;
        if (currentUsed > peakUsed) {
            peakUsed = currentUsed;
        }
        ++allocationCount;
    }

    //! @brief 解放を記録
    void RecordDeallocation(size_t size) noexcept {
        currentUsed -= size;
        ++deallocationCount;
    }

    //! @brief 統計をリセット
    void Reset() noexcept {
        totalAllocated = 0;
        currentUsed = 0;
        peakUsed = 0;
        allocationCount = 0;
        deallocationCount = 0;
    }
};

//============================================================================
//! @brief アロケータ基底インターフェース
//!
//! 全てのカスタムアロケータはこのインターフェースを実装する。
//! 統一されたAPIでメモリ確保/解放と統計収集を行う。
//============================================================================
class IAllocator {
public:
    virtual ~IAllocator() = default;

    //------------------------------------------------------------------------
    // 基本操作
    //------------------------------------------------------------------------

    //------------------------------------------------------------------------
    //! @brief メモリ確保
    //! @param size 確保サイズ（バイト）
    //! @param alignment アラインメント要件（デフォルト: 最大基本アラインメント）
    //! @return 確保されたメモリへのポインタ。失敗時はnullptr
    //------------------------------------------------------------------------
    [[nodiscard]] virtual void* Allocate(
        size_t size,
        size_t alignment = alignof(std::max_align_t)) = 0;

    //------------------------------------------------------------------------
    //! @brief メモリ解放
    //! @param ptr 解放するポインタ
    //! @param size 確保時のサイズ（統計用、一部アロケータでは必須）
    //------------------------------------------------------------------------
    virtual void Deallocate(void* ptr, size_t size) = 0;

    //------------------------------------------------------------------------
    // 情報取得
    //------------------------------------------------------------------------

    //------------------------------------------------------------------------
    //! @brief アロケータ名取得（デバッグ用）
    //! @return アロケータ名の文字列
    //------------------------------------------------------------------------
    [[nodiscard]] virtual const char* GetName() const noexcept = 0;

    //------------------------------------------------------------------------
    //! @brief 統計情報取得
    //! @return 現在の統計情報
    //------------------------------------------------------------------------
    [[nodiscard]] virtual AllocatorStats GetStats() const noexcept = 0;

    //------------------------------------------------------------------------
    // オプション操作
    //------------------------------------------------------------------------

    //------------------------------------------------------------------------
    //! @brief メモリをリセット（LinearAllocator等で使用）
    //!
    //! デフォルト実装は何もしない。
    //------------------------------------------------------------------------
    virtual void Reset() {}

    //------------------------------------------------------------------------
    //! @brief 指定ポインタがこのアロケータで確保されたか確認
    //! @param ptr 確認するポインタ
    //! @return このアロケータで確保された場合はtrue
    //!
    //! デフォルト実装は常にfalseを返す。
    //------------------------------------------------------------------------
    [[nodiscard]] virtual bool Owns([[maybe_unused]] void* ptr) const noexcept {
        return false;
    }

protected:
    //------------------------------------------------------------------------
    //! @brief アラインメントを適用したポインタを計算
    //! @param ptr 元のポインタ
    //! @param alignment アラインメント要件
    //! @return アラインメント調整後のポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] static void* AlignPointer(void* ptr, size_t alignment) noexcept {
        const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void*>(aligned);
    }

    //------------------------------------------------------------------------
    //! @brief アラインメント調整に必要なバイト数を計算
    //! @param ptr 元のポインタ
    //! @param alignment アラインメント要件
    //! @return 必要な追加バイト数
    //------------------------------------------------------------------------
    [[nodiscard]] static size_t AlignmentAdjustment(void* ptr, size_t alignment) noexcept {
        const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
        return static_cast<size_t>(aligned - addr);
    }
};

} // namespace Memory
