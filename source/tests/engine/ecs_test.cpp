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
#include "engine/ecs/chunk.h"
#include "engine/ecs/detail/query_impl.h"
#include "engine/ecs/component_cache.h"
#include "engine/ecs/detail/component_cache_impl.h"
#include "engine/ecs/entity_command_buffer.h"
#include <vector>
#include <atomic>

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

// 可変長ForEachテスト用の追加コンポーネント
struct AccelerationData {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;

    AccelerationData() = default;
    AccelerationData(float x, float y, float z) : ax(x), ay(y), az(z) {}
};

struct RotationData {
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    RotationData() = default;
    RotationData(float p, float y, float r) : pitch(p), yaw(y), roll(r) {}
};

struct ScaleData {
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;

    ScaleData() = default;
    ScaleData(float x, float y, float z) : sx(x), sy(y), sz(z) {}
};

struct ColorData {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    ColorData() = default;
    ColorData(float pr, float pg, float pb, float pa) : r(pr), g(pg), b(pb), a(pa) {}
};

struct AlphaData {
    float alpha = 1.0f;

    AlphaData() = default;
    AlphaData(float a) : alpha(a) {}
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
    void OnUpdate(ECS::World&, float dt) override {
        g_systemCalled = true;
        g_receivedDt = dt;
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestUpdateSystem"; }
};

// 優先度付きシステム
class PrioritySystem1 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_executionOrder.push_back(1); }
    int Priority() const override { return 0; }
    const char* Name() const override { return "PrioritySystem1"; }
};

class PrioritySystem2 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_executionOrder.push_back(2); }
    int Priority() const override { return 100; }
    const char* Name() const override { return "PrioritySystem2"; }
};

class PrioritySystem3 final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World&, float) override { g_executionOrder.push_back(3); }
    int Priority() const override { return 200; }
    const char* Name() const override { return "PrioritySystem3"; }
};

// 基本的な描画システム
class TestRenderSystem final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float alpha) override {
        g_systemCalled = true;
        g_receivedAlpha = alpha;
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestRenderSystem"; }
};

// 優先度付き描画システム
class RenderPrioritySystem1 final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_executionOrder.push_back(1); }
    int Priority() const override { return 0; }
    const char* Name() const override { return "RenderPrioritySystem1"; }
};

class RenderPrioritySystem2 final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_executionOrder.push_back(2); }
    int Priority() const override { return 100; }
    const char* Name() const override { return "RenderPrioritySystem2"; }
};

class RenderPrioritySystem3 final : public ECS::IRenderSystem {
public:
    void OnRender(ECS::World&, float) override { g_executionOrder.push_back(3); }
    int Priority() const override { return 200; }
    const char* Name() const override { return "RenderPrioritySystem3"; }
};

// コンポーネント変更システム
class MoveSystem final : public ECS::ISystem {
public:
    void OnUpdate(ECS::World& w, float dt) override {
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

//============================================================================
// ArchetypeStorage テスト
//============================================================================
class ArchetypeStorageTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ArchetypeStorageTest, EmptyArchetypeIdIsUnique)
{
    // 空のArchetypeを作成
    ECS::Actor emptyActor = world_.CreateActor();
    EXPECT_TRUE(emptyActor.IsValid());

    // コンポーネントを持つActorを作成
    ECS::Actor actorWithComp = world_.CreateActor();
    world_.AddComponent<PositionData>(actorWithComp);

    // 両方のActorが有効で、異なるArchetypeに配置されていることを確認
    EXPECT_TRUE(world_.IsAlive(emptyActor));
    EXPECT_TRUE(world_.IsAlive(actorWithComp));

    // 空ActorにはPositionDataがない
    EXPECT_EQ(world_.GetComponent<PositionData>(emptyActor), nullptr);
    EXPECT_NE(world_.GetComponent<PositionData>(actorWithComp), nullptr);
}

TEST_F(ArchetypeStorageTest, EmptyArchetypeDoesNotCollideWithComponentArchetypes)
{
    // 多数のコンポーネント組み合わせを作成しても
    // 空Archetypeと衝突しないことを確認
    for (int i = 0; i < 100; ++i) {
        ECS::Actor a = world_.CreateActor();
        if (i % 3 == 0) world_.AddComponent<PositionData>(a);
        if (i % 3 == 1) world_.AddComponent<VelocityData>(a);
        if (i % 3 == 2) {
            world_.AddComponent<PositionData>(a);
            world_.AddComponent<VelocityData>(a);
        }
    }

    // 空のActorも追加
    ECS::Actor empty = world_.CreateActor();
    EXPECT_TRUE(world_.IsAlive(empty));
    EXPECT_EQ(world_.GetComponent<PositionData>(empty), nullptr);
    EXPECT_EQ(world_.GetComponent<VelocityData>(empty), nullptr);
}

//============================================================================
// Deferred操作 テスト
//============================================================================
class DeferredOperationsTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(DeferredOperationsTest, DestroyActorDeferredDelaysDestruction)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 遅延破棄をリクエスト
    world_.Deferred().DestroyActor(actor);

    // まだ生きている
    EXPECT_TRUE(world_.IsAlive(actor));
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);

    // BeginFrameで実際に破棄
    world_.BeginFrame();

    // 破棄されている
    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(DeferredOperationsTest, AddComponentDeferredDelaysAddition)
{
    ECS::Actor actor = world_.CreateActor();

    // 遅延追加をリクエスト
    world_.Deferred().AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);

    // まだ追加されていない
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);

    // BeginFrameで実際に追加
    world_.BeginFrame();

    // 追加されている
    auto* pos = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(pos, nullptr);
    EXPECT_NEAR(pos->x, 10.0f, 0.001f);
    EXPECT_NEAR(pos->y, 20.0f, 0.001f);
    EXPECT_NEAR(pos->z, 30.0f, 0.001f);
}

TEST_F(DeferredOperationsTest, RemoveComponentDeferredDelaysRemoval)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 遅延削除をリクエスト
    world_.Deferred().RemoveComponent<PositionData>(actor);

    // まだ存在する
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);

    // BeginFrameで実際に削除
    world_.BeginFrame();

    // 削除されている
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);
}

TEST_F(DeferredOperationsTest, MultipleDeferredOperationsInSameFrame)
{
    ECS::Actor a1 = world_.CreateActor();
    ECS::Actor a2 = world_.CreateActor();
    world_.AddComponent<PositionData>(a1);

    // 複数の遅延操作
    world_.Deferred().DestroyActor(a1);
    world_.Deferred().AddComponent<VelocityData>(a2, 1.0f, 2.0f, 3.0f);

    // まだ処理されていない
    EXPECT_TRUE(world_.IsAlive(a1));
    EXPECT_EQ(world_.GetComponent<VelocityData>(a2), nullptr);

    // 一括処理
    world_.BeginFrame();

    EXPECT_FALSE(world_.IsAlive(a1));
    EXPECT_NE(world_.GetComponent<VelocityData>(a2), nullptr);
}

TEST_F(DeferredOperationsTest, DeferredOnDeadActorIsIgnored)
{
    ECS::Actor actor = world_.CreateActor();
    world_.DestroyActor(actor);  // 即時破棄

    // 死んだActorへの遅延操作は無視される（クラッシュしない）
    world_.Deferred().AddComponent<PositionData>(actor);
    world_.Deferred().RemoveComponent<PositionData>(actor);
    world_.Deferred().DestroyActor(actor);

    // BeginFrameでクラッシュしない
    world_.BeginFrame();

    EXPECT_FALSE(world_.IsAlive(actor));
}

//============================================================================
// Chunk テスト
//============================================================================
TEST(ChunkTest, Size)
{
    // Chunkは正確に16KB
    EXPECT_EQ(sizeof(ECS::Chunk), ECS::Chunk::kSize);
    EXPECT_EQ(ECS::Chunk::kSize, 16u * 1024u);
}

TEST(ChunkTest, Alignment)
{
    // Chunkは64バイトアラインメント
    EXPECT_EQ(alignof(ECS::Chunk), 64u);
}

TEST(ChunkTest, DataAccess)
{
    auto chunk = std::make_unique<ECS::Chunk>();

    // データアクセス
    std::byte* data = chunk->Data();
    ASSERT_NE(data, nullptr);

    // 書き込みテスト（クラッシュしない）
    data[0] = std::byte{0x42};
    data[ECS::Chunk::kSize - 1] = std::byte{0x24};

    EXPECT_EQ(data[0], std::byte{0x42});
    EXPECT_EQ(data[ECS::Chunk::kSize - 1], std::byte{0x24});
}

TEST(ChunkTest, ConstDataAccess)
{
    auto chunk = std::make_unique<ECS::Chunk>();
    const ECS::Chunk* constChunk = chunk.get();

    const std::byte* data = constChunk->Data();
    ASSERT_NE(data, nullptr);
}

//============================================================================
// ParallelForEach テスト
//============================================================================
class ParallelForEachTest : public ::testing::Test {
protected:
    void SetUp() override {
        // JobSystemを作成（シングルトン）
        JobSystem::Create();
    }

    void TearDown() override {
        // JobSystemを破棄
        JobSystem::Destroy();
    }

    ECS::World world_;
};

TEST_F(ParallelForEachTest, SingleComponent)
{
    // 100個のActorを作成
    constexpr int kCount = 100;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
    }

    // シリアルForEachで確認（デバッグ用）
    int serialCount = 0;
    world_.ForEach<PositionData>([&serialCount](ECS::Actor, PositionData&) {
        ++serialCount;
    });
    ASSERT_EQ(serialCount, kCount) << "Serial ForEach should find all actors";

    // Archetype情報の確認（デバッグ用）
    size_t archetypeCount = world_.GetArchetypeStorage().GetArchetypeCount();
    ASSERT_GT(archetypeCount, 0u) << "Should have at least one archetype";

    // 並列でx座標を+10
    std::atomic<int> processedCount{0};
    JobHandle handle = world_.ParallelForEach<PositionData>(
        [&processedCount](ECS::Actor, PositionData& pos) {
            pos.x += 10.0f;
            processedCount.fetch_add(1, std::memory_order_relaxed);
        });

    // 完了待機
    handle.Wait();

    // 全て処理された
    EXPECT_EQ(processedCount.load(), kCount);

    // 値が更新されているか確認
    int idx = 0;
    world_.ForEach<PositionData>([&idx](ECS::Actor, PositionData& pos) {
        EXPECT_NEAR(pos.x, static_cast<float>(idx) + 10.0f, 0.001f);
        ++idx;
    });
}

TEST_F(ParallelForEachTest, TwoComponents)
{
    constexpr int kCount = 50;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, 0.0f, 0.0f, 0.0f);
        world_.AddComponent<VelocityData>(a, 1.0f, 2.0f, 3.0f);
    }

    // 並列で位置に速度を加算
    JobHandle handle = world_.ParallelForEach<PositionData, VelocityData>(
        [](ECS::Actor, PositionData& pos, VelocityData& vel) {
            pos.x += vel.vx;
            pos.y += vel.vy;
            pos.z += vel.vz;
        });

    handle.Wait();

    // 値が更新されているか確認
    world_.ForEach<PositionData>([](ECS::Actor, PositionData& pos) {
        EXPECT_NEAR(pos.x, 1.0f, 0.001f);
        EXPECT_NEAR(pos.y, 2.0f, 0.001f);
        EXPECT_NEAR(pos.z, 3.0f, 0.001f);
    });
}

