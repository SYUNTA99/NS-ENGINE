//----------------------------------------------------------------------------
//! @file   hierarchy_registry_test.cpp
//! @brief  HierarchyRegistry テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/hierarchy_registry.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/transform/children.h"

namespace
{

//============================================================================
// HierarchyRegistry 基本機能 テスト
//============================================================================
class HierarchyRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(HierarchyRegistryTest, InitiallyEmpty)
{
    EXPECT_EQ(registry_->GetRootCount(), 0u);
}

TEST_F(HierarchyRegistryTest, RegisterAsRoot)
{
    ECS::Actor actor = world_->CreateActor();
    registry_->RegisterAsRoot(actor);

    EXPECT_EQ(registry_->GetRootCount(), 1u);
    const auto& roots = registry_->GetRoots();
    EXPECT_EQ(roots[0], actor);
}

TEST_F(HierarchyRegistryTest, UnregisterFromRoot)
{
    ECS::Actor actor = world_->CreateActor();
    registry_->RegisterAsRoot(actor);
    EXPECT_EQ(registry_->GetRootCount(), 1u);

    registry_->UnregisterFromRoot(actor);
    EXPECT_EQ(registry_->GetRootCount(), 0u);
}

TEST_F(HierarchyRegistryTest, SetParentAddsChild)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(registry_->HasChildren(parent, *world_));
    EXPECT_EQ(registry_->GetChildCount(parent, *world_), 1u);
}

TEST_F(HierarchyRegistryTest, ClearParentRemovesChild)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    EXPECT_EQ(registry_->GetChildCount(parent, *world_), 1u);

    registry_->ClearParent(child, *world_);
    EXPECT_EQ(registry_->GetChildCount(parent, *world_), 0u);
}

TEST_F(HierarchyRegistryTest, GetChildrenReturnsCorrectList)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child1 = world_->CreateActor();
    ECS::Actor child2 = world_->CreateActor();

    registry_->SetParent(child1, parent, *world_);
    registry_->SetParent(child2, parent, *world_);

    auto children = registry_->GetChildren(parent, *world_);
    EXPECT_TRUE(children.IsCreated());
    EXPECT_EQ(children.Length(), 2);

    // Children stored in buffer (order may vary due to swap-back removal in other tests)
    bool foundChild1 = false, foundChild2 = false;
    for (const auto& child : children) {
        if (child.value == child1) foundChild1 = true;
        if (child.value == child2) foundChild2 = true;
    }
    EXPECT_TRUE(foundChild1);
    EXPECT_TRUE(foundChild2);
}

TEST_F(HierarchyRegistryTest, GetChildrenReturnsInvalidForNonParent)
{
    ECS::Actor actor = world_->CreateActor();
    auto children = registry_->GetChildren(actor, *world_);
    // No buffer added, so either invalid or empty
    EXPECT_TRUE(!children.IsCreated() || children.Length() == 0);
}

TEST_F(HierarchyRegistryTest, HasChildrenReturnsFalseForNonParent)
{
    ECS::Actor actor = world_->CreateActor();
    EXPECT_FALSE(registry_->HasChildren(actor, *world_));
}

TEST_F(HierarchyRegistryTest, Clear)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->RegisterAsRoot(parent);
    registry_->SetParent(child, parent, *world_);

    registry_->Clear();

    EXPECT_EQ(registry_->GetRootCount(), 0u);
    // Note: Child buffer still exists on the actor, but registry roots are cleared
}

TEST_F(HierarchyRegistryTest, NoDuplicateChildren)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    registry_->SetParent(child, parent, *world_);  // Duplicate add (same parent)

    EXPECT_EQ(registry_->GetChildCount(parent, *world_), 1u);
}

TEST_F(HierarchyRegistryTest, NoDuplicateRoots)
{
    ECS::Actor actor = world_->CreateActor();

    registry_->RegisterAsRoot(actor);
    registry_->RegisterAsRoot(actor);  // Duplicate add

    EXPECT_EQ(registry_->GetRootCount(), 1u);
}

