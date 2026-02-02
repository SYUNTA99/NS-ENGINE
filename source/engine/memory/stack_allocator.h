//----------------------------------------------------------------------------
//! @file   stack_allocator.h
//! @brief  スタックアロケータ - LIFO順の高速確保/解放
//----------------------------------------------------------------------------
#pragma once

#include "allocator.h"
#include "heap_allocator.h"
#include <cassert>
#include <cstring>

namespace Memory {

//============================================================================
//! @brief スタックアロケータ
//!
//! LIFO（後入れ先出し）順でメモリを確保/解放する。
//! マーカー機能により、特定位置まで一括解放が可能。
//!
//! 特徴:
//! - 確保: O(1)、非常に高速
//! - 解放: LIFO順のみ可能、マーカーまで一括解放
//! - 用途: 一時的な計算バッファ、階層的なスコープ管理
//!
//! @note スレッドセーフではない（シングルスレッド使用前提）
//============================================================================
class StackAllocator final : public IAllocator {
public:
    //! スタックマーカー型
    using Marker = size_t;

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param capacity バッファ容量（バイト）
    //! @param backing バッキングアロケータ（nullptrでデフォルトHeapAllocator）
    //------------------------------------------------------------------------
    explicit StackAllocator(size_t capacity, IAllocator* backing = nullptr)
        : capacity_(capacity)
        , backing_(backing ? backing : &defaultBacking_)
    {
        assert(capacity > 0 && "Capacity must be greater than 0");

        buffer_ = static_cast<std::byte*>(
            backing_->Allocate(capacity_, alignof(std::max_align_t)));
        assert(buffer_ && "Failed to allocate buffer");
    }

    //------------------------------------------------------------------------
    //! @brief デストラクタ
    //------------------------------------------------------------------------
    ~StackAllocator() override {
        if (buffer_) {
            backing_->Deallocate(buffer_, capacity_);
            buffer_ = nullptr;
        }
    }

    // コピー/ムーブ禁止
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    //------------------------------------------------------------------------
    //! @brief メモリ確保
    //! @param size 確保サイズ（バイト）
    //! @param alignment アラインメント要件
    //! @return 確保されたメモリへのポインタ。容量不足時はnullptr
    //------------------------------------------------------------------------
    [[nodiscard]] void* Allocate(size_t size, size_t alignment) override {
        if (size == 0) {
            return nullptr;
        }

        // アラインメント調整
        const size_t alignedOffset = AlignOffset(offset_, alignment);

        // 容量チェック
        if (alignedOffset + size > capacity_) {
            assert(false && "StackAllocator: Out of memory");
            return nullptr;
        }

        void* ptr = buffer_ + alignedOffset;
        offset_ = alignedOffset + size;

#if defined(_DEBUG)
        // デバッグ用：最後の確保ポインタを記録
        lastAllocation_ = ptr;
        lastAllocationSize_ = size;
#endif

        stats_.RecordAllocation(size);
        return ptr;
    }

    //------------------------------------------------------------------------
    //! @brief メモリ解放（LIFO順のみ）
    //! @param ptr 解放するポインタ
    //! @param size 確保時のサイズ
    //!
    //! @note 最後に確保したメモリのみ解放可能。
    //!       順序が異なる場合はassertで停止（Debug）。
    //------------------------------------------------------------------------
    void Deallocate(void* ptr, size_t size) override {
        if (!ptr) {
            return;
        }

#if defined(_DEBUG)
        // LIFO順序の検証
        assert(ptr == lastAllocation_ &&
               "StackAllocator: Deallocation order violation (LIFO required)");
#endif

        // オフセットを戻す
        const uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t bufferStart = reinterpret_cast<uintptr_t>(buffer_);

        assert(ptrAddr >= bufferStart && ptrAddr < bufferStart + capacity_ &&
               "Pointer not from this allocator");

        offset_ = static_cast<size_t>(ptrAddr - bufferStart);

#if defined(_DEBUG)
        lastAllocation_ = nullptr;
        lastAllocationSize_ = 0;
#endif

        stats_.RecordDeallocation(size);
    }

    //------------------------------------------------------------------------
    //! @brief アロケータ名取得
    //------------------------------------------------------------------------
    [[nodiscard]] const char* GetName() const noexcept override {
        return "StackAllocator";
    }

    //------------------------------------------------------------------------
    //! @brief 統計情報取得
    //------------------------------------------------------------------------
    [[nodiscard]] AllocatorStats GetStats() const noexcept override {
        return stats_;
    }

