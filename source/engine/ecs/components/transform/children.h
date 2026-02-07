//----------------------------------------------------------------------------
//! @file   children.h
//! @brief  ECS Children - 子エンティティリスト（DynamicBuffer）
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/actor.h"
#include "engine/ecs/buffer/buffer_element.h"
#include "engine/ecs/buffer/internal_buffer_capacity.h"

namespace ECS {

//============================================================================
//! @brief 子エンティティ参照（バッファ要素）
//!
//! 4バイト。親が持つ子リストの各要素。
//! DynamicBuffer<Child>として使用する。
//!
//! @code
//! // 子リストへのアクセス
//! auto children = world.GetBuffer<Child>(parent);
//! for (const auto& child : children) {
//!     // child.value でActorを取得
//! }
//! @endcode
//============================================================================
struct Child : public IBufferElement {
    Actor value = Actor::Invalid();  //!< 子エンティティ (4 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Child() = default;

    explicit Child(Actor c) noexcept
        : value(c) {}

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief 子が有効か
    [[nodiscard]] bool IsValid() const noexcept {
        return value.IsValid();
    }

    //------------------------------------------------------------------------
    // 比較演算子
    //------------------------------------------------------------------------

    [[nodiscard]] bool operator==(const Child& other) const noexcept {
        return value == other.value;
    }

    [[nodiscard]] bool operator!=(const Child& other) const noexcept {
        return value != other.value;
    }
};

// コンパイル時検証
ECS_BUFFER_ELEMENT(Child);
static_assert(sizeof(Child) == 4, "Child must be 4 bytes");

} // namespace ECS

//============================================================================
// インライン容量の特殊化
//
// デフォルト: (128-24)/4 = 26 子までインライン
// 26は多くのケースで十分（ゲームオブジェクトの子は通常10以下）
// 必要に応じて以下のように特殊化可能:
//
// template<>
// struct ECS::InternalBufferCapacity<ECS::Child> {
//     static constexpr int32_t Value = 8;  // 8子までインライン
// };
//============================================================================
