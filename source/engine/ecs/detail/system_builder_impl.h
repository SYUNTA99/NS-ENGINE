//----------------------------------------------------------------------------
//! @file   system_builder_impl.h
//! @brief  SystemBuilder::Commit() 実装
//!
//! world.h と system_builder.h の循環依存を解決するための実装ファイル。
//! world.h の末尾でインクルードすること。
//----------------------------------------------------------------------------
#pragma once

#include "system_builder.h"
#include "world.h"

namespace ECS {

//----------------------------------------------------------------------------
// SystemBuilder::Commit()
//----------------------------------------------------------------------------
template<typename T>
void SystemBuilder<T>::Commit() {
    if (!world_ || !system_) {
        return;
    }

    // SystemEntryを構築
    SystemEntry entry;
    entry.id = id_;
    entry.system = std::move(system_);
    entry.priority = priority_;
    entry.runAfter = std::move(runAfter_);
    entry.runBefore = std::move(runBefore_);
    entry.name = name_;

    // Worldに登録
    world_->CommitSystem(std::move(entry));
    world_ = nullptr;  // 二重登録防止
}

//----------------------------------------------------------------------------
// RenderSystemBuilder::Commit()
//----------------------------------------------------------------------------
template<typename T>
void RenderSystemBuilder<T>::Commit() {
    if (!world_ || !system_) {
        return;
    }

    // RenderSystemEntryを構築
    RenderSystemEntry entry;
    entry.id = id_;
    entry.system = std::move(system_);
    entry.priority = priority_;
    entry.runAfter = std::move(runAfter_);
    entry.runBefore = std::move(runBefore_);
    entry.name = name_;

    // Worldに登録
    world_->CommitRenderSystem(std::move(entry));
    world_ = nullptr;  // 二重登録防止
}

} // namespace ECS
