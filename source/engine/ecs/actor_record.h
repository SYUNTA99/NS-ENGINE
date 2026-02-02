//----------------------------------------------------------------------------
//! @file   actor_record.h
//! @brief  ECS ActorRecord - Actor→Archetype/Chunk位置の逆引き
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_common.h"

namespace ECS {

// 前方宣言
class Archetype;

//============================================================================
//! @brief ActorRecord
//!
//! Actor → Archetype/Chunk/Index の高速ルックアップ情報。
//! ActorManagerが配列として保持し、Actor.Index() で O(1) アクセス可能。
//!
//! サイズ: 16バイト（ポインタ8バイト + uint32_t 4バイト + uint16_t×2 4バイト）
//============================================================================
struct ActorRecord {
    Archetype* archetype = nullptr;  //!< 所属Archetype（nullptrならArchetype未割当）
    uint32_t chunkIndex = 0;         //!< Archetype内のChunkインデックス
    uint16_t indexInChunk = 0;       //!< Chunk内のインデックス
    uint16_t reserved = 0;           //!< 予約（アラインメント調整）

    //! Archetypeが割り当てられているか
    [[nodiscard]] bool HasArchetype() const noexcept {
        return archetype != nullptr;
    }

    //! レコードをクリア
    void Clear() noexcept {
        archetype = nullptr;
        chunkIndex = 0;
        indexInChunk = 0;
        reserved = 0;
    }
};

} // namespace ECS