TEST_F(ParallelForEachTest, EmptyWorldReturnsEmptyHandle)
{
    // Actorなし
    JobHandle handle = world_.ParallelForEach<PositionData>(
        [](ECS::Actor, PositionData&) {});

    // 空のハンドル（すぐにWaitしても問題なし）
    handle.Wait();
}

TEST_F(ParallelForEachTest, NoMatchingComponentsReturnsEmptyHandle)
{
    // PositionDataのみ追加
    for (int i = 0; i < 10; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a);
    }

    // VelocityDataを要求（マッチしない）
    JobHandle handle = world_.ParallelForEach<VelocityData>(
        [](ECS::Actor, VelocityData&) {});

    handle.Wait();
}

TEST_F(ParallelForEachTest, LargeDataSet)
{
    // 大量のActorでパフォーマンス確認
    constexpr int kCount = 10000;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, 0.0f, 0.0f, 0.0f);
    }

    std::atomic<int> count{0};
    JobHandle handle = world_.ParallelForEach<PositionData>(
        [&count](ECS::Actor, PositionData& pos) {
            pos.x = 1.0f;
            count.fetch_add(1, std::memory_order_relaxed);
        });

    handle.Wait();

    EXPECT_EQ(count.load(), kCount);
}

// アクセスモード付きParallelForEachテスト
TEST_F(ParallelForEachTest, TypedParallelForEach_InOut_In)
{
    constexpr int kCount = 100;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, 0.0f, 0.0f, 0.0f);
        world_.AddComponent<VelocityData>(a, 1.0f, 2.0f, 3.0f);
    }

    // InOut<Position>とIn<Velocity>で並列処理
    std::atomic<int> processedCount{0};
    JobHandle handle = world_.ParallelForEach<ECS::InOut<PositionData>, ECS::In<VelocityData>>(
        [&processedCount](ECS::Actor, PositionData& pos, const VelocityData& vel) {
            pos.x += vel.vx;
            pos.y += vel.vy;
            pos.z += vel.vz;
            processedCount.fetch_add(1, std::memory_order_relaxed);
        });

    handle.Wait();

    EXPECT_EQ(processedCount.load(), kCount);
    world_.ForEach<PositionData>([](ECS::Actor, PositionData& pos) {
        EXPECT_NEAR(pos.x, 1.0f, 0.001f);
        EXPECT_NEAR(pos.y, 2.0f, 0.001f);
        EXPECT_NEAR(pos.z, 3.0f, 0.001f);
    });
}

TEST_F(ParallelForEachTest, TypedParallelForEach_Out_WriteOnly)
{
    constexpr int kCount = 50;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
    }

    // InOut<Position>で書き込み
    JobHandle handle = world_.ParallelForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor, PositionData& pos) {
            pos.x = 99.0f;
            pos.y = 99.0f;
            pos.z = 99.0f;
        });

    handle.Wait();

    world_.ForEach<PositionData>([](ECS::Actor, PositionData& pos) {
        EXPECT_EQ(pos.x, 99.0f);
        EXPECT_EQ(pos.y, 99.0f);
        EXPECT_EQ(pos.z, 99.0f);
    });
}

TEST_F(ParallelForEachTest, TypedParallelForEach_ThreeComponents)
{
    constexpr int kCount = 30;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, 0.0f, 0.0f, 0.0f);
        world_.AddComponent<VelocityData>(a, 1.0f, 1.0f, 1.0f);
        world_.AddComponent<HealthData>(a, 100, 100);
    }

    // 3コンポーネント混合アクセス
    JobHandle handle = world_.ParallelForEach<
        ECS::InOut<PositionData>,
        ECS::In<VelocityData>,
        ECS::InOut<HealthData>
    >(
        [](ECS::Actor, PositionData& pos, const VelocityData& vel, HealthData& hp) {
            pos.x += vel.vx;
            hp.hp -= 10;
        });

    handle.Wait();

    world_.ForEach<PositionData, HealthData>(
        [](ECS::Actor, PositionData& pos, HealthData& hp) {
            EXPECT_NEAR(pos.x, 1.0f, 0.001f);
            EXPECT_EQ(hp.hp, 90);
        });
}

//============================================================================
// EntityCommandBuffer テスト
//============================================================================
class EntityCommandBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        JobSystem::Create();
    }

    void TearDown() override {
        JobSystem::Destroy();
    }

    ECS::World world_;
};

TEST_F(EntityCommandBufferTest, DestroyActor_SingleThread)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::EntityCommandBuffer ecb;
    ecb.DestroyActor(actor);

    // Playback前はまだ生きている
    EXPECT_TRUE(world_.IsAlive(actor));

    ecb.Playback(world_);

    // Playback後は破棄されている
    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(EntityCommandBufferTest, AddComponent_SingleThread)
{
    ECS::Actor actor = world_.CreateActor();

    ECS::EntityCommandBuffer ecb;
    ecb.AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);

    // Playback前はコンポーネントがない
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);

    ecb.Playback(world_);

    // Playback後はコンポーネントがある
    auto* pos = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 10.0f);
    EXPECT_EQ(pos->y, 20.0f);
    EXPECT_EQ(pos->z, 30.0f);
}

TEST_F(EntityCommandBufferTest, RemoveComponent_SingleThread)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::EntityCommandBuffer ecb;
    ecb.RemoveComponent<PositionData>(actor);

    // Playback前はまだある
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);

    ecb.Playback(world_);

    // Playback後は削除されている
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);
}

TEST_F(EntityCommandBufferTest, ParallelDestroy)
{
    // 並列処理中にEntityCommandBufferを使用
    constexpr int kCount = 100;
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<HealthData>(a, i, 100);  // HP = i
        actors.push_back(a);
    }

    ECS::EntityCommandBuffer ecb;

    // 並列でHP < 50のActorを破棄リストに追加
    JobHandle handle = world_.ParallelForEach<ECS::In<HealthData>>(
        [&ecb](ECS::Actor e, const HealthData& hp) {
            if (hp.hp < 50) {
                ecb.DestroyActor(e);
            }
        });

    handle.Wait();

    // Playbackで実際に破棄
    ecb.Playback(world_);

    // HP >= 50のActorだけ生きている
    int aliveCount = 0;
    for (const auto& a : actors) {
        if (world_.IsAlive(a)) {
            ++aliveCount;
        }
    }
    EXPECT_EQ(aliveCount, 50);  // HP 50-99の50体
}

TEST_F(EntityCommandBufferTest, MultipleOperations)
{
    ECS::Actor a1 = world_.CreateActor();
    ECS::Actor a2 = world_.CreateActor();
    world_.AddComponent<PositionData>(a1, 1.0f, 0.0f, 0.0f);

    ECS::EntityCommandBuffer ecb;
    ecb.DestroyActor(a1);
    ecb.AddComponent<PositionData>(a2, 2.0f, 0.0f, 0.0f);
    ecb.AddComponent<VelocityData>(a2, 10.0f, 0.0f, 0.0f);

    EXPECT_EQ(ecb.Size(), 3u);

    ecb.Playback(world_);

    // a1は破棄
    EXPECT_FALSE(world_.IsAlive(a1));

    // a2にはPositionとVelocityが追加
    EXPECT_TRUE(world_.IsAlive(a2));
    auto* pos = world_.GetComponent<PositionData>(a2);
    auto* vel = world_.GetComponent<VelocityData>(a2);
    ASSERT_NE(pos, nullptr);
    ASSERT_NE(vel, nullptr);
    EXPECT_EQ(pos->x, 2.0f);
    EXPECT_EQ(vel->vx, 10.0f);
}

TEST_F(EntityCommandBufferTest, Clear_DiscardsOperations)
{
    ECS::Actor actor = world_.CreateActor();

    ECS::EntityCommandBuffer ecb;
    ecb.DestroyActor(actor);
    EXPECT_EQ(ecb.Size(), 1u);

    ecb.Clear();
    EXPECT_TRUE(ecb.IsEmpty());

    ecb.Playback(world_);

    // Clearしたので破棄されていない
    EXPECT_TRUE(world_.IsAlive(actor));
}

//============================================================================
// AddComponent エッジケース テスト
//============================================================================
class AddComponentEdgeCaseTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(AddComponentEdgeCaseTest, AlreadyHasComponent_ReturnsExisting)
{
    ECS::Actor actor = world_.CreateActor();
    auto* first = world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    auto* second = world_.AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);

    // 同じポインタが返される（上書きされない）
    EXPECT_EQ(first, second);
    // 元の値が保持される
    EXPECT_EQ(first->x, 1.0f);
    EXPECT_EQ(first->y, 2.0f);
    EXPECT_EQ(first->z, 3.0f);
}

TEST_F(AddComponentEdgeCaseTest, ToInvalidActor_ReturnsNull)
{
    ECS::Actor invalid = ECS::Actor::Invalid();
    auto* pos = world_.AddComponent<PositionData>(invalid);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(AddComponentEdgeCaseTest, ToDeadActor_ReturnsNull)
{
    ECS::Actor actor = world_.CreateActor();
    world_.DestroyActor(actor);
    auto* pos = world_.AddComponent<PositionData>(actor);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(AddComponentEdgeCaseTest, MultipleComponentsPreserveData)
{
    ECS::Actor actor = world_.CreateActor();

    // 複数コンポーネントを追加
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);
    world_.AddComponent<HealthData>(actor, 100, 200);

    // すべてのデータが保持されている
    auto* pos = world_.GetComponent<PositionData>(actor);
    auto* vel = world_.GetComponent<VelocityData>(actor);
    auto* hp = world_.GetComponent<HealthData>(actor);

    ASSERT_NE(pos, nullptr);
    ASSERT_NE(vel, nullptr);
    ASSERT_NE(hp, nullptr);

    EXPECT_EQ(pos->x, 1.0f);
    EXPECT_EQ(vel->vx, 10.0f);
    EXPECT_EQ(hp->hp, 100);
}

TEST_F(AddComponentEdgeCaseTest, RemoveThenAddSameComponent)
{
    ECS::Actor actor = world_.CreateActor();

    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.RemoveComponent<PositionData>(actor);

    // 削除後に再追加
    auto* pos = world_.AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 10.0f);
    EXPECT_EQ(pos->y, 20.0f);
    EXPECT_EQ(pos->z, 30.0f);
}

//============================================================================
// Archetype Migration (Swap-and-Pop) テスト
//============================================================================
class ArchetypeMigrationTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ArchetypeMigrationTest, SingleActorInChunk_MigrationWorks)
{
    // Chunk内に1つのActorだけの場合
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // コンポーネント追加でArchetype移動
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // 両方のコンポーネントが正しく存在
    auto* pos = world_.GetComponent<PositionData>(actor);
    auto* vel = world_.GetComponent<VelocityData>(actor);
    ASSERT_NE(pos, nullptr);
    ASSERT_NE(vel, nullptr);
    EXPECT_EQ(pos->x, 1.0f);
    EXPECT_EQ(vel->vx, 10.0f);
}

TEST_F(ArchetypeMigrationTest, MiddleActorMigration_SwapAndPopCorrect)
{
    // 複数のActorを作成（同じArchetype）
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 5; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(a);
    }

    // 中間のActorにコンポーネントを追加（Archetype移動）
    ECS::Actor middleActor = actors[2];
    world_.AddComponent<VelocityData>(middleActor, 100.0f, 0.0f, 0.0f);

    // 中間Actorの元のコンポーネントが保持されている
    auto* pos = world_.GetComponent<PositionData>(middleActor);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 2.0f);

    // 他のActorも正常
    for (int i = 0; i < 5; ++i) {
        if (i == 2) continue;
        auto* p = world_.GetComponent<PositionData>(actors[i]);
        ASSERT_NE(p, nullptr) << "Actor " << i << " should have PositionData";
        EXPECT_EQ(p->x, static_cast<float>(i)) << "Actor " << i << " has wrong x value";
    }
}

