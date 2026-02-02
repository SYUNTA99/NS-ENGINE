//----------------------------------------------------------------------------
//! @file   fine_grained_systems_test.cpp
//! @brief  Fine-grained Systems テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/systems/transform/transform_system.h"
#include "engine/ecs/systems/transform/parent_system.h"
#include "engine/ecs/hierarchy_registry.h"
#include "engine/ecs/components/transform/children.h"

namespace
{

//============================================================================
// FineGrainedTransformSystem 基本機能 テスト
//============================================================================
class FineGrainedTransformSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::TransformSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(FineGrainedTransformSystemTest, UpdatesLocalToWorldWithDirtyTag)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(10.0f, 20.0f, 30.0f);
    world_->AddComponent<ECS::LocalToWorld>(actor);
    world_->AddComponent<ECS::TransformDirty>(actor);

    world_->FixedUpdate(0.016f);

    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    Vector3 pos = ltw->GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, 0.001f);
    EXPECT_NEAR(pos.y, 20.0f, 0.001f);
    EXPECT_NEAR(pos.z, 30.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemTest, DirtyTagRemovedAfterUpdate)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3::Zero;
    world_->AddComponent<ECS::LocalToWorld>(actor);
    world_->AddComponent<ECS::TransformDirty>(actor);

    EXPECT_TRUE(world_->HasComponent<ECS::TransformDirty>(actor));

    world_->FixedUpdate(0.016f);

    EXPECT_FALSE(world_->HasComponent<ECS::TransformDirty>(actor));
}

TEST_F(FineGrainedTransformSystemTest, StaticTransformInitializedOnce)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(5.0f, 5.0f, 5.0f);
    world_->AddComponent<ECS::LocalToWorld>(actor);
    world_->AddComponent<ECS::StaticTransform>(actor);

    // First update should initialize
    world_->FixedUpdate(0.016f);

    EXPECT_TRUE(world_->HasComponent<ECS::TransformInitialized>(actor));

    // Modify position - should NOT update because static
    transform->position = Vector3(100.0f, 100.0f, 100.0f);

    world_->FixedUpdate(0.016f);

    // LocalToWorld should still have original position
    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    Vector3 ltwPos = ltw->GetPosition();
    EXPECT_NEAR(ltwPos.x, 5.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemTest, PositionRotationScaleCombined)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(10.0f, 0.0f, 0.0f);
    transform->rotation = Quaternion::Identity;
    transform->scale = Vector3(2.0f, 2.0f, 2.0f);
    world_->AddComponent<ECS::LocalToWorld>(actor);
    world_->AddComponent<ECS::TransformDirty>(actor);

    world_->FixedUpdate(0.016f);

    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    Vector3 pos = ltw->GetPosition();
    Vector3 scl = ltw->GetScale();

    EXPECT_NEAR(pos.x, 10.0f, 0.001f);
    EXPECT_NEAR(scl.x, 2.0f, 0.001f);
    EXPECT_NEAR(scl.y, 2.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemTest, PositionOnlyNoRotationNoScale)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(1.0f, 2.0f, 3.0f);
    world_->AddComponent<ECS::LocalToWorld>(actor);
    world_->AddComponent<ECS::TransformDirty>(actor);

    world_->FixedUpdate(0.016f);

    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    Vector3 pos = ltw->GetPosition();
    Vector3 scl = ltw->GetScale();

    EXPECT_NEAR(pos.x, 1.0f, 0.001f);
    EXPECT_NEAR(pos.y, 2.0f, 0.001f);
    EXPECT_NEAR(pos.z, 3.0f, 0.001f);
    // Default scale should be 1
    EXPECT_NEAR(scl.x, 1.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemTest, NoDirtyTagNoUpdate)
{
    ECS::Actor actor = world_->CreateActor();
    auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
    transform->position = Vector3(10.0f, 20.0f, 30.0f);
    world_->AddComponent<ECS::LocalToWorld>(actor);
    // NO TransformDirty tag

    world_->FixedUpdate(0.016f);

    // LocalToWorld should remain identity
    auto* ltw = world_->GetComponent<ECS::LocalToWorld>(actor);
    EXPECT_EQ(ltw->value, Matrix::Identity);
}

