//----------------------------------------------------------------------------
//! @file   game_object_impl.h
//! @brief  ゲームオブジェクト - テンプレート実装
//!
//! game_object.h と world.h の循環依存を解決するための実装ファイル。
//! world.h をインクルードした後にこのファイルをインクルードすること。
//----------------------------------------------------------------------------
#pragma once

#include "game_object.h"
#include "require_component.h"
#include "engine/ecs/component.h"
#include "engine/ecs/detail/component_impl.h"
#include "engine/ecs/world.h"
#include "engine/ecs/detail/component_cache_impl.h"

//============================================================================
// Component::GetComponent<T>() 実装
// （循環参照を避けるためここで定義）
//============================================================================

template<typename T>
T* Component::GetComponent() {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponent<T>();
}

template<typename T>
const T* Component::GetComponent() const {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponent<T>();
}

template<typename T>
bool Component::HasComponent() const {
    if (!gameObject_) return false;
    return gameObject_->HasComponent<T>();
}

//============================================================================
// Component::GetComponentInChildren/Parent<T>() 実装
//============================================================================

template<typename T>
T* Component::GetComponentInChildren() {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponentInChildren<T>();
}

template<typename T>
const T* Component::GetComponentInChildren() const {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponentInChildren<T>();
}

template<typename T>
T* Component::GetComponentInParent() {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponentInParent<T>();
}

template<typename T>
const T* Component::GetComponentInParent() const {
    if (!gameObject_) return nullptr;
    return gameObject_->GetComponentInParent<T>();
}

//============================================================================
// ECSデータ操作（旧API）
//============================================================================

template<typename T, typename... Args>
void GameObject::Add(Args&&... args) {
    world_->AddComponent<T>(actor_, std::forward<Args>(args)...);
}

template<typename T>
T& GameObject::Get() {
    T* ptr = cache_.GetOrFetch<T>(world_, actor_);
    assert(ptr && "Component not found. Use Has<T>() to check first.");
    return *ptr;
}

template<typename T>
const T& GameObject::Get() const {
    T* ptr = cache_.GetOrFetch<T>(world_, actor_);
    assert(ptr && "Component not found. Use Has<T>() to check first.");
    return *ptr;
}

template<typename T>
bool GameObject::Has() const {
    return world_->HasComponent<T>(actor_);
}

template<typename T>
void GameObject::Remove() {
    world_->RemoveComponent<T>(actor_);
    cache_.Invalidate<T>();
}

//============================================================================
// ECSデータ操作（新API）
//============================================================================

template<typename T, typename... Args>
void GameObject::AddECS(Args&&... args) {
    world_->AddComponent<T>(actor_, std::forward<Args>(args)...);
}

template<typename T>
T& GameObject::GetECS() {
    T* ptr = cache_.GetOrFetch<T>(world_, actor_);
    assert(ptr && "ECS Component not found. Use HasECS<T>() to check first.");
    return *ptr;
}

template<typename T>
const T& GameObject::GetECS() const {
    T* ptr = cache_.GetOrFetch<T>(world_, actor_);
    assert(ptr && "ECS Component not found. Use HasECS<T>() to check first.");
    return *ptr;
}

template<typename T>
bool GameObject::HasECS() const {
    return world_->HasComponent<T>(actor_);
}

template<typename T>
void GameObject::RemoveECS() {
    world_->RemoveComponent<T>(actor_);
    cache_.Invalidate<T>();
}

//============================================================================
// OOPコンポーネント操作
//============================================================================

template<typename T, typename... Args>
T* GameObject::AddComponent(Args&&... args) {
    static_assert(std::is_base_of_v<Component, T>,
        "T must inherit from Component");

    std::type_index typeId = typeid(T);

    // 既に持っている場合は既存を返す
    auto it = componentIndex_.find(typeId);
    if (it != componentIndex_.end()) {
        return static_cast<T*>(components_[it->second].get());
    }

    // RequireComponent: 依存ECSコンポーネントを先に追加
    detail::AddRequiredECSComponents<T>(this);

    // RequireComponent: 依存OOPコンポーネントを先に追加
    detail::AddRequiredOOPComponents<T>(this);

    // 新規作成
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    T* rawPtr = comp.get();

    // 内部初期化
    comp->Initialize(this, actor_, world_, typeId);

    // コンテナに追加
    size_t index = components_.size();
    components_.push_back(std::move(comp));
    componentIndex_[typeId] = index;

    // ライフサイクルコールバック（Unity互換順序）
    // 1. Awake() - 即時呼び出し
    rawPtr->Awake();
    rawPtr->OnAttach();  // 後方互換

    // 2. OnEnable() - 有効状態なら呼び出し
    if (rawPtr->IsEnabled()) {
        rawPtr->OnEnable();
    }

    // 3. Start()待ちキューに登録（次のUpdate前に呼ばれる）
    world_->Container().GameObjects().RegisterForStart(rawPtr);

    return rawPtr;
}

