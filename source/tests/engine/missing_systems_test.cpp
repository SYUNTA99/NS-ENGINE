//----------------------------------------------------------------------------
//! @file   missing_systems_test.cpp
//! @brief  Missing Systems テスト（Phase 4で追加したシステム）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/systems/common/lifetime_system.h"
#include "engine/ecs/systems/camera/camera2d_system.h"
#include "engine/ecs/systems/camera/camera3d_system.h"
#include "engine/ecs/systems/physics/physics_system.h"
#include "engine/ecs/systems/rendering/render_bounds_update_system.h"
#include "engine/ecs/systems/rendering/lod_system.h"
#include "engine/ecs/components/common/lifetime_data.h"
#include "engine/ecs/components/common/entity_tags.h"
#include "engine/ecs/components/camera/camera2d_data.h"
#include "engine/ecs/components/camera/camera3d_data.h"
#include "engine/ecs/components/movement/velocity_data.h"
#include "engine/ecs/components/movement/angular_velocity_data.h"
#include "engine/ecs/components/physics/physics_components.h"
#include "engine/ecs/components/rendering/render_components.h"
#include "engine/ecs/components/transform/transform_components.h"

namespace
{

//============================================================================
// LifetimeSystem テスト
//============================================================================
class LifetimeSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::LifetimeSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(LifetimeSystemTest, ActorDestroyedWhenExpired)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::LifetimeData>(actor, 0.5f);

    EXPECT_TRUE(world_->IsAlive(actor));

    // 0.3秒経過
    world_->FixedUpdate(0.3f);
    EXPECT_TRUE(world_->IsAlive(actor));

    // 0.3秒経過（合計0.6秒）
    world_->FixedUpdate(0.3f);
    EXPECT_FALSE(world_->IsAlive(actor));
}

TEST_F(LifetimeSystemTest, ActorNotDestroyedWithExtension)
{
    ECS::Actor actor = world_->CreateActor();
    auto* life = world_->AddComponent<ECS::LifetimeData>(actor, 0.5f);

    world_->FixedUpdate(0.4f);
    EXPECT_TRUE(world_->IsAlive(actor));

    // 寿命を延長
    life->Extend(1.0f);

    world_->FixedUpdate(0.3f);
    EXPECT_TRUE(world_->IsAlive(actor));  // まだ生きている
}

TEST_F(LifetimeSystemTest, MultipleActorsDestroyed)
{
    ECS::Actor actor1 = world_->CreateActor();
    ECS::Actor actor2 = world_->CreateActor();
    ECS::Actor actor3 = world_->CreateActor();

    world_->AddComponent<ECS::LifetimeData>(actor1, 0.1f);
    world_->AddComponent<ECS::LifetimeData>(actor2, 0.2f);
    world_->AddComponent<ECS::LifetimeData>(actor3, 0.5f);

    world_->FixedUpdate(0.15f);
    EXPECT_FALSE(world_->IsAlive(actor1));
    EXPECT_TRUE(world_->IsAlive(actor2));
    EXPECT_TRUE(world_->IsAlive(actor3));

    world_->FixedUpdate(0.1f);
    EXPECT_FALSE(world_->IsAlive(actor2));
    EXPECT_TRUE(world_->IsAlive(actor3));
}

TEST_F(LifetimeSystemTest, ImmediateDestroyOnZeroLifetime)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::LifetimeData>(actor, ECS::LifetimeData::Immediate());

    world_->FixedUpdate(0.016f);
    EXPECT_FALSE(world_->IsAlive(actor));
}

//============================================================================
// Camera2DSystem テスト
//============================================================================
class Camera2DSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::Camera2DSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(Camera2DSystemTest, UpdatesMatrixWhenDirty)
{
    ECS::Actor camera = world_->CreateActor();
    auto* cam = world_->AddComponent<ECS::Camera2DData>(camera, 1280.0f, 720.0f);
    cam->position = Vector2(100.0f, 50.0f);
    cam->dirty = true;

    world_->FixedUpdate(0.016f);

    EXPECT_FALSE(cam->dirty);
}

TEST_F(Camera2DSystemTest, SyncsPositionFromLocalTransform)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera2DData>(camera, 1280.0f, 720.0f);
    auto* transform = world_->AddComponent<ECS::LocalTransform>(camera);
    transform->position = Vector3(200.0f, 150.0f, 0.0f);

    // Re-fetch component pointer after archetype migration
    auto* cam = world_->GetComponent<ECS::Camera2DData>(camera);
    ASSERT_NE(cam, nullptr);

    world_->FixedUpdate(0.016f);

    EXPECT_NEAR(cam->position.x, 200.0f, 0.001f);
    EXPECT_NEAR(cam->position.y, 150.0f, 0.001f);
}