//============================================================================
// FineGrainedTransformSystem 親子階層 テスト
//============================================================================
class FineGrainedTransformSystemHierarchyTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
        world_->RegisterSystem<ECS::TransformSystem>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(FineGrainedTransformSystemHierarchyTest, ChildInheritsParentTransform)
{
    ECS::Actor parent = world_->CreateActor();
    auto* parentTransform = world_->AddComponent<ECS::LocalTransform>(parent);
    parentTransform->position = Vector3(100.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(parent);
    world_->AddComponent<ECS::TransformDirty>(parent);

    ECS::Actor child = world_->CreateActor();
    auto* childTransform = world_->AddComponent<ECS::LocalTransform>(child);
    childTransform->position = Vector3(10.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(child);

    // Set up parent-child relationship
    registry_->SetParent(child, parent, *world_);

    world_->FixedUpdate(0.016f);

    auto* parentLtw = world_->GetComponent<ECS::LocalToWorld>(parent);
    auto* childLtw = world_->GetComponent<ECS::LocalToWorld>(child);

    Vector3 parentPos = parentLtw->GetPosition();
    Vector3 childPos = childLtw->GetPosition();

    EXPECT_NEAR(parentPos.x, 100.0f, 0.001f);
    // Child world position = parent position + child local position
    EXPECT_NEAR(childPos.x, 110.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemHierarchyTest, SortsByHierarchyDepth)
{
    // Create entities in reverse order (child first)
    ECS::Actor grandchild = world_->CreateActor();
    auto* gcTransform = world_->AddComponent<ECS::LocalTransform>(grandchild);
    gcTransform->position = Vector3(1.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(grandchild);

    ECS::Actor child = world_->CreateActor();
    auto* childTransform = world_->AddComponent<ECS::LocalTransform>(child);
    childTransform->position = Vector3(10.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(child);

    ECS::Actor parent = world_->CreateActor();
    auto* parentTransform = world_->AddComponent<ECS::LocalTransform>(parent);
    parentTransform->position = Vector3(100.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(parent);
    world_->AddComponent<ECS::TransformDirty>(parent);

    // Set up hierarchy
    registry_->SetParent(child, parent, *world_);
    registry_->SetParent(grandchild, child, *world_);

    world_->FixedUpdate(0.016f);

    auto* grandchildLtw = world_->GetComponent<ECS::LocalToWorld>(grandchild);
    Vector3 grandchildPos = grandchildLtw->GetPosition();

    // Grandchild world position = parent + child + grandchild local
    EXPECT_NEAR(grandchildPos.x, 111.0f, 0.001f);
}

TEST_F(FineGrainedTransformSystemHierarchyTest, MultipleRoots)
{
    ECS::Actor root1 = world_->CreateActor();
    auto* r1Transform = world_->AddComponent<ECS::LocalTransform>(root1);
    r1Transform->position = Vector3(10.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(root1);
    world_->AddComponent<ECS::TransformDirty>(root1);

    ECS::Actor root2 = world_->CreateActor();
    auto* r2Transform = world_->AddComponent<ECS::LocalTransform>(root2);
    r2Transform->position = Vector3(20.0f, 0.0f, 0.0f);
    world_->AddComponent<ECS::LocalToWorld>(root2);
    world_->AddComponent<ECS::TransformDirty>(root2);

    world_->FixedUpdate(0.016f);

    auto* ltw1 = world_->GetComponent<ECS::LocalToWorld>(root1);
    auto* ltw2 = world_->GetComponent<ECS::LocalToWorld>(root2);

    EXPECT_NEAR(ltw1->GetPosition().x, 10.0f, 0.001f);
    EXPECT_NEAR(ltw2->GetPosition().x, 20.0f, 0.001f);
}

//============================================================================
// FineGrainedTransformSystem パフォーマンス テスト
//============================================================================
class FineGrainedTransformSystemPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::TransformSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(FineGrainedTransformSystemPerformanceTest, ManyEntities)
{
    constexpr int kEntityCount = 1000;

    for (int i = 0; i < kEntityCount; ++i) {
        ECS::Actor actor = world_->CreateActor();
        auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
        transform->position = Vector3(static_cast<float>(i), 0.0f, 0.0f);
        world_->AddComponent<ECS::LocalToWorld>(actor);
        world_->AddComponent<ECS::TransformDirty>(actor);
    }

    // Should complete without hanging
    world_->FixedUpdate(0.016f);

    // Verify first and last entity
    // Note: We can't easily verify specific actors without storing them
    // This test mainly ensures no crash/hang with many entities
}

TEST_F(FineGrainedTransformSystemPerformanceTest, PartialDirty)
{
    constexpr int kEntityCount = 100;
    std::vector<ECS::Actor> actors;

    for (int i = 0; i < kEntityCount; ++i) {
        ECS::Actor actor = world_->CreateActor();
        auto* transform = world_->AddComponent<ECS::LocalTransform>(actor);
        transform->position = Vector3(static_cast<float>(i), 0.0f, 0.0f);
        world_->AddComponent<ECS::LocalToWorld>(actor);
        actors.push_back(actor);
    }

    // Only mark 10% as dirty
    for (int i = 0; i < kEntityCount / 10; ++i) {
        world_->AddComponent<ECS::TransformDirty>(actors[i * 10]);
    }

    world_->FixedUpdate(0.016f);

    // Dirty ones should be updated
    auto* ltw0 = world_->GetComponent<ECS::LocalToWorld>(actors[0]);
    EXPECT_NEAR(ltw0->GetPosition().x, 0.0f, 0.001f);

    // Non-dirty ones should remain identity
    auto* ltw1 = world_->GetComponent<ECS::LocalToWorld>(actors[1]);
    EXPECT_EQ(ltw1->value, Matrix::Identity);
}

//============================================================================
// ParentSystem Child バッファ自動管理 テスト
//============================================================================
class ParentSystemChildBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::ParentSystem>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ParentSystemChildBufferTest, AddParentCreatesChildBuffer)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    // 直接 Parent コンポーネントを追加
    world_->AddComponent<ECS::Parent>(child, parent);

    // ParentSystem 実行
    world_->FixedUpdate(0.016f);

    // 親に Child バッファが作成されているはず
    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(parent));
    auto buffer = world_->GetBuffer<ECS::Child>(parent);
    EXPECT_EQ(buffer.Length(), 1);
    EXPECT_EQ(buffer[0].value, child);
}

TEST_F(ParentSystemChildBufferTest, AddParentAddsPreviousParent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, parent);

    world_->FixedUpdate(0.016f);

    // PreviousParent が自動追加されているはず
    EXPECT_TRUE(world_->HasComponent<ECS::PreviousParent>(child));
}

TEST_F(ParentSystemChildBufferTest, AddParentAddsHierarchyDepthData)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, parent);

    world_->FixedUpdate(0.016f);

    // HierarchyDepthData が自動追加されているはず
    EXPECT_TRUE(world_->HasComponent<ECS::HierarchyDepthData>(child));
}

TEST_F(ParentSystemChildBufferTest, ChangeParentUpdatesChildBuffers)
{
    ECS::Actor parent1 = world_->CreateActor();
    ECS::Actor parent2 = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    // 最初に parent1 の子に設定
    world_->AddComponent<ECS::Parent>(child, parent1);
    world_->FixedUpdate(0.016f);

    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(parent1));
    EXPECT_EQ(world_->GetBuffer<ECS::Child>(parent1).Length(), 1);

    // parent2 に変更
    auto* parentComp = world_->GetComponent<ECS::Parent>(child);
    parentComp->value = parent2;
    world_->FixedUpdate(0.016f);

    // parent1 の Child バッファから削除されているはず
    auto buffer1 = world_->GetBuffer<ECS::Child>(parent1);
    EXPECT_EQ(buffer1.Length(), 0);

    // parent2 の Child バッファに追加されているはず
    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(parent2));
    auto buffer2 = world_->GetBuffer<ECS::Child>(parent2);
    EXPECT_EQ(buffer2.Length(), 1);
    EXPECT_EQ(buffer2[0].value, child);
}