TEST_F(ArchetypeMigrationTest, LastActorMigration_NoSwapNeeded)
{
    // 3つのActorを作成
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 3; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(a);
    }

    // 最後のActorにコンポーネント追加
    ECS::Actor lastActor = actors[2];
    world_.AddComponent<VelocityData>(lastActor, 100.0f, 0.0f, 0.0f);

    // すべてのActorが正常
    for (int i = 0; i < 3; ++i) {
        auto* p = world_.GetComponent<PositionData>(actors[i]);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(p->x, static_cast<float>(i));
    }
}

TEST_F(ArchetypeMigrationTest, ChainMigration_MultipleComponentChanges)
{
    // A → AB → ABC → AB → A の連続Archetype変更
    ECS::Actor actor = world_.CreateActor();

    // A (Position only)
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_FALSE(world_.HasComponent<VelocityData>(actor));

    // AB (Position + Velocity)
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));

    // ABC (Position + Velocity + Health)
    world_.AddComponent<HealthData>(actor, 100, 200);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));
    EXPECT_TRUE(world_.HasComponent<HealthData>(actor));

    // データ確認
    EXPECT_EQ(world_.GetComponent<PositionData>(actor)->x, 1.0f);
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor)->vx, 10.0f);
    EXPECT_EQ(world_.GetComponent<HealthData>(actor)->hp, 100);

    // AB (Healthを削除)
    world_.RemoveComponent<HealthData>(actor);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));
    EXPECT_FALSE(world_.HasComponent<HealthData>(actor));

    // A (Velocityを削除)
    world_.RemoveComponent<VelocityData>(actor);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_FALSE(world_.HasComponent<VelocityData>(actor));
    EXPECT_FALSE(world_.HasComponent<HealthData>(actor));

    // 最初のデータが保持されている
    EXPECT_EQ(world_.GetComponent<PositionData>(actor)->x, 1.0f);
}

TEST_F(ArchetypeMigrationTest, DestroyMiddleActor_SwapAndPopCorrect)
{
    // 5つのActorを作成（同じArchetype）
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 5; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i * 10), 0.0f, 0.0f);
        actors.push_back(a);
    }

    // 中間のActorを破棄
    world_.DestroyActor(actors[2]);

    // 残りのActorが正常
    EXPECT_FALSE(world_.IsAlive(actors[2]));

    for (int i = 0; i < 5; ++i) {
        if (i == 2) continue;
        EXPECT_TRUE(world_.IsAlive(actors[i])) << "Actor " << i << " should be alive";
        auto* p = world_.GetComponent<PositionData>(actors[i]);
        ASSERT_NE(p, nullptr) << "Actor " << i << " should have PositionData";
        EXPECT_EQ(p->x, static_cast<float>(i * 10)) << "Actor " << i << " has wrong x value";
    }
}

TEST_F(ArchetypeMigrationTest, RemoveMiddleActor_SwapAndPopCorrect)
{
    // 5つのActorを作成（同じArchetype: Position + Velocity）
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 5; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
        world_.AddComponent<VelocityData>(a, static_cast<float>(i * 10), 0.0f, 0.0f);
        actors.push_back(a);
    }

    // 中間のActorからVelocityを削除（Archetype移動）
    world_.RemoveComponent<VelocityData>(actors[2]);

    // 中間ActorはPositionのみ
    EXPECT_TRUE(world_.HasComponent<PositionData>(actors[2]));
    EXPECT_FALSE(world_.HasComponent<VelocityData>(actors[2]));
    EXPECT_EQ(world_.GetComponent<PositionData>(actors[2])->x, 2.0f);

    // 他のActorは両方持つ
    for (int i = 0; i < 5; ++i) {
        if (i == 2) continue;
        EXPECT_TRUE(world_.HasComponent<PositionData>(actors[i]));
        EXPECT_TRUE(world_.HasComponent<VelocityData>(actors[i]));
        EXPECT_EQ(world_.GetComponent<PositionData>(actors[i])->x, static_cast<float>(i));
        EXPECT_EQ(world_.GetComponent<VelocityData>(actors[i])->vx, static_cast<float>(i * 10));
    }
}

TEST_F(ArchetypeMigrationTest, EmptyToNonEmpty_Works)
{
    // 空Actorからコンポーネント追加
    ECS::Actor actor = world_.CreateActor();

    // 空の状態でArchetype確認
    EXPECT_FALSE(world_.HasComponent<PositionData>(actor));

    // コンポーネント追加
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_EQ(world_.GetComponent<PositionData>(actor)->x, 1.0f);
}

TEST_F(ArchetypeMigrationTest, NonEmptyToEmpty_Works)
{
    // コンポーネントを持つActorから全て削除
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // 全削除
    world_.RemoveComponent<PositionData>(actor);
    world_.RemoveComponent<VelocityData>(actor);

    // Actorは生存、コンポーネントなし
    EXPECT_TRUE(world_.IsAlive(actor));
    EXPECT_FALSE(world_.HasComponent<PositionData>(actor));
    EXPECT_FALSE(world_.HasComponent<VelocityData>(actor));
}

TEST_F(ArchetypeMigrationTest, ManyActors_AllDataPreserved)
{
    // 100個のActorを作成し、様々なArchetypeに分散
    constexpr int kCount = 100;
    std::vector<ECS::Actor> actors;

    for (int i = 0; i < kCount; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);

        if (i % 2 == 0) {
            world_.AddComponent<VelocityData>(a, static_cast<float>(i * 10), 0.0f, 0.0f);
        }
        if (i % 3 == 0) {
            world_.AddComponent<HealthData>(a, i, i * 2);
        }
        actors.push_back(a);
    }

    // 全てのデータが正しいか確認
    for (int i = 0; i < kCount; ++i) {
        auto* pos = world_.GetComponent<PositionData>(actors[i]);
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(pos->x, static_cast<float>(i));

        if (i % 2 == 0) {
            auto* vel = world_.GetComponent<VelocityData>(actors[i]);
            ASSERT_NE(vel, nullptr);
            EXPECT_EQ(vel->vx, static_cast<float>(i * 10));
        } else {
            EXPECT_EQ(world_.GetComponent<VelocityData>(actors[i]), nullptr);
        }

        if (i % 3 == 0) {
            auto* hp = world_.GetComponent<HealthData>(actors[i]);
            ASSERT_NE(hp, nullptr);
            EXPECT_EQ(hp->hp, i);
        } else {
            EXPECT_EQ(world_.GetComponent<HealthData>(actors[i]), nullptr);
        }
    }
}

//============================================================================
// ポインタ安定性 テスト
//============================================================================
class PointerStabilityTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(PointerStabilityTest, DeferredAdd_PointersStableDuringFrame)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // ポインタ取得
    auto* posPtr = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(posPtr, nullptr);
    float originalX = posPtr->x;

    // 遅延追加（まだ実行されない）
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // ポインタはまだ有効
    EXPECT_EQ(posPtr->x, originalX);

    // VelocityDataはまだ追加されていない
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor), nullptr);

    // BeginFrame後
    world_.BeginFrame();

    // VelocityDataが追加された
    EXPECT_NE(world_.GetComponent<VelocityData>(actor), nullptr);
}

TEST_F(PointerStabilityTest, DeferredRemove_PointersStableDuringFrame)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // ポインタ取得
    auto* velPtr = world_.GetComponent<VelocityData>(actor);
    ASSERT_NE(velPtr, nullptr);
    float originalVx = velPtr->vx;

    // 遅延削除（まだ実行されない）
    world_.Deferred().RemoveComponent<VelocityData>(actor);

    // ポインタはまだ有効
    EXPECT_EQ(velPtr->vx, originalVx);
    EXPECT_NE(world_.GetComponent<VelocityData>(actor), nullptr);

    // BeginFrame後
    world_.BeginFrame();

    // VelocityDataが削除された
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor), nullptr);
}

TEST_F(PointerStabilityTest, DeferredDestroy_PointersStableDuringFrame)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // ポインタ取得
    auto* posPtr = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(posPtr, nullptr);

    // 遅延破棄（まだ実行されない）
    world_.Deferred().DestroyActor(actor);

    // Actorはまだ生存
    EXPECT_TRUE(world_.IsAlive(actor));
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);

    // BeginFrame後
    world_.BeginFrame();

    // Actorが破棄された
    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(PointerStabilityTest, MultipleDeferred_OrderPreserved)
{
    ECS::Actor actor = world_.CreateActor();

    // 遅延操作をキューに追加（順序が重要）
    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // まだ追加されていない
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor), nullptr);

    // BeginFrame後
    world_.BeginFrame();

    // 両方追加された
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);
    EXPECT_NE(world_.GetComponent<VelocityData>(actor), nullptr);
}

TEST_F(PointerStabilityTest, BeginFrame_ExecutesDeferredOperations)
{
    ECS::Actor actor = world_.CreateActor();

    // 複数の遅延操作
    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.BeginFrame();

    // 1回目のBeginFrameで実行
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);

    // 追加の遅延操作
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // まだ実行されていない
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor), nullptr);

    // 2回目のBeginFrame
    world_.BeginFrame();

    // 実行された
    EXPECT_NE(world_.GetComponent<VelocityData>(actor), nullptr);
}

TEST_F(PointerStabilityTest, FrameCounter_IncrementsOnBeginFrame)
{
    uint32_t initialFrame = world_.GetFrameCounter();

    world_.BeginFrame();
    EXPECT_EQ(world_.GetFrameCounter(), initialFrame + 1);

    world_.BeginFrame();
    EXPECT_EQ(world_.GetFrameCounter(), initialFrame + 2);
}

//============================================================================
// Query API テスト
//============================================================================

// Query用マーカーコンポーネント
struct DeadTag {
    bool dummy = false;
};

struct DisabledTag {
    bool dummy = false;
};

class QueryApiTest : public ::testing::Test {
protected:
    ECS::World world_;

    void SetUp() override {
        // テスト用Actorをセットアップ
        // Actor 0: Position only
        // Actor 1: Position + Velocity
        // Actor 2: Position + Velocity + Health
        // Actor 3: Position + Dead
        // Actor 4: Position + Disabled

        for (int i = 0; i < 5; ++i) {
            ECS::Actor a = world_.CreateActor();
            world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
            actors_.push_back(a);
        }

        // Actor 1, 2: Velocity
        world_.AddComponent<VelocityData>(actors_[1], 10.0f, 0.0f, 0.0f);
        world_.AddComponent<VelocityData>(actors_[2], 20.0f, 0.0f, 0.0f);

        // Actor 2: Health
        world_.AddComponent<HealthData>(actors_[2], 100, 200);

        // Actor 3: Dead
        world_.AddComponent<DeadTag>(actors_[3]);

        // Actor 4: Disabled
        world_.AddComponent<DisabledTag>(actors_[4]);
    }

    std::vector<ECS::Actor> actors_;
};

TEST_F(QueryApiTest, ForEach_SingleComponent)
{
    int count = 0;
    world_.Query<PositionData>().ForEach([&count](ECS::Actor, PositionData&) {
        ++count;
    });

    // 全5つのActorがPositionを持つ
    EXPECT_EQ(count, 5);
}