TEST_F(Camera2DSystemTest, SyncsRotationFromLocalTransform)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera2DData>(camera, 1280.0f, 720.0f);
    auto* transform = world_->AddComponent<ECS::LocalTransform>(camera);
    transform->rotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, 1.57f);

    // Re-fetch component pointer after archetype migration
    auto* cam = world_->GetComponent<ECS::Camera2DData>(camera);
    ASSERT_NE(cam, nullptr);

    world_->FixedUpdate(0.016f);

    EXPECT_NEAR(cam->rotation, 1.57f, 0.01f);
}

//============================================================================
// Camera3DSystem テスト
//============================================================================
class Camera3DSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::Camera3DSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(Camera3DSystemTest, UpdatesMatricesWhenDirty)
{
    ECS::Actor camera = world_->CreateActor();
    auto* cam = world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    cam->position = Vector3(0.0f, 5.0f, -10.0f);
    cam->target = Vector3(0.0f, 0.0f, 0.0f);
    cam->dirty = true;

    world_->FixedUpdate(0.016f);

    EXPECT_FALSE(cam->dirty);
}

TEST_F(Camera3DSystemTest, SyncsPositionFromLocalTransform)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    auto* transform = world_->AddComponent<ECS::LocalTransform>(camera);
    transform->position = Vector3(10.0f, 20.0f, 30.0f);

    // Re-fetch component pointer after archetype migration
    auto* cam = world_->GetComponent<ECS::Camera3DData>(camera);
    ASSERT_NE(cam, nullptr);

    world_->FixedUpdate(0.016f);

    EXPECT_NEAR(cam->position.x, 10.0f, 0.001f);
    EXPECT_NEAR(cam->position.y, 20.0f, 0.001f);
    EXPECT_NEAR(cam->position.z, 30.0f, 0.001f);
}

TEST_F(Camera3DSystemTest, ComputesTargetFromRotation)
{
    ECS::Actor camera = world_->CreateActor();
    auto* cam = world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    auto* transform = world_->AddComponent<ECS::LocalTransform>(camera);
    transform->position = Vector3::Zero;
    transform->rotation = Quaternion::Identity;  // 前方向 = +Z

    world_->FixedUpdate(0.016f);

    // target = position + forward
    Vector3 expectedTarget = transform->position + transform->GetForward();
    EXPECT_NEAR(cam->target.x, expectedTarget.x, 0.001f);
    EXPECT_NEAR(cam->target.y, expectedTarget.y, 0.001f);
    EXPECT_NEAR(cam->target.z, expectedTarget.z, 0.001f);
}

//============================================================================
// PhysicsSystem テスト
//============================================================================
class PhysicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::PhysicsSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(PhysicsSystemTest, AppliesGravityToVelocity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3::Zero);
    world_->AddComponent<ECS::PhysicsMassData>(actor, ECS::PhysicsMassData::CreateDynamic(1.0f));

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(1.0f);  // 1秒

    // デフォルト重力 = (0, -9.81, 0)
    EXPECT_NEAR(vel->value.x, 0.0f, 0.001f);
    EXPECT_NEAR(vel->value.y, -9.81f, 0.01f);
    EXPECT_NEAR(vel->value.z, 0.0f, 0.001f);
}

TEST_F(PhysicsSystemTest, GravityFactorScalesGravity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3::Zero);
    world_->AddComponent<ECS::PhysicsMassData>(actor, ECS::PhysicsMassData::CreateDynamic(1.0f));
    world_->AddComponent<ECS::PhysicsGravityFactorData>(actor, ECS::PhysicsGravityFactorData::Light());  // 0.16 (月面重力)

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(1.0f);

    EXPECT_NEAR(vel->value.y, -9.81f * 0.16f, 0.01f);  // Light = 0.16 (月面重力)
}

TEST_F(PhysicsSystemTest, ZeroGravityFactorNoGravity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3::Zero);
    world_->AddComponent<ECS::PhysicsMassData>(actor, ECS::PhysicsMassData::CreateDynamic(1.0f));
    world_->AddComponent<ECS::PhysicsGravityFactorData>(actor, ECS::PhysicsGravityFactorData::Zero());

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(1.0f);

    EXPECT_NEAR(vel->value.y, 0.0f, 0.001f);
}

TEST_F(PhysicsSystemTest, KinematicSkipsPhysics)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3::Zero);
    world_->AddComponent<ECS::PhysicsMassData>(actor, ECS::PhysicsMassData::CreateDynamic(1.0f));
    world_->AddComponent<ECS::PhysicsMassOverrideData>(actor, ECS::PhysicsMassOverrideData::Kinematic());

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(1.0f);

    // キネマティックなので重力が適用されない
    EXPECT_NEAR(vel->value.y, 0.0f, 0.001f);
}

