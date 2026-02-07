//----------------------------------------------------------------------------
//! @file   pool_allocator.h
//! @brief  プールアロケータ - 固定サイズブロックの高速確保
//----------------------------------------------------------------------------
#pragma once


#include "allocator.h"
#include "heap_allocator.h"
#include <vector>
#include <cassert>
#include <cstring>

namespace Memory {

//============================================================================
//! @brief プールアロケータ
//!
//! 固定サイズのメモリブロックを高速に確保/解放する。
//! フリーリストを使用し、確保/解放がO(1)で完了する。
//!
//! @tparam BlockSize 1ブロックのサイズ（バイト）
//! @tparam BlocksPerChunk 1チャンクあたりのブロック数
//!
//! @note Chunk（16KB）専用には PoolAllocator<16384, 64> を使用
//! @note スレッドセーフではない（シングルスレッド使用前提）
//============================================================================
template<size_t BlockSize, size_t BlocksPerChunk = 64>
class PoolAllocator final : public IAllocator {
public:
    //! 実際のブロックサイズ（フリーリストノード分を確保）
    static constexpr size_t kActualBlockSize =
        (BlockSize >= sizeof(void*)) ? BlockSize : sizeof(void*);

    //! ブロックのアラインメント
    static constexpr size_t kBlockAlignment =
        (alignof(std::max_align_t) > BlockSize) ? alignof(std::max_align_t) :
        (BlockSize >= 64) ? 64 :
        (BlockSize >= 16) ? 16 : alignof(std::max_align_t);

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param backing バッキングアロケータ（nullptrでデフォルトHeapAllocator）
    //------------------------------------------------------------------------
    explicit PoolAllocator(IAllocator* backing = nullptr)
        : backing_(backing ? backing : &defaultBacking_)
    {
        // 最初のチャンクを確保
        AllocateChunk();
    }

    //------------------------------------------------------------------------
    //! @brief デストラクタ
    //------------------------------------------------------------------------
    ~PoolAllocator() override {
        // 全チャンクを解放
        for (void* chunk : chunks_) {
            backing_->Deallocate(chunk, kActualBlockSize * BlocksPerChunk);
        }
        chunks_.clear();
    }

    // コピー/ムーブ禁止
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    //------------------------------------------------------------------------
    //! @brief メモリ確保
    //! @param size 確保サイズ（BlockSize以下である必要あり）
    //! @param alignment アラインメント（無視される）
    //! @return 確保されたメモリへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] void* Allocate([[maybe_unused]] size_t size, [[maybe_unused]] size_t alignment) override {
        assert(size <= BlockSize && "Requested size exceeds block size");

        // フリーリストが空なら新しいチャンクを確保
        if (!freeList_) {
            AllocateChunk();
        }

        // フリーリストの先頭を取得
        FreeNode* node = freeList_;
        freeList_ = node->next;

        stats_.RecordAllocation(BlockSize);
        return node;
    }

    //------------------------------------------------------------------------
    //! @brief メモリ解放
    //! @param ptr 解放するポインタ
    //! @param size サイズ（無視される、統計用にBlockSizeを使用）
    //------------------------------------------------------------------------
    void Deallocate(void* ptr, [[maybe_unused]] size_t size) override {
        if (!ptr) {
            return;
        }

        assert(Owns(ptr) && "Pointer was not allocated by this pool");

        // フリーリストの先頭に追加
        FreeNode* node = static_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;

        stats_.RecordDeallocation(BlockSize);
    }

    //------------------------------------------------------------------------
    //! @brief アロケータ名取得
    //------------------------------------------------------------------------
    [[nodiscard]] const char* GetName() const noexcept override {
        return "PoolAllocator";
    }

    //------------------------------------------------------------------------
    //! @brief 統計情報取得
    //------------------------------------------------------------------------
    [[nodiscard]] AllocatorStats GetStats() const noexcept override {
        return stats_;
    }

    //------------------------------------------------------------------------
    //! @brief 指定ポインタがこのプールで確保されたか確認
    //! @param ptr 確認するポインタ
    //! @return このプールで確保された場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Owns(void* ptr) const noexcept override {
        if (!ptr) {
            return false;
        }

        const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        const size_t chunkSize = kActualBlockSize * BlocksPerChunk;

        for (void* chunk : chunks_) {
            const uintptr_t chunkStart = reinterpret_cast<uintptr_t>(chunk);
            const uintptr_t chunkEnd = chunkStart + chunkSize;

            if (addr >= chunkStart && addr < chunkEnd) {
                // ブロック境界上にあるか確認
                return ((addr - chunkStart) % kActualBlockSize) == 0;
            }
        }

        return false;
    }

    //------------------------------------------------------------------------
    //! @brief プールをリセット（全ブロックを解放状態に）
    //------------------------------------------------------------------------
    void Reset() override {
        // フリーリストを再構築
        freeList_ = nullptr;

        for (void* chunk : chunks_) {
            InitializeChunkFreeList(chunk);
        }

        // 統計をリセット
        stats_.Reset();
    }

    //------------------------------------------------------------------------
    //! @brief ブロックサイズ取得
    //------------------------------------------------------------------------
    [[nodiscard]] static constexpr size_t GetBlockSize() noexcept {
        return BlockSize;
    }

    //------------------------------------------------------------------------
    //! @brief チャンク数取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetChunkCount() const noexcept {
        return chunks_.size();
    }

    //------------------------------------------------------------------------
    //! @brief 総ブロック数取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetTotalBlockCount() const noexcept {
        return chunks_.size() * BlocksPerChunk;
    }

    //------------------------------------------------------------------------
    //! @brief 使用中ブロック数取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetUsedBlockCount() const noexcept {
        return stats_.allocationCount - stats_.deallocationCount;
    }

private:
    //! フリーリストノード（ブロック内に埋め込み）
    struct FreeNode {
        FreeNode* next;
    };

    //------------------------------------------------------------------------
    //! @brief 新しいチャンクを確保
    //------------------------------------------------------------------------
    void AllocateChunk() {
        const size_t chunkSize = kActualBlockSize * BlocksPerChunk;

        void* chunk = backing_->Allocate(chunkSize, kBlockAlignment);
        assert(chunk && "Failed to allocate chunk");

        chunks_.push_back(chunk);
        InitializeChunkFreeList(chunk);
    }

    //------------------------------------------------------------------------
    //! @brief チャンク内のフリーリストを初期化
    //! @param chunk チャンクの先頭ポインタ
    //------------------------------------------------------------------------
    void InitializeChunkFreeList(void* chunk) {
        std::byte* ptr = static_cast<std::byte*>(chunk);

        // 各ブロックをフリーリストに連結
        for (size_t i = 0; i < BlocksPerChunk; ++i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(ptr + i * kActualBlockSize);
            node->next = freeList_;
            freeList_ = node;
        }
    }

private:
    FreeNode* freeList_ = nullptr;          //!< フリーリスト先頭
    std::vector<void*> chunks_;             //!< 確保済みチャンク
    IAllocator* backing_ = nullptr;         //!< バッキングアロケータ
    HeapAllocator defaultBacking_;          //!< デフォルトバッキング
    AllocatorStats stats_;                  //!< 統計情報
};

//============================================================================
// 特殊化エイリアス
//============================================================================

//! Chunk用プール（16KB）
using ChunkPoolAllocator = PoolAllocator<16 * 1024, 64>;

//! 小オブジェクト用プール
using SmallObjectPool = PoolAllocator<64, 256>;

//! 中オブジェクト用プール
using MediumObjectPool = PoolAllocator<256, 128>;

} // namespace Memory