TEST_F(QueryApiTest, ForEach_TwoComponents)
{
    int count = 0;
    world_.Query<PositionData, VelocityData>().ForEach(
        [&count](ECS::Actor, PositionData&, VelocityData&) {
            ++count;
        });

    // Actor 1, 2 のみ
    EXPECT_EQ(count, 2);
}

TEST_F(QueryApiTest, ForEach_ThreeComponents)
{
    int count = 0;
    world_.Query<PositionData, VelocityData, HealthData>().ForEach(
        [&count](ECS::Actor, PositionData&, VelocityData&, HealthData&) {
            ++count;
        });

    // Actor 2 のみ
    EXPECT_EQ(count, 1);
}

TEST_F(QueryApiTest, Exclude_FiltersOut)
{
    int count = 0;
    std::vector<float> xValues;

    // DeadTagを持たないActorをクエリ
    world_.Query<PositionData, ECS::Exclude<DeadTag>>().ForEach(
        [&count, &xValues](ECS::Actor, PositionData& pos) {
            ++count;
            xValues.push_back(pos.x);
        });

    // Actor 0, 1, 2, 4 (Actor 3はDeadTagを持つ)
    EXPECT_EQ(count, 4);

    // Actor 3 (x=3.0f) が除外されている
    EXPECT_TRUE(std::find(xValues.begin(), xValues.end(), 0.0f) != xValues.end());
    EXPECT_TRUE(std::find(xValues.begin(), xValues.end(), 1.0f) != xValues.end());
    EXPECT_TRUE(std::find(xValues.begin(), xValues.end(), 2.0f) != xValues.end());
    EXPECT_TRUE(std::find(xValues.begin(), xValues.end(), 4.0f) != xValues.end());
    EXPECT_FALSE(std::find(xValues.begin(), xValues.end(), 3.0f) != xValues.end());
}

TEST_F(QueryApiTest, Exclude_Multiple)
{
    int count = 0;

    // DeadTagとDisabledTagを両方持たないActorをクエリ
    world_.Query<PositionData, ECS::Exclude<DeadTag>, ECS::Exclude<DisabledTag>>().ForEach(
        [&count](ECS::Actor, PositionData&) {
            ++count;
        });

    // Actor 0, 1, 2 (Actor 3はDead, Actor 4はDisabled)
    EXPECT_EQ(count, 3);
}

TEST_F(QueryApiTest, Exclude_ChainedCall)
{
    int count = 0;

    // .Exclude<T>()メソッドチェーン
    world_.Query<PositionData>().Exclude<DeadTag>().Exclude<DisabledTag>().ForEach(
        [&count](ECS::Actor, PositionData&) {
            ++count;
        });

    // Actor 0, 1, 2
    EXPECT_EQ(count, 3);
}

TEST_F(QueryApiTest, WithPredicate_Filters)
{
    int count = 0;

    // x > 1.5 のActorのみ
    world_.Query<PositionData>()
        .With([this](ECS::Actor a) {
            auto* pos = world_.GetComponent<PositionData>(a);
            return pos && pos->x > 1.5f;
        })
        .ForEach([&count](ECS::Actor, PositionData&) {
            ++count;
        });

    // Actor 2, 3, 4 (x = 2, 3, 4)
    EXPECT_EQ(count, 3);
}

TEST_F(QueryApiTest, WithPredicate_Multiple_ANDed)
{
    int count = 0;

    // x > 0.5 AND x < 3.5
    world_.Query<PositionData>()
        .With([this](ECS::Actor a) {
            auto* pos = world_.GetComponent<PositionData>(a);
            return pos && pos->x > 0.5f;
        })
        .With([this](ECS::Actor a) {
            auto* pos = world_.GetComponent<PositionData>(a);
            return pos && pos->x < 3.5f;
        })
        .ForEach([&count](ECS::Actor, PositionData&) {
            ++count;
        });

    // Actor 1, 2, 3 (x = 1, 2, 3)
    EXPECT_EQ(count, 3);
}

TEST_F(QueryApiTest, Count_ReturnsCorrect)
{
    size_t count = world_.Query<PositionData>().Count();
    EXPECT_EQ(count, 5u);

    size_t countWithVel = world_.Query<PositionData, VelocityData>().Count();
    EXPECT_EQ(countWithVel, 2u);
}

TEST_F(QueryApiTest, Count_WithExclude)
{
    size_t count = world_.Query<PositionData, ECS::Exclude<DeadTag>>().Count();
    EXPECT_EQ(count, 4u);
}

TEST_F(QueryApiTest, First_ReturnsFirst)
{
    auto [pos] = world_.Query<PositionData>().First();
    EXPECT_NE(pos, nullptr);
}

TEST_F(QueryApiTest, First_NoMatch_ReturnsNullptr)
{
    // HealthとDeadTagを両方持つActorはいない
    auto [pos, hp] = world_.Query<PositionData, HealthData, ECS::Exclude<VelocityData>>().First();
    // Actor 2 は Health を持つが Velocity も持つので除外される
    EXPECT_EQ(pos, nullptr);
    EXPECT_EQ(hp, nullptr);
}

TEST_F(QueryApiTest, Any_ReturnsTrue)
{
    EXPECT_TRUE(world_.Query<PositionData>().Any());
    EXPECT_TRUE(world_.Query<VelocityData>().Any());
    EXPECT_TRUE(world_.Query<HealthData>().Any());
}

TEST_F(QueryApiTest, Any_ReturnsFalse_WhenNoMatch)
{
    // DisabledとVelocityを両方持つActorはいない
    bool hasAny = world_.Query<DisabledTag, VelocityData>().Any();
    EXPECT_FALSE(hasAny);
}

TEST_F(QueryApiTest, Empty_ReturnsTrue_WhenNoMatch)
{
    bool isEmpty = world_.Query<DisabledTag, VelocityData>().Empty();
    EXPECT_TRUE(isEmpty);
}

TEST_F(QueryApiTest, Empty_ReturnsFalse_WhenMatch)
{
    EXPECT_FALSE(world_.Query<PositionData>().Empty());
}

TEST_F(QueryApiTest, Exclude_And_Required_Combined)
{
    int count = 0;

    // VelocityDataを持ち、DeadTagを持たない
    world_.Query<PositionData, VelocityData, ECS::Exclude<DeadTag>>().ForEach(
        [&count](ECS::Actor, PositionData&, VelocityData&) {
            ++count;
        });

    // Actor 1, 2 (両方Velocityを持ち、Deadではない)
    EXPECT_EQ(count, 2);
}

TEST_F(QueryApiTest, DataModification_Works)
{
    // Queryでデータを変更
    world_.Query<PositionData, ECS::Exclude<DeadTag>>().ForEach(
        [](ECS::Actor, PositionData& pos) {
            pos.x += 100.0f;
        });

    // 確認
    EXPECT_EQ(world_.GetComponent<PositionData>(actors_[0])->x, 100.0f);
    EXPECT_EQ(world_.GetComponent<PositionData>(actors_[1])->x, 101.0f);
    EXPECT_EQ(world_.GetComponent<PositionData>(actors_[2])->x, 102.0f);
    // Actor 3 は Dead なので変更されない
    EXPECT_EQ(world_.GetComponent<PositionData>(actors_[3])->x, 3.0f);
    EXPECT_EQ(world_.GetComponent<PositionData>(actors_[4])->x, 104.0f);
}

//============================================================================
// Deferred操作 高度なケース テスト
//============================================================================
class DeferredAdvancedTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(DeferredAdvancedTest, AddThenRemoveSameComponent)
{
    ECS::Actor actor = world_.CreateActor();

    // 同フレームで追加→削除
    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.Deferred().RemoveComponent<PositionData>(actor);

    world_.BeginFrame();

    // 追加→削除の順序で実行されるので、最終的には削除される
    EXPECT_FALSE(world_.HasComponent<PositionData>(actor));
}

TEST_F(DeferredAdvancedTest, RemoveThenAddSameComponent)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 同フレームで削除→追加
    world_.Deferred().RemoveComponent<PositionData>(actor);
    world_.Deferred().AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);

    world_.BeginFrame();

    // 削除→追加の順序で実行されるので、最終的には追加される
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_EQ(world_.GetComponent<PositionData>(actor)->x, 10.0f);
}

TEST_F(DeferredAdvancedTest, MultipleAddsDifferentTypes)
{
    ECS::Actor actor = world_.CreateActor();

    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);
    world_.Deferred().AddComponent<HealthData>(actor, 100, 200);

    world_.BeginFrame();

    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));
    EXPECT_TRUE(world_.HasComponent<HealthData>(actor));
}

TEST_F(DeferredAdvancedTest, DestroyThenAdd_Ignored)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 破棄後の追加は無視される
    world_.Deferred().DestroyActor(actor);
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    world_.BeginFrame();

    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(DeferredAdvancedTest, AddToMultipleActors)
{
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 10; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.Deferred().AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(a);
    }

    world_.BeginFrame();

    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(world_.HasComponent<PositionData>(actors[i]));
        EXPECT_EQ(world_.GetComponent<PositionData>(actors[i])->x, static_cast<float>(i));
    }
}

TEST_F(DeferredAdvancedTest, CascadingArchetypeChanges)
{
    ECS::Actor actor = world_.CreateActor();

    // A → AB → ABC の連続変更
    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.Deferred().AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);
    world_.Deferred().AddComponent<HealthData>(actor, 100, 200);

    world_.BeginFrame();

    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));
    EXPECT_TRUE(world_.HasComponent<HealthData>(actor));

    // データも正しい
    EXPECT_EQ(world_.GetComponent<PositionData>(actor)->x, 1.0f);
    EXPECT_EQ(world_.GetComponent<VelocityData>(actor)->vx, 10.0f);
    EXPECT_EQ(world_.GetComponent<HealthData>(actor)->hp, 100);
}

TEST_F(DeferredAdvancedTest, QueueClearedAfterBeginFrame)
{
    ECS::Actor actor = world_.CreateActor();
    world_.Deferred().AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    world_.BeginFrame();
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));

    // 2回目のBeginFrameでは何も起きない
    world_.BeginFrame();
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
}

TEST_F(DeferredAdvancedTest, DoubleDestroy_NoOp)
{
    ECS::Actor actor = world_.CreateActor();

    world_.Deferred().DestroyActor(actor);
    world_.Deferred().DestroyActor(actor);

    // クラッシュしない
    world_.BeginFrame();

    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(DeferredAdvancedTest, ComponentDataCopiedCorrectly)
{
    ECS::Actor actor = world_.CreateActor();

    // 複雑なデータ
    PositionData posData{123.456f, 789.012f, 345.678f};
    world_.Deferred().AddComponent<PositionData>(actor, posData.x, posData.y, posData.z);

    world_.BeginFrame();

    auto* pos = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(pos, nullptr);
    EXPECT_NEAR(pos->x, 123.456f, 0.001f);
    EXPECT_NEAR(pos->y, 789.012f, 0.001f);
    EXPECT_NEAR(pos->z, 345.678f, 0.001f);
}

//============================================================================
// Chunk / メモリ管理 テスト
//============================================================================
class ChunkMemoryTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ChunkMemoryTest, Chunk_Size_Exactly16KB)
{
    EXPECT_EQ(sizeof(ECS::Chunk), 16u * 1024u);
    EXPECT_EQ(ECS::Chunk::kSize, 16u * 1024u);
}

TEST_F(ChunkMemoryTest, Chunk_Alignment_64Byte)
{
    EXPECT_EQ(alignof(ECS::Chunk), 64u);
}