template<typename T>
T* GameObject::GetComponent() {
    std::type_index typeId = typeid(T);
    auto it = componentIndex_.find(typeId);
    if (it == componentIndex_.end()) {
        return nullptr;
    }
    return static_cast<T*>(components_[it->second].get());
}

template<typename T>
const T* GameObject::GetComponent() const {
    std::type_index typeId = typeid(T);
    auto it = componentIndex_.find(typeId);
    if (it == componentIndex_.end()) {
        return nullptr;
    }
    return static_cast<const T*>(components_[it->second].get());
}

template<typename T>
bool GameObject::HasComponent() const {
    return componentIndex_.find(typeid(T)) != componentIndex_.end();
}

template<typename T>
void GameObject::RemoveComponent() {
    std::type_index typeId = typeid(T);
    auto it = componentIndex_.find(typeId);
    if (it == componentIndex_.end()) {
        return;
    }

    size_t index = it->second;
    Component* comp = components_[index].get();

    // ライフサイクルコールバック（Unity互換順序）
    if (comp->IsEnabled()) {
        comp->OnDisable();
    }
    comp->OnDestroy();   // Unity互換
    comp->OnDetach();    // 後方互換

    // コンテナから削除（swap-and-pop）
    if (index != components_.size() - 1) {
        // 末尾要素と交換
        std::swap(components_[index], components_.back());
        // インデックスマッピングを更新
        componentIndex_[components_[index]->GetTypeId()] = index;
    }
    components_.pop_back();
    componentIndex_.erase(it);
}

//============================================================================
// OOPコンポーネント更新
//============================================================================

inline void GameObject::UpdateComponents(float dt) {
    if (!active_) return;

    for (auto& comp : components_) {
        if (comp && comp->IsEnabled()) {
            comp->Update(dt);
        }
    }
}

inline void GameObject::FixedUpdateComponents(float dt) {
    if (!active_) return;

    for (auto& comp : components_) {
        if (comp && comp->IsEnabled()) {
            comp->FixedUpdate(dt);
        }
    }
}

inline void GameObject::LateUpdateComponents(float dt) {
    if (!active_) return;

    for (auto& comp : components_) {
        if (comp && comp->IsEnabled()) {
            comp->LateUpdate(dt);
        }
    }
}

//============================================================================
// メッセージング
//============================================================================

template<typename T>
void GameObject::SendMsg(const T& msg) {
    static_assert(std::is_base_of_v<IMessage, T>,
        "Message type must inherit from IMessage or Message<T>");

    SendMsg(static_cast<const IMessage&>(msg));
}

inline void GameObject::SendMsg(const IMessage& msg) {
    if (!active_) return;

    for (auto& comp : components_) {
        if (comp && comp->IsEnabled()) {
            comp->ReceiveMessage(msg);
        }
    }
}

template<typename T>
void GameObject::BroadcastMsg(const T& msg) {
    static_assert(std::is_base_of_v<IMessage, T>,
        "Message type must inherit from IMessage or Message<T>");

    BroadcastMsg(static_cast<const IMessage&>(msg));
}

inline void GameObject::BroadcastMsg(const IMessage& msg) {
    // 自身にメッセージを送信
    SendMsg(msg);

    // 子GameObjectにも再帰的に送信
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    auto children = hierarchy.GetChildren(actor_, *world_);

    if (children) {
        for (const auto& child : children) {
            // 子Actorに対応するGameObjectを取得してBroadcastMsg
            auto* childGO = world_->Container().GameObjects().GetByActor(child.value);
            if (childGO) {
                childGO->BroadcastMsg(msg);
            }
        }
    }
}

