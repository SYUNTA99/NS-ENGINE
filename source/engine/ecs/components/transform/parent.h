//----------------------------------------------------------------------------
//! @file   parent.h
//! @brief  ECS Parent - 親エンティティ参照
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/actor.h"
#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief 親エンティティ参照コンポーネント
//!
//! 4バイト。親子階層を構築するための親参照。
//! 子エンティティのみが持つ（ルートエンティティは不要）。
//!
//! @note ユーザーが管理するコンポーネント。
//!       ParentSystemがPreviousParentとHierarchyDepthDataを自動更新。
//!
//! @code
//! // 親子関係の構築
//! auto parent = world.CreateActor();
//! auto child = world.CreateActor();
//! world.AddComponent<Parent>(child, parent);
//!
//! // ParentSystemが自動で以下を処理:
//! // - PreviousParent の更新
//! // - HierarchyDepthData の計算
//! @endcode
//============================================================================
struct Parent : public IComponentData {
    Actor value = Actor::Invalid();          //!< 親エンティティ (4 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Parent() = default;

    explicit Parent(Actor p) noexcept
        : value(p) {}

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief 親が存在するか
    [[nodiscard]] bool HasParent() const noexcept {
        return value.IsValid();
    }

    //! @brief 親を設定
    void SetParent(Actor p) noexcept {
        value = p;
    }

    //! @brief 親を解除
    void ClearParent() noexcept {
        value = Actor::Invalid();
    }
};

// コンパイル時検証
ECS_COMPONENT(Parent);
static_assert(sizeof(Parent) == 4, "Parent must be 4 bytes");

} // namespace ECS
