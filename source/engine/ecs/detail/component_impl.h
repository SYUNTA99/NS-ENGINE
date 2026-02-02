//----------------------------------------------------------------------------
//! @file   component_impl.h
//! @brief  OOP Component - テンプレート実装
//!
//! component.h と world.h の循環依存を解決するための実装ファイル。
//! world.h をインクルードした後にこのファイルをインクルードすること。
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/ecs/world.h"

template<typename T>
T* Component::GetECS() {
    if (!world_) return nullptr;
    return world_->GetComponent<T>(actor_);
}

template<typename T>
const T* Component::GetECS() const {
    if (!world_) return nullptr;
    return world_->GetComponent<T>(actor_);
}

template<typename T>
bool Component::HasECS() const {
    if (!world_) return false;
    return world_->HasComponent<T>(actor_);
}

inline void Component::SetEnabled(bool enabled) {
    if (enabled_ == enabled) return;

    enabled_ = enabled;
    if (enabled) {
        OnEnable();
    } else {
        OnDisable();
    }
}
