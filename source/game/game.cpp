//----------------------------------------------------------------------------
//! @file   game.cpp
//! @brief  ゲームクラス実装
//----------------------------------------------------------------------------
#include "game.h"
#include "engine/platform/application.h"
#include "engine/fs/file_system_manager.h"
#include "engine/fs/host_file_system.h"
#include "engine/fs/path_utility.h"
#include "engine/texture/texture_manager.h"
#include "engine/shader/shader_manager.h"
#include "engine/input/input_manager.h"
#include "common/logging/logging.h"
#include "engine/scene/scene_manager.h"
#include "engine/c_systems/collision_manager.h"
#include "dx11/graphics_context.h"
#include "dx11/graphics_device.h"
#include "engine/platform/renderer.h"
#include "engine/graphics2d/render_state_manager.h"
#include "engine/c_systems/sprite_batch.h"
#include "engine/c_systems/mesh_batch.h"
#include "engine/debug/debug_draw.h"
#include "engine/debug/circle_renderer.h"
#include "engine/core/job_system.h"
#include "engine/core/service_locator.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/material/material_manager.h"
#include "engine/lighting/lighting_manager.h"
#include "engine/mesh/mesh_loader.h"
#include "engine/mesh/mesh_loader_assimp.h"
#include "model_scene.h"

//----------------------------------------------------------------------------
Game::Game() = default;

//----------------------------------------------------------------------------
bool Game::Initialize()
{
    // 0. エンジンシングルトン生成
    // Note: TextureManager, Renderer は Application層で管理
    JobSystem::Create();
    InputManager::Create();
    FileSystemManager::Create();
    ShaderManager::Create();
    RenderStateManager::Create();
    SpriteBatch::Create();
    MeshBatch::Create();
    CollisionManager::Create();
    MeshManager::Create();
    MaterialManager::Create();
    LightingManager::Create();
    SceneManager::Create();
#ifdef _DEBUG
    DebugDraw::Create();
    CircleRenderer::Create();
#endif

    // ServiceLocatorに登録
    Services::Provide(&JobSystem::Get());
    Services::Provide(&InputManager::Get());
    Services::Provide(&FileSystemManager::Get());
    Services::Provide(&ShaderManager::Get());
    Services::Provide(&SpriteBatch::Get());
    Services::Provide(&CollisionManager::Get());
    Services::Provide(&SceneManager::Get());

    std::wstring projectRoot = FileSystemManager::GetProjectRoot();
    std::wstring assetsRoot = FileSystemManager::GetAssetsDirectory();

    // 1. CollisionManager初期化
    CollisionManager::Get().Initialize(64);

    // 2. ファイルシステムマウント
    LOG_INFO("[Game] Project root: " + PathUtility::toNarrowString(projectRoot));
    LOG_INFO("[Game] Assets root: " + PathUtility::toNarrowString(assetsRoot));

    auto& fsManager = FileSystemManager::Get();
    fsManager.Mount("shader", std::make_unique<HostFileSystem>(assetsRoot + L"shader/"));
    fsManager.Mount("texture", std::make_unique<HostFileSystem>(assetsRoot + L"texture/"));
    fsManager.Mount("model", std::make_unique<HostFileSystem>(assetsRoot + L"model/"));
    fsManager.Mount("material", std::make_unique<HostFileSystem>(assetsRoot + L"material/"));

    // 3. TextureManager初期化（Application層でCreate済み）
    auto* textureFs = fsManager.GetFileSystem("texture");
    TextureManager::Get().Initialize(textureFs);
    Services::Provide(&TextureManager::Get());

    // 4. ShaderManager初期化
    auto* shaderFs = fsManager.GetFileSystem("shader");
    if (shaderFs) {
        ShaderManager::Get().Initialize(shaderFs);
    }

    // 5. RenderStateManager初期化
    if (!RenderStateManager::Get().Initialize()) {
        LOG_ERROR("[Game] RenderStateManagerの初期化に失敗");
        return false;
    }

    // 6. SpriteBatch初期化
    if (!SpriteBatch::Get().Initialize()) {
        LOG_ERROR("[Game] SpriteBatchの初期化に失敗");
        return false;
    }

    // 7. MeshBatch初期化
    if (!MeshBatch::Get().Initialize()) {
        LOG_ERROR("[Game] MeshBatchの初期化に失敗");
        return false;
    }

    // 8. MeshManager初期化
    auto* modelFs = fsManager.GetFileSystem("model");
    MeshManager::Get().Initialize(modelFs);

    // メッシュローダー登録
    MeshLoaderRegistry::Get().Register(std::make_unique<MeshLoaderAssimp>());

    // 9. MaterialManager初期化
    MaterialManager::Get().Initialize();

    // 10. LightingManager初期化
    LightingManager::Get().Initialize();

    LOG_INFO("[Game] サブシステム初期化完了");

    // 初期シーン: 3Dモデル表示
    SceneManager::Get().Load<ModelScene>();

    return true;
}

//----------------------------------------------------------------------------
void Game::Shutdown() noexcept
{
    // パイプラインから全リソースをアンバインド（テクスチャ解放前に必須）
    if (auto* ctx = GraphicsContext::Get().GetContext()) {
        ctx->ClearState();
        ctx->Flush();
    }

    if (currentScene_) {
        currentScene_->OnExit();
        currentScene_.reset();
    }

    // パイプラインからすべてのステートをアンバインド（リソース解放前に必須）
    auto* d3dCtx = GraphicsContext::Get().GetContext();
    if (d3dCtx) {
        d3dCtx->ClearState();
        d3dCtx->Flush();
    }

    // 逆順でシャットダウン
#ifdef _DEBUG
    CircleRenderer::Get().Shutdown();
    DebugDraw::Get().Shutdown();
#endif
    LightingManager::Get().Shutdown();
    MeshBatch::Get().Shutdown();
    SpriteBatch::Get().Shutdown();
    RenderStateManager::Get().Shutdown();
    ShaderManager::Get().Shutdown();
    MaterialManager::Get().Shutdown();
    MeshManager::Get().Shutdown();
    Renderer::Get().Shutdown();
    TextureManager::Get().Shutdown();
    FileSystemManager::Get().UnmountAll();
    CollisionManager::Get().Shutdown();

    // ServiceLocatorをクリア
    Services::Clear();

    // エンジンシングルトン破棄（逆順）
#ifdef _DEBUG
    CircleRenderer::Destroy();
    DebugDraw::Destroy();
#endif
    SceneManager::Destroy();
    LightingManager::Destroy();
    MaterialManager::Destroy();
    MeshManager::Destroy();
    CollisionManager::Destroy();
    MeshBatch::Destroy();
    SpriteBatch::Destroy();
    RenderStateManager::Destroy();
    ShaderManager::Destroy();
    FileSystemManager::Destroy();
    InputManager::Destroy();
    JobSystem::Destroy();

    LOG_INFO("[Game] シャットダウン完了");
}

//----------------------------------------------------------------------------
void Game::Update()
{
    // フレーム開始（ジョブカウンターリセット）
    JobSystem::Get().BeginFrame();

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
        currentScene_->Render();
    }
}

//----------------------------------------------------------------------------
void Game::EndFrame()
{
    // フレーム内ジョブの完了を待機
    JobSystem::Get().EndFrame();

    SceneManager::Get().ApplyPendingChange(currentScene_);
}
