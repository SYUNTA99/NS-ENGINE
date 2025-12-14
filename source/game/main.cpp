//----------------------------------------------------------------------------
//! @file   main.cpp
//! @brief  ゲーム エントリーポイント
//----------------------------------------------------------------------------
#include "engine/platform/application.h"
#include "engine/fs/file_system_manager.h"
#include "engine/fs/path_utility.h"
#include "common/logging/logging.h"
#include "game.h"

#include <Windows.h>

//! WinMainエントリーポイント
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // アプリケーション設定
    ApplicationDesc desc;
    desc.window.title = L"HEW2026 Game";
    desc.window.width = 1280;
    desc.window.height = 720;
    desc.enableDebugLayer = true;
    desc.vsync = VSyncMode::On;

    // 初期化
    if (!Application::Get().Initialize(desc)) {
        return -1;
    }

    // ログシステム初期化
#ifdef _DEBUG
    std::wstring projectRoot = FileSystemManager::GetProjectRoot();
    std::wstring debugDir = PathUtility::normalizeW(projectRoot + L"debug");
    FileSystemManager::CreateDirectories(debugDir);
    std::wstring logPath = debugDir + L"/debug_log.txt";
    LogSystem::Initialize(logPath);
#endif

    // ゲーム初期化
    Game game;
    if (!game.Initialize()) {
        Application::Get().Shutdown();
        return -1;
    }

    // ゲーム実行
    Application::Get().Run(game);

    // 終了
    game.Shutdown();
    Application::Get().Shutdown();

    // ログシステム終了
    LogSystem::Shutdown();

    return 0;
}
