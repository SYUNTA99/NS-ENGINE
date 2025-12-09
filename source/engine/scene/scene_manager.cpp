//----------------------------------------------------------------------------
//! @file   scene_manager.cpp
//! @brief  シーンマネージャー実装
//----------------------------------------------------------------------------
#include "scene_manager.h"

//----------------------------------------------------------------------------
SceneManager& SceneManager::Get() noexcept
{
    static SceneManager instance;
    return instance;
}

//----------------------------------------------------------------------------
void SceneManager::ApplyPendingChange(std::unique_ptr<Scene>& current)
{
    if (!pendingFactory_) {
        return;
    }

    // 現在のシーンを終了
    if (current) {
        current->OnExit();
    }

    // 新しいシーンに切り替え
    current = pendingFactory_();
    pendingFactory_ = nullptr;

    if (current) {
        current->OnEnter();
    }
}
