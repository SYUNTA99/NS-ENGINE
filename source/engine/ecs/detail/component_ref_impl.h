//----------------------------------------------------------------------------
//! @file   component_ref_impl.h
//! @brief  ComponentRef テンプレート実装
//!
//! @note このファイルはworld.h末尾でインクルードされる。
//!       Worldの完全な定義が必要なため、単体でインクルードしないこと。
//----------------------------------------------------------------------------
#pragma once

#include "component_ref.h"
#include "ecs_assert.h"
#include "world.h"

namespace ECS {

//============================================================================
// ComponentRef<T> 実装
//============================================================================

template<typename T>
void ComponentRef<T>::RefreshCache() const {
    if (!world_ || !actor_.IsValid()) {
        cached_ = nullptr;
        return;
    }

    uint32_t currentFrame = world_->GetFrameCounter();
    if (version_ != currentFrame) {
        cached_ = world_->GetComponent<T>(actor_);
        version_ = currentFrame;
    }
}

template<typename T>
T& ComponentRef<T>::Get() {
    RefreshCache();
    ECS_ASSERT(cached_ != nullptr,
        "ComponentRef::Get() called but component does not exist or actor is dead");
    return *cached_;
}

template<typename T>
T* ComponentRef<T>::TryGet() {
    RefreshCache();
    return cached_;
}

template<typename T>
const T& ComponentRef<T>::Get() const {
    RefreshCache();
    ECS_ASSERT(cached_ != nullptr,
        "ComponentRef::Get() called but component does not exist or actor is dead");
    return *cached_;
}

template<typename T>
const T* ComponentRef<T>::TryGet() const {
    RefreshCache();
    return cached_;
}

//============================================================================
// ComponentConstRef<T> 実装
//============================================================================

template<typename T>
void ComponentConstRef<T>::RefreshCache() const {
    if (!world_ || !actor_.IsValid()) {
        cached_ = nullptr;
        return;
    }

    uint32_t currentFrame = world_->GetFrameCounter();
    if (version_ != currentFrame) {
        cached_ = world_->GetComponent<T>(actor_);
        version_ = currentFrame;
    }
}

template<typename T>
const T& ComponentConstRef<T>::Get() const {
    RefreshCache();
    ECS_ASSERT(cached_ != nullptr,
        "ComponentConstRef::Get() called but component does not exist or actor is dead");
    return *cached_;
}

template<typename T>
const T* ComponentConstRef<T>::TryGet() const {
    RefreshCache();
    return cached_;
}

} // namespace ECS
