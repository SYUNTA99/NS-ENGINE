//----------------------------------------------------------------------------
//! @file   linear_allocator.h
//! @brief  リニアアロケータ - 高速な線形確保
//----------------------------------------------------------------------------
#pragma once

#include "allocator.h"
#include "heap_allocator.h"
#include <cassert>
#include <cstring>

namespace Memory {

//============================================================================
//! @brief リニアアロケータ
//!
//! 事前確保したバッファから順次メモリを切り出す。
//! 個別解放はできず、Reset()で一括解放する。
//!
//! 特徴:
//! - 確保: O(1)、非常に高速（ポインタ加算のみ）
//! - 解放: 個別解放不可、Reset()で一括
//! - 用途: フレーム一時データ、スコープ限定の一時データ
//!
//! @note スレッドセーフではない（シングルスレッド使用前提）
//============================================================================
class LinearAllocator final : public IAllocator {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param capacity バッファ容量（バイト）
    //! @param backing バッキングアロケータ（nullptrでデフォルトHeapAllocator）
    //------------------------------------------------------------------------
    explicit LinearAllocator(size_t capacity, IAllocator* backing = nullptr)
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
    ~LinearAllocator() override {
        if (buffer_) {
            backing_->Deallocate(buffer_, capacity_);
            buffer_ = nullptr;
        }
    }

    // コピー禁止
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    // ムーブ許可
    LinearAllocator(LinearAllocator&& other) noexcept
        : buffer_(other.buffer_)
        , capacity_(other.capacity_)
        , offset_(other.offset_)
        , backing_(other.backing_)
        , stats_(other.stats_)
    {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.offset_ = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& other) noexcept {
        if (this != &other) {
            // 既存バッファを解放
            if (buffer_) {
                backing_->Deallocate(buffer_, capacity_);
            }

            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            offset_ = other.offset_;
            backing_ = other.backing_;
            stats_ = other.stats_;

            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.offset_ = 0;
        }
        return *this;
    }

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
            assert(false && "LinearAllocator: Out of memory");
            return nullptr;
        }

        void* ptr = buffer_ + alignedOffset;
        offset_ = alignedOffset + size;

        stats_.RecordAllocation(size);
        return ptr;
    }

    //------------------------------------------------------------------------
    //! @brief メモリ解放（何もしない）
    //! @param ptr 解放するポインタ（無視される）
    //! @param size サイズ（統計用）
    //!
    //! @note LinearAllocatorでは個別解放はできない。
    //!       Reset()を使用して一括解放すること。
    //------------------------------------------------------------------------
    void Deallocate([[maybe_unused]] void* ptr, size_t size) override {
        // 個別解放は何もしない（統計のみ更新）
        stats_.RecordDeallocation(size);
    }

    //------------------------------------------------------------------------
    //! @brief アロケータ名取得
    //------------------------------------------------------------------------
    [[nodiscard]] const char* GetName() const noexcept override {
        return "LinearAllocator";
    }

    //------------------------------------------------------------------------
    //! @brief 統計情報取得
    //------------------------------------------------------------------------
    [[nodiscard]] AllocatorStats GetStats() const noexcept override {
        return stats_;
    }

    //------------------------------------------------------------------------
    //! @brief バッファをリセット（一括解放）
    //!
    //! オフセットを先頭に戻し、全てのメモリを再利用可能にする。
    //! 確保されたオブジェクトのデストラクタは呼ばれないことに注意。
    //------------------------------------------------------------------------
    void Reset() override {
        offset_ = 0;
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

    //! @brief 使用率取得（0.0〜1.0）
    [[nodiscard]] float GetUsageRatio() const noexcept {
        return capacity_ > 0 ? static_cast<float>(offset_) / static_cast<float>(capacity_) : 0.0f;
    }

    //! @brief バッファ先頭取得（デバッグ用）
    [[nodiscard]] const void* GetBuffer() const noexcept {
        return buffer_;
    }

private:
    //------------------------------------------------------------------------
    //! @brief オフセットをアラインメントに合わせて調整
    //! @param offset 現在のオフセット
    //! @param alignment アラインメント要件
    //! @return 調整後のオフセット
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
};

//============================================================================
//! @brief スコープ付きリニアアロケータ
//!
//! RAIIパターンでLinearAllocatorを管理する。
//! スコープ終了時に自動的にリセットされる。
//============================================================================
class ScopedLinearAllocator final {
public:
    //! @brief コンストラクタ
    //! @param capacity バッファ容量（バイト）
    //! @param backing バッキングアロケータ
    explicit ScopedLinearAllocator(size_t capacity, IAllocator* backing = nullptr)
        : allocator_(capacity, backing)
    {}

    ~ScopedLinearAllocator() = default;

    // コピー禁止、ムーブ許可
    ScopedLinearAllocator(const ScopedLinearAllocator&) = delete;
    ScopedLinearAllocator& operator=(const ScopedLinearAllocator&) = delete;
    ScopedLinearAllocator(ScopedLinearAllocator&&) = default;
    ScopedLinearAllocator& operator=(ScopedLinearAllocator&&) = default;

    //! @brief アロケータ取得
    [[nodiscard]] LinearAllocator& Get() noexcept {
        return allocator_;
    }

    //! @brief アロケータ取得（const版）
    [[nodiscard]] const LinearAllocator& Get() const noexcept {
        return allocator_;
    }

    //! @brief 直接確保
    [[nodiscard]] void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        return allocator_.Allocate(size, alignment);
    }

private:
    LinearAllocator allocator_;
};

} // namespace Memory
