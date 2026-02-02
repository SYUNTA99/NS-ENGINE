//----------------------------------------------------------------------------
//! @file   internal_buffer_capacity.h
//! @brief  ECS InternalBufferCapacity - DynamicBufferのインライン容量設定
//----------------------------------------------------------------------------
#pragma once

#include "buffer_header.h"
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief DynamicBufferのインライン容量を定義するtrait
//!
//! デフォルトでは128バイトのインラインストレージを確保する。
//! 特定の要素型に対してテンプレート特殊化で容量をカスタマイズ可能。
//!
//! @tparam T Buffer要素型（IBufferElement継承）
//!
//! @code
//! // デフォルト容量を使用
//! struct Waypoint : IBufferElement { float x, y, z; };  // 12B
//! // InternalBufferCapacity<Waypoint>::Value = (128-24)/12 = 8
//!
//! // 容量をカスタマイズ
//! template<>
//! struct InternalBufferCapacity<Child> {
//!     static constexpr int32_t Value = 16;  // 16子までインライン
//! };
//! @endcode
//============================================================================
template<typename T>
struct InternalBufferCapacity {
    //! デフォルト: 128Bのインラインストレージ（ヘッダー分を除く）
    //! 最低1要素は確保
    static constexpr int32_t Value =
        ((128 - static_cast<int32_t>(sizeof(BufferHeader))) / static_cast<int32_t>(sizeof(T))) > 0
            ? (128 - static_cast<int32_t>(sizeof(BufferHeader))) / static_cast<int32_t>(sizeof(T))
            : 1;
};

//============================================================================
//! @brief Buffer全体のサイズを計算するヘルパー
//!
//! BufferHeader + InlineData のサイズを計算する。
//!
//! @tparam T Buffer要素型
//! @return Chunk内で確保するサイズ（バイト）
//============================================================================
template<typename T>
constexpr size_t CalculateBufferSlotSize() {
    return sizeof(BufferHeader) +
           static_cast<size_t>(InternalBufferCapacity<T>::Value) * sizeof(T);
}

} // namespace ECS
