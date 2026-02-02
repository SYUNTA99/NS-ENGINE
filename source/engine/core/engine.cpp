//----------------------------------------------------------------------------
//! @file   engine.cpp
//! @brief  エンジンコア実装
//----------------------------------------------------------------------------
#include "engine.h"
#include "job_system.h"
#include "service_locator.h"
#include "engine/memory/memory_system.h"
#include "engine/input/input_manager.h"
#include "engine/fs/file_system_manager.h"
#include "engine/fs/host_file_system.h"
#include "engine/fs/path_utility.h"
#include "engine/texture/texture_manager.h"
#include "engine/shader/shader_manager.h"
#include "engine/scene/scene_manager.h"
// CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
#include "engine/graphics/sprite_batch.h"
#include "engine/graphics/mesh_batch.h"
#include "engine/graphics/render_state_manager.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/mesh/mesh_loader.h"
#include "engine/mesh/mesh_loader_assimp.h"
#include "engine/material/material_manager.h"
// LightingManager削除 - ECS::LightingSystemに移行
#include "engine/platform/renderer.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"

#ifdef _DEBUG
#include "engine/debug/debug_draw.h"
#include "engine/debug/circle_renderer.h"
#endif

//============================================================================
// グローバル変数定義
//============================================================================
Engine g_Engine;
ECS::World g_World;

//----------------------------------------------------------------------------
bool Engine::Initialize()
{
    if (initialized_) {
        LOG_WARN("[Engine] Already initialized");
        return true;
    }

    LOG_INFO("[Engine] Initializing...");

    // 0. メモリシステム初期化（最初に行う）
    Memory::MemorySystem::Get().Initialize();

    // 1. シングルトン作成
    if (!CreateSingletons()) {
        LOG_ERROR("[Engine] Failed to create singletons");
        return false;
    }

    // 2. ServiceLocator登録
    RegisterServices();

    // 3. ファイルシステムマウント
    MountFileSystems();

    // 4. サブシステム初期化
    if (!InitializeSubsystems()) {
        LOG_ERROR("[Engine] Failed to initialize subsystems");
        // ロールバック: 作成済みのシングルトンとサービスをクリーンアップ
        Services::Clear();
        FileSystemManager::Get().UnmountAll();
        DestroySingletons();
        Memory::MemorySystem::Get().Shutdown();
        return false;
    }

    initialized_ = true;
    LOG_INFO("[Engine] Initialization complete");
    return true;
}

//----------------------------------------------------------------------------
void Engine::Shutdown() noexcept
{
    if (!initialized_) {
        return;
    }

    LOG_INFO("[Engine] Shutting down...");

    // パイプラインから全リソースをアンバインド（テクスチャ解放前に必須）
    if (auto* ctx = GraphicsContext::Get().GetContext()) {
        ctx->ClearState();
        ctx->Flush();
    }

    // ECS Worldクリア
    g_World.ClearAll();

    // 逆順でシャットダウン
#ifdef _DEBUG
    CircleRenderer::Get().Shutdown();
    DebugDraw::Get().Shutdown();
#endif
    // LightingManager削除 - ECS::LightingSystemに移行
    MeshBatch::Get().Shutdown();
    SpriteBatch::Get().Shutdown();
    RenderStateManager::Get().Shutdown();
    ShaderManager::Get().Shutdown();
    MaterialManager::Get().Shutdown();
    MeshManager::Get().Shutdown();
    Renderer::Get().Shutdown();
    TextureManager::Get().Shutdown();
    FileSystemManager::Get().UnmountAll();
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行

    // ServiceLocatorをクリア
    Services::Clear();

    // シングルトン破棄
    DestroySingletons();

    // メモリシステム終了（最後に行う - 統計出力）
    Memory::MemorySystem::Get().Shutdown();

    initialized_ = false;
    LOG_INFO("[Engine] Shutdown complete");
}

