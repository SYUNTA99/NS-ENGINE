//----------------------------------------------------------------------------
//! @file   chunk_storage.h
//! @brief  ECS ChunkStorage - チャンク一元管理
//----------------------------------------------------------------------------
#pragma once


#include "chunk.h"
#include "common/utility/non_copyable.h"
#include <vector>
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief ChunkStorage
//!
//! WorldContainer内でチャンクの確保/解放を一元管理する。
//! 空きチャンクをFreeListで管理し、再利用を促進する。
//!
//! @note 現在はArchetypeが直接std::make_unique<Chunk>()を使用しているが、
//!       段階的にChunkStorage経由に移行予定。
//!
//! メモリ確保は Memory::GetChunkPool() を使用（Chunk::operator new経由）。
//============================================================================
class ChunkStorage : private NonCopyable {
public:
    ChunkStorage() = default;
    ~ChunkStorage() { Clear(); }

    //------------------------------------------------------------------------
    //! @brief チャンクを確保
    //!
    //! FreeListに空きがあればそこから、なければ新規確保。
    //!
    //! @return 確保されたChunkへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] Chunk* Allocate() {
        Chunk* chunk = nullptr;

        if (!freeList_.empty()) {
            // FreeListから再利用
            chunk = freeList_.back();
            freeList_.pop_back();
        } else {
            // 新規確保
            chunk = new Chunk();
            allChunks_.push_back(chunk);
        }

        ++allocatedCount_;
        return chunk;
    }

    //------------------------------------------------------------------------
    //! @brief チャンクを解放（FreeListに戻す）
    //!
    //! 実際にメモリは解放せず、再利用可能としてFreeListに追加。
    //!
    //! @param chunk 解放するチャンク
    //------------------------------------------------------------------------
    void Deallocate(Chunk* chunk) {
        if (!chunk) return;

        // FreeListに追加（メモリは解放しない）
        freeList_.push_back(chunk);
        --allocatedCount_;
    }

    //------------------------------------------------------------------------
    //! @brief 全チャンクをクリア
    //!
    //! 全てのチャンクを解放する。WorldContainer破棄時に呼ばれる。
    //------------------------------------------------------------------------
    void Clear() {
        // 全チャンクを解放
        for (Chunk* chunk : allChunks_) {
            delete chunk;
        }
        allChunks_.clear();
        freeList_.clear();
        allocatedCount_ = 0;
    }

    //------------------------------------------------------------------------
    //! @brief 未使用チャンクをトリム
    //!
    //! FreeList内のチャンクを実際に解放してメモリを返却する。
    //------------------------------------------------------------------------
    void Trim() {
        for (Chunk* chunk : freeList_) {
            // allChunks_からも削除
            auto it = std::find(allChunks_.begin(), allChunks_.end(), chunk);
            if (it != allChunks_.end()) {
                allChunks_.erase(it);
            }
            delete chunk;
        }
        freeList_.clear();
    }

    //------------------------------------------------------------------------
    // 統計情報
    //------------------------------------------------------------------------

    //! 確保中（使用中）のチャンク数
    [[nodiscard]] size_t GetAllocatedCount() const noexcept {
        return allocatedCount_;
    }

    //! FreeList内のチャンク数
    [[nodiscard]] size_t GetFreeCount() const noexcept {
        return freeList_.size();
    }

    //! 確保済み（使用中 + FreeList）の総チャンク数
    [[nodiscard]] size_t GetTotalCount() const noexcept {
        return allChunks_.size();
    }

    //! 使用中メモリ量（バイト）
    [[nodiscard]] size_t GetAllocatedBytes() const noexcept {
        return allocatedCount_ * Chunk::kSize;
    }

    //! 確保済み総メモリ量（バイト）
    [[nodiscard]] size_t GetTotalBytes() const noexcept {
        return allChunks_.size() * Chunk::kSize;
    }

private:
    std::vector<Chunk*> allChunks_;     //!< 確保した全チャンク
    std::vector<Chunk*> freeList_;       //!< 再利用可能なチャンク
    size_t allocatedCount_ = 0;          //!< 使用中のチャンク数
};

} // namespace ECS
