//----------------------------------------------------------------------------
//! @file   rotation_update_system.h
//! @brief  ECS RotationUpdateSystem - 回転更新システム（更新システム）
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/movement/angular_velocity_data.h"

namespace ECS {

//============================================================================
//! @brief 回転更新システム（更新システム）
//!
//! 入力: AngularVelocityData（読み取り専用）
//! 更新: LocalTransform.rotation
//!
//! 単一責任: 角速度を回転に変換
//!
//! @note 優先度6（MovementSystemの後、LocalToWorldSystemの前）
//============================================================================
class RotationUpdateSystem final : public ISystem {
public:
    void OnUpdate(World& world, float dt) override {
        world.ForEach<AngularVelocityData, LocalTransform>(
            [dt](Actor, const AngularVelocityData& angVel, LocalTransform& transform) {
                // 角速度が0なら何もしない
                float lengthSq = angVel.value.LengthSquared();
                if (lengthSq < 0.0001f) return;

                // 角速度から回転量を計算
                float length = std::sqrt(lengthSq);
                float angle = length * dt;
                Vector3 axis = angVel.value / length;  // 正規化

                // クォータニオンを更新
                Quaternion deltaRot = Quaternion::CreateFromAxisAngle(axis, angle);
                transform.rotation = transform.rotation * deltaRot;
                transform.rotation.Normalize();
            });
    }

    int Priority() const override { return 6; }
    const char* Name() const override { return "RotationUpdateSystem"; }
};

} // namespace ECS
