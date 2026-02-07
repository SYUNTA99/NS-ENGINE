//----------------------------------------------------------------------------
//! @file   chunk.h
//! @brief  ECS Chunk - 純粋な16KB固定サイズバッファ
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"
#include "engine/memory/memory_system.h"

namespace ECS {

//============================================================================
//! @brief チャンク
//!
//! 純粋な16KBの固定サイズメモリバッファ。
//! メタデータは一切持たず、Archetypeが管理する。
//!
//! メモリレイアウト（Archetypeが決定）:
//! [Actor0][Actor1]...[ActorN] | [Comp0_data][Comp1_data]...[CompN_data]
//! <---- Actor配列 -------->   <---- コンポーネントデータ ------------>
//!
//! Actor配列とコンポーネントデータの境界はArchetypeが計算。
//============================================================================
class alignas(64) Chunk {
public:
    //! チャンクサイズ（16KB、L1キャッシュに適合）
    static constexpr size_t kSize = 16 * 1024;

    //! キャッシュラインサイズ
    static constexpr size_t kCacheLineSize = 64;

    //------------------------------------------------------------------------
    // メモリ確保演算子（MemorySystemのChunkPoolを使用）
    //------------------------------------------------------------------------

    [[nodiscard]] static void* operator new(size_t size) {
        if (Memory::MemorySystem::Get().IsInitialized()) {
            return Memory::GetChunkPool().Allocate(size, alignof(Chunk));
        }
        return ::operator new(size);
    }

    static void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        if (Memory::MemorySystem::Get().IsInitialized() &&
            Memory::GetChunkPool().Owns(ptr)) {
            Memory::GetChunkPool().Deallocate(ptr, sizeof(Chunk));
            return;
        }
        ::operator delete(ptr);
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    Chunk() = default;
    ~Chunk() = default;

    // コピー禁止
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // ムーブ禁止（純粋なバッファなのでムーブの意味がない）
    Chunk(Chunk&&) = delete;
    Chunk& operator=(Chunk&&) = delete;

    //------------------------------------------------------------------------
    //! @brief 生データへのアクセス
    //------------------------------------------------------------------------
    [[nodiscard]] std::byte* Data() noexcept { return data_; }
    [[nodiscard]] const std::byte* Data() const noexcept { return data_; }

private:
    std::byte data_[kSize]{};
};

//============================================================================
// コンパイル時サイズ検証
//============================================================================
static_assert(sizeof(Chunk) == Chunk::kSize, "Chunk must be exactly 16KB");
static_assert(alignof(Chunk) == 64, "Chunk must be 64-byte aligned");

} // namespace ECS
