//----------------------------------------------------------------------------
//! @file   deferred_context_impl.h
//! @brief  DeferredContext テンプレート実装
//!
//! @note このファイルはworld.h末尾でインクルードされる。
//!       Worldの完全な定義が必要なため、単体でインクルードしないこと。
//----------------------------------------------------------------------------
#pragma once

#include "deferred_context.h"
#include "world.h"

namespace ECS {

template<typename T, typename... Args>
DeferredContext& DeferredContext::AddComponent(Actor actor, Args&&... args) {
    if (!world_->IsAlive(actor)) {
        return *this;  // 無効なActorは無視
    }
    queue_->PushAdd<T>(actor, T(std::forward<Args>(args)...));
    return *this;
}

template<typename T>
DeferredContext& DeferredContext::RemoveComponent(Actor actor) {
    if (!world_->IsAlive(actor)) {
        return *this;
    }
    queue_->PushRemove<T>(actor);
    return *this;
}

inline DeferredContext& DeferredContext::DestroyActor(Actor actor) {
    if (!world_->IsAlive(actor)) {
        return *this;
    }
    queue_->PushDestroy(actor);
    return *this;
}

inline DeferredContext& DeferredContext::DestroyActors(const std::vector<Actor>& actors) {
    for (Actor actor : actors) {
        DestroyActor(actor);
    }
    return *this;
}

} // namespace ECS