//============================================================================
// HierarchyRegistry SetParent テスト
//============================================================================
class HierarchyRegistrySetParentTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(HierarchyRegistrySetParentTest, SetParentAddsToChildrenList)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(registry_->HasChildren(parent, *world_));
    EXPECT_EQ(registry_->GetChildCount(parent, *world_), 1u);
}

TEST_F(HierarchyRegistrySetParentTest, SetParentAddsParentDataComponent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(world_->HasComponent<ECS::Parent>(child));
    auto* pd = world_->GetComponent<ECS::Parent>(child);
    EXPECT_EQ(pd->value, parent);
}

TEST_F(HierarchyRegistrySetParentTest, SetParentAddsChildBuffer)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(world_->HasBuffer<ECS::Child>(parent));
    auto buffer = world_->GetBuffer<ECS::Child>(parent);
    EXPECT_EQ(buffer.Length(), 1);
    EXPECT_EQ(buffer[0].value, child);
}

TEST_F(HierarchyRegistrySetParentTest, SetParentUpdatesHierarchyDepth)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(world_->HasComponent<ECS::HierarchyDepthData>(child));
    auto* hd = world_->GetComponent<ECS::HierarchyDepthData>(child);
    EXPECT_EQ(hd->depth, 1);  // Parent is depth 0, child is depth 1
}

TEST_F(HierarchyRegistrySetParentTest, SetParentAddsTransformDirtyTag)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(world_->HasComponent<ECS::TransformDirty>(child));
}

TEST_F(HierarchyRegistrySetParentTest, SetParentToInvalidClearsParent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    EXPECT_TRUE(world_->HasComponent<ECS::Parent>(child));

    registry_->SetParent(child, ECS::Actor::Invalid(), *world_);
    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(child));
}

TEST_F(HierarchyRegistrySetParentTest, SetParentToInvalidAddsHierarchyRootTag)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    EXPECT_FALSE(world_->HasComponent<ECS::HierarchyRoot>(child));

    registry_->SetParent(child, ECS::Actor::Invalid(), *world_);
    EXPECT_TRUE(world_->HasComponent<ECS::HierarchyRoot>(child));
}

TEST_F(HierarchyRegistrySetParentTest, ClearParentEquivalentToSetParentInvalid)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    registry_->ClearParent(child, *world_);

    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(child));
    EXPECT_TRUE(world_->HasComponent<ECS::HierarchyRoot>(child));
}

TEST_F(HierarchyRegistrySetParentTest, ChangeParentUpdatesOldParentChildren)
{
    ECS::Actor parent1 = world_->CreateActor();
    ECS::Actor parent2 = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent1, *world_);
    EXPECT_TRUE(registry_->HasChildren(parent1, *world_));

    registry_->SetParent(child, parent2, *world_);
    EXPECT_FALSE(registry_->HasChildren(parent1, *world_));
    EXPECT_TRUE(registry_->HasChildren(parent2, *world_));
}

TEST_F(HierarchyRegistrySetParentTest, NestedHierarchyDepth)
{
    ECS::Actor root = world_->CreateActor();
    ECS::Actor child1 = world_->CreateActor();
    ECS::Actor child2 = world_->CreateActor();

    registry_->SetParent(child1, root, *world_);
    registry_->SetParent(child2, child1, *world_);

    auto* hd1 = world_->GetComponent<ECS::HierarchyDepthData>(child1);
    auto* hd2 = world_->GetComponent<ECS::HierarchyDepthData>(child2);

    EXPECT_EQ(hd1->depth, 1);
    EXPECT_EQ(hd2->depth, 2);
}

//============================================================================
// HierarchyRegistry RemoveActor テスト
//============================================================================
class HierarchyRegistryRemoveActorTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(HierarchyRegistryRemoveActorTest, RemoveActorClearsChildrenParent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    registry_->RemoveActor(parent, *world_);

    // Child should now be root (no parent)
    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(child));
}