//----------------------------------------------------------------------------
bool Engine::CreateSingletons()
{
    // 1. Core
    JobSystem::Create();
    InputManager::Create();
    FileSystemManager::Create();

    // 2. Graphics (D3D既に初期化済み)
    ShaderManager::Create();
    RenderStateManager::Create();

    // 3. Rendering
    SpriteBatch::Create();
    MeshBatch::Create();

    // 4. Systems
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    MeshManager::Create();
    MaterialManager::Create();
    // LightingManager削除 - ECS::LightingSystemに移行
    SceneManager::Create();

#ifdef _DEBUG
    DebugDraw::Create();
    CircleRenderer::Create();
#endif

    return true;
}

//----------------------------------------------------------------------------
void Engine::DestroySingletons() noexcept
{
    // 逆順で破棄
#ifdef _DEBUG
    CircleRenderer::Destroy();
    DebugDraw::Destroy();
#endif
    SceneManager::Destroy();
    // LightingManager削除 - ECS::LightingSystemに移行
    MaterialManager::Destroy();
    MeshManager::Destroy();
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    MeshBatch::Destroy();
    SpriteBatch::Destroy();
    RenderStateManager::Destroy();
    ShaderManager::Destroy();
    FileSystemManager::Destroy();
    InputManager::Destroy();
    JobSystem::Destroy();
}

//----------------------------------------------------------------------------
void Engine::MountFileSystems()
{
    std::wstring projectRoot = FileSystemManager::GetProjectRoot();
    std::wstring assetsRoot = FileSystemManager::GetAssetsDirectory();

    LOG_INFO("[Engine] Project root: " + PathUtility::toNarrowString(projectRoot));
    LOG_INFO("[Engine] Assets root: " + PathUtility::toNarrowString(assetsRoot));

    auto& fsManager = FileSystemManager::Get();
    fsManager.Mount("shader", std::make_unique<HostFileSystem>(assetsRoot + L"shader/"));
    fsManager.Mount("texture", std::make_unique<HostFileSystem>(assetsRoot + L"texture/"));
    fsManager.Mount("model", std::make_unique<HostFileSystem>(assetsRoot + L"model/"));
    fsManager.Mount("material", std::make_unique<HostFileSystem>(assetsRoot + L"material/"));
}

//----------------------------------------------------------------------------
void Engine::RegisterServices()
{
    Services::Provide(&JobSystem::Get());
    Services::Provide(&InputManager::Get());
    Services::Provide(&FileSystemManager::Get());
    Services::Provide(&ShaderManager::Get());
    Services::Provide(&SpriteBatch::Get());
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    Services::Provide(&SceneManager::Get());
}

//----------------------------------------------------------------------------
bool Engine::InitializeSubsystems()
{
    auto& fsManager = FileSystemManager::Get();

    // 1. CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行

    // 2. TextureManager初期化（Application層でCreate済み）
    auto* textureFs = fsManager.GetFileSystem("texture");
    TextureManager::Get().Initialize(textureFs);
    Services::Provide(&TextureManager::Get());

    // 3. ShaderManager初期化
    auto* shaderFs = fsManager.GetFileSystem("shader");
    if (shaderFs) {
        ShaderManager::Get().Initialize(shaderFs);
    }

    // 4. RenderStateManager初期化
    if (!RenderStateManager::Get().Initialize()) {
        LOG_ERROR("[Engine] RenderStateManager initialization failed");
        return false;
    }

    // 5. SpriteBatch初期化
    if (!SpriteBatch::Get().Initialize()) {
        LOG_ERROR("[Engine] SpriteBatch initialization failed");
        return false;
    }

    // 6. MeshBatch初期化
    if (!MeshBatch::Get().Initialize()) {
        LOG_ERROR("[Engine] MeshBatch initialization failed");
        return false;
    }

    // 7. MeshManager初期化
    auto* modelFs = fsManager.GetFileSystem("model");
    MeshManager::Get().Initialize(modelFs);

    // メッシュローダー登録
    MeshLoaderRegistry::Get().Register(std::make_unique<MeshLoaderAssimp>());

    // 8. MaterialManager初期化
    MaterialManager::Get().Initialize();

    // 9. LightingManager削除 - ECS::LightingSystemに移行

    LOG_INFO("[Engine] Subsystem initialization complete");
    return true;
}
