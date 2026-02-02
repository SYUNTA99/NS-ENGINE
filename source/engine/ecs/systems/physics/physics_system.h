//----------------------------------------------------------------------------
//! @file   physics_system.h
//! @brief  ECS PhysicsSystem - 物理演算システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/movement/velocity_data.h"
#include "engine/ecs/components/movement/angular_velocity_data.h"
#include "engine/ecs/components/physics/physics_components.h"

namespace ECS {

//============================================================================
//! @brief 物理演算システム（更新システム）
//!
//! 入力: VelocityData, AngularVelocityData, PhysicsMassData, PhysicsDampingData,
//!       PhysicsGravityFactorData, PhysicsMassOverrideData
//! 出力: VelocityData, AngularVelocityData
//!
//! 処理内容:
//! 1. 重力適用（PhysicsGravityFactorData考慮）
//! 2. 減衰適用（PhysicsDampingData）
//! 3. キネマティック処理（PhysicsMassOverrideData）
//!
//! @note 優先度4（MovementSystemの前）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<PhysicsSystem>();
//!
//! // 動的物理オブジェクト
//! auto ball = world.CreateActor();
//! world.AddComponent<LocalTransform>(ball, position);
//! world.AddComponent<VelocityData>(ball, Vector3::Zero);
//! world.AddComponent<PhysicsMassData>(ball, PhysicsMassData::CreateDynamic(1.0f));
//! world.AddComponent<PhysicsDampingData>(ball, PhysicsDampingData::Air());
//!
//! // 浮遊オブジェクト（重力なし）
//! world.AddComponent<PhysicsGravityFactorData>(balloon, PhysicsGravityFactorData::Zero());
//!
//! // キネマティック（物理演算スキップ）
//! world.AddComponent<PhysicsMassOverrideData>(platform, PhysicsMassOverrideData::Kinematic());
//! @endcode
//============================================================================
class PhysicsSystem final : public ISystem {
public:
    void OnUpdate(World& world, float dt) override {
        //================================================================
        // Pass 1: 基本重力適用（VelocityData + PhysicsMassData のみ）
        // 最も一般的なケース - 全物理オブジェクトに重力を適用
        //================================================================
        world.ForEach<VelocityData, PhysicsMassData>(
            [this, dt]([[maybe_unused]] Actor actor, VelocityData& vel,
                       [[maybe_unused]] const PhysicsMassData& mass) {
                vel.value += gravity_ * dt;
            });

        //================================================================
        // Pass 2: GravityFactor補正
        // GravityFactorがある場合、Pass1で適用した重力を補正
        // 打ち消し方式: 標準重力を取り消し、スケール適用した重力を再適用
        //================================================================
        world.ForEach<VelocityData, PhysicsMassData, PhysicsGravityFactorData>(
            [this, dt]([[maybe_unused]] Actor actor, VelocityData& vel,
                       [[maybe_unused]] const PhysicsMassData& mass,
                       const PhysicsGravityFactorData& gravityFactor) {
                vel.value -= gravity_ * dt;  // Pass1の標準重力を打ち消し
                vel.value += gravity_ * gravityFactor.value * dt;  // スケール適用で再適用
            });

        //================================================================
        // Pass 3: Kinematic処理（重力を打ち消し）
        // Kinematicオブジェクトは物理演算をスキップ
        // Note: Kinematic + GravityFactor の組み合わせは稀なので GetComponent で対応
        //================================================================
        world.ForEach<VelocityData, PhysicsMassData, PhysicsMassOverrideData>(
            [this, &world, dt](Actor actor, VelocityData& vel,
                               [[maybe_unused]] const PhysicsMassData& mass,
                               const PhysicsMassOverrideData& override) {
                if (override.IsKinematic()) {
                    // GravityFactorがあるかチェック（稀なケース - GetComponent許容）
                    auto* gf = world.GetComponent<PhysicsGravityFactorData>(actor);
                    if (gf) {
                        // Pass2で適用された factor * gravity を打ち消し
                        vel.value -= gravity_ * gf->value * dt;
                    } else {
                        // Pass1で適用された標準重力を打ち消し
                        vel.value -= gravity_ * dt;
                    }
                }
            });

        //================================================================
        // Pass 4: 線形減衰適用
        // DampingDataを持つオブジェクトの速度を減衰
        //================================================================
        world.ForEach<VelocityData, PhysicsDampingData>(
            [dt]([[maybe_unused]] Actor actor, VelocityData& vel,
                 const PhysicsDampingData& damping) {
                vel.value = damping.ApplyLinear(vel.value, dt);
            });

        //================================================================
        // Pass 5: 角速度減衰
        // 回転運動の減衰を適用
        //================================================================
        world.ForEach<AngularVelocityData, PhysicsDampingData>(
            [dt]([[maybe_unused]] Actor actor, AngularVelocityData& angVel,
                 const PhysicsDampingData& damping) {
                angVel.value = damping.ApplyAngular(angVel.value, dt);
            });

        //================================================================
        // Pass 6: MassOverrideによる速度ゼロ化
        // 特定フラグが設定されているオブジェクトの速度を強制ゼロ化
        //================================================================
        world.ForEach<VelocityData, PhysicsMassOverrideData>(
            []([[maybe_unused]] Actor actor, VelocityData& vel,
               const PhysicsMassOverrideData& override) {
                if (override.ShouldSetVelocityToZero()) {
                    vel.value = Vector3::Zero;
                }
            });
    }

    //------------------------------------------------------------------------
    //! @name 重力設定
    //------------------------------------------------------------------------
    //!@{

    //! @brief 重力ベクトルを設定
    void SetGravity(const Vector3& g) noexcept { gravity_ = g; }

    //! @brief 重力ベクトルを取得
    [[nodiscard]] const Vector3& GetGravity() const noexcept { return gravity_; }

    //!@}

    int Priority() const override { return 4; }
    const char* Name() const override { return "PhysicsSystem"; }

private:
    Vector3 gravity_ = Vector3(0.0f, -9.81f, 0.0f);  //!< 重力ベクトル（デフォルト: 地球の重力）
};

} // namespace ECS
