//----------------------------------------------------------------------------
//! @file   ecs_test.cpp
//! @brief  ECSコア（Actor, ActorManager, ComponentStorage, World）のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/actor.h"
#include "engine/ecs/actor_manager.h"
#include "engine/ecs/component_storage.h"
#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include <vector>

namespace
{

//============================================================================
// テスト用コンポーネントデータ
//============================================================================
struct PositionData {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    PositionData() = default;
    PositionData(float px, float py, float pz) : x(px), y(py), z(pz) {}
};

struct VelocityData {
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;

    VelocityData() = default;
    VelocityData(float pvx, float pvy, float pvz) : vx(pvx), vy(pvy), vz(pvz) {}
};

struct HealthData {
    int hp = 100;
    int maxHp = 100;

    HealthData() = default;
    HealthData(int h, int m) : hp(h), maxHp(m) {}
};

//============================================================================
// テスト用システム
//============================================================================

// テスト結果を外部から参照可能にするためのグローバル変数
static bool g_systemCalled = false;
static float g_receivedDt = 0.0f;
static float g_receivedAlpha = 0.0f;
static std::vector<int> g_executionOrder;

// 基本的な更新システム
class TestUpdateSystem final : public ECS::ISystem {
public:
    void Execute(ECS::World&, float dt) override {
        g_systemCalled = true;
        g_receivedDt = dt;
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestUpdateSystem"; }
};

// 優先度付きシステム
class PrioritySystem1 final : public ECS::ISystem {
public:
    void Execute(ECS::World&, float) override { g_executionOrder.push_back(1); }
    int Priority() const override { return 0; }
    const char* Name() const override { return "PrioritySystem1"; }
};

class PrioritySystem2 final : public ECS::ISystem {
public:
    void Execute(ECS::World&, float) override { g_executionOrder.push_back(2); }
    int Priority() const override { return 100; }
    const char* Name() const override { return "PrioritySystem2"; }
};

class PrioritySystem3 final : public ECS::ISystem {
public:
    void Execute(ECS::World&, float) override { g_executionOrder.push_back(3); }
    int Priority() const override { return 200; }
    const char* Name() const override { return "PrioritySystem3"; }
};

// 基本的な描画システム
class TestRenderSystem final : public ECS::IRenderSystem {
public:
    void Render(ECS::World&, float alpha) override {
        g_systemCalled = true;
        g_receivedAlpha = alpha;
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestRenderSystem"; }
};

// 優先度付き描画システム
class RenderPrioritySystem1 final : public ECS::IRenderSystem {
public:
    void Render(ECS::World&, float) override { g_executionOrder.push_back(1); }
    int Priority() const override { return 0; }
    const char* Name() const override { return "RenderPrioritySystem1"; }
};

class RenderPrioritySystem2 final : public ECS::IRenderSystem {
public:
    void Render(ECS::World&, float) override { g_executionOrder.push_back(2); }
    int Priority() const override { return 100; }
    const char* Name() const override { return "RenderPrioritySystem2"; }
};

class RenderPrioritySystem3 final : public ECS::IRenderSystem {
public:
    void Render(ECS::World&, float) override { g_executionOrder.push_back(3); }
    int Priority() const override { return 200; }
    const char* Name() const override { return "RenderPrioritySystem3"; }
};

// コンポーネント変更システム
class MoveSystem final : public ECS::ISystem {
public:
    void Execute(ECS::World& w, float dt) override {
        w.ForEach<PositionData, VelocityData>(
            [dt](ECS::Actor, PositionData& pos, VelocityData& vel) {
                pos.x += vel.vx * dt;
            });
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "MoveSystem"; }
};

// テスト状態リセット用ヘルパー
void ResetTestState() {
    g_systemCalled = false;
    g_receivedDt = 0.0f;
    g_receivedAlpha = 0.0f;
    g_executionOrder.clear();
}

//============================================================================
// Actor テスト
//============================================================================
class ActorTest : public ::testing::Test {};

TEST_F(ActorTest, DefaultConstructorIsInvalid)
{
    ECS::Actor e;
    EXPECT_FALSE(e.IsValid());
    EXPECT_EQ(e.id, ECS::Actor::kInvalidId);
}

TEST_F(ActorTest, InvalidEntityConstant)
{
    ECS::Actor e = ECS::Actor::Invalid();
    EXPECT_FALSE(e.IsValid());
}

TEST_F(ActorTest, ConstructFromIndexAndGeneration)
{
    ECS::Actor e(42, 5);
    EXPECT_TRUE(e.IsValid());
    EXPECT_EQ(e.Index(), 42u);
    EXPECT_EQ(e.Generation(), 5u);
}

TEST_F(ActorTest, IndexMasking)
{
    // インデックスは20ビットまで
    uint32_t maxIndex = (1u << 20) - 1;
    ECS::Actor e(maxIndex, 0);
    EXPECT_EQ(e.Index(), maxIndex);
}

TEST_F(ActorTest, GenerationMasking)
{
    // 世代は12ビットまで
    uint32_t maxGen = (1u << 12) - 1;
    ECS::Actor e(0, maxGen);
    EXPECT_EQ(e.Generation(), maxGen);
}

TEST_F(ActorTest, EqualityOperator)
{
    ECS::Actor e1(10, 5);
    ECS::Actor e2(10, 5);
    ECS::Actor e3(10, 6);
    ECS::Actor e4(11, 5);

    EXPECT_TRUE(e1 == e2);
    EXPECT_FALSE(e1 == e3);
    EXPECT_FALSE(e1 == e4);
}

TEST_F(ActorTest, InequalityOperator)
{
    ECS::Actor e1(10, 5);
    ECS::Actor e2(10, 6);

    EXPECT_TRUE(e1 != e2);
}

TEST_F(ActorTest, LessThanOperator)
{
    ECS::Actor e1(10, 5);
    ECS::Actor e2(20, 5);

    EXPECT_TRUE(e1 < e2);
}

TEST_F(ActorTest, HashSupport)
{
    ECS::Actor e1(10, 5);
    ECS::Actor e2(10, 5);

    std::hash<ECS::Actor> hasher;
    EXPECT_EQ(hasher(e1), hasher(e2));
}

//============================================================================
// ActorManager テスト
//============================================================================
class ActorManagerTest : public ::testing::Test
{
protected:
    ECS::ActorManager manager_;
};

TEST_F(ActorManagerTest, InitiallyEmpty)
{
    EXPECT_EQ(manager_.Count(), 0u);
}

TEST_F(ActorManagerTest, CreateReturnsValidEntity)
{
    ECS::Actor e = manager_.Create();
    EXPECT_TRUE(e.IsValid());
}

TEST_F(ActorManagerTest, CreateIncrementsCount)
{
    manager_.Create();
    EXPECT_EQ(manager_.Count(), 1u);

    manager_.Create();
    EXPECT_EQ(manager_.Count(), 2u);
}

TEST_F(ActorManagerTest, CreateAssignsSequentialIndices)
{
    ECS::Actor e1 = manager_.Create();
    ECS::Actor e2 = manager_.Create();
    ECS::Actor e3 = manager_.Create();

    EXPECT_EQ(e1.Index(), 0u);
    EXPECT_EQ(e2.Index(), 1u);
    EXPECT_EQ(e3.Index(), 2u);
}

TEST_F(ActorManagerTest, IsAliveReturnsTrueForNewEntity)
{
    ECS::Actor e = manager_.Create();
    EXPECT_TRUE(manager_.IsAlive(e));
}

TEST_F(ActorManagerTest, IsAliveReturnsFalseForInvalidEntity)
{
    EXPECT_FALSE(manager_.IsAlive(ECS::Actor::Invalid()));
}

TEST_F(ActorManagerTest, DestroyDecrementsCount)
{
    ECS::Actor e1 = manager_.Create();
    ECS::Actor e2 = manager_.Create();
    EXPECT_EQ(manager_.Count(), 2u);

    manager_.Destroy(e1);
    EXPECT_EQ(manager_.Count(), 1u);
}

TEST_F(ActorManagerTest, DestroyMakesEntityNotAlive)
{
    ECS::Actor e = manager_.Create();
    manager_.Destroy(e);
    EXPECT_FALSE(manager_.IsAlive(e));
}

TEST_F(ActorManagerTest, DestroyedEntityIndexReused)
{
    ECS::Actor e1 = manager_.Create();
    uint32_t index = e1.Index();

    manager_.Destroy(e1);
    ECS::Actor e2 = manager_.Create();

    // 同じインデックスが再利用される
    EXPECT_EQ(e2.Index(), index);
    // 世代が異なる
    EXPECT_NE(e2.Generation(), e1.Generation());
}

TEST_F(ActorManagerTest, StaleEntityNotAlive)
{
    ECS::Actor e1 = manager_.Create();
    manager_.Destroy(e1);
    ECS::Actor e2 = manager_.Create();

    // 古いエンティティは生存していない
    EXPECT_FALSE(manager_.IsAlive(e1));
    // 新しいエンティティは生存している
    EXPECT_TRUE(manager_.IsAlive(e2));
}

TEST_F(ActorManagerTest, ClearRemovesAllEntities)
{
    manager_.Create();
    manager_.Create();
    manager_.Create();

    manager_.Clear();
    EXPECT_EQ(manager_.Count(), 0u);
}

TEST_F(ActorManagerTest, ForEachIteratesAliveEntities)
{
    ECS::Actor e1 = manager_.Create();
    ECS::Actor e2 = manager_.Create();
    ECS::Actor e3 = manager_.Create();
    manager_.Destroy(e2);

    std::vector<ECS::Actor> visited;
    manager_.ForEach([&visited](ECS::Actor e) {
        visited.push_back(e);
    });

    EXPECT_EQ(visited.size(), 2u);
    EXPECT_TRUE(std::find(visited.begin(), visited.end(), e1) != visited.end());
    EXPECT_TRUE(std::find(visited.begin(), visited.end(), e3) != visited.end());
}

TEST_F(ActorManagerTest, DoubleDestroyIsNoOp)
{
    ECS::Actor e = manager_.Create();
    manager_.Destroy(e);
    EXPECT_EQ(manager_.Count(), 0u);

    // 2回目のDestroyは何もしない
    manager_.Destroy(e);
    EXPECT_EQ(manager_.Count(), 0u);
}

//============================================================================
// ComponentStorage テスト
//============================================================================
class ComponentStorageTest : public ::testing::Test
{
protected:
    ECS::ComponentStorage<PositionData> storage_;
    ECS::Actor entity1_{0, 0};
    ECS::Actor entity2_{1, 0};
    ECS::Actor entity3_{2, 0};
};

TEST_F(ComponentStorageTest, InitiallyEmpty)
{
    EXPECT_EQ(storage_.Size(), 0u);
}

TEST_F(ComponentStorageTest, AddReturnsPointer)
{
    auto* comp = storage_.Add(entity1_);
    EXPECT_NE(comp, nullptr);
}

TEST_F(ComponentStorageTest, AddIncrementsSize)
{
    storage_.Add(entity1_);
    EXPECT_EQ(storage_.Size(), 1u);

    storage_.Add(entity2_);
    EXPECT_EQ(storage_.Size(), 2u);
}

TEST_F(ComponentStorageTest, AddWithArgs)
{
    auto* comp = storage_.Add(entity1_, 10.0f, 20.0f, 30.0f);
    EXPECT_EQ(comp->x, 10.0f);
    EXPECT_EQ(comp->y, 20.0f);
    EXPECT_EQ(comp->z, 30.0f);
}

TEST_F(ComponentStorageTest, GetReturnsNullIfNotPresent)
{
    EXPECT_EQ(storage_.Get(entity1_), nullptr);
}

TEST_F(ComponentStorageTest, GetReturnsSameAsAdd)
{
    auto* added = storage_.Add(entity1_);
    auto* got = storage_.Get(entity1_);
    EXPECT_EQ(added, got);
}

TEST_F(ComponentStorageTest, HasReturnsFalseIfNotPresent)
{
    EXPECT_FALSE(storage_.Has(entity1_));
}

TEST_F(ComponentStorageTest, HasReturnsTrueIfPresent)
{
    storage_.Add(entity1_);
    EXPECT_TRUE(storage_.Has(entity1_));
}

TEST_F(ComponentStorageTest, RemoveDecrementsSize)
{
    storage_.Add(entity1_);
    storage_.Add(entity2_);
    EXPECT_EQ(storage_.Size(), 2u);

    storage_.Remove(entity1_);
    EXPECT_EQ(storage_.Size(), 1u);
}

TEST_F(ComponentStorageTest, RemoveMakesHasReturnFalse)
{
    storage_.Add(entity1_);
    storage_.Remove(entity1_);
    EXPECT_FALSE(storage_.Has(entity1_));
}

TEST_F(ComponentStorageTest, RemovePreservesOtherEntities)
{
    storage_.Add(entity1_, 1.0f, 0.0f, 0.0f);
    storage_.Add(entity2_, 2.0f, 0.0f, 0.0f);
    storage_.Add(entity3_, 3.0f, 0.0f, 0.0f);

    storage_.Remove(entity2_);

    EXPECT_TRUE(storage_.Has(entity1_));
    EXPECT_FALSE(storage_.Has(entity2_));
    EXPECT_TRUE(storage_.Has(entity3_));

    // 値も保持
    EXPECT_EQ(storage_.Get(entity1_)->x, 1.0f);
    EXPECT_EQ(storage_.Get(entity3_)->x, 3.0f);
}

TEST_F(ComponentStorageTest, OnEntityDestroyedRemovesComponent)
{
    storage_.Add(entity1_);
    storage_.OnEntityDestroyed(entity1_);
    EXPECT_FALSE(storage_.Has(entity1_));
}

TEST_F(ComponentStorageTest, ClearRemovesAll)
{
    storage_.Add(entity1_);
    storage_.Add(entity2_);

    storage_.Clear();
    EXPECT_EQ(storage_.Size(), 0u);
}

TEST_F(ComponentStorageTest, ForEachIteratesAllComponents)
{
    storage_.Add(entity1_, 1.0f, 0.0f, 0.0f);
    storage_.Add(entity2_, 2.0f, 0.0f, 0.0f);
    storage_.Add(entity3_, 3.0f, 0.0f, 0.0f);

    float sum = 0.0f;
    storage_.ForEach([&sum](PositionData& pos) {
        sum += pos.x;
    });

    EXPECT_EQ(sum, 6.0f);
}

TEST_F(ComponentStorageTest, ForEachWithEntityIterates)
{
    storage_.Add(entity1_, 1.0f, 0.0f, 0.0f);
    storage_.Add(entity2_, 2.0f, 0.0f, 0.0f);

    std::vector<ECS::Actor> entities;
    storage_.ForEachWithEntity([&entities](ECS::Actor e, PositionData&) {
        entities.push_back(e);
    });

    EXPECT_EQ(entities.size(), 2u);
}

TEST_F(ComponentStorageTest, GetRawDataReturnsVector)
{
    storage_.Add(entity1_, 1.0f, 0.0f, 0.0f);
    storage_.Add(entity2_, 2.0f, 0.0f, 0.0f);

    auto& raw = storage_.GetRawData();
    EXPECT_EQ(raw.size(), 2u);
}

//============================================================================
// World テスト
//============================================================================
class WorldTest : public ::testing::Test
{
protected:
    ECS::World world_;
};

TEST_F(WorldTest, InitiallyEmpty)
{
    EXPECT_EQ(world_.ActorCount(), 0u);
}

TEST_F(WorldTest, CreateActorIncrementsCount)
{
    world_.CreateActor();
    EXPECT_EQ(world_.ActorCount(), 1u);
}

TEST_F(WorldTest, CreateActorReturnsValid)
{
    ECS::Actor e = world_.CreateActor();
    EXPECT_TRUE(e.IsValid());
    EXPECT_TRUE(world_.IsAlive(e));
}

TEST_F(WorldTest, DestroyActorDecrementsCount)
{
    ECS::Actor e = world_.CreateActor();
    world_.DestroyActor(e);
    EXPECT_EQ(world_.ActorCount(), 0u);
}

TEST_F(WorldTest, DestroyActorMakesNotAlive)
{
    ECS::Actor e = world_.CreateActor();
    world_.DestroyActor(e);
    EXPECT_FALSE(world_.IsAlive(e));
}

TEST_F(WorldTest, AddComponentReturnsPointer)
{
    ECS::Actor e = world_.CreateActor();
    auto* pos = world_.AddComponent<PositionData>(e);
    EXPECT_NE(pos, nullptr);
}

TEST_F(WorldTest, AddComponentWithArgs)
{
    ECS::Actor e = world_.CreateActor();
    auto* pos = world_.AddComponent<PositionData>(e, 10.0f, 20.0f, 30.0f);
    EXPECT_EQ(pos->x, 10.0f);
    EXPECT_EQ(pos->y, 20.0f);
    EXPECT_EQ(pos->z, 30.0f);
}

TEST_F(WorldTest, AddComponentToDeadEntityReturnsNull)
{
    ECS::Actor e = world_.CreateActor();
    world_.DestroyActor(e);
    auto* pos = world_.AddComponent<PositionData>(e);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(WorldTest, GetComponentReturnsNullIfNotAdded)
{
    ECS::Actor e = world_.CreateActor();
    EXPECT_EQ(world_.GetComponent<PositionData>(e), nullptr);
}

TEST_F(WorldTest, GetComponentReturnsSameAsAdd)
{
    ECS::Actor e = world_.CreateActor();
    auto* added = world_.AddComponent<PositionData>(e);
    auto* got = world_.GetComponent<PositionData>(e);
    EXPECT_EQ(added, got);
}

TEST_F(WorldTest, HasComponentReturnsFalseIfNotAdded)
{
    ECS::Actor e = world_.CreateActor();
    EXPECT_FALSE(world_.HasComponent<PositionData>(e));
}

TEST_F(WorldTest, HasComponentReturnsTrueIfAdded)
{
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e);
    EXPECT_TRUE(world_.HasComponent<PositionData>(e));
}

TEST_F(WorldTest, RemoveComponentMakesHasReturnFalse)
{
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e);
    world_.RemoveComponent<PositionData>(e);
    EXPECT_FALSE(world_.HasComponent<PositionData>(e));
}

TEST_F(WorldTest, DestroyActorRemovesAllComponents)
{
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e);
    world_.AddComponent<VelocityData>(e);