TEST_F(ChunkMemoryTest, Archetype_ChunkCreatedOnDemand)
{
    // 最初はチャンクなし
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // Actorが有効
    EXPECT_TRUE(world_.IsAlive(actor));
    EXPECT_NE(world_.GetComponent<PositionData>(actor), nullptr);
}

TEST_F(ChunkMemoryTest, Archetype_MultipleActors_SameArchetype)
{
    // 同じArchetypeに複数のActorを作成
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 100; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(a);
    }

    // すべてのActorが有効
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(world_.IsAlive(actors[i]));
        auto* pos = world_.GetComponent<PositionData>(actors[i]);
        ASSERT_NE(pos, nullptr);
        EXPECT_EQ(pos->x, static_cast<float>(i));
    }
}

TEST_F(ChunkMemoryTest, Archetype_LargeComponent_FitsInChunk)
{
    // 大きなコンポーネント（1KB）
    struct LargeComponent {
        std::array<float, 256> data;  // 1KB
    };
    static_assert(std::is_trivially_copyable_v<LargeComponent>);

    ECS::Actor actor = world_.CreateActor();
    auto* comp = world_.AddComponent<LargeComponent>(actor);
    ASSERT_NE(comp, nullptr);

    // データが正しくアクセスできる
    comp->data[0] = 1.0f;
    comp->data[255] = 255.0f;

    auto* got = world_.GetComponent<LargeComponent>(actor);
    EXPECT_EQ(got->data[0], 1.0f);
    EXPECT_EQ(got->data[255], 255.0f);
}

TEST_F(ChunkMemoryTest, Archetype_EmptyArchetype_Works)
{
    // 空のArchetype（コンポーネントなし）
    ECS::Actor actor = world_.CreateActor();

    EXPECT_TRUE(world_.IsAlive(actor));
    EXPECT_EQ(world_.GetComponent<PositionData>(actor), nullptr);
}

TEST_F(ChunkMemoryTest, Archetype_ComponentAlignment)
{
    // 異なるアラインメントのコンポーネント
    struct alignas(16) AlignedComponent {
        float x, y, z, w;
    };
    static_assert(std::is_trivially_copyable_v<AlignedComponent>);

    ECS::Actor actor = world_.CreateActor();
    auto* comp = world_.AddComponent<AlignedComponent>(actor);
    ASSERT_NE(comp, nullptr);

    // ポインタがアラインされている
    EXPECT_EQ(reinterpret_cast<uintptr_t>(comp) % 16, 0u);
}

TEST_F(ChunkMemoryTest, ArchetypeStorage_GetArchetypeCount)
{
    // 異なるArchetypeを作成
    ECS::Actor a1 = world_.CreateActor();
    world_.AddComponent<PositionData>(a1);

    ECS::Actor a2 = world_.CreateActor();
    world_.AddComponent<VelocityData>(a2);

    ECS::Actor a3 = world_.CreateActor();
    world_.AddComponent<PositionData>(a3);
    world_.AddComponent<VelocityData>(a3);

    // 少なくとも3つ（空のArchetype + Position + Velocity + Position+Velocity）
    EXPECT_GE(world_.GetArchetypeStorage().GetArchetypeCount(), 3u);
}

//============================================================================
// ComponentCache テスト
//============================================================================

class ComponentCacheTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ComponentCacheTest, GetOrFetch_FirstCall_FetchesFromWorld)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::ComponentCache cache;
    auto* pos = cache.GetOrFetch<PositionData>(&world_, actor);

    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 1.0f);
}

TEST_F(ComponentCacheTest, GetOrFetch_SecondCall_ReturnsCached)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::ComponentCache cache;
    auto* first = cache.GetOrFetch<PositionData>(&world_, actor);
    auto* second = cache.GetOrFetch<PositionData>(&world_, actor);

    // 同じポインタが返される
    EXPECT_EQ(first, second);
}

TEST_F(ComponentCacheTest, GetOrFetch_NewFrame_Refetches)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::ComponentCache cache;
    auto* first = cache.GetOrFetch<PositionData>(&world_, actor);
    ASSERT_NE(first, nullptr);

    // フレームカウンタを進める
    world_.BeginFrame();

    // 再取得される（同じ値だが再取得が行われる）
    auto* second = cache.GetOrFetch<PositionData>(&world_, actor);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->x, 1.0f);
}

TEST_F(ComponentCacheTest, Clear_ClearsAll)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    ECS::ComponentCache cache;
    cache.GetOrFetch<PositionData>(&world_, actor);
    cache.GetOrFetch<VelocityData>(&world_, actor);

    cache.Clear();

    // クリア後も再取得可能
    auto* pos = cache.GetOrFetch<PositionData>(&world_, actor);
    EXPECT_NE(pos, nullptr);
}

TEST_F(ComponentCacheTest, Invalidate_SpecificType)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::ComponentCache cache;
    cache.GetOrFetch<PositionData>(&world_, actor);

    // 特定タイプを無効化
    cache.Invalidate<PositionData>();

    // 再取得可能
    auto* pos = cache.GetOrFetch<PositionData>(&world_, actor);
    EXPECT_NE(pos, nullptr);
}

TEST_F(ComponentCacheTest, GetOrFetch_InvalidActor_ReturnsNull)
{
    ECS::ComponentCache cache;
    auto* pos = cache.GetOrFetch<PositionData>(&world_, ECS::Actor::Invalid());
    EXPECT_EQ(pos, nullptr);
}

TEST_F(ComponentCacheTest, GetOrFetch_DeadActor_ReturnsNull)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    ECS::ComponentCache cache;
    auto* first = cache.GetOrFetch<PositionData>(&world_, actor);
    ASSERT_NE(first, nullptr);

    world_.DestroyActor(actor);
    world_.BeginFrame();  // フレームを進めてキャッシュを無効化

    auto* second = cache.GetOrFetch<PositionData>(&world_, actor);
    EXPECT_EQ(second, nullptr);
}

TEST_F(ComponentCacheTest, GetOrFetch_NoComponent_ReturnsNull)
{
    ECS::Actor actor = world_.CreateActor();
    // PositionDataを追加しない

    ECS::ComponentCache cache;
    auto* pos = cache.GetOrFetch<PositionData>(&world_, actor);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(ComponentCacheTest, FastPath_First8Types)
{
    // 8タイプまで高速パス
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);
    world_.AddComponent<HealthData>(actor, 100, 200);

    ECS::ComponentCache cache;
    EXPECT_NE(cache.GetOrFetch<PositionData>(&world_, actor), nullptr);
    EXPECT_NE(cache.GetOrFetch<VelocityData>(&world_, actor), nullptr);
    EXPECT_NE(cache.GetOrFetch<HealthData>(&world_, actor), nullptr);
}

//============================================================================
// TypedForEach (In/Out/InOut) テスト
//============================================================================
class TypedForEachTest : public ::testing::Test
{
protected:
    ECS::World world_;
};

TEST_F(TypedForEachTest, InOut_SingleComponent)
{
    // InOut<T>: 読み書き可能
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 2.0f, 3.0f);

    int count = 0;
    world_.ForEach<ECS::InOut<PositionData>>(
        [&count](ECS::Actor, PositionData& pos) {
            pos.x += 10.0f;
            ++count;
        });

    EXPECT_EQ(count, 1);
    EXPECT_EQ(world_.GetComponent<PositionData>(e)->x, 11.0f);
}

TEST_F(TypedForEachTest, In_SingleComponent_ReadOnly)
{
    // In<T>: 読み取り専用
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 5.0f, 10.0f, 15.0f);

    float sum = 0.0f;
    world_.ForEach<ECS::In<PositionData>>(
        [&sum](ECS::Actor, const PositionData& pos) {
            sum = pos.x + pos.y + pos.z;
        });

    EXPECT_EQ(sum, 30.0f);
}

TEST_F(TypedForEachTest, InOut_SingleComponent_Write)
{
    // InOut<T>: 読み書き（初期化用途にも使える）
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 2.0f, 3.0f);

    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor, PositionData& pos) {
            pos.x = 0.0f;
            pos.y = 0.0f;
            pos.z = 0.0f;
        });

    auto* p = world_.GetComponent<PositionData>(e);
    EXPECT_EQ(p->x, 0.0f);
    EXPECT_EQ(p->y, 0.0f);
    EXPECT_EQ(p->z, 0.0f);
}

TEST_F(TypedForEachTest, InOut_In_TwoComponents)
{
    // InOut<T1>, In<T2>: T1を変更、T2は読み取りのみ
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 10.0f, 20.0f, 30.0f);

    float dt = 0.5f;
    world_.ForEach<ECS::InOut<PositionData>, ECS::In<VelocityData>>(
        [dt](ECS::Actor, PositionData& pos, const VelocityData& vel) {
            pos.x += vel.vx * dt;
            pos.y += vel.vy * dt;
            pos.z += vel.vz * dt;
        });

    auto* p = world_.GetComponent<PositionData>(e);
    EXPECT_EQ(p->x, 5.0f);   // 10 * 0.5
    EXPECT_EQ(p->y, 10.0f);  // 20 * 0.5
    EXPECT_EQ(p->z, 15.0f);  // 30 * 0.5
}

TEST_F(TypedForEachTest, ThreeComponents_MixedAccess)
{
    // InOut + In + In の3コンポーネント
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 100.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<HealthData>(e, 50, 100);

    world_.ForEach<ECS::InOut<PositionData>, ECS::In<VelocityData>, ECS::In<HealthData>>(
        [](ECS::Actor, PositionData& pos, const VelocityData& vel, const HealthData& hp) {
            // HPに応じて移動速度を変更
            float speedMult = static_cast<float>(hp.hp) / static_cast<float>(hp.maxHp);
            pos.x += vel.vx * speedMult;
            pos.y += vel.vy * speedMult;
            pos.z += vel.vz * speedMult;
        });

    auto* p = world_.GetComponent<PositionData>(e);
    EXPECT_EQ(p->x, 100.5f);  // 100 + 1 * (50/100)
    EXPECT_EQ(p->y, 1.0f);    // 0 + 2 * 0.5
    EXPECT_EQ(p->z, 1.5f);    // 0 + 3 * 0.5
}

TEST_F(TypedForEachTest, MultipleActors)
{
    // 複数アクターへの反映
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 10; ++i) {
        ECS::Actor e = world_.CreateActor();
        world_.AddComponent<PositionData>(e, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(e);
    }

    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor, PositionData& pos) {
            pos.x *= 2.0f;
        });

    for (int i = 0; i < 10; ++i) {
        auto* p = world_.GetComponent<PositionData>(actors[i]);
        EXPECT_EQ(p->x, static_cast<float>(i * 2));
    }
}

TEST_F(TypedForEachTest, OnlyMatchingActors)
{
    // 条件を満たすActorのみ処理される
    ECS::Actor e1 = world_.CreateActor();
    world_.AddComponent<PositionData>(e1, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e1, 10.0f, 0.0f, 0.0f);

    ECS::Actor e2 = world_.CreateActor();
    world_.AddComponent<PositionData>(e2, 2.0f, 0.0f, 0.0f);
    // e2にはVelocityDataがない

    int count = 0;
    world_.ForEach<ECS::InOut<PositionData>, ECS::In<VelocityData>>(
        [&count](ECS::Actor, PositionData& pos, const VelocityData& vel) {
            pos.x += vel.vx;
            ++count;
        });

    // e1のみ処理される
    EXPECT_EQ(count, 1);
    EXPECT_EQ(world_.GetComponent<PositionData>(e1)->x, 11.0f);
    EXPECT_EQ(world_.GetComponent<PositionData>(e2)->x, 2.0f);  // 変更されない
}