TEST_F(PhysicsSystemTest, DampingReducesVelocity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3(10.0f, 0.0f, 0.0f));
    world_->AddComponent<ECS::PhysicsMassData>(actor, ECS::PhysicsMassData::CreateDynamic(1.0f));
    world_->AddComponent<ECS::PhysicsDampingData>(actor, ECS::PhysicsDampingData::HighFriction());  // linear=0.5
    world_->AddComponent<ECS::PhysicsGravityFactorData>(actor, ECS::PhysicsGravityFactorData::Zero());  // 重力無効化

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(0.1f);

    // 減衰によりX速度が減少
    EXPECT_LT(vel->value.x, 10.0f);
}

TEST_F(PhysicsSystemTest, MassOverrideZerosVelocity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::VelocityData>(actor, Vector3(100.0f, 100.0f, 100.0f));
    world_->AddComponent<ECS::PhysicsMassOverrideData>(actor, ECS::PhysicsMassOverrideData::Frozen());

    // Re-fetch component pointer after archetype migration
    auto* vel = world_->GetComponent<ECS::VelocityData>(actor);
    ASSERT_NE(vel, nullptr);

    world_->FixedUpdate(0.016f);

    EXPECT_NEAR(vel->value.x, 0.0f, 0.001f);
    EXPECT_NEAR(vel->value.y, 0.0f, 0.001f);
    EXPECT_NEAR(vel->value.z, 0.0f, 0.001f);
}

TEST_F(PhysicsSystemTest, AngularDampingReducesAngularVelocity)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::AngularVelocityData>(actor, Vector3(10.0f, 0.0f, 0.0f));
    world_->AddComponent<ECS::PhysicsDampingData>(actor, ECS::PhysicsDampingData::Water());  // angular=0.5

    // Re-fetch component pointer after archetype migration
    auto* angVel = world_->GetComponent<ECS::AngularVelocityData>(actor);
    ASSERT_NE(angVel, nullptr);

    world_->FixedUpdate(0.1f);

    EXPECT_LT(angVel->value.x, 10.0f);
}

//============================================================================
// RenderBoundsUpdateSystem テスト
//============================================================================
class RenderBoundsUpdateSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::RenderBoundsUpdateSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(RenderBoundsUpdateSystemTest, IdentityMatrixPreservesBounds)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::RenderBoundsData>(actor, ECS::RenderBoundsData::UnitCube());
    auto* ltw = world_->AddComponent<ECS::LocalToWorld>(actor);
    ltw->value = Matrix::Identity;
    auto* worldBounds = world_->AddComponent<ECS::WorldRenderBoundsData>(actor);

    world_->FixedUpdate(0.016f);

    // 単位キューブ: center=(0,0,0), extents=(0.5,0.5,0.5) → min=(-0.5,-0.5,-0.5), max=(0.5,0.5,0.5)
    EXPECT_NEAR(worldBounds->minPoint.x, -0.5f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.y, -0.5f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.z, -0.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.x, 0.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.y, 0.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.z, 0.5f, 0.001f);
}

TEST_F(RenderBoundsUpdateSystemTest, TranslationOffsetsBounds)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::RenderBoundsData>(actor, ECS::RenderBoundsData::UnitCube());
    auto* ltw = world_->AddComponent<ECS::LocalToWorld>(actor);
    ltw->value = Matrix::CreateTranslation(10.0f, 20.0f, 30.0f);
    auto* worldBounds = world_->AddComponent<ECS::WorldRenderBoundsData>(actor);

    world_->FixedUpdate(0.016f);

    EXPECT_NEAR(worldBounds->minPoint.x, 9.5f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.y, 19.5f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.z, 29.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.x, 10.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.y, 20.5f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.z, 30.5f, 0.001f);
}

TEST_F(RenderBoundsUpdateSystemTest, ScaleExpandsBounds)
{
    ECS::Actor actor = world_->CreateActor();
    world_->AddComponent<ECS::RenderBoundsData>(actor, ECS::RenderBoundsData::UnitCube());
    auto* ltw = world_->AddComponent<ECS::LocalToWorld>(actor);
    ltw->value = Matrix::CreateScale(2.0f, 2.0f, 2.0f);
    auto* worldBounds = world_->AddComponent<ECS::WorldRenderBoundsData>(actor);

    world_->FixedUpdate(0.016f);

    // スケール2倍 → extents も2倍
    EXPECT_NEAR(worldBounds->minPoint.x, -1.0f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.y, -1.0f, 0.001f);
    EXPECT_NEAR(worldBounds->minPoint.z, -1.0f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.x, 1.0f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.y, 1.0f, 0.001f);
    EXPECT_NEAR(worldBounds->maxPoint.z, 1.0f, 0.001f);
}