    //------------------------------------------------------------------------
    //! @brief バッファをリセット（全解放）
    //------------------------------------------------------------------------
    void Reset() override {
        offset_ = 0;

#if defined(_DEBUG)
        lastAllocation_ = nullptr;
        lastAllocationSize_ = 0;
#endif

        stats_.Reset();
    }

    //------------------------------------------------------------------------
    //! @brief 指定ポインタがこのアロケータで確保されたか確認
    //! @param ptr 確認するポインタ
    //! @return このアロケータで確保された場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Owns(void* ptr) const noexcept override {
        if (!ptr || !buffer_) {
            return false;
        }

        const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t start = reinterpret_cast<uintptr_t>(buffer_);
        const uintptr_t end = start + capacity_;

        return addr >= start && addr < end;
    }

    //------------------------------------------------------------------------
    // マーカー機能
    //------------------------------------------------------------------------

    //------------------------------------------------------------------------
    //! @brief 現在位置のマーカーを取得
    //! @return 現在のオフセット（マーカー）
    //------------------------------------------------------------------------
    [[nodiscard]] Marker GetMarker() const noexcept {
        return offset_;
    }

    //------------------------------------------------------------------------
    //! @brief マーカー位置まで解放
    //! @param marker 解放先のマーカー
    //!
    //! @note マーカー以降に確保されたメモリは全て無効になる。
    //!       デストラクタは呼ばれないことに注意。
    //------------------------------------------------------------------------
    void FreeToMarker(Marker marker) {
        assert(marker <= offset_ && "Invalid marker");

        offset_ = marker;

#if defined(_DEBUG)
        lastAllocation_ = nullptr;
        lastAllocationSize_ = 0;
#endif

        // 統計は正確に更新できないので、おおよその値
        stats_.currentUsed = marker;
    }

    //------------------------------------------------------------------------
    // 情報取得
    //------------------------------------------------------------------------

    //! @brief 総容量取得（バイト）
    [[nodiscard]] size_t GetCapacity() const noexcept {
        return capacity_;
    }

    //! @brief 使用量取得（バイト）
    [[nodiscard]] size_t GetUsed() const noexcept {
        return offset_;
    }

    //! @brief 残り容量取得（バイト）
    [[nodiscard]] size_t GetRemaining() const noexcept {
        return capacity_ - offset_;
    }

private:
    //------------------------------------------------------------------------
    //! @brief オフセットをアラインメントに合わせて調整
    //------------------------------------------------------------------------
    [[nodiscard]] static size_t AlignOffset(size_t offset, size_t alignment) noexcept {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

private:
    std::byte* buffer_ = nullptr;           //!< メモリバッファ
    size_t capacity_ = 0;                   //!< 総容量
    size_t offset_ = 0;                     //!< 現在のオフセット
    IAllocator* backing_ = nullptr;         //!< バッキングアロケータ
    HeapAllocator defaultBacking_;          //!< デフォルトバッキング
    AllocatorStats stats_;                  //!< 統計情報

#if defined(_DEBUG)
    void* lastAllocation_ = nullptr;        //!< 最後の確保ポインタ（LIFO検証用）
    size_t lastAllocationSize_ = 0;         //!< 最後の確保サイズ
#endif
};

//============================================================================
//! @brief スタックマーカースコープガード
//!
//! RAIIパターンでマーカーまでの解放を自動化する。
//============================================================================
class ScopedStackMarker final {
public:
    //! @brief コンストラクタ（現在位置をマーカーとして記録）
    //! @param allocator 対象のスタックアロケータ
    explicit ScopedStackMarker(StackAllocator& allocator)
        : allocator_(allocator)
        , marker_(allocator.GetMarker())
    {}

    //! @brief デストラクタ（マーカー位置まで解放）
    ~ScopedStackMarker() {
        allocator_.FreeToMarker(marker_);
    }

    // コピー/ムーブ禁止
    ScopedStackMarker(const ScopedStackMarker&) = delete;
    ScopedStackMarker& operator=(const ScopedStackMarker&) = delete;
    ScopedStackMarker(ScopedStackMarker&&) = delete;
    ScopedStackMarker& operator=(ScopedStackMarker&&) = delete;

    //! @brief マーカー取得
    [[nodiscard]] StackAllocator::Marker GetMarker() const noexcept {
        return marker_;
    }

private:
    StackAllocator& allocator_;             //!< 対象アロケータ
    StackAllocator::Marker marker_;         //!< 記録されたマーカー
};

} // namespace Memory
