//----------------------------------------------------------------------------
//! @file   scene_test.cpp
//! @brief  Sceneクラスのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/scene/scene.h"

namespace
{

//============================================================================
// テスト用シーン派生クラス
//============================================================================
class TestScene : public Scene
{
public:
    bool onEnterCalled = false;
    bool onExitCalled = false;
    bool onLoadAsyncCalled = false;
    bool onLoadCompleteCalled = false;
    bool updateCalled = false;
    bool renderCalled = false;

    void OnEnter() override { onEnterCalled = true; }
    void OnExit() override { onExitCalled = true; }
    void OnLoadAsync() override { onLoadAsyncCalled = true; }
    void OnLoadComplete() override { onLoadCompleteCalled = true; }
    void Update() override { updateCalled = true; }
    void Render([[maybe_unused]] float alpha) override { renderCalled = true; }
    const char* GetName() const override { return "TestScene"; }
};

//============================================================================
// Scene 基本テスト
//============================================================================
TEST(SceneTest, DefaultName)
{
    Scene scene;
    EXPECT_STREQ(scene.GetName(), "Scene");
}

TEST(SceneTest, CustomName)
{
    TestScene scene;
    EXPECT_STREQ(scene.GetName(), "TestScene");
}

//============================================================================
// ロード進捗テスト
//============================================================================
TEST(SceneTest, InitialLoadProgressIsZero)
{
    Scene scene;
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.0f);
}

TEST(SceneTest, SetLoadProgress)
{
    Scene scene;
    scene.SetLoadProgress(0.5f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.5f);
}

TEST(SceneTest, SetLoadProgressClampsToZero)
{
    Scene scene;
    scene.SetLoadProgress(-1.0f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.0f);
}

TEST(SceneTest, SetLoadProgressClampsToOne)
{
    Scene scene;
    scene.SetLoadProgress(2.0f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 1.0f);
}

TEST(SceneTest, SetLoadProgressFullRange)
{
    Scene scene;

    scene.SetLoadProgress(0.0f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.0f);

    scene.SetLoadProgress(0.25f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.25f);

    scene.SetLoadProgress(0.5f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.5f);

    scene.SetLoadProgress(0.75f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 0.75f);

    scene.SetLoadProgress(1.0f);
    EXPECT_FLOAT_EQ(scene.GetLoadProgress(), 1.0f);
}

//============================================================================
// テクスチャスコープテスト
//============================================================================
TEST(SceneTest, DefaultTextureScopeIsGlobal)
{
    Scene scene;
    EXPECT_EQ(scene.GetTextureScopeId(), TextureManager::kGlobalScope);
}

TEST(SceneTest, SetTextureScopeId)
{
    Scene scene;
    scene.SetTextureScopeId(42);
    EXPECT_EQ(scene.GetTextureScopeId(), 42u);
}

//============================================================================
// ライフサイクルコールバックテスト
//============================================================================
TEST(SceneTest, OnEnterCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.onEnterCalled);
    scene.OnEnter();
    EXPECT_TRUE(scene.onEnterCalled);
}

TEST(SceneTest, OnExitCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.onExitCalled);
    scene.OnExit();
    EXPECT_TRUE(scene.onExitCalled);
}

TEST(SceneTest, OnLoadAsyncCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.onLoadAsyncCalled);
    scene.OnLoadAsync();
    EXPECT_TRUE(scene.onLoadAsyncCalled);
}

TEST(SceneTest, OnLoadCompleteCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.onLoadCompleteCalled);
    scene.OnLoadComplete();
    EXPECT_TRUE(scene.onLoadCompleteCalled);
}

TEST(SceneTest, UpdateCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.updateCalled);
    scene.Update();
    EXPECT_TRUE(scene.updateCalled);
}

TEST(SceneTest, RenderCanBeCalled)
{
    TestScene scene;
    EXPECT_FALSE(scene.renderCalled);
    scene.Render(1.0f);
    EXPECT_TRUE(scene.renderCalled);
}

//============================================================================
// コピー/ムーブセマンティクステスト
//============================================================================
// Note: Scene declares move constructor/assignment as default, but
// std::atomic<float> member makes the class effectively non-movable.
// This is by design - scenes are meant to be managed by SceneManager.

TEST(SceneTest, IsNotCopyConstructible)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Scene>);
}

TEST(SceneTest, IsNotCopyAssignable)
{
    EXPECT_FALSE(std::is_copy_assignable_v<Scene>);
}

} // namespace