template<typename T>
void GameObject::SendMsgUpwards(const T& msg) {
    static_assert(std::is_base_of_v<IMessage, T>,
        "Message type must inherit from IMessage or Message<T>");

    SendMsgUpwards(static_cast<const IMessage&>(msg));
}

inline void GameObject::SendMsgUpwards(const IMessage& msg) {
    // 自身にメッセージを送信
    SendMsg(msg);

    // 親GameObjectにも送信
    auto* parent = world_->GetComponent<ECS::Parent>(actor_);
    if (parent && parent->value.IsValid()) {
        auto* parentGO = world_->Container().GameObjects().GetByActor(parent->value);
        if (parentGO) {
            parentGO->SendMsgUpwards(msg);
        }
    }
}

//============================================================================
// 階層操作
//============================================================================

inline void GameObject::SetParent(GameObject* parent) {
    auto& hierarchy = world_->Container().ECS().GetHierarchy();

    if (parent) {
        hierarchy.SetParent(actor_, parent->GetActor(), *world_);
    } else {
        hierarchy.ClearParent(actor_, *world_);
    }
}

inline GameObject* GameObject::GetParent() {
    auto* parentComp = world_->GetComponent<ECS::Parent>(actor_);
    if (!parentComp || !parentComp->value.IsValid()) {
        return nullptr;
    }
    return world_->Container().GameObjects().GetByActor(parentComp->value);
}

inline const GameObject* GameObject::GetParent() const {
    auto* parentComp = world_->GetComponent<ECS::Parent>(actor_);
    if (!parentComp || !parentComp->value.IsValid()) {
        return nullptr;
    }
    return world_->Container().GameObjects().GetByActor(parentComp->value);
}

inline size_t GameObject::GetChildCount() const {
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    return hierarchy.GetChildCount(actor_, *world_);
}

template<typename T>
T* GameObject::GetComponentInChildren() {
    static_assert(std::is_base_of_v<Component, T>,
        "T must inherit from Component");

    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    auto children = hierarchy.GetChildren(actor_, *world_);

    if (!children) {
        return nullptr;
    }

    for (const auto& child : children) {
        auto* childGO = world_->Container().GameObjects().GetByActor(child.value);
        if (!childGO) continue;

        // 子のコンポーネントを確認
        auto* comp = childGO->GetComponent<T>();
        if (comp) {
            return comp;
        }

        // 孫以下を再帰的に検索
        auto* result = childGO->GetComponentInChildren<T>();
        if (result) {
            return result;
        }
    }

    return nullptr;
}

template<typename T>
const T* GameObject::GetComponentInChildren() const {
    return const_cast<GameObject*>(this)->GetComponentInChildren<T>();
}

template<typename T>
T* GameObject::GetComponentInParent() {
    static_assert(std::is_base_of_v<Component, T>,
        "T must inherit from Component");

    auto* parentGO = GetParent();
    if (!parentGO) {
        return nullptr;
    }

    // 親のコンポーネントを確認
    auto* comp = parentGO->GetComponent<T>();
    if (comp) {
        return comp;
    }

    // 祖父母以上を再帰的に検索
    return parentGO->GetComponentInParent<T>();
}

template<typename T>
const T* GameObject::GetComponentInParent() const {
    return const_cast<GameObject*>(this)->GetComponentInParent<T>();
}

template<typename Func>
void GameObject::ForEachChild(Func&& func) {
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    auto children = hierarchy.GetChildren(actor_, *world_);

    if (!children) {
        return;
    }

    for (const auto& child : children) {
        auto* childGO = world_->Container().GameObjects().GetByActor(child.value);
        if (childGO) {
            func(childGO);
        }
    }
}

template<typename Func>
void GameObject::ForEachChild(Func&& func) const {
    auto& hierarchy = world_->Container().ECS().GetHierarchy();
    auto children = hierarchy.GetChildren(actor_, *const_cast<ECS::World*>(world_));

    if (!children) {
        return;
    }

    for (const auto& child : children) {
        auto* childGO = world_->Container().GameObjects().GetByActor(child.value);
        if (childGO) {
            func(const_cast<const GameObject*>(childGO));
        }
    }
}
