//----------------------------------------------------------------------------
//! @file   local_to_world_system.h
//! @brief  ECS LocalToWorldSystem - ローカル→ワールド変換システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include <algorithm>

namespace ECS {

//============================================================================
//! @brief ローカル→ワールド変換システム（変換システム）
//!
//! 入力: LocalTransform（読み取り専用）
//! 出力: LocalToWorld
//!
//! 毎フレーム全エンティティを処理。HierarchyDepthでソートして親→子の順で計算。
//!
//! @note 優先度10（更新システムの後）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<LocalToWorldSystem>();
//!
//! // アクター作成
//! auto actor = world.CreateActor();
//! world.AddComponent<LocalTransform>(actor, Vector3(1, 0, 0));
//! world.AddComponent<LocalToWorld>(actor);
//!
//! // 毎フレーム自動更新
//! world.FixedUpdate(dt);
//! @endcode
//============================================================================
class LocalToWorldSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // LocalToWorldを持つ全Actorを収集
        actors_.clear();

        world.ForEach<LocalToWorld>([this](Actor actor, LocalToWorld&) {
            actors_.push_back(actor);
        });

        if (actors_.empty()) return;

        // HierarchyDepthでソート（親→子の順）
        SortByDepth(world);

        // ソート順でLocalToWorld行列を計算
        for (Actor actor : actors_) {
            ComputeLocalToWorld(world, actor);
        }
    }

private:
    //------------------------------------------------------------------------
    //! @brief HierarchyDepthでソート
    //------------------------------------------------------------------------
    void SortByDepth(World& world) {
        std::sort(actors_.begin(), actors_.end(),
            [&world](Actor a, Actor b) {
                auto* depthA = world.GetComponent<HierarchyDepthData>(a);
                auto* depthB = world.GetComponent<HierarchyDepthData>(b);
                uint16_t dA = depthA ? depthA->depth : 0;
                uint16_t dB = depthB ? depthB->depth : 0;
                return dA < dB;
            });
    }

    //------------------------------------------------------------------------
    //! @brief LocalToWorld行列を計算
    //------------------------------------------------------------------------
    void ComputeLocalToWorld(World& world, Actor actor) {
        auto* ltw = world.GetComponent<LocalToWorld>(actor);
        if (!ltw) return;

        // ローカル行列を計算
        Matrix localMatrix = ComputeLocalMatrix(world, actor);

        // PostTransformMatrix があれば適用
        auto* postTransform = world.GetComponent<PostTransformMatrix>(actor);
        if (postTransform) {
            localMatrix = localMatrix * postTransform->value;
        }

        // 親のLocalToWorldを取得して乗算
        auto* parentData = world.GetComponent<Parent>(actor);
        if (parentData && parentData->HasParent() && world.IsAlive(parentData->value)) {
            auto* parentLtw = world.GetComponent<LocalToWorld>(parentData->value);
            if (parentLtw) {
                ltw->value = localMatrix * parentLtw->value;
            } else {
                ltw->value = localMatrix;
            }
        } else {
            ltw->value = localMatrix;
        }
    }

    //------------------------------------------------------------------------
    //! @brief ローカル行列を計算
    //------------------------------------------------------------------------
    [[nodiscard]] Matrix ComputeLocalMatrix(World& world, Actor actor) const {
        auto* transform = world.GetComponent<LocalTransform>(actor);

        if (transform) {
            return transform->ToMatrix();
        }

        // LocalTransformがない場合は単位行列
        return Matrix::Identity;
    }

public:
    int Priority() const override { return 10; }
    const char* Name() const override { return "LocalToWorldSystem"; }

private:
    std::vector<Actor> actors_;   //!< 処理対象Actor
};

} // namespace ECS