    world_.DestroyActor(e);

    // エンティティは死んでいるのでコンポーネント取得不可
    EXPECT_FALSE(world_.IsAlive(e));
}

TEST_F(WorldTest, MultipleEntityComponents)
{
    ECS::Actor e1 = world_.CreateActor();
    ECS::Actor e2 = world_.CreateActor();

    world_.AddComponent<PositionData>(e1, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<PositionData>(e2, 2.0f, 0.0f, 0.0f);

    EXPECT_EQ(world_.GetComponent<PositionData>(e1)->x, 1.0f);
    EXPECT_EQ(world_.GetComponent<PositionData>(e2)->x, 2.0f);
}

TEST_F(WorldTest, ForEachSingleComponent)
{
    ECS::Actor e1 = world_.CreateActor();
    ECS::Actor e2 = world_.CreateActor();

    world_.AddComponent<PositionData>(e1, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<PositionData>(e2, 2.0f, 0.0f, 0.0f);

    float sum = 0.0f;
    world_.ForEach<PositionData>([&sum](ECS::Actor, PositionData& pos) {
        sum += pos.x;
    });

    EXPECT_EQ(sum, 3.0f);
}

TEST_F(WorldTest, ForEachTwoComponents)
{
    ECS::Actor e1 = world_.CreateActor();
    ECS::Actor e2 = world_.CreateActor();
    ECS::Actor e3 = world_.CreateActor();

    world_.AddComponent<PositionData>(e1, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e1, 10.0f, 0.0f, 0.0f);

    world_.AddComponent<PositionData>(e2, 2.0f, 0.0f, 0.0f);
    // e2にはVelocityなし

    world_.AddComponent<PositionData>(e3, 3.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e3, 30.0f, 0.0f, 0.0f);

    int count = 0;
    float posSum = 0.0f;
    float velSum = 0.0f;
    world_.ForEach<PositionData, VelocityData>(
        [&](ECS::Actor, PositionData& pos, VelocityData& vel) {
            ++count;
            posSum += pos.x;
            velSum += vel.vx;
        });

    // e1とe3のみ（両方持っているもの）
    EXPECT_EQ(count, 2);
    EXPECT_EQ(posSum, 4.0f);  // 1 + 3
    EXPECT_EQ(velSum, 40.0f); // 10 + 30
}

TEST_F(WorldTest, ForEachThreeComponents)
{
    ECS::Actor e1 = world_.CreateActor();
    ECS::Actor e2 = world_.CreateActor();

    world_.AddComponent<PositionData>(e1, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e1, 10.0f, 0.0f, 0.0f);
    world_.AddComponent<HealthData>(e1, 100, 100);

    world_.AddComponent<PositionData>(e2, 2.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e2, 20.0f, 0.0f, 0.0f);
    // e2にはHealthなし

    int count = 0;
    world_.ForEach<PositionData, VelocityData, HealthData>(
        [&](ECS::Actor, PositionData&, VelocityData&, HealthData&) {
            ++count;
        });

    // e1のみ（3つ全部持っている）
    EXPECT_EQ(count, 1);
}

TEST_F(WorldTest, RegisterSystemAndFixedUpdate)
{
    ResetTestState();

    world_.RegisterSystem<TestUpdateSystem>();
    world_.FixedUpdate(0.016f);

    EXPECT_TRUE(g_systemCalled);
    EXPECT_EQ(g_receivedDt, 0.016f);
}

TEST_F(WorldTest, SystemsExecuteInPriorityOrder)
{
    ResetTestState();

    world_.RegisterSystem<PrioritySystem2>();  // priority 100
    world_.RegisterSystem<PrioritySystem1>();  // priority 0
    world_.RegisterSystem<PrioritySystem3>();  // priority 200

    world_.FixedUpdate(0.016f);

    ASSERT_EQ(g_executionOrder.size(), 3u);
    EXPECT_EQ(g_executionOrder[0], 1);
    EXPECT_EQ(g_executionOrder[1], 2);
    EXPECT_EQ(g_executionOrder[2], 3);
}

TEST_F(WorldTest, RegisterRenderSystemAndRender)
{
    ResetTestState();

    world_.RegisterRenderSystem<TestRenderSystem>();
    world_.Render(0.5f);

    EXPECT_TRUE(g_systemCalled);
    EXPECT_EQ(g_receivedAlpha, 0.5f);
}

TEST_F(WorldTest, RenderSystemsExecuteInPriorityOrder)
{
    ResetTestState();

    world_.RegisterRenderSystem<RenderPrioritySystem2>();  // priority 100
    world_.RegisterRenderSystem<RenderPrioritySystem1>();  // priority 0
    world_.RegisterRenderSystem<RenderPrioritySystem3>();  // priority 200

    world_.Render(0.0f);

    ASSERT_EQ(g_executionOrder.size(), 3u);
    EXPECT_EQ(g_executionOrder[0], 1);
    EXPECT_EQ(g_executionOrder[1], 2);
    EXPECT_EQ(g_executionOrder[2], 3);
}

TEST_F(WorldTest, SystemCanModifyComponents)
{
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 10.0f, 0.0f, 0.0f);

    world_.RegisterSystem<MoveSystem>();
    world_.FixedUpdate(1.0f);

    EXPECT_EQ(world_.GetComponent<PositionData>(e)->x, 10.0f);
}

TEST_F(WorldTest, ClearRemovesEntitiesAndComponents)
{
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e);

    world_.Clear();

    EXPECT_EQ(world_.ActorCount(), 0u);
}

TEST_F(WorldTest, ClearPreservesSystems)
{
    ResetTestState();
    world_.RegisterSystem<TestUpdateSystem>();

    world_.Clear();
    world_.FixedUpdate(0.016f);

    EXPECT_TRUE(g_systemCalled);
}

TEST_F(WorldTest, ClearAllRemovesSystems)
{
    ResetTestState();
    world_.RegisterSystem<TestUpdateSystem>();

    world_.ClearAll();
    world_.FixedUpdate(0.016f);

    EXPECT_FALSE(g_systemCalled);
}

} // namespace