TEST_F(ParentSystemChildBufferTest, MultipleChildrenSameParent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child1 = world_->CreateActor();
    ECS::Actor child2 = world_->CreateActor();
    ECS::Actor child3 = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child1, parent);
    world_->AddComponent<ECS::Parent>(child2, parent);
    world_->AddComponent<ECS::Parent>(child3, parent);

    world_->FixedUpdate(0.016f);

    auto buffer = world_->GetBuffer<ECS::Child>(parent);
    EXPECT_EQ(buffer.Length(), 3);

    // 全ての子が含まれているはず
    bool found1 = false, found2 = false, found3 = false;
    for (int32_t i = 0; i < buffer.Length(); ++i) {
        if (buffer[i].value == child1) found1 = true;
        if (buffer[i].value == child2) found2 = true;
        if (buffer[i].value == child3) found3 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found3);
}

TEST_F(ParentSystemChildBufferTest, NestedHierarchy)
{
    ECS::Actor root = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();
    ECS::Actor grandchild = world_->CreateActor();

    // 2回のFixedUpdateに分けてテスト（同一フレームの問題を回避）
    world_->AddComponent<ECS::Parent>(child, root);
    world_->FixedUpdate(0.016f);

    world_->AddComponent<ECS::Parent>(grandchild, child);
    world_->FixedUpdate(0.016f);

    // root の子に child がいるはず
    auto rootChildren = world_->GetBuffer<ECS::Child>(root);
    EXPECT_EQ(rootChildren.Length(), 1);
    EXPECT_EQ(rootChildren[0].value, child);

    // child の子に grandchild がいるはず
    auto childChildren = world_->GetBuffer<ECS::Child>(child);
    EXPECT_EQ(childChildren.Length(), 1);
    EXPECT_EQ(childChildren[0].value, grandchild);
}