TEST_F(HierarchyRegistryRemoveActorTest, RemoveActorUnregistersFromRoot)
{
    ECS::Actor actor = world_->CreateActor();
    registry_->RegisterAsRoot(actor);
    EXPECT_EQ(registry_->GetRootCount(), 1u);

    registry_->RemoveActor(actor, *world_);
    EXPECT_EQ(registry_->GetRootCount(), 0u);
}

TEST_F(HierarchyRegistryRemoveActorTest, RemoveActorRemovesFromParentChildren)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);
    EXPECT_TRUE(registry_->HasChildren(parent, *world_));

    registry_->RemoveActor(child, *world_);
    EXPECT_FALSE(registry_->HasChildren(parent, *world_));
}

//============================================================================
// HierarchyRegistry 深度更新 テスト
//============================================================================
class HierarchyRegistryDepthUpdateTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(HierarchyRegistryDepthUpdateTest, ReparentUpdatesChildrenDepth)
{
    ECS::Actor root = world_->CreateActor();
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();
    ECS::Actor grandchild = world_->CreateActor();

    // Build hierarchy: root -> parent -> child -> grandchild
    registry_->SetParent(parent, root, *world_);
    registry_->SetParent(child, parent, *world_);
    registry_->SetParent(grandchild, child, *world_);

    auto* hdParent = world_->GetComponent<ECS::HierarchyDepthData>(parent);
    auto* hdChild = world_->GetComponent<ECS::HierarchyDepthData>(child);
    auto* hdGrandchild = world_->GetComponent<ECS::HierarchyDepthData>(grandchild);

    EXPECT_EQ(hdParent->depth, 1);
    EXPECT_EQ(hdChild->depth, 2);
    EXPECT_EQ(hdGrandchild->depth, 3);
}

TEST_F(HierarchyRegistryDepthUpdateTest, MoveToRootUpdatesDepth)
{
    ECS::Actor root = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();
    ECS::Actor grandchild = world_->CreateActor();

    registry_->SetParent(child, root, *world_);
    registry_->SetParent(grandchild, child, *world_);

    // Move child to root level
    registry_->ClearParent(child, *world_);

    auto* hdChild = world_->GetComponent<ECS::HierarchyDepthData>(child);
    auto* hdGrandchild = world_->GetComponent<ECS::HierarchyDepthData>(grandchild);

    EXPECT_EQ(hdChild->depth, 0);
    EXPECT_EQ(hdGrandchild->depth, 1);
}

//============================================================================
// DynamicBuffer<Child> 直接操作テスト
//============================================================================
class ChildBufferDirectTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
    }

    void TearDown() override {
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
};

TEST_F(ChildBufferDirectTest, AddBufferDirectly)
{
    ECS::Actor parent = world_->CreateActor();

    // DynamicBuffer<Child>を直接追加
    auto buffer = world_->AddBuffer<ECS::Child>(parent);
    EXPECT_TRUE(buffer.IsCreated());
    EXPECT_EQ(buffer.Length(), 0);
}

TEST_F(ChildBufferDirectTest, AddChildrenDirectly)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child1 = world_->CreateActor();
    ECS::Actor child2 = world_->CreateActor();

    auto buffer = world_->AddBuffer<ECS::Child>(parent);
    buffer.Add(ECS::Child{child1});
    buffer.Add(ECS::Child{child2});

    EXPECT_EQ(buffer.Length(), 2);
    EXPECT_EQ(buffer[0].value, child1);
    EXPECT_EQ(buffer[1].value, child2);
}

