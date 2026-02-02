//----------------------------------------------------------------------------
//! @file   component_cache_impl.h
//! @brief  ECS ComponentCache - テンプレート実装
//!
//! component_cache.h と world.h の循環依存を解決するための実装ファイル。
//! world.h をインクルードした後にこのファイルをインクルードすること。
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_cache.h"
#include "engine/ecs/world.h"

namespace ECS {

template<typename T>
T* ComponentCache::GetOrFetch(World* world, Actor actor) {
    if (!world || !actor.IsValid()) {
        return nullptr;
    }

    const uint32_t currentFrame = world->GetFrameCounter();
    size_t slot = GetTypeSlot<T>();

    // 高速パスを試す
    if (slot < kFastPathSize) {
        auto& entry = fastPath_[slot];
        if (entry.IsValid(currentFrame)) {
            // キャッシュヒット
            return static_cast<T*>(entry.ptr);
        }

        // キャッシュミス - Worldから取得して更新
        T* ptr = world->GetComponent<T>(actor);
        entry.ptr = ptr;
        entry.frame = currentFrame;
        return ptr;
    }

    // オーバーフローパス
    std::type_index typeIdx(typeid(T));
    auto it = overflow_.find(typeIdx);

    if (it != overflow_.end() && it->second.IsValid(currentFrame)) {
        // キャッシュヒット
        return static_cast<T*>(it->second.ptr);
    }

    // キャッシュミス - Worldから取得して更新
    T* ptr = world->GetComponent<T>(actor);
    overflow_[typeIdx] = CacheEntry{ ptr, currentFrame, 0 };
    return ptr;
}

} // namespace ECS
