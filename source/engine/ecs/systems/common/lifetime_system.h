//----------------------------------------------------------------------------
//! @file   lifetime_system.h
//! @brief  ECS LifetimeSystem - 寿命管理システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/common/lifetime_data.h"
#include <vector>

namespace ECS {

//============================================================================
//! @brief 寿命管理システム（更新システム）
//!
//! 入力: LifetimeData
//! 出力: Actor破棄
//!
//! LifetimeDataの残り時間をデクリメントし、期限切れActorを自動破棄する。
//! 弾、パーティクル、エフェクトなどの一時オブジェクト管理に使用。
//!
//! @note 優先度100（全システムの最後に実行）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<LifetimeSystem>();
//!
//! // 5秒後に消える弾を作成
//! auto bullet = world.CreateActor();
//! world.AddComponent<LocalTransform>(bullet, position);
//! world.AddComponent<VelocityData>(bullet, velocity);
//! world.AddComponent<LifetimeData>(bullet, 5.0f);
//!
//! // 5秒後に自動破棄される
//! @endcode
//============================================================================
class LifetimeSystem final : public ISystem {
public:
    LifetimeSystem() {
        expiredActors_.reserve(64);  // 初期容量を確保して再アロケーションを防ぐ
    }

    void OnUpdate(World& world, float dt) override {
        // 破棄対象を収集（ForEach中のDestroyを避ける）
        expiredActors_.clear();

        world.ForEach<LifetimeData>([this, dt](Actor actor, LifetimeData& life) {
            if (life.Tick(dt)) {
                expiredActors_.push_back(actor);
            }
        });

        // 一括破棄
        for (Actor actor : expiredActors_) {
            world.DestroyActor(actor);
        }
    }

    int Priority() const override { return 100; }
    const char* Name() const override { return "LifetimeSystem"; }

private:
    std::vector<Actor> expiredActors_;  //!< 破棄対象Actor
};

} // namespace ECS