TEST_F(ChildBufferDirectTest, IterateChildren)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child1 = world_->CreateActor();
    ECS::Actor child2 = world_->CreateActor();
    ECS::Actor child3 = world_->CreateActor();

    auto buffer = world_->AddBuffer<ECS::Child>(parent);
    buffer.Add(ECS::Child{child1});
    buffer.Add(ECS::Child{child2});
    buffer.Add(ECS::Child{child3});

    int count = 0;
    for (const auto& child : buffer) {
        EXPECT_TRUE(child.value.IsValid());
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(ChildBufferDirectTest, InlineCapacity)
{
    // Child is 4 bytes
    // Default inline storage = 128 - 24 = 104 bytes
    // 104 / 4 = 26 children inline
    constexpr int32_t expectedCapacity = (128 - 24) / sizeof(ECS::Child);
    EXPECT_EQ(ECS::InternalBufferCapacity<ECS::Child>::Value, expectedCapacity);
}

//============================================================================
// HierarchyRegistry 循環参照検出 テスト
//============================================================================
class HierarchyRegistryCycleDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_ = std::make_unique<ECS::World>();
        registry_ = std::make_unique<ECS::HierarchyRegistry>();
    }

    void TearDown() override {
        registry_.reset();
        world_.reset();
    }

    std::unique_ptr<ECS::World> world_;
    std::unique_ptr<ECS::HierarchyRegistry> registry_;
};

