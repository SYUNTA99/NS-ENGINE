//----------------------------------------------------------------------------
//! @file   ecs_systems_test.cpp
//! @brief  ECSシステムのテスト（優先度、SystemGraph等）
//!
//! 注: TransformSystem のテストは fine_grained_systems_test.cpp に移行済み
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/systems/transform/transform_system.h"
#include <vector>

namespace
{

//============================================================================
// テスト用システム（優先度テスト用）
//============================================================================
static std::vector<int> g_priorityOrder;
static bool g_updateCalled = false;
static bool g_renderCalled = false;

class PrioritySystem0 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(0); }
    int Priority() const override { return 0; }
    const char* Name() const override { return "PrioritySystem0"; }
};

class PrioritySystem50 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(50); }
    int Priority() const override { return 50; }
    const char* Name() const override { return "PrioritySystem50"; }
};

class PrioritySystem100 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(100); }
    int Priority() const override { return 100; }
    const char* Name() const override { return "PrioritySystem100"; }
};

class TestUpdateOnlySystem final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_updateCalled = true; }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestUpdateOnlySystem"; }
};

class TestRenderOnlySystem final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_renderCalled = true; }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestRenderOnlySystem"; }
};

//============================================================================
// システム優先度テスト
//============================================================================
TEST(SystemPriorityTest, SystemsExecuteInPriorityOrder)
{
    g_priorityOrder.clear();
    ECS::World world;

    // 異なる優先度でシステムを登録（順番をシャッフル）
    world.RegisterSystem<PrioritySystem100>();  // priority 100
    world.RegisterSystem<PrioritySystem0>();    // priority 0
    world.RegisterSystem<PrioritySystem50>();   // priority 50

    world.FixedUpdate(0.016f);

    ASSERT_EQ(g_priorityOrder.size(), 3u);
    EXPECT_EQ(g_priorityOrder[0], 0);
    EXPECT_EQ(g_priorityOrder[1], 50);
    EXPECT_EQ(g_priorityOrder[2], 100);
}

//============================================================================
// RenderSystem登録テスト
//============================================================================
TEST(RenderSystemTest, UpdateAndRenderSystemsExecuteSeparately)
{
    g_updateCalled = false;
    g_renderCalled = false;
    ECS::World world;

    world.RegisterSystem<TestUpdateOnlySystem>();
    world.RegisterRenderSystem<TestRenderOnlySystem>();

    // FixedUpdateのみ呼び出し
    world.FixedUpdate(0.016f);
    EXPECT_TRUE(g_updateCalled);
    EXPECT_FALSE(g_renderCalled);

    // Renderのみ呼び出し
    g_updateCalled = false;
    world.Render(0.5f);
    EXPECT_FALSE(g_updateCalled);
    EXPECT_TRUE(g_renderCalled);
}

//============================================================================
// SystemGraph テスト
//============================================================================
#include "engine/ecs/system_graph.h"

// SystemGraphテスト用のダミーシステム
class SystemA final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(1); }
    int Priority() const override { return 10; }
    const char* Name() const override { return "SystemA"; }
};

class SystemB final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(2); }
    int Priority() const override { return 20; }
    const char* Name() const override { return "SystemB"; }
};

class SystemC final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_priorityOrder.push_back(3); }
    int Priority() const override { return 30; }
    const char* Name() const override { return "SystemC"; }
};

class SystemGraphTest : public ::testing::Test {
protected:
    ECS::SystemGraph graph_;

    void SetUp() override {
        graph_.Clear();
    }

    // ヘルパー: 新APIでノードを追加
    void AddNode(ECS::SystemId id, int priority, const char* name,
                 const std::vector<ECS::SystemId>& runAfter = {},
                 const std::vector<ECS::SystemId>& runBefore = {}) {
        graph_.AddNode(id, priority, runAfter, runBefore, name);
    }
};

TEST_F(SystemGraphTest, AddNode_Works)
{
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    AddNode(idA, 10, "SystemA");

    EXPECT_TRUE(graph_.HasNode(idA));
    EXPECT_EQ(graph_.NodeCount(), 1u);
}

TEST_F(SystemGraphTest, TopologicalSort_Empty)
{
    auto sorted = graph_.TopologicalSort();
    EXPECT_TRUE(sorted.empty());
}

TEST_F(SystemGraphTest, TopologicalSort_SingleNode)
{
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    AddNode(idA, 10, "SystemA");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 1u);
    EXPECT_EQ(sorted[0], idA);
}

TEST_F(SystemGraphTest, TopologicalSort_LinearChain)
{
    // A -> B -> C (AはBの前、BはCの前)
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    ECS::SystemId idB = std::type_index(typeid(SystemB));
    ECS::SystemId idC = std::type_index(typeid(SystemC));

    AddNode(idC, 30, "SystemC", {idB}, {});  // 逆順で追加
    AddNode(idB, 20, "SystemB", {idA}, {});
    AddNode(idA, 10, "SystemA");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 3u);
    EXPECT_EQ(sorted[0], idA);
    EXPECT_EQ(sorted[1], idB);
    EXPECT_EQ(sorted[2], idC);
}

