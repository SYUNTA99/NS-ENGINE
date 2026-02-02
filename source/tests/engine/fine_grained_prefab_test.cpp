//----------------------------------------------------------------------------
//! @file   fine_grained_prefab_test.cpp
//! @brief  Fine-grained Components + Prefab 統合テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/prefab.h"
#include "engine/ecs/components/transform/transform_components.h"

namespace
{

//============================================================================
// Prefab + LocalTransform/LocalToWorld テスト
//============================================================================
class FineGrainedPrefabTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(FineGrainedPrefabTest, CreatePrefabWithLocalTransform)
{
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>()
        .Build();

    EXPECT_TRUE(prefab.IsValid());
}

TEST_F(FineGrainedPrefabTest, InstantiatePrefabWithLocalTransform)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    t.rotation = Quaternion::Identity;
    t.scale = Vector3::One;

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    EXPECT_TRUE(actor.IsValid());
    EXPECT_TRUE(world_->HasComponent<ECS::LocalTransform>(actor));

    auto* transform = world_->GetComponent<ECS::LocalTransform>(actor);
    EXPECT_NEAR(transform->position.x, 10.0f, 0.001f);
    EXPECT_NEAR(transform->position.y, 20.0f, 0.001f);
    EXPECT_NEAR(transform->position.z, 30.0f, 0.001f);
}

TEST_F(FineGrainedPrefabTest, CreatePrefabWithLocalToWorld)
{
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalToWorld>()
        .Build();

    EXPECT_TRUE(prefab.IsValid());
}

TEST_F(FineGrainedPrefabTest, InstantiatePrefabWithLocalToWorld)
{
    Matrix mat = Matrix::CreateTranslation(1.0f, 2.0f, 3.0f);
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalToWorld>(mat)
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    EXPECT_EQ(ltw->value, mat);
}

TEST_F(FineGrainedPrefabTest, CreatePrefabWithFullTransform)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    t.rotation = Quaternion::Identity;
    t.scale = Vector3(2.0f, 2.0f, 2.0f);

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::LocalToWorld>()
        .Build();

    EXPECT_TRUE(prefab.IsValid());
}

TEST_F(FineGrainedPrefabTest, InstantiatePrefabWithFullTransform)
{
    ECS::LocalTransform t;
    t.position = Vector3(100.0f, 200.0f, 300.0f);
    t.rotation = Quaternion::Identity;
    t.scale = Vector3(5.0f, 5.0f, 5.0f);

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::LocalToWorld>()
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    EXPECT_TRUE(world_->HasComponent<ECS::LocalTransform>(actor));
    EXPECT_TRUE(world_->HasComponent<ECS::LocalToWorld>(actor));

    auto* transform = world_->GetComponent<ECS::LocalTransform>(actor);
    EXPECT_NEAR(transform->position.x, 100.0f, 0.001f);
}

TEST_F(FineGrainedPrefabTest, InstantiateMultipleTimes)
{
    ECS::LocalTransform t;
    t.position = Vector3(1.0f, 2.0f, 3.0f);

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Build();

    ECS::Actor actor1 = world_->Instantiate(prefab);
    ECS::Actor actor2 = world_->Instantiate(prefab);
    ECS::Actor actor3 = world_->Instantiate(prefab);

    EXPECT_NE(actor1, actor2);
    EXPECT_NE(actor2, actor3);
    EXPECT_NE(actor1, actor3);

    // All should have the same initial position
    auto* t1 = world_->GetComponent<ECS::LocalTransform>(actor1);
    auto* t2 = world_->GetComponent<ECS::LocalTransform>(actor2);
    auto* t3 = world_->GetComponent<ECS::LocalTransform>(actor3);

    EXPECT_NEAR(t1->position.x, 1.0f, 0.001f);
    EXPECT_NEAR(t2->position.x, 1.0f, 0.001f);
    EXPECT_NEAR(t3->position.x, 1.0f, 0.001f);
}

TEST_F(FineGrainedPrefabTest, ModifyInstanceDoesNotAffectOthers)
{
    ECS::LocalTransform t;
    t.position = Vector3::Zero;

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Build();

    ECS::Actor actor1 = world_->Instantiate(prefab);
    ECS::Actor actor2 = world_->Instantiate(prefab);

    // Modify actor1's position
    auto* t1 = world_->GetComponent<ECS::LocalTransform>(actor1);
    t1->position = Vector3(999.0f, 999.0f, 999.0f);

    // actor2 should be unaffected
    auto* t2 = world_->GetComponent<ECS::LocalTransform>(actor2);
    EXPECT_NEAR(t2->position.x, 0.0f, 0.001f);
}

TEST_F(FineGrainedPrefabTest, PrefabWithTagComponent)
{
    ECS::LocalTransform t;
    t.position = Vector3::Zero;

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::LocalToWorld>()
        .Add<ECS::TransformDirty>()
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    EXPECT_TRUE(world_->HasComponent<ECS::TransformDirty>(actor));
}

TEST_F(FineGrainedPrefabTest, PrefabWithStaticTransformTag)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 0.0f, 0.0f);

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::LocalToWorld>()
        .Add<ECS::StaticTransform>()
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    EXPECT_TRUE(world_->HasComponent<ECS::StaticTransform>(actor));
}

TEST_F(FineGrainedPrefabTest, PrefabWithHierarchyRootTag)
{
    ECS::LocalTransform t;
    t.position = Vector3::Zero;

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::LocalToWorld>()
        .Add<ECS::HierarchyRoot>()
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    EXPECT_TRUE(world_->HasComponent<ECS::HierarchyRoot>(actor));
}

TEST_F(FineGrainedPrefabTest, PrefabWithHierarchyDepthData)
{
    ECS::LocalTransform t;
    t.position = Vector3::Zero;

    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>(t)
        .Add<ECS::HierarchyDepthData>(0)
        .Build();

    ECS::Actor actor = world_->Instantiate(prefab);

    auto* hd = world_->GetComponent<ECS::HierarchyDepthData>(actor);
    EXPECT_EQ(hd->depth, 0);
}

//============================================================================
// Prefab メモリレイアウト テスト
//============================================================================
class FineGrainedPrefabMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(FineGrainedPrefabMemoryTest, PrefabDataSizeCorrect)
{
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>()
        .Build();

    EXPECT_EQ(prefab.GetComponentDataSize(), 48u);  // LocalTransform is 48 bytes
}

TEST_F(FineGrainedPrefabMemoryTest, PrefabDataSizeWithMultipleComponents)
{
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>()       // 48 bytes
        .Add<ECS::LocalToWorld>()         // 64 bytes
        .Build();

    // Archetype aligns components, so size may include padding
    EXPECT_GE(prefab.GetComponentDataSize(), 112u);
}

TEST_F(FineGrainedPrefabMemoryTest, GetComponentOffset)
{
    auto prefab = world_->CreatePrefab()
        .Add<ECS::LocalTransform>()
        .Add<ECS::LocalToWorld>()
        .Build();

    size_t transformOffset = prefab.GetComponentOffset<ECS::LocalTransform>();
    size_t ltwOffset = prefab.GetComponentOffset<ECS::LocalToWorld>();

    // Offsets should be valid (not SIZE_MAX)
    EXPECT_NE(transformOffset, SIZE_MAX);
    EXPECT_NE(ltwOffset, SIZE_MAX);

    // Offsets should be different
    EXPECT_NE(transformOffset, ltwOffset);
}

} // namespace
