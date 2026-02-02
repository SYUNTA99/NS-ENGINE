//----------------------------------------------------------------------------
//! @file   scale_update_system.h
//! @brief  ECS ScaleUpdateSystem - スケール更新システム（更新システム）
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/movement/scale_velocity_data.h"
#include <algorithm>

namespace ECS {

//============================================================================
//! @brief スケール更新システム（更新システム）
//!
//! 入力: ScaleVelocityData（読み取り専用）
//! 更新: LocalTransform.scale
//!
//! 単一責任: スケール変化速度をスケールに変換
//!
//! @note 優先度7（RotationUpdateSystemの後、LocalToWorldSystemの前）
//============================================================================
class ScaleUpdateSystem final : public ISystem {
public:
    void OnUpdate(World& world, float dt) override {
        world.ForEach<ScaleVelocityData, LocalTransform>(
            [dt](Actor, const ScaleVelocityData& scaleVel, LocalTransform& transform) {
                // 変化速度が0なら何もしない
                if (scaleVel.value.LengthSquared() < 0.0001f) return;

                // スケールを更新
                transform.scale += scaleVel.value * dt;

                // 負のスケールを防ぐ（最小値0.001）
                transform.scale.x = (std::max)(0.001f, transform.scale.x);
                transform.scale.y = (std::max)(0.001f, transform.scale.y);
                transform.scale.z = (std::max)(0.001f, transform.scale.z);
            });
    }

    int Priority() const override { return 7; }
    const char* Name() const override { return "ScaleUpdateSystem"; }
};

} // namespace ECS