//============================================================================
// LODSystem テスト
//============================================================================
class LODSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::LODSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(LODSystemTest, MeshVisibleWhenInRange)
{
    // アクティブカメラを作成
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    world_->AddComponent<ECS::ActiveCameraTag>(camera);
    // Re-fetch and set position after archetype migration
    auto* cam = world_->GetComponent<ECS::Camera3DData>(camera);
    cam->position = Vector3::Zero;

    // LOD対応メッシュを作成（100mの位置、Medium範囲=50-200m）
    ECS::Actor mesh = world_->CreateActor();
    world_->AddComponent<ECS::LocalToWorld>(mesh);
    world_->AddComponent<ECS::MeshData>(mesh);
    world_->AddComponent<ECS::LODRangeData>(mesh, ECS::LODRangeData::Medium());
    // Re-fetch pointers after all components added
    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(mesh);
    auto* meshData = world_->GetComponent<ECS::MeshData>(mesh);
    ltw->value = Matrix::CreateTranslation(100.0f, 0.0f, 0.0f);
    meshData->visible = false;

    world_->FixedUpdate(0.016f);

    EXPECT_TRUE(meshData->visible);
}

TEST_F(LODSystemTest, MeshNotVisibleWhenTooClose)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    world_->AddComponent<ECS::ActiveCameraTag>(camera);
    auto* cam = world_->GetComponent<ECS::Camera3DData>(camera);
    cam->position = Vector3::Zero;

    // 10mの位置、Medium範囲=50-200m → 範囲外（近すぎる）
    ECS::Actor mesh = world_->CreateActor();
    world_->AddComponent<ECS::LocalToWorld>(mesh);
    world_->AddComponent<ECS::MeshData>(mesh);
    world_->AddComponent<ECS::LODRangeData>(mesh, ECS::LODRangeData::Medium());
    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(mesh);
    auto* meshData = world_->GetComponent<ECS::MeshData>(mesh);
    ltw->value = Matrix::CreateTranslation(10.0f, 0.0f, 0.0f);
    meshData->visible = true;

    world_->FixedUpdate(0.016f);

    EXPECT_FALSE(meshData->visible);
}

TEST_F(LODSystemTest, MeshNotVisibleWhenTooFar)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);
    world_->AddComponent<ECS::ActiveCameraTag>(camera);
    auto* cam = world_->GetComponent<ECS::Camera3DData>(camera);
    cam->position = Vector3::Zero;

    // 300mの位置、Medium範囲=50-200m → 範囲外（遠すぎる）
    ECS::Actor mesh = world_->CreateActor();
    world_->AddComponent<ECS::LocalToWorld>(mesh);
    world_->AddComponent<ECS::MeshData>(mesh);
    world_->AddComponent<ECS::LODRangeData>(mesh, ECS::LODRangeData::Medium());
    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(mesh);
    auto* meshData = world_->GetComponent<ECS::MeshData>(mesh);
    ltw->value = Matrix::CreateTranslation(300.0f, 0.0f, 0.0f);
    meshData->visible = true;

    world_->FixedUpdate(0.016f);

    EXPECT_FALSE(meshData->visible);
}

TEST_F(LODSystemTest, NoCameraDoesNotCrash)
{
    // アクティブカメラなしでもクラッシュしない
    ECS::Actor mesh = world_->CreateActor();
    auto* ltw = world_->AddComponent<ECS::LocalToWorld>(mesh);
    ltw->value = Matrix::Identity;
    auto* meshData = world_->AddComponent<ECS::MeshData>(mesh);
    meshData->visible = true;
    world_->AddComponent<ECS::LODRangeData>(mesh, ECS::LODRangeData::Medium());

    // クラッシュしないことを確認
    EXPECT_NO_THROW(world_->FixedUpdate(0.016f));

    // カメラがないのでvisibleは変更されない（元の値を維持）
    EXPECT_TRUE(meshData->visible);
}

//============================================================================
// ActiveCameraTag テスト
//============================================================================
class ActiveCameraTagTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ActiveCameraTagTest, TagIsZeroSize)
{
    EXPECT_EQ(sizeof(ECS::ActiveCameraTag), 1);  // 空の構造体は最小1バイト
}

TEST_F(ActiveCameraTagTest, CanAddAndRemoveTag)
{
    ECS::Actor camera = world_->CreateActor();
    world_->AddComponent<ECS::Camera3DData>(camera, 60.0f, 16.0f / 9.0f);

    EXPECT_FALSE(world_->HasComponent<ECS::ActiveCameraTag>(camera));

    world_->AddComponent<ECS::ActiveCameraTag>(camera);
    EXPECT_TRUE(world_->HasComponent<ECS::ActiveCameraTag>(camera));

    world_->RemoveComponent<ECS::ActiveCameraTag>(camera);
    EXPECT_FALSE(world_->HasComponent<ECS::ActiveCameraTag>(camera));
}

} // namespace
