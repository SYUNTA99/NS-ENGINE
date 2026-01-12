//----------------------------------------------------------------------------
//! @file   world.cpp
//! @brief  ECS World - OOP互換API実装
//----------------------------------------------------------------------------
#include "world.h"
#include "engine/component/game_object.h"
#include <algorithm>

namespace ECS {

//----------------------------------------------------------------------------
// 特殊メンバー関数（unique_ptr<GameObject>のため完全型が必要）
//----------------------------------------------------------------------------
World::World() = default;
World::~World() = default;

//----------------------------------------------------------------------------
//! @brief ゲームオブジェクトを生成
//----------------------------------------------------------------------------
GameObject* World::CreateGameObject(const std::string& name) {
    auto go = std::make_unique<GameObject>(name);
    GameObject* ptr = go.get();
    gameObjects_.push_back(std::move(go));
    return ptr;
}

//----------------------------------------------------------------------------
//! @brief ゲームオブジェクトを破棄
//----------------------------------------------------------------------------
void World::DestroyGameObject(GameObject* go) {
    if (!go) return;

    auto it = std::find_if(gameObjects_.begin(), gameObjects_.end(),
        [go](const auto& ptr) { return ptr.get() == go; });

    if (it != gameObjects_.end()) {
        gameObjects_.erase(it);
    }
}

//----------------------------------------------------------------------------
//! @brief 名前でゲームオブジェクトを検索
//----------------------------------------------------------------------------
GameObject* World::FindGameObject(const std::string& name) {
    auto it = std::find_if(gameObjects_.begin(), gameObjects_.end(),
        [&name](const auto& ptr) { return ptr->GetName() == name; });

    if (it != gameObjects_.end()) {
        return it->get();
    }
    return nullptr;
}

//----------------------------------------------------------------------------
//! @brief 全アクターとコンポーネントをクリア
//----------------------------------------------------------------------------
void World::Clear() {
    for (auto& [typeIndex, storage] : storages_) {
        storage->Clear();
    }
    entities_.Clear();
    gameObjects_.clear();
    // Systemは保持
}

//----------------------------------------------------------------------------
//! @brief Systemもクリア
//----------------------------------------------------------------------------
void World::ClearAll() {
    Clear();
    systems_.clear();
    renderSystems_.clear();
}

} // namespace ECS
