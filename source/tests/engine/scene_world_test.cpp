//----------------------------------------------------------------------------
//! @file   scene_world_test.cpp
//! @brief  Scene と ECS World 統合テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/scene/scene.h"
#include "engine/ecs/system.h"
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/systems/transform_system.h"

namespace
{

// テスト用RenderSystem
static bool g_renderSystemCalled = false;

class TestRenderSystem final : public ECS::IRenderSystem {
public:
    void Render(ECS::World&, float) override {
        g_renderSystemCalled = true;
    }
    int Priority() const override { return 0; }
    const char* Name() const override { return "TestRenderSystem"; }
};

//============================================================================
// テスト用シーン
//============================================================================
class TestECSScene : public Scene
{
public:
    bool fixedUpdateCalled = false;
    bool renderCalled = false;
    float lastDt = 0.0f;
    float lastAlpha = 0.0f;
    int fixedUpdateCount = 0;

    void OnEnter() override {
        InitializeWorld();
        GetWorldRef().RegisterSystem<ECS::TransformSystem>();
    }

    void FixedUpdate(float dt) override {
        Scene::FixedUpdate(dt);  // Worldを更新
        fixedUpdateCalled = true;
        lastDt = dt;
        ++fixedUpdateCount;
    }

    void Render(float alpha) override {
        Scene::Render(alpha);  // Worldを描画
        renderCalled = true;
        lastAlpha = alpha;
    }

    const char* GetName() const override { return "TestECSScene"; }
};

//============================================================================
// 従来互換テスト用シーン
//============================================================================
class LegacyScene : public Scene
{
public:
    bool updateCalled = false;
    bool renderCalled = false;

    void Update() override {
        updateCalled = true;
    }

    void Render(float) override {
        renderCalled = true;
    }

    const char* GetName() const override { return "LegacyScene"; }
};

//============================================================================
// Scene + World 統合テスト
//============================================================================
class SceneWorldTest : public ::testing::Test {};

TEST_F(SceneWorldTest, InitializeWorld)
{
    TestECSScene scene;
    EXPECT_FALSE(scene.HasWorld());

    scene.OnEnter();

    EXPECT_TRUE(scene.HasWorld());
    EXPECT_NE(scene.GetWorld(), nullptr);
}

TEST_F(SceneWorldTest, FixedUpdateCallsWorldFixedUpdate)
{
    TestECSScene scene;
    scene.OnEnter();

    // エンティティ追加
    ECS::Actor e = scene.GetWorld()->CreateActor();
    auto* t = scene.GetWorld()->AddComponent<ECS::TransformData>(e);
    t->position = Vector3(10.0f, 20.0f, 30.0f);
    EXPECT_TRUE(t->dirty);

    // FixedUpdate呼び出し
    scene.FixedUpdate(1.0f / 60.0f);

    EXPECT_TRUE(scene.fixedUpdateCalled);
    EXPECT_FALSE(t->dirty);  // TransformSystemが処理済み
    EXPECT_NEAR(t->worldMatrix._41, 10.0f, 0.001f);
}

TEST_F(SceneWorldTest, RenderCallsWorldRender)
{
    TestECSScene scene;
    scene.OnEnter();

    g_renderSystemCalled = false;
    scene.GetWorld()->RegisterRenderSystem<TestRenderSystem>();

    scene.Render(0.5f);

    EXPECT_TRUE(scene.renderCalled);
    EXPECT_TRUE(g_renderSystemCalled);
    EXPECT_FLOAT_EQ(scene.lastAlpha, 0.5f);
}

TEST_F(SceneWorldTest, FixedUpdateDeltaTime)
{
    TestECSScene scene;
    scene.OnEnter();

    constexpr float kFixedDt = 1.0f / 60.0f;
    scene.FixedUpdate(kFixedDt);

    EXPECT_FLOAT_EQ(scene.lastDt, kFixedDt);
}

TEST_F(SceneWorldTest, MultipleFixedUpdates)
{
    TestECSScene scene;
    scene.OnEnter();

    scene.FixedUpdate(1.0f / 60.0f);
    scene.FixedUpdate(1.0f / 60.0f);
    scene.FixedUpdate(1.0f / 60.0f);

    EXPECT_EQ(scene.fixedUpdateCount, 3);
}

//============================================================================
// 従来互換テスト
//============================================================================
TEST_F(SceneWorldTest, LegacySceneUpdateWorks)
{
    LegacyScene scene;

    scene.Update();
    scene.Render(1.0f);

    EXPECT_TRUE(scene.updateCalled);
    EXPECT_TRUE(scene.renderCalled);
}

TEST_F(SceneWorldTest, SceneWithoutWorldDoesNotCrash)
{
    // Worldなしでも FixedUpdate/Render が呼べる
    Scene scene;

    // クラッシュしないことを確認
    scene.FixedUpdate(1.0f / 60.0f);
    scene.Render(1.0f);

    EXPECT_FALSE(scene.HasWorld());
}

//============================================================================
// Actor ライフサイクルテスト
//============================================================================
TEST_F(SceneWorldTest, EntityCreationInScene)
{
    TestECSScene scene;
    scene.OnEnter();

    auto* world = scene.GetWorld();
    EXPECT_EQ(world->ActorCount(), 0u);

    ECS::Actor e1 = world->CreateActor();
    ECS::Actor e2 = world->CreateActor();
    ECS::Actor e3 = world->CreateActor();

    EXPECT_EQ(world->ActorCount(), 3u);
    EXPECT_TRUE(world->IsAlive(e1));
    EXPECT_TRUE(world->IsAlive(e2));
    EXPECT_TRUE(world->IsAlive(e3));
}

TEST_F(SceneWorldTest, ComponentsUpdateCorrectly)
{
    TestECSScene scene;
    scene.OnEnter();

    auto* world = scene.GetWorld();

    // 複数エンティティ
    std::vector<ECS::Actor> entities;
    for (int i = 0; i < 10; ++i) {
        ECS::Actor e = world->CreateActor();
        auto* t = world->AddComponent<ECS::TransformData>(e);
        t->position = Vector3(static_cast<float>(i * 10), 0.0f, 0.0f);
        entities.push_back(e);
    }

    // FixedUpdate
    scene.FixedUpdate(1.0f / 60.0f);

    // 全て更新されていることを確認
    for (int i = 0; i < 10; ++i) {
        auto* t = world->GetComponent<ECS::TransformData>(entities[i]);
        EXPECT_FALSE(t->dirty);
        EXPECT_NEAR(t->worldMatrix._41, static_cast<float>(i * 10), 0.001f);
    }
}

//============================================================================
// Scene OnExit テスト
//============================================================================
class CleanupTestScene : public Scene
{
public:
    bool exitCalled = false;

    void OnEnter() override {
        InitializeWorld();
    }

    void OnExit() override {
        exitCalled = true;
        // Worldは自動的にクリーンアップされる
    }
};

TEST_F(SceneWorldTest, OnExitCleanup)
{
    auto scene = std::make_unique<CleanupTestScene>();
    scene->OnEnter();

    auto* world = scene->GetWorld();
    ECS::Actor e = world->CreateActor();
    world->AddComponent<ECS::TransformData>(e);
    EXPECT_EQ(world->ActorCount(), 1u);

    scene->OnExit();
    EXPECT_TRUE(scene->exitCalled);

    // シーン破棄後、Worldも破棄される
    scene.reset();
    // クラッシュしなければOK
}

} // namespace
