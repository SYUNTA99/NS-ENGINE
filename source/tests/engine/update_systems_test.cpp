//----------------------------------------------------------------------------
//! @file   update_systems_test.cpp
//! @brief  更新システム（MovementSystem等）とWorld APIのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/movement/velocity_data.h"
#include "engine/ecs/components/movement/angular_velocity_data.h"
#include "engine/ecs/components/movement/scale_velocity_data.h"
#include "engine/ecs/systems/transform/movement_system.h"
#include "engine/ecs/systems/transform/rotation_update_system.h"
#include "engine/ecs/systems/transform/scale_update_system.h"
#include "engine/ecs/systems/transform/local_to_world_system.h"
#include <cmath>

namespace
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kEpsilon = 0.0001f;

//============================================================================
// VelocityData テスト
//============================================================================
TEST(VelocityDataTest, DefaultConstruction)
{
    ECS::VelocityData vel;
    EXPECT_FLOAT_EQ(vel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.value.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.value.z, 0.0f);
}

TEST(VelocityDataTest, Vector3Construction)
{
    ECS::VelocityData vel(Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(vel.value.x, 1.0f);
    EXPECT_FLOAT_EQ(vel.value.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.value.z, 3.0f);
}

TEST(VelocityDataTest, FloatConstruction)
{
    ECS::VelocityData vel(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(vel.value.x, 1.0f);
    EXPECT_FLOAT_EQ(vel.value.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.value.z, 3.0f);
}

//============================================================================
// AngularVelocityData テスト
//============================================================================
TEST(AngularVelocityDataTest, DefaultConstruction)
{
    ECS::AngularVelocityData angVel;
    EXPECT_FLOAT_EQ(angVel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.y, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.z, 0.0f);
}

TEST(AngularVelocityDataTest, SetYawSpeed)
{
    ECS::AngularVelocityData angVel;
    angVel.SetYawSpeed(kPi);
    EXPECT_FLOAT_EQ(angVel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.y, kPi);
    EXPECT_FLOAT_EQ(angVel.value.z, 0.0f);
}

TEST(AngularVelocityDataTest, SetPitchSpeed)
{
    ECS::AngularVelocityData angVel;
    angVel.SetPitchSpeed(kPi);
    EXPECT_FLOAT_EQ(angVel.value.x, kPi);
    EXPECT_FLOAT_EQ(angVel.value.y, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.z, 0.0f);
}

TEST(AngularVelocityDataTest, SetRollSpeed)
{
    ECS::AngularVelocityData angVel;
    angVel.SetRollSpeed(kPi);
    EXPECT_FLOAT_EQ(angVel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.y, 0.0f);
    EXPECT_FLOAT_EQ(angVel.value.z, kPi);
}

//============================================================================
// ScaleVelocityData テスト
//============================================================================
TEST(ScaleVelocityDataTest, DefaultConstruction)
{
    ECS::ScaleVelocityData scaleVel;
    EXPECT_FLOAT_EQ(scaleVel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(scaleVel.value.y, 0.0f);
    EXPECT_FLOAT_EQ(scaleVel.value.z, 0.0f);
}

TEST(ScaleVelocityDataTest, SetUniform)
{
    ECS::ScaleVelocityData scaleVel;
    scaleVel.SetUniform(2.0f);
    EXPECT_FLOAT_EQ(scaleVel.value.x, 2.0f);
    EXPECT_FLOAT_EQ(scaleVel.value.y, 2.0f);
    EXPECT_FLOAT_EQ(scaleVel.value.z, 2.0f);
}

//============================================================================
// MovementSystem テスト
//============================================================================
TEST(MovementSystemTest, UpdatesPositionWithVelocity)
{
    ECS::World world;
    world.RegisterSystem<ECS::MovementSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(0.0f, 0.0f, 0.0f);
    world.AddComponent<ECS::VelocityData>(actor, Vector3(10.0f, 0.0f, 0.0f));

    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    EXPECT_NEAR(t->position.x, 10.0f, kEpsilon);
    EXPECT_NEAR(t->position.y, 0.0f, kEpsilon);
    EXPECT_NEAR(t->position.z, 0.0f, kEpsilon);
}

TEST(MovementSystemTest, ZeroVelocityNoUpdate)
{
    ECS::World world;
    world.RegisterSystem<ECS::MovementSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(5.0f, 5.0f, 5.0f);
    world.AddComponent<ECS::VelocityData>(actor, Vector3(0.0f, 0.0f, 0.0f));

    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    EXPECT_NEAR(t->position.x, 5.0f, kEpsilon);
    EXPECT_NEAR(t->position.y, 5.0f, kEpsilon);
    EXPECT_NEAR(t->position.z, 5.0f, kEpsilon);
}

TEST(MovementSystemTest, MultipleFrameUpdate)
{
    ECS::World world;
    world.RegisterSystem<ECS::MovementSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(0.0f, 0.0f, 0.0f);
    world.AddComponent<ECS::VelocityData>(actor, Vector3(1.0f, 2.0f, 3.0f));

    // 10フレーム更新（dt=0.1）
    for (int i = 0; i < 10; ++i) {
        world.FixedUpdate(0.1f);
    }

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    EXPECT_NEAR(t->position.x, 1.0f, kEpsilon);
    EXPECT_NEAR(t->position.y, 2.0f, kEpsilon);
    EXPECT_NEAR(t->position.z, 3.0f, kEpsilon);
}

//============================================================================
// RotationUpdateSystem テスト
//============================================================================
TEST(RotationUpdateSystemTest, UpdatesRotationWithAngularVelocity)
{
    ECS::World world;
    world.RegisterSystem<ECS::RotationUpdateSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->rotation = Quaternion::Identity;
    world.AddComponent<ECS::AngularVelocityData>(actor, Vector3(0.0f, kPi, 0.0f));

    // 1秒後にPI rad（180度）回転
    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);

    // Y軸周りに180度回転したクォータニオン
    Quaternion expected = Quaternion::CreateFromAxisAngle(Vector3::Up, kPi);
    float dot = std::abs(t->rotation.x * expected.x + t->rotation.y * expected.y +
                         t->rotation.z * expected.z + t->rotation.w * expected.w);
    EXPECT_NEAR(dot, 1.0f, kEpsilon);
}

TEST(RotationUpdateSystemTest, ZeroAngularVelocityNoUpdate)
{
    ECS::World world;
    world.RegisterSystem<ECS::RotationUpdateSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->rotation = Quaternion::Identity;
    world.AddComponent<ECS::AngularVelocityData>(actor, Vector3(0.0f, 0.0f, 0.0f));

    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    EXPECT_NEAR(t->rotation.x, 0.0f, kEpsilon);
    EXPECT_NEAR(t->rotation.y, 0.0f, kEpsilon);
    EXPECT_NEAR(t->rotation.z, 0.0f, kEpsilon);
    EXPECT_NEAR(t->rotation.w, 1.0f, kEpsilon);
}

//============================================================================
// ScaleUpdateSystem テスト
//============================================================================
TEST(ScaleUpdateSystemTest, UpdatesScaleWithVelocity)
{
    ECS::World world;
    world.RegisterSystem<ECS::ScaleUpdateSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->scale = Vector3(1.0f, 1.0f, 1.0f);
    world.AddComponent<ECS::ScaleVelocityData>(actor, Vector3(1.0f, 2.0f, 3.0f));

    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    EXPECT_NEAR(t->scale.x, 2.0f, kEpsilon);
    EXPECT_NEAR(t->scale.y, 3.0f, kEpsilon);
    EXPECT_NEAR(t->scale.z, 4.0f, kEpsilon);
}

TEST(ScaleUpdateSystemTest, PreventsNegativeScale)
{
    ECS::World world;
    world.RegisterSystem<ECS::ScaleUpdateSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->scale = Vector3(1.0f, 1.0f, 1.0f);
    world.AddComponent<ECS::ScaleVelocityData>(actor, Vector3(-10.0f, -10.0f, -10.0f));

    world.FixedUpdate(1.0f);

    auto* t = world.GetComponent<ECS::LocalTransform>(actor);
    ASSERT_NE(t, nullptr);
    // 最小値0.001に制限される
    EXPECT_NEAR(t->scale.x, 0.001f, kEpsilon);
    EXPECT_NEAR(t->scale.y, 0.001f, kEpsilon);
    EXPECT_NEAR(t->scale.z, 0.001f, kEpsilon);
}

//============================================================================
// LocalToWorldSystem テスト
//============================================================================
TEST(LocalToWorldSystemTest, ComputesWorldMatrix)
{
    ECS::World world;
    world.RegisterSystem<ECS::LocalToWorldSystem>();

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(10.0f, 20.0f, 30.0f);
    transform->rotation = Quaternion::Identity;
    transform->scale = Vector3(2.0f, 2.0f, 2.0f);
    world.AddComponent<ECS::LocalToWorld>(actor);

    world.FixedUpdate(0.016f);

    auto* ltw = world.GetComponent<ECS::LocalToWorld>(actor);
    ASSERT_NE(ltw, nullptr);

    Vector3 pos = ltw->GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, kEpsilon);
    EXPECT_NEAR(pos.y, 20.0f, kEpsilon);
    EXPECT_NEAR(pos.z, 30.0f, kEpsilon);
}

//============================================================================
// World::GetWorldMatrix テスト
//============================================================================
TEST(WorldGetWorldMatrixTest, ReturnsLocalToWorld)
{
    ECS::World world;

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(1.0f, 2.0f, 3.0f);
    auto* ltw = world.AddComponent<ECS::LocalToWorld>(actor);
    ltw->value = Matrix::CreateTranslation(100.0f, 200.0f, 300.0f);

    // LocalToWorldがあればその値を返す
    Matrix mat = world.GetWorldMatrix(actor);
    Vector3 pos = mat.Translation();
    EXPECT_NEAR(pos.x, 100.0f, kEpsilon);
    EXPECT_NEAR(pos.y, 200.0f, kEpsilon);
    EXPECT_NEAR(pos.z, 300.0f, kEpsilon);
}

TEST(WorldGetWorldMatrixTest, ComputesOnDemandWithoutLocalToWorld)
{
    ECS::World world;

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(5.0f, 10.0f, 15.0f);
    // LocalToWorldなし

    // LocalTransformから計算
    Matrix mat = world.GetWorldMatrix(actor);
    Vector3 pos = mat.Translation();
    EXPECT_NEAR(pos.x, 5.0f, kEpsilon);
    EXPECT_NEAR(pos.y, 10.0f, kEpsilon);
    EXPECT_NEAR(pos.z, 15.0f, kEpsilon);
}

TEST(WorldGetWorldMatrixTest, ReturnsIdentityForInvalidActor)
{
    ECS::World world;

    ECS::Actor invalidActor;
    Matrix mat = world.GetWorldMatrix(invalidActor);

    // Identity行列を返す
    EXPECT_NEAR(mat._11, 1.0f, kEpsilon);
    EXPECT_NEAR(mat._22, 1.0f, kEpsilon);
    EXPECT_NEAR(mat._33, 1.0f, kEpsilon);
    EXPECT_NEAR(mat._44, 1.0f, kEpsilon);
}

//============================================================================
// World::DestroyAfter テスト
//============================================================================
TEST(WorldDestroyAfterTest, DestroysActorAfterDelay)
{
    ECS::World world;

    auto actor = world.CreateActor();
    world.AddComponent<ECS::LocalTransform>(actor);

    EXPECT_TRUE(world.IsAlive(actor));

    world.DestroyAfter(actor, 1.0f);

    // 0.5秒後：まだ生きている
    world.FixedUpdate(0.5f);
    EXPECT_TRUE(world.IsAlive(actor));

    // さらに0.6秒後（合計1.1秒）：破棄される
    world.FixedUpdate(0.6f);
    EXPECT_FALSE(world.IsAlive(actor));
}

TEST(WorldDestroyAfterTest, ImmediateDestroyWithZeroDelay)
{
    ECS::World world;

    auto actor = world.CreateActor();
    EXPECT_TRUE(world.IsAlive(actor));

    world.DestroyAfter(actor, 0.0f);
    EXPECT_FALSE(world.IsAlive(actor));
}

TEST(WorldDestroyAfterTest, CancelDestroyAfter)
{
    ECS::World world;

    auto actor = world.CreateActor();
    world.DestroyAfter(actor, 1.0f);

    // キャンセル
    bool cancelled = world.CancelDestroyAfter(actor);
    EXPECT_TRUE(cancelled);

    // 2秒経過しても生きている
    world.FixedUpdate(2.0f);
    EXPECT_TRUE(world.IsAlive(actor));
}

TEST(WorldDestroyAfterTest, CancelNonExistentReturnsFlase)
{
    ECS::World world;

    auto actor = world.CreateActor();

    // DestroyAfterしていないアクターのキャンセル
    bool cancelled = world.CancelDestroyAfter(actor);
    EXPECT_FALSE(cancelled);
}

//============================================================================
// システム優先度テスト（更新システム → LocalToWorldSystem の順）
//============================================================================
TEST(SystemPriorityTest, UpdateSystemsRunBeforeLocalToWorld)
{
    ECS::World world;
    world.RegisterSystem<ECS::MovementSystem>();           // priority 5
    world.RegisterSystem<ECS::RotationUpdateSystem>();     // priority 6
    world.RegisterSystem<ECS::ScaleUpdateSystem>();        // priority 7
    world.RegisterSystem<ECS::LocalToWorldSystem>();       // priority 10

    auto actor = world.CreateActor();
    auto* transform = world.AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(0.0f, 0.0f, 0.0f);
    transform->rotation = Quaternion::Identity;
    transform->scale = Vector3(1.0f, 1.0f, 1.0f);
    world.AddComponent<ECS::LocalToWorld>(actor);
    world.AddComponent<ECS::VelocityData>(actor, Vector3(100.0f, 0.0f, 0.0f));

    world.FixedUpdate(1.0f);

    // MovementSystemで位置が更新され、LocalToWorldSystemでワールド行列に反映される
    auto* ltw = world.GetComponent<ECS::LocalToWorld>(actor);
    ASSERT_NE(ltw, nullptr);

    Vector3 worldPos = ltw->GetPosition();
    EXPECT_NEAR(worldPos.x, 100.0f, kEpsilon);
}

} // namespace