TEST_F(TypedForEachTest, FiveComponents)
{
    // 5コンポーネント対応テスト
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(e, 10.0f, 20.0f, 30.0f);
    world_.AddComponent<HealthData>(e, 100, 200);
    world_.AddComponent<AccelerationData>(e, 0.1f, 0.2f, 0.3f);
    world_.AddComponent<RotationData>(e, 45.0f, 90.0f, 0.0f);

    int count = 0;
    world_.ForEach<
        ECS::InOut<PositionData>,
        ECS::In<VelocityData>,
        ECS::In<HealthData>,
        ECS::In<AccelerationData>,
        ECS::InOut<RotationData>
    >(
        [&count](ECS::Actor, PositionData& pos, const VelocityData& vel,
                 const HealthData& hp, const AccelerationData& acc, RotationData& rot) {
            pos.x += vel.vx;
            rot.yaw += 1.0f;
            ++count;
            // 読み取り専用の確認
            EXPECT_EQ(hp.hp, 100);
            EXPECT_NEAR(acc.ax, 0.1f, 0.001f);
        });

    EXPECT_EQ(count, 1);
    EXPECT_EQ(world_.GetComponent<PositionData>(e)->x, 11.0f);
    EXPECT_EQ(world_.GetComponent<RotationData>(e)->yaw, 91.0f);
}

TEST_F(TypedForEachTest, SixComponents)
{
    // 6コンポーネント対応テスト
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<HealthData>(e, 50, 100);
    world_.AddComponent<AccelerationData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<RotationData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<ScaleData>(e, 2.0f, 2.0f, 2.0f);

    float result = 0.0f;
    world_.ForEach<
        ECS::In<PositionData>,
        ECS::In<VelocityData>,
        ECS::In<HealthData>,
        ECS::In<AccelerationData>,
        ECS::In<RotationData>,
        ECS::In<ScaleData>
    >(
        [&result](ECS::Actor, const PositionData& pos, const VelocityData& vel,
                  const HealthData& hp, const AccelerationData& acc,
                  const RotationData& rot, const ScaleData& scale) {
            // 全コンポーネントから値を読み取る
            result = pos.x + vel.vx + static_cast<float>(hp.hp) + acc.ax + rot.pitch + scale.sx;
        });

    // 1 + 1 + 50 + 1 + 0 + 2 = 55
    EXPECT_NEAR(result, 55.0f, 0.001f);
}

TEST_F(TypedForEachTest, SevenComponents)
{
    // 7コンポーネント対応テスト
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 2.0f, 0.0f, 0.0f);
    world_.AddComponent<HealthData>(e, 3, 100);
    world_.AddComponent<AccelerationData>(e, 4.0f, 0.0f, 0.0f);
    world_.AddComponent<RotationData>(e, 5.0f, 0.0f, 0.0f);
    world_.AddComponent<ScaleData>(e, 6.0f, 0.0f, 0.0f);
    world_.AddComponent<ColorData>(e, 7.0f, 0.0f, 0.0f, 1.0f);

    float result = 0.0f;
    world_.ForEach<
        ECS::In<PositionData>,
        ECS::In<VelocityData>,
        ECS::In<HealthData>,
        ECS::In<AccelerationData>,
        ECS::In<RotationData>,
        ECS::In<ScaleData>,
        ECS::In<ColorData>
    >(
        [&result](ECS::Actor, const PositionData& pos, const VelocityData& vel,
                  const HealthData& hp, const AccelerationData& acc,
                  const RotationData& rot, const ScaleData& scale, const ColorData& color) {
            result = pos.x + vel.vx + static_cast<float>(hp.hp) + acc.ax + rot.pitch + scale.sx + color.r;
        });

    // 1 + 2 + 3 + 4 + 5 + 6 + 7 = 28
    EXPECT_NEAR(result, 28.0f, 0.001f);
}

TEST_F(TypedForEachTest, EightComponents)
{
    // 8コンポーネント対応テスト（最大）
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 2.0f, 0.0f, 0.0f);
    world_.AddComponent<HealthData>(e, 3, 100);
    world_.AddComponent<AccelerationData>(e, 4.0f, 0.0f, 0.0f);
    world_.AddComponent<RotationData>(e, 5.0f, 0.0f, 0.0f);
    world_.AddComponent<ScaleData>(e, 6.0f, 0.0f, 0.0f);
    world_.AddComponent<ColorData>(e, 7.0f, 0.0f, 0.0f, 1.0f);
    world_.AddComponent<AlphaData>(e, 8.0f);

    float result = 0.0f;
    world_.ForEach<
        ECS::In<PositionData>,
        ECS::In<VelocityData>,
        ECS::In<HealthData>,
        ECS::In<AccelerationData>,
        ECS::In<RotationData>,
        ECS::In<ScaleData>,
        ECS::In<ColorData>,
        ECS::In<AlphaData>
    >(
        [&result](ECS::Actor, const PositionData& pos, const VelocityData& vel,
                  const HealthData& hp, const AccelerationData& acc,
                  const RotationData& rot, const ScaleData& scale,
                  const ColorData& color, const AlphaData& alpha) {
            result = pos.x + vel.vx + static_cast<float>(hp.hp) + acc.ax
                   + rot.pitch + scale.sx + color.r + alpha.alpha;
        });

    // 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 = 36
    EXPECT_NEAR(result, 36.0f, 0.001f);
}

TEST_F(TypedForEachTest, EightComponents_MixedAccess)
{
    // 8コンポーネント混合アクセスモード
    ECS::Actor e = world_.CreateActor();
    world_.AddComponent<PositionData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<VelocityData>(e, 1.0f, 0.0f, 0.0f);
    world_.AddComponent<HealthData>(e, 100, 100);
    world_.AddComponent<AccelerationData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<RotationData>(e, 0.0f, 0.0f, 0.0f);
    world_.AddComponent<ScaleData>(e, 1.0f, 1.0f, 1.0f);
    world_.AddComponent<ColorData>(e, 0.5f, 0.5f, 0.5f, 1.0f);
    world_.AddComponent<AlphaData>(e, 1.0f);

    world_.ForEach<
        ECS::InOut<PositionData>,   // 読み書き
        ECS::In<VelocityData>,      // 読み取り
        ECS::InOut<HealthData>,     // 読み書き
        ECS::InOut<AccelerationData>, // 読み書き
        ECS::In<RotationData>,      // 読み取り
        ECS::InOut<ScaleData>,      // 読み書き
        ECS::In<ColorData>,         // 読み取り
        ECS::InOut<AlphaData>       // 読み書き
    >(
        [](ECS::Actor, PositionData& pos, const VelocityData& vel,
           HealthData& hp, AccelerationData& acc, const RotationData&,
           ScaleData& scale, const ColorData& color, AlphaData& alpha) {
            pos.x = vel.vx * 10.0f;
            hp.hp -= 10;
            acc.ax = 9.8f;
            scale.sx *= 2.0f;
            alpha.alpha = color.r;
        });

    EXPECT_EQ(world_.GetComponent<PositionData>(e)->x, 10.0f);
    EXPECT_EQ(world_.GetComponent<HealthData>(e)->hp, 90);
    EXPECT_NEAR(world_.GetComponent<AccelerationData>(e)->ax, 9.8f, 0.001f);
    EXPECT_EQ(world_.GetComponent<ScaleData>(e)->sx, 2.0f);
    EXPECT_EQ(world_.GetComponent<AlphaData>(e)->alpha, 0.5f);
}

//============================================================================
// DeferredQueue 例外安全性テスト
//============================================================================
class DeferredQueueExceptionSafetyTest : public ::testing::Test {
protected:
    ECS::DeferredQueue queue_;
};

TEST_F(DeferredQueueExceptionSafetyTest, ScopedClear_ClearsOnScopeExit)
{
    // キューに操作を追加
    queue_.PushCreate(ECS::Actor{1});
    queue_.PushCreate(ECS::Actor{2});
    EXPECT_EQ(queue_.Size(), 2u);

    {
        auto guard = queue_.ScopedClear();
        // スコープ内ではまだクリアされていない
        EXPECT_EQ(queue_.Size(), 2u);
    }

    // スコープを抜けるとクリアされる
    EXPECT_TRUE(queue_.IsEmpty());
}

TEST_F(DeferredQueueExceptionSafetyTest, ScopedClear_Release_PreventsAutoClear)
{
    queue_.PushCreate(ECS::Actor{1});
    EXPECT_EQ(queue_.Size(), 1u);

    {
        auto guard = queue_.ScopedClear();
        guard.Release();  // 自動クリアを無効化
    }

    // Release()したのでクリアされない
    EXPECT_EQ(queue_.Size(), 1u);
}

TEST_F(DeferredQueueExceptionSafetyTest, ScopedClear_MoveGuard)
{
    queue_.PushCreate(ECS::Actor{1});
    EXPECT_EQ(queue_.Size(), 1u);

    {
        auto guard1 = queue_.ScopedClear();
        auto guard2 = std::move(guard1);  // ムーブ
        // guard1はnullptrになる（デストラクタで何もしない）
    }

    // guard2のデストラクタでクリアされる
    EXPECT_TRUE(queue_.IsEmpty());
}

//============================================================================
// BeginFrame 例外安全性テスト
//============================================================================
class BeginFrameExceptionSafetyTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(BeginFrameExceptionSafetyTest, BeginFrame_QueueClearedAfterProcessing)
{
    // 遅延操作を追加（有効なActorを作成してから破棄）
    ECS::Actor actor = world_.CreateActor();
    world_.Deferred().DestroyActor(actor);

    // 遅延キューにデータがある
    EXPECT_FALSE(world_.Deferred().IsEmpty());

    world_.BeginFrame();

    // BeginFrame後にキューはクリアされる
    EXPECT_TRUE(world_.Deferred().IsEmpty());
    // Actorは破棄されている
    EXPECT_FALSE(world_.IsAlive(actor));
}

TEST_F(BeginFrameExceptionSafetyTest, BeginFrame_FrameCounterIncrements)
{
    uint32_t initialFrame = world_.GetFrameCounter();

    world_.BeginFrame();

    EXPECT_EQ(world_.GetFrameCounter(), initialFrame + 1);
}

TEST_F(BeginFrameExceptionSafetyTest, BeginFrame_ContinuesAfterInvalidActorDestroy)
{
    // 有効なActorを作成
    ECS::Actor validActor = world_.CreateActor();
    world_.AddComponent<PositionData>(validActor, 1.0f, 2.0f, 3.0f);

    // 無効なActorを遅延破棄
    world_.Deferred().DestroyActor(ECS::Actor::Invalid());

    // 有効なActorを遅延破棄
    world_.Deferred().DestroyActor(validActor);

    // BeginFrameで両方処理される（無効な方はスキップ）
    world_.BeginFrame();

    // 有効なActorは正しく破棄されている
    EXPECT_FALSE(world_.IsAlive(validActor));

    // キューはクリアされている
    EXPECT_TRUE(world_.Deferred().IsEmpty());
}

TEST_F(BeginFrameExceptionSafetyTest, BeginFrame_ProcessesAllOperations)
{
    // 複数の遅延操作を追加
    ECS::Actor actor1 = world_.CreateActor();
    ECS::Actor actor2 = world_.CreateActor();

    world_.Deferred().DestroyActor(actor1);
    world_.Deferred().AddComponent<PositionData>(actor2, 5.0f, 10.0f, 15.0f);

    world_.BeginFrame();

    // actor1は破棄されている
    EXPECT_FALSE(world_.IsAlive(actor1));

    // actor2にPositionDataが追加されている
    EXPECT_TRUE(world_.IsAlive(actor2));
    auto* pos = world_.GetComponent<PositionData>(actor2);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->x, 5.0f);
}