TEST_F(SystemGraphTest, TopologicalSort_Diamond)
{
    // ダイヤモンド依存: A -> B, A -> C, B -> D, C -> D
    // 結果: A, [B または C], [C または B], D
    class SystemD final : public ECS::ISystem {
    public:
        void OnUpdate(ECS::World&, float) override {}
        int Priority() const override { return 40; }
        const char* Name() const override { return "SystemD"; }
    };

    ECS::SystemId idA = std::type_index(typeid(SystemA));
    ECS::SystemId idB = std::type_index(typeid(SystemB));
    ECS::SystemId idC = std::type_index(typeid(SystemC));
    ECS::SystemId idD = std::type_index(typeid(SystemD));

    AddNode(idD, 40, "SystemD", {idB, idC}, {});
    AddNode(idC, 25, "SystemC", {idA}, {});
    AddNode(idB, 20, "SystemB", {idA}, {});
    AddNode(idA, 10, "SystemA");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 4u);

    // Aは最初
    EXPECT_EQ(sorted[0], idA);

    // Dは最後
    EXPECT_EQ(sorted[3], idD);

    // BとCは中間（順序は優先度による）
    // B(priority 20) < C(priority 25) なのでBが先
    EXPECT_EQ(sorted[1], idB);
    EXPECT_EQ(sorted[2], idC);
}

TEST_F(SystemGraphTest, PriorityFallback_NoDependencies)
{
    // 依存関係がない場合、優先度順にソート
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    ECS::SystemId idB = std::type_index(typeid(SystemB));
    ECS::SystemId idC = std::type_index(typeid(SystemC));

    AddNode(idC, 30, "SystemC");
    AddNode(idA, 10, "SystemA");
    AddNode(idB, 20, "SystemB");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 3u);

    // 優先度順: A(10) < B(20) < C(30)
    EXPECT_EQ(sorted[0], idA);
    EXPECT_EQ(sorted[1], idB);
    EXPECT_EQ(sorted[2], idC);
}

TEST_F(SystemGraphTest, RunBefore_CreatesEdge)
{
    // runBefore: A runs before B -> A -> B
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    ECS::SystemId idB = std::type_index(typeid(SystemB));

    AddNode(idB, 10, "SystemB");
    AddNode(idA, 100, "SystemA", {}, {idB});  // runBefore = {idB}

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 2u);

    // AがBの前
    EXPECT_EQ(sorted[0], idA);
    EXPECT_EQ(sorted[1], idB);
}

TEST_F(SystemGraphTest, Clear_RemovesAll)
{
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    AddNode(idA, 10, "SystemA");
    EXPECT_EQ(graph_.NodeCount(), 1u);

    graph_.Clear();
    EXPECT_EQ(graph_.NodeCount(), 0u);
}

TEST_F(SystemGraphTest, HasNode_ReturnsFalse_WhenNotPresent)
{
    EXPECT_FALSE(graph_.HasNode(std::type_index(typeid(SystemA))));
}

TEST_F(SystemGraphTest, TopologicalSort_ReturnsSortedIds)
{
    // 優先度順に並ぶことを確認（SystemEntryは使わない新API版）
    ECS::SystemId idA = std::type_index(typeid(SystemA));
    ECS::SystemId idB = std::type_index(typeid(SystemB));

    AddNode(idB, 20, "SystemB");
    AddNode(idA, 10, "SystemA");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 2u);

    // 優先度順
    EXPECT_EQ(sorted[0], idA);
    EXPECT_EQ(sorted[1], idB);
}

//============================================================================
// RenderSystemGraph テスト
//============================================================================
class RenderSystemA final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_priorityOrder.push_back(1); }
    int Priority() const override { return 10; }
    const char* Name() const override { return "RenderSystemA"; }
};

class RenderSystemB final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_priorityOrder.push_back(2); }
    int Priority() const override { return 20; }
    const char* Name() const override { return "RenderSystemB"; }
};

class RenderSystemGraphTest : public ::testing::Test {
protected:
    ECS::RenderSystemGraph graph_;

    void SetUp() override {
        graph_.Clear();
    }

    // ヘルパー: 新APIでノードを追加
    void AddNode(ECS::SystemId id, int priority, const char* name,
                 const std::vector<ECS::SystemId>& runAfter = {},
                 const std::vector<ECS::SystemId>& runBefore = {}) {
        graph_.AddNode(id, priority, runAfter, runBefore, name);
    }
};

TEST_F(RenderSystemGraphTest, AddNode_Works)
{
    ECS::SystemId idA = std::type_index(typeid(RenderSystemA));
    AddNode(idA, 10, "RenderSystemA");

    EXPECT_TRUE(graph_.HasNode(idA));
    EXPECT_EQ(graph_.NodeCount(), 1u);
}

TEST_F(RenderSystemGraphTest, TopologicalSort_PriorityOrder)
{
    ECS::SystemId idA = std::type_index(typeid(RenderSystemA));
    ECS::SystemId idB = std::type_index(typeid(RenderSystemB));

    AddNode(idB, 20, "RenderSystemB");
    AddNode(idA, 10, "RenderSystemA");

    auto sorted = graph_.TopologicalSort();
    ASSERT_EQ(sorted.size(), 2u);

    // 優先度順
    EXPECT_EQ(sorted[0], idA);
    EXPECT_EQ(sorted[1], idB);
}

TEST_F(RenderSystemGraphTest, Clear_RemovesAll)
{
    ECS::SystemId idA = std::type_index(typeid(RenderSystemA));
    AddNode(idA, 10, "RenderSystemA");
    EXPECT_EQ(graph_.NodeCount(), 1u);

    graph_.Clear();
    EXPECT_EQ(graph_.NodeCount(), 0u);
}

} // namespace
