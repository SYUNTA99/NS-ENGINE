//----------------------------------------------------------------------------
//! @file   game.cpp
//! @brief  ゲームクラス実装
//----------------------------------------------------------------------------
#include "game.h"
#include "engine/core/engine.h"
#include "engine/platform/application.h"
#include "engine/core/job_system.h"
#include "engine/memory/memory_system.h"
#include "engine/scene/scene_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"
#include "title_scene.h"
#include "game_scene.h"
#include "result_scene.h"
#include "animation_test_scene.h"
#include "cube_editor_scene.h"

//----------------------------------------------------------------------------
Game::Game() = default;

//----------------------------------------------------------------------------
bool Game::Initialize()
{
    // エンジン初期化（シングルトン管理はg_Engineに委譲）
    if (!g_Engine.Initialize()) {
        LOG_ERROR("[Game] Engine initialization failed");
        return false;
    }

    // 初期シーン: CubeEditorScene
    SceneManager::Get().Load<TitleScene>();
    //SceneManager::Get().Load<GameScene>();
    //SceneManager::Get().Load<AnimationTestScene>();
    LOG_INFO("[Game] Initialization complete");
    return true;
}

//----------------------------------------------------------------------------
void Game::Shutdown() noexcept
{
    // パイプラインから全リソースをアンバインド
    if (auto* ctx = GraphicsContext::Get().GetContext()) {
        ctx->ClearState();
        ctx->Flush();
    }

    // 現在のシーン終了
    if (currentScene_) {
        currentScene_->OnExit();
        currentScene_.reset();
    }

    // エンジン終了（シングルトン破棄はg_Engineに委譲）
    g_Engine.Shutdown();

    LOG_INFO("[Game] Shutdown complete");
}

//----------------------------------------------------------------------------
void Game::FixedUpdate(float dt)
{
    // フレーム開始
    Memory::MemorySystem::Get().BeginFrame();  // フレームアロケータリセット
    JobSystem::Get().BeginFrame();             // ジョブカウンターリセット

    if (currentScene_) {
        currentScene_->FixedUpdate(dt);
    }

    // メインスレッドジョブを処理
    JobSystem::Get().ProcessMainThreadJobs();
}

//----------------------------------------------------------------------------
void Game::Update()
{
    // フレーム開始（従来互換用）
    Memory::MemorySystem::Get().BeginFrame();  // フレームアロケータリセット
    JobSystem::Get().BeginFrame();             // ジョブカウンターリセット

    if (currentScene_) {
        currentScene_->Update();
    }

    // メインスレッドジョブを処理
    JobSystem::Get().ProcessMainThreadJobs();
}

//----------------------------------------------------------------------------
void Game::Render()
{
    if (currentScene_) {
        // Application から補間係数を取得して描画
        float alpha = Application::Get().GetAlpha();
        currentScene_->Render(alpha);
    }
}

//----------------------------------------------------------------------------
void Game::EndFrame()
{
    // フレーム内ジョブの完了を待機
    JobSystem::Get().EndFrame();

    // メモリシステムのフレーム終了処理
    Memory::MemorySystem::Get().EndFrame();

    SceneManager::Get().ApplyPendingChange(currentScene_);
}