//============================================================================
// ComponentRef テスト (Phase 2: 安全なポインタ管理)
//============================================================================
class ComponentRefTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ComponentRefTest, GetRef_ReturnsValidRef)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    EXPECT_TRUE(ref.IsValid());
    EXPECT_EQ(ref.GetActor(), actor);
}

TEST_F(ComponentRefTest, Get_ReturnsComponent)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 10.0f, 20.0f, 30.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    EXPECT_EQ(ref.Get().x, 10.0f);
    EXPECT_EQ(ref.Get().y, 20.0f);
    EXPECT_EQ(ref.Get().z, 30.0f);
}

TEST_F(ComponentRefTest, TryGet_ReturnsPointer)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 5.0f, 10.0f, 15.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    auto* ptr = ref.TryGet();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->x, 5.0f);
}

TEST_F(ComponentRefTest, ArrowOperator_Works)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 100.0f, 200.0f, 300.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    EXPECT_EQ(ref->x, 100.0f);
    EXPECT_EQ(ref->y, 200.0f);
    EXPECT_EQ(ref->z, 300.0f);
}

TEST_F(ComponentRefTest, DereferenceOperator_Works)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    PositionData& pos = *ref;
    EXPECT_EQ(pos.x, 1.0f);
}

TEST_F(ComponentRefTest, Modification_ThroughRef)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 0.0f, 0.0f, 0.0f);

    auto ref = world_.GetRef<PositionData>(actor);
    ref->x = 50.0f;
    ref->y = 100.0f;

    // 直接取得しても変更が反映されている
    auto* pos = world_.GetComponent<PositionData>(actor);
    EXPECT_EQ(pos->x, 50.0f);
    EXPECT_EQ(pos->y, 100.0f);
}

TEST_F(ComponentRefTest, CacheRefresh_AfterBeginFrame)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    // 初回アクセス
    EXPECT_EQ(ref->x, 1.0f);

    // フレームを進める
    world_.BeginFrame();

    // キャッシュが更新されても正しい値が返る
    EXPECT_EQ(ref->x, 1.0f);
}

TEST_F(ComponentRefTest, CacheRefresh_AfterComponentChange)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);
    EXPECT_EQ(ref->x, 1.0f);

    // コンポーネント追加でArchetype移行が発生する可能性
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    // フレームを進めてキャッシュを無効化
    world_.BeginFrame();

    // 再取得して正しいポインタを得る
    EXPECT_EQ(ref->x, 1.0f);  // 値は保持されている
}

TEST_F(ComponentRefTest, InvalidRef_FromDeadActor)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);
    EXPECT_TRUE(ref.IsValid());

    // Actorを破棄
    world_.DestroyActor(actor);

    // refは依然としてactor IDを持っているが、TryGetはnullを返す
    world_.BeginFrame();  // キャッシュ無効化

    // TryGetはnullptr（Actorが死んでいるため）
    EXPECT_EQ(ref.TryGet(), nullptr);
}

TEST_F(ComponentRefTest, InvalidRef_Default)
{
    ECS::ComponentRef<PositionData> ref;

    EXPECT_FALSE(ref.IsValid());
    EXPECT_EQ(ref.TryGet(), nullptr);
}

TEST_F(ComponentRefTest, Invalidate_ClearsCache)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);
    EXPECT_EQ(ref->x, 1.0f);  // キャッシュされる

    ref.Invalidate();

    // 再アクセスすると再取得される
    EXPECT_EQ(ref->x, 1.0f);
}

TEST_F(ComponentRefTest, BoolConversion)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);
    EXPECT_TRUE(static_cast<bool>(ref));

    ECS::ComponentRef<PositionData> invalidRef;
    EXPECT_FALSE(static_cast<bool>(invalidRef));
}

TEST_F(ComponentRefTest, ConstRef_ReadOnly)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 5.0f, 10.0f, 15.0f);

    const ECS::World& constWorld = world_;
    auto constRef = constWorld.GetRef<PositionData>(actor);

    EXPECT_TRUE(constRef.IsValid());
    EXPECT_EQ(constRef->x, 5.0f);
    EXPECT_EQ(constRef->y, 10.0f);
    EXPECT_EQ(constRef->z, 15.0f);
}

TEST_F(ComponentRefTest, MultipleRefs_SameActor)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    auto posRef = world_.GetRef<PositionData>(actor);
    auto velRef = world_.GetRef<VelocityData>(actor);

    EXPECT_TRUE(posRef.IsValid());
    EXPECT_TRUE(velRef.IsValid());

    EXPECT_EQ(posRef->x, 1.0f);
    EXPECT_EQ(velRef->vx, 10.0f);
}

TEST_F(ComponentRefTest, Ref_SurvivesMultipleFrames)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 0.0f, 0.0f, 0.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    // 複数フレームにわたって使用
    for (int i = 0; i < 10; ++i) {
        ref->x += 1.0f;
        world_.BeginFrame();
    }

    EXPECT_EQ(ref->x, 10.0f);
}

TEST_F(ComponentRefTest, GetWorld_ReturnsWorld)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    auto ref = world_.GetRef<PositionData>(actor);

    EXPECT_EQ(&ref.GetWorld(), &world_);
}

//============================================================================
// 変更追跡テスト (Phase 6)
//============================================================================
class ChangeTrackingTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(ChangeTrackingTest, ForEach_InOut_UpdatesVersion)
{
    // Actorを作成してコンポーネントを追加
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 初期バージョンを記録
    uint32_t initialVersion = world_.GetFrameCounter();

    // フレームを進める
    world_.BeginFrame();

    // InOut（書き込み）でForEach
    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor e, PositionData& p) {
            p.x += 1.0f;
        });

    // 変更フィルターでinitialVersion以降のものを取得
    size_t count = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(initialVersion)
        .Count();

    // 書き込みがあったのでカウントされる
    EXPECT_EQ(count, 1);
}

TEST_F(ChangeTrackingTest, ForEach_In_DoesNotUpdateVersion)
{
    // Actorを作成してコンポーネントを追加
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // フレームを進めてバージョンを初期化
    world_.BeginFrame();
    uint32_t initialVersion = world_.GetFrameCounter();

    // 再度フレームを進める
    world_.BeginFrame();

    // In（読み取りのみ）でForEach
    world_.ForEach<ECS::In<PositionData>>(
        [](ECS::Actor e, const PositionData& p) {
            auto x = p.x;  // 読み取りのみ
            (void)x;
        });

    // 変更フィルターでinitialVersion以降のものを取得
    size_t count = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(initialVersion)
        .Count();

    // 読み取りのみなのでバージョンは更新されていない
    EXPECT_EQ(count, 0);
}

TEST_F(ChangeTrackingTest, WithChangeFilter_FiltersOldChunks)
{
    // 複数Actorを作成
    std::vector<ECS::Actor> actors;
    for (int i = 0; i < 5; ++i) {
        ECS::Actor actor = world_.CreateActor();
        world_.AddComponent<PositionData>(actor, static_cast<float>(i), 0.0f, 0.0f);
        actors.push_back(actor);
    }

    // 初期バージョンを記録
    world_.BeginFrame();
    uint32_t v1 = world_.GetFrameCounter();

    // 最初のActorのみ変更
    world_.BeginFrame();
    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor e, PositionData& p) {
            p.x += 10.0f;
        });

    // v1以降に変更されたものを取得
    size_t countAfterV1 = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(v1)
        .Count();

    // 全て同じChunkにあるので、全Actor分カウントされる
    EXPECT_EQ(countAfterV1, 5);
}

TEST_F(ChangeTrackingTest, WithChangeFilter_NoChanges_ReturnsZero)
{
    // Actorを作成
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 最新フレームでフィルター（変更なし）
    world_.BeginFrame();
    uint32_t currentVersion = world_.GetFrameCounter();

    size_t count = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(currentVersion)
        .Count();

    // 現在のバージョン以降の変更はないので0
    EXPECT_EQ(count, 0);
}

TEST_F(ChangeTrackingTest, WithChangeFilter_InOut_UpdatesVersion)
{
    // Actorを作成
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 0.0f, 0.0f, 0.0f);

    world_.BeginFrame();
    uint32_t beforeOut = world_.GetFrameCounter();

    world_.BeginFrame();

    // InOut（読み書き）でForEach
    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor e, PositionData& p) {
            p.x = 100.0f;  // 初期化
        });

    size_t count = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(beforeOut)
        .Count();

    // Out書き込みでバージョンが更新された
    EXPECT_EQ(count, 1);
}

TEST_F(ChangeTrackingTest, MultipleComponents_IndependentVersions)
{
    // Actorを作成して複数コンポーネントを追加
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 20.0f, 30.0f);

    world_.BeginFrame();
    uint32_t v1 = world_.GetFrameCounter();

    world_.BeginFrame();

    // PositionDataのみ変更
    world_.ForEach<ECS::InOut<PositionData>, ECS::In<VelocityData>>(
        [](ECS::Actor e, PositionData& p, const VelocityData& v) {
            p.x += v.vx;
        });

    // PositionDataの変更をフィルター
    size_t posCount = world_.Query<PositionData>()
        .WithChangeFilter<PositionData>(v1)
        .Count();
    EXPECT_EQ(posCount, 1);

    // VelocityDataの変更をフィルター（読み取りのみなので更新されていない）
    size_t velCount = world_.Query<VelocityData>()
        .WithChangeFilter<VelocityData>(v1)
        .Count();
    EXPECT_EQ(velCount, 0);
}

TEST_F(ChangeTrackingTest, Query_WithChangeFilter_ChainedFilters)
{
    // 複数のフィルターをチェーン
    ECS::Actor actor1 = world_.CreateActor();
    world_.AddComponent<PositionData>(actor1, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor1, 0.0f, 0.0f, 0.0f);

    ECS::Actor actor2 = world_.CreateActor();
    world_.AddComponent<PositionData>(actor2, 4.0f, 5.0f, 6.0f);
    // actor2にはVelocityDataを追加しない

    world_.BeginFrame();
    uint32_t v1 = world_.GetFrameCounter();

    world_.BeginFrame();

    // PositionDataのみ変更
    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor e, PositionData& p) {
            p.x *= 2.0f;
        });

    // PositionDataとVelocityData両方を持ち、PositionDataが変更されたもの
    int count = 0;
    world_.Query<PositionData, VelocityData>()
        .WithChangeFilter<PositionData>(v1)
        .ForEach([&count](ECS::Actor e, PositionData& p, VelocityData& v) {
            count++;
        });

    // actor1のみがマッチ（actor2はVelocityDataを持たない）
    EXPECT_EQ(count, 1);
}

TEST_F(ChangeTrackingTest, GetComponentVersion_ReturnsCorrectVersion)
{
    // Actorを作成
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    world_.BeginFrame();  // v1

    // 書き込みでバージョン更新
    world_.ForEach<ECS::InOut<PositionData>>(
        [](ECS::Actor e, PositionData& p) {
            p.x += 1.0f;
        });

    // ArchetypeStorageからバージョンを確認
    auto& storage = world_.GetArchetypeStorage();
    storage.ForEachMatching<PositionData>([&](ECS::Archetype& arch) {
        uint32_t version = arch.GetComponentVersion<PositionData>(0);
        EXPECT_EQ(version, world_.GetFrameCounter());
    });
}

//============================================================================
// Tag Component テスト
//============================================================================