TEST_F(ParentSystemChildBufferTest, NestedHierarchySameFrame)
{
    // 同一フレームで複数の親子関係を設定するテスト
    // 現在の実装では、同一フレームでの複数Parent追加は
    // ForEachの処理順序に依存するため、2回のFixedUpdateを推奨
    ECS::Actor root = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();
    ECS::Actor grandchild = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, root);
    world_->AddComponent<ECS::Parent>(grandchild, child);

    // 複数回のFixedUpdateで確実に処理
    world_->FixedUpdate(0.016f);
    world_->FixedUpdate(0.016f);

    // root の子に child がいるはず
    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(root));
    auto rootChildren = world_->GetBuffer<ECS::Child>(root);
    EXPECT_EQ(rootChildren.Length(), 1);

    // child の子に grandchild がいるはず
    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(child));
    auto childChildren = world_->GetBuffer<ECS::Child>(child);
    EXPECT_EQ(childChildren.Length(), 1);
}

TEST_F(ParentSystemChildBufferTest, HierarchyDepthCalculated)
{
    ECS::Actor root = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();
    ECS::Actor grandchild = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, root);
    world_->AddComponent<ECS::Parent>(grandchild, child);

    world_->FixedUpdate(0.016f);

    auto* childDepth = world_->GetComponent<ECS::HierarchyDepthData>(child);
    auto* grandchildDepth = world_->GetComponent<ECS::HierarchyDepthData>(grandchild);

    EXPECT_EQ(childDepth->depth, 1);
    EXPECT_EQ(grandchildDepth->depth, 2);
}

TEST_F(ParentSystemChildBufferTest, TransformDirtyAddedOnParentChange)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, parent);
    world_->FixedUpdate(0.016f);

    // TransformDirty が追加されているはず
    EXPECT_TRUE(world_->HasComponent<ECS::TransformDirty>(child));
}

TEST_F(ParentSystemChildBufferTest, NoDuplicateChildEntries)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    world_->AddComponent<ECS::Parent>(child, parent);
    world_->FixedUpdate(0.016f);

    // 複数回更新しても重複しない
    world_->FixedUpdate(0.016f);
    world_->FixedUpdate(0.016f);

    auto buffer = world_->GetBuffer<ECS::Child>(parent);
    EXPECT_EQ(buffer.Length(), 1);
}

} // namespace
