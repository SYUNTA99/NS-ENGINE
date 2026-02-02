//----------------------------------------------------------------------------
//! @file   previous_parent.h
//! @brief  ECS PreviousParent - 前フレームの親エンティティ参照
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/actor.h"
#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief 前フレームの親エンティティ参照コンポーネント
//!
//! 4バイト。ParentSystemが自動管理。
//! Parent と比較して親変更を検出するために使用。
//!
//! @note システムが自動管理するコンポーネント。
//!       ユーザーは直接操作しない。
//!
//! @code
//! // ParentSystem内での使用
//! world.ForEach<Parent, PreviousParent>([&](Actor e, Parent& p, PreviousParent& pp) {
//!     if (p.value != pp.value) {
//!         // 親が変更された
//!         OnParentChanged(e, pp.value, p.value);
//!         pp.value = p.value;
//!     }
//! });
//! @endcode
//============================================================================
struct PreviousParent : public IComponentData {
    Actor value = Actor::Invalid();          //!< 前フレームの親エンティティ (4 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PreviousParent() = default;

    explicit PreviousParent(Actor p) noexcept
        : value(p) {}
};

// コンパイル時検証
ECS_COMPONENT(PreviousParent);
static_assert(sizeof(PreviousParent) == 4, "PreviousParent must be 4 bytes");

} // namespace ECS
