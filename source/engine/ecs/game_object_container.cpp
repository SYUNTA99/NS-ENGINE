//----------------------------------------------------------------------------
//! @file   game_object_container.cpp
//! @brief  GameObjectContainer - OOP GameObjectの管理
//----------------------------------------------------------------------------

#include "game_object_container.h"
#include "world.h"
#include "engine/game_object/game_object_impl.h"

namespace ECS {

//----------------------------------------------------------------------------
GameObject* GameObjectContainer::Create(const std::string& name) {
    if (!world_) {
        return nullptr;
    }

    // ECS Actorを生成
    Actor actor = world_->CreateActor();

    // GameObjectを生成
    auto go = std::make_unique<GameObject>(world_, actor, name);
    GameObject* ptr = go.get();

    // 空きスロットがあれば再利用
    if (!freeIndices_.empty()) {
        size_t index = freeIndices_.back();
        freeIndices_.pop_back();
        gameObjects_[index] = std::move(go);
        actorToIndex_[actor.id] = index;
    } else {
        size_t index = gameObjects_.size();
        gameObjects_.push_back(std::move(go));
        actorToIndex_[actor.id] = index;
    }

    return ptr;
}

//----------------------------------------------------------------------------
void GameObjectContainer::Destroy(GameObject* gameObject) {
    if (!gameObject || !world_) {
        return;
    }

    Actor actor = gameObject->GetActor();
    auto it = actorToIndex_.find(actor.id);
    if (it == actorToIndex_.end()) {
        return;
    }

    size_t index = it->second;

    // ECS Actorを破棄
    world_->DestroyActor(actor);

    // マッピングを削除
    actorToIndex_.erase(it);

    // GameObjectをnullにして空きスロットに追加
    gameObjects_[index].reset();
    freeIndices_.push_back(index);
}

//----------------------------------------------------------------------------
void GameObjectContainer::Destroy(Actor actor) {
    auto it = actorToIndex_.find(actor.id);
    if (it == actorToIndex_.end()) {
        return;
    }

    size_t index = it->second;
    GameObject* go = gameObjects_[index].get();
    if (go) {
        Destroy(go);
    }
}

//----------------------------------------------------------------------------
GameObject* GameObjectContainer::Find(const std::string& name) {
    for (auto& go : gameObjects_) {
        if (go && go->GetName() == name) {
            return go.get();
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------
GameObject* GameObjectContainer::GetByActor(Actor actor) {
    auto it = actorToIndex_.find(actor.id);
    if (it == actorToIndex_.end()) {
        return nullptr;
    }

    size_t index = it->second;
    return (index < gameObjects_.size()) ? gameObjects_[index].get() : nullptr;
}

//----------------------------------------------------------------------------
void GameObjectContainer::ProcessPendingStarts() {
    if (pendingStarts_.empty()) {
        return;
    }

    // 処理中に新しいコンポーネントが追加される可能性があるので
    // 現在のキューをローカルにコピーして処理
    std::vector<Component*> toProcess;
    toProcess.swap(pendingStarts_);

    for (Component* comp : toProcess) {
        if (comp && !comp->HasStarted() && comp->IsEnabled()) {
            comp->InvokeStart();
        }
    }
}

//----------------------------------------------------------------------------
void GameObjectContainer::UpdateAll(float dt) {
    for (auto& go : gameObjects_) {
        if (go && go->IsActive()) {
            go->UpdateComponents(dt);
        }
    }
}

//----------------------------------------------------------------------------
void GameObjectContainer::FixedUpdateAll(float dt) {
    for (auto& go : gameObjects_) {
        if (go && go->IsActive()) {
            go->FixedUpdateComponents(dt);
        }
    }
}

//----------------------------------------------------------------------------
void GameObjectContainer::Clear() {
    // 全GameObjectを破棄（ECS Actorも破棄）
    for (auto& go : gameObjects_) {
        if (go && world_) {
            world_->DestroyActor(go->GetActor());
        }
    }

    gameObjects_.clear();
    actorToIndex_.clear();
    freeIndices_.clear();
    pendingStarts_.clear();
}

} // namespace ECS
