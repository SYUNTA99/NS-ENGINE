//----------------------------------------------------------------------------
//! @file   movement_system.h
//! @brief  ECS MovementSystem - 移動システム（更新システム）
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/movement/velocity_data.h"

namespace ECS {

//============================================================================
//! @brief 移動システム（更新システム）
//!
//! 入力: VelocityData（読み取り専用）
//! 更新: LocalTransform.position
//!
//! 単一責任: 速度を位置に変換
//!
//! @note 優先度5（LocalToWorldSystemの前）
//============================================================================
class MovementSystem final : public ISystem {
public:
    void OnUpdate(World& world, float dt) override {
        world.ForEach<VelocityData, LocalTransform>(
            [dt](Actor, const VelocityData& vel, LocalTransform& transform) {
                // 速度が0なら何もしない
                if (vel.value.LengthSquared() < 0.0001f) return;

                // 位置を更新
                transform.position += vel.value * dt;
            });
    }

    int Priority() const override { return 5; }
    const char* Name() const override { return "MovementSystem"; }
};

} // namespace ECS