TEST_F(HierarchyRegistryCycleDetectionTest, IsAncestorOfReturnsTrueForDirectParent)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(registry_->IsAncestorOf(parent, child, *world_));
    EXPECT_FALSE(registry_->IsAncestorOf(child, parent, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, IsAncestorOfReturnsTrueForGrandparent)
{
    ECS::Actor grandparent = world_->CreateActor();
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(parent, grandparent, *world_);
    registry_->SetParent(child, parent, *world_);

    EXPECT_TRUE(registry_->IsAncestorOf(grandparent, child, *world_));
    EXPECT_TRUE(registry_->IsAncestorOf(grandparent, parent, *world_));
    EXPECT_TRUE(registry_->IsAncestorOf(parent, child, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, IsAncestorOfReturnsFalseForUnrelatedActors)
{
    ECS::Actor a = world_->CreateActor();
    ECS::Actor b = world_->CreateActor();

    EXPECT_FALSE(registry_->IsAncestorOf(a, b, *world_));
    EXPECT_FALSE(registry_->IsAncestorOf(b, a, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, IsAncestorOfReturnsFalseForSameActor)
{
    ECS::Actor a = world_->CreateActor();

    EXPECT_FALSE(registry_->IsAncestorOf(a, a, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, IsAncestorOfReturnsFalseForInvalidActors)
{
    ECS::Actor valid = world_->CreateActor();

    EXPECT_FALSE(registry_->IsAncestorOf(ECS::Actor::Invalid(), valid, *world_));
    EXPECT_FALSE(registry_->IsAncestorOf(valid, ECS::Actor::Invalid(), *world_));
    EXPECT_FALSE(registry_->IsAncestorOf(ECS::Actor::Invalid(), ECS::Actor::Invalid(), *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, WouldCreateCycleDetectsDirectCycle)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    // Setting parent's parent to child would create: child -> parent -> child
    EXPECT_TRUE(registry_->WouldCreateCycle(parent, child, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, WouldCreateCycleDetectsIndirectCycle)
{
    ECS::Actor a = world_->CreateActor();
    ECS::Actor b = world_->CreateActor();
    ECS::Actor c = world_->CreateActor();

    // Build: a -> b -> c
    registry_->SetParent(b, a, *world_);
    registry_->SetParent(c, b, *world_);

    // Setting a's parent to c would create: c -> a -> b -> c
    EXPECT_TRUE(registry_->WouldCreateCycle(a, c, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, WouldCreateCycleReturnsFalseForValidHierarchy)
{
    ECS::Actor a = world_->CreateActor();
    ECS::Actor b = world_->CreateActor();
    ECS::Actor c = world_->CreateActor();

    registry_->SetParent(b, a, *world_);

    // Setting c's parent to b is valid (no cycle)
    EXPECT_FALSE(registry_->WouldCreateCycle(c, b, *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, WouldCreateCycleReturnsFalseForInvalidParent)
{
    ECS::Actor child = world_->CreateActor();

    // Setting parent to Invalid is always valid (clears parent)
    EXPECT_FALSE(registry_->WouldCreateCycle(child, ECS::Actor::Invalid(), *world_));
}

TEST_F(HierarchyRegistryCycleDetectionTest, SetParentPreventsDirectCycle)
{
    ECS::Actor parent = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    registry_->SetParent(child, parent, *world_);

    // Attempt to create cycle: parent -> child -> parent
    registry_->SetParent(parent, child, *world_);

    // parent should NOT have child as parent (cycle prevented)
    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(parent));
}

TEST_F(HierarchyRegistryCycleDetectionTest, SetParentPreventsIndirectCycle)
{
    ECS::Actor a = world_->CreateActor();
    ECS::Actor b = world_->CreateActor();
    ECS::Actor c = world_->CreateActor();

    // Build: a -> b -> c
    registry_->SetParent(b, a, *world_);
    registry_->SetParent(c, b, *world_);

    // Verify hierarchy
    EXPECT_TRUE(world_->HasComponent<ECS::Parent>(b));
    EXPECT_EQ(world_->GetComponent<ECS::Parent>(b)->value, a);
    EXPECT_TRUE(world_->HasComponent<ECS::Parent>(c));
    EXPECT_EQ(world_->GetComponent<ECS::Parent>(c)->value, b);

    // Attempt to create cycle: a -> c (would create c -> a -> b -> c)
    registry_->SetParent(a, c, *world_);

    // a should NOT have c as parent (cycle prevented)
    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(a));
}

TEST_F(HierarchyRegistryCycleDetectionTest, SetParentPreventsSelfParent)
{
    ECS::Actor actor = world_->CreateActor();

    // Attempt to set self as parent
    registry_->SetParent(actor, actor, *world_);

    // actor should NOT have itself as parent
    EXPECT_FALSE(world_->HasComponent<ECS::Parent>(actor));
}

TEST_F(HierarchyRegistryCycleDetectionTest, SetParentAllowsValidReparenting)
{
    ECS::Actor root1 = world_->CreateActor();
    ECS::Actor root2 = world_->CreateActor();
    ECS::Actor child = world_->CreateActor();

    // Initial: root1 -> child
    registry_->SetParent(child, root1, *world_);
    EXPECT_EQ(world_->GetComponent<ECS::Parent>(child)->value, root1);

    // Reparent: root2 -> child (should succeed, no cycle)
    registry_->SetParent(child, root2, *world_);
    EXPECT_EQ(world_->GetComponent<ECS::Parent>(child)->value, root2);
}

TEST_F(HierarchyRegistryCycleDetectionTest, DeepHierarchyCycleDetection)
{
    // Create a deep hierarchy: a -> b -> c -> d -> e
    ECS::Actor a = world_->CreateActor();
    ECS::Actor b = world_->CreateActor();
    ECS::Actor c = world_->CreateActor();
    ECS::Actor d = world_->CreateActor();
    ECS::Actor e = world_->CreateActor();

    registry_->SetParent(b, a, *world_);
    registry_->SetParent(c, b, *world_);
    registry_->SetParent(d, c, *world_);
    registry_->SetParent(e, d, *world_);

    // Verify hierarchy depths
    EXPECT_EQ(world_->GetComponent<ECS::HierarchyDepthData>(b)->depth, 1);
    EXPECT_EQ(world_->GetComponent<ECS::HierarchyDepthData>(c)->depth, 2);
    EXPECT_EQ(world_->GetComponent<ECS::HierarchyDepthData>(d)->depth, 3);
    EXPECT_EQ(world_->GetComponent<ECS::HierarchyDepthData>(e)->depth, 4);

    // Attempt to create cycle at any level
    EXPECT_TRUE(registry_->WouldCreateCycle(a, e, *world_));
    EXPECT_TRUE(registry_->WouldCreateCycle(a, d, *world_));
    EXPECT_TRUE(registry_->WouldCreateCycle(a, c, *world_));
    EXPECT_TRUE(registry_->WouldCreateCycle(a, b, *world_));

    EXPECT_TRUE(registry_->WouldCreateCycle(b, e, *world_));
    EXPECT_TRUE(registry_->WouldCreateCycle(c, e, *world_));
    EXPECT_TRUE(registry_->WouldCreateCycle(d, e, *world_));
}

} // namespace