// テスト用Tagコンポーネント
struct Tag_Player : ECS::ITagComponentData {};
struct Tag_Enemy : ECS::ITagComponentData {};
struct Tag_Inactive : ECS::ITagComponentData {};

ECS_TAG_COMPONENT(Tag_Player);
ECS_TAG_COMPONENT(Tag_Enemy);
ECS_TAG_COMPONENT(Tag_Inactive);

class TagComponentTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(TagComponentTest, IsTagComponentTrait)
{
    // Tagコンポーネントはtrueを返す
    EXPECT_TRUE(ECS::is_tag_component_v<Tag_Player>);
    EXPECT_TRUE(ECS::is_tag_component_v<Tag_Enemy>);
    EXPECT_TRUE(ECS::is_tag_component_v<Tag_Inactive>);

    // 通常のコンポーネントはfalseを返す
    EXPECT_FALSE(ECS::is_tag_component_v<PositionData>);
    EXPECT_FALSE(ECS::is_tag_component_v<VelocityData>);
    EXPECT_FALSE(ECS::is_tag_component_v<HealthData>);
}

TEST_F(TagComponentTest, AddTagComponent)
{
    ECS::Actor actor = world_.CreateActor();

    // Tagを追加
    world_.AddComponent<Tag_Player>(actor);

    // HasComponentで確認できる
    EXPECT_TRUE(world_.HasComponent<Tag_Player>(actor));
    EXPECT_FALSE(world_.HasComponent<Tag_Enemy>(actor));
}

TEST_F(TagComponentTest, TagWithDataComponent)
{
    ECS::Actor actor = world_.CreateActor();

    // データコンポーネントとTag両方を追加
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<Tag_Player>(actor);

    // 両方持っているか確認
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<Tag_Player>(actor));

    // データコンポーネントは正常にアクセスできる
    auto* pos = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(pos, nullptr);
    EXPECT_FLOAT_EQ(pos->x, 1.0f);
    EXPECT_FLOAT_EQ(pos->y, 2.0f);
    EXPECT_FLOAT_EQ(pos->z, 3.0f);
}

TEST_F(TagComponentTest, FilterByTag)
{
    // Player 2体、Enemy 3体
    for (int i = 0; i < 2; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, float(i), 0.0f, 0.0f);
        world_.AddComponent<Tag_Player>(a);
    }
    for (int i = 0; i < 3; ++i) {
        ECS::Actor a = world_.CreateActor();
        world_.AddComponent<PositionData>(a, float(i + 10), 0.0f, 0.0f);
        world_.AddComponent<Tag_Enemy>(a);
    }

    // Playerのみカウント
    int playerCount = 0;
    world_.GetArchetypeStorage().ForEachMatching<PositionData, Tag_Player>(
        [&](ECS::Archetype& arch) {
            playerCount += static_cast<int>(arch.GetActorCount());
        });
    EXPECT_EQ(playerCount, 2);

    // Enemyのみカウント
    int enemyCount = 0;
    world_.GetArchetypeStorage().ForEachMatching<PositionData, Tag_Enemy>(
        [&](ECS::Archetype& arch) {
            enemyCount += static_cast<int>(arch.GetActorCount());
        });
    EXPECT_EQ(enemyCount, 3);
}

TEST_F(TagComponentTest, TagArchetypeSize)
{
    // Tagコンポーネントはサイズ0として扱われる
    ECS::Actor actor1 = world_.CreateActor();
    world_.AddComponent<PositionData>(actor1, 1.0f, 2.0f, 3.0f);

    ECS::Actor actor2 = world_.CreateActor();
    world_.AddComponent<PositionData>(actor2, 4.0f, 5.0f, 6.0f);
    world_.AddComponent<Tag_Player>(actor2);

    // 両方のArchetypeを確認
    auto& storage = world_.GetArchetypeStorage();

    // PositionDataのみのArchetype
    size_t posOnlySize = 0;
    storage.ForEachMatching<PositionData>([&](ECS::Archetype& arch) {
        if (!arch.HasComponent<Tag_Player>()) {
            posOnlySize = arch.GetComponentDataSize();
        }
    });

    // PositionData + Tag_PlayerのArchetype
    size_t posTagSize = 0;
    storage.ForEachMatching<PositionData, Tag_Player>([&](ECS::Archetype& arch) {
        posTagSize = arch.GetComponentDataSize();
    });

    // Tagはサイズ0なので、両者のComponentDataSizeは同じはず
    EXPECT_EQ(posOnlySize, posTagSize);
}

TEST_F(TagComponentTest, RemoveTagComponent)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<Tag_Player>(actor);

    EXPECT_TRUE(world_.HasComponent<Tag_Player>(actor));

    // Tagを削除
    world_.RemoveComponent<Tag_Player>(actor);

    EXPECT_FALSE(world_.HasComponent<Tag_Player>(actor));
    // データコンポーネントは残っている
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
}

//============================================================================
// Enableable Component テスト
//============================================================================

class EnableableComponentTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(EnableableComponentTest, DefaultEnabled)
{
    // 新規追加されたコンポーネントはデフォルトで有効
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    EXPECT_TRUE(world_.IsEnabled<PositionData>(actor));
}

TEST_F(EnableableComponentTest, SetEnabledFalse)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 無効化
    world_.SetEnabled<PositionData>(actor, false);

    EXPECT_FALSE(world_.IsEnabled<PositionData>(actor));
    // コンポーネント自体は存在する
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
}

TEST_F(EnableableComponentTest, SetEnabledTrue)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 無効化→有効化
    world_.SetEnabled<PositionData>(actor, false);
    EXPECT_FALSE(world_.IsEnabled<PositionData>(actor));

    world_.SetEnabled<PositionData>(actor, true);
    EXPECT_TRUE(world_.IsEnabled<PositionData>(actor));
}

TEST_F(EnableableComponentTest, NoArchetypeChange)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 0.0f, 0.0f);

    // Archetypeを記録
    auto* archBefore = &world_.GetArchetypeStorage();
    int archetypeCountBefore = 0;
    archBefore->ForEachMatching<PositionData>([&](ECS::Archetype&) {
        archetypeCountBefore++;
    });

    // 無効化
    world_.SetEnabled<PositionData>(actor, false);

    // Archetype数は変わらない
    int archetypeCountAfter = 0;
    archBefore->ForEachMatching<PositionData>([&](ECS::Archetype&) {
        archetypeCountAfter++;
    });
    EXPECT_EQ(archetypeCountBefore, archetypeCountAfter);
}

TEST_F(EnableableComponentTest, MultipleComponents)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);
    world_.AddComponent<VelocityData>(actor, 10.0f, 0.0f, 0.0f);

    // 個別に有効/無効を設定できる
    world_.SetEnabled<PositionData>(actor, false);
    world_.SetEnabled<VelocityData>(actor, true);

    EXPECT_FALSE(world_.IsEnabled<PositionData>(actor));
    EXPECT_TRUE(world_.IsEnabled<VelocityData>(actor));
}

TEST_F(EnableableComponentTest, IsEnabledForNonExistentComponent)
{
    ECS::Actor actor = world_.CreateActor();
    world_.AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f);

    // 持っていないコンポーネントはfalse
    EXPECT_FALSE(world_.IsEnabled<VelocityData>(actor));
}

//============================================================================
// Prefab テスト
//============================================================================

class PrefabTest : public ::testing::Test {
protected:
    ECS::World world_;
};

TEST_F(PrefabTest, CreatePrefab)
{
    // Prefabを作成
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(1.0f, 2.0f, 3.0f)
        .Add<VelocityData>(10.0f, 20.0f, 30.0f)
        .Build();

    EXPECT_TRUE(prefab.IsValid());
    EXPECT_NE(prefab.GetArchetype(), nullptr);
}

TEST_F(PrefabTest, InstantiateSingle)
{
    // Prefabを作成
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(1.0f, 2.0f, 3.0f)
        .Add<VelocityData>(10.0f, 20.0f, 30.0f)
        .Build();

    // インスタンス化
    ECS::Actor actor = world_.Instantiate(prefab);

    EXPECT_TRUE(actor.IsValid());
    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));

    // 値が正しくコピーされている
    auto* pos = world_.GetComponent<PositionData>(actor);
    ASSERT_NE(pos, nullptr);
    EXPECT_FLOAT_EQ(pos->x, 1.0f);
    EXPECT_FLOAT_EQ(pos->y, 2.0f);
    EXPECT_FLOAT_EQ(pos->z, 3.0f);

    auto* vel = world_.GetComponent<VelocityData>(actor);
    ASSERT_NE(vel, nullptr);
    EXPECT_FLOAT_EQ(vel->vx, 10.0f);
    EXPECT_FLOAT_EQ(vel->vy, 20.0f);
    EXPECT_FLOAT_EQ(vel->vz, 30.0f);
}

TEST_F(PrefabTest, InstantiateMultiple)
{
    // Prefabを作成
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(0.0f, 0.0f, 0.0f)
        .Add<VelocityData>(1.0f, 1.0f, 1.0f)
        .Build();

    // 複数インスタンス化
    constexpr size_t count = 100;
    auto actors = world_.Instantiate(prefab, count);

    EXPECT_EQ(actors.size(), count);

    // 全てのActorが正しいコンポーネントを持つ
    for (const auto& actor : actors) {
        EXPECT_TRUE(actor.IsValid());
        EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
        EXPECT_TRUE(world_.HasComponent<VelocityData>(actor));
    }
}

TEST_F(PrefabTest, InstantiateWithTag)
{
    // TagコンポーネントありのPrefab
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(5.0f, 5.0f, 5.0f)
        .Add<Tag_Player>()
        .Build();

    ECS::Actor actor = world_.Instantiate(prefab);

    EXPECT_TRUE(world_.HasComponent<PositionData>(actor));
    EXPECT_TRUE(world_.HasComponent<Tag_Player>(actor));

    auto* pos = world_.GetComponent<PositionData>(actor);
    EXPECT_FLOAT_EQ(pos->x, 5.0f);
}

TEST_F(PrefabTest, ModifyAfterInstantiate)
{
    // Prefabを作成
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(0.0f, 0.0f, 0.0f)
        .Build();

    // インスタンス化後に値を変更
    ECS::Actor actor = world_.Instantiate(prefab);
    auto* pos = world_.GetComponent<PositionData>(actor);
    pos->x = 100.0f;
    pos->y = 200.0f;

    EXPECT_FLOAT_EQ(pos->x, 100.0f);
    EXPECT_FLOAT_EQ(pos->y, 200.0f);
}

TEST_F(PrefabTest, InvalidPrefab)
{
    // 空のPrefab
    ECS::Prefab emptyPrefab;
    EXPECT_FALSE(emptyPrefab.IsValid());

    // 無効なPrefabからのインスタンス化
    ECS::Actor actor = world_.Instantiate(emptyPrefab);
    EXPECT_FALSE(actor.IsValid());
}

TEST_F(PrefabTest, GetComponentOffset)
{
    auto prefab = world_.CreatePrefab()
        .Add<PositionData>(1.0f, 2.0f, 3.0f)
        .Add<VelocityData>(4.0f, 5.0f, 6.0f)
        .Build();

    // オフセットが取得できる
    size_t posOffset = prefab.GetComponentOffset<PositionData>();
    size_t velOffset = prefab.GetComponentOffset<VelocityData>();

    EXPECT_NE(posOffset, SIZE_MAX);
    EXPECT_NE(velOffset, SIZE_MAX);
    EXPECT_NE(posOffset, velOffset);  // 異なるオフセット
}

} // namespace
