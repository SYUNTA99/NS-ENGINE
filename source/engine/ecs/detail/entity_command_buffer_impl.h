//----------------------------------------------------------------------------
//! @file   entity_command_buffer_impl.h
//! @brief  EntityCommandBuffer テンプレート/インライン実装
//!
//! @note このファイルはworld.h末尾でインクルードされる。
//!       Worldの完全な定義が必要なため、単体でインクルードしないこと。
//----------------------------------------------------------------------------
#pragma once

#include "entity_command_buffer.h"
#include "world.h"

namespace ECS {

inline void EntityCommandBuffer::Playback(World& world) {
    // ロックして全コマンドを取り出す
    std::vector<EntityCommand> localCommands;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        localCommands = std::move(commands_);
        commands_.clear();
    }

    // 各コマンドを実行
    for (auto& cmd : localCommands) {
        // 死んでいるActorへの操作はスキップ
        if (cmd.type != CommandType::DestroyActor && !world.IsAlive(cmd.actor)) {
            continue;
        }

        switch (cmd.type) {
        case CommandType::DestroyActor:
            world.DestroyActor(cmd.actor);
            break;

        case CommandType::AddComponent:
            if (cmd.applier) {
                cmd.applier(world, cmd.actor, cmd.componentData.get());
                // applier実行後、データの所有権はWorldに移ったのでデストラクタを無効化
                cmd.destructor = nullptr;
            }
            break;

        case CommandType::RemoveComponent:
            if (cmd.applier) {
                cmd.applier(world, cmd.actor, nullptr);
            }
            break;
        }
    }
}

} // namespace ECS
