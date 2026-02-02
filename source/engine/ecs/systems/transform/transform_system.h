//----------------------------------------------------------------------------
//! @file   transform_system.h
//! @brief  ECS TransformSystem - トランスフォーム更新システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include <algorithm>

namespace ECS {

//============================================================================
//! @brief トランスフォームシステム（変換システム）
//!
//! 入力: LocalTransform
//! 出力: LocalToWorld
//!
//! HierarchyDepthでソートして親→子の順で処理。
//! TransformDirtyタグを使用した増分更新をサポート。
//!
//! 対応コンポーネント:
//! - LocalTransform: TRS統合 (必須)
//! - LocalToWorld: 出力ワールド行列 (必須)
//! - PostTransformMatrix: シアー変換 (オプション)
//! - Parent: 親参照 (オプション)
//! - HierarchyDepthData: 階層深度 (オプション、ソート用)
//! - TransformDirty: 変更タグ (オプション、最適化用)
//! - StaticTransform: 静的フラグ (オプション、初回のみ計算)
//!
//! @note 優先度0（最初に実行）
//============================================================================
class TransformSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // 変更されたActorを収集（TransformDirtyタグ付き、または初回処理）
        dirtyActors_.clear();

        // TransformDirtyタグがあるActorを収集
        world.ForEach<TransformDirty, LocalToWorld>([this](Actor actor, TransformDirty&, LocalToWorld&) {
            dirtyActors_.push_back(actor);
        });

        // StaticTransformだが未初期化のActorを収集
        world.ForEach<StaticTransform, LocalToWorld>([this, &world](Actor actor, StaticTransform&, LocalToWorld&) {
            if (!world.HasComponent<TransformInitialized>(actor)) {
                dirtyActors_.push_back(actor);
            }
        });

        if (dirtyActors_.empty()) return;

        // HierarchyDepthでソート（親→子の順）
        SortByDepth(world);

        // ソート順でLocalToWorld行列を計算
        for (Actor actor : sortedActors_) {
            ComputeLocalToWorld(world, actor);
        }

        // TransformDirtyタグを削除
        for (Actor actor : dirtyActors_) {
            world.RemoveComponent<TransformDirty>(actor);
        }
    }

private:
    //------------------------------------------------------------------------
    //! @brief HierarchyDepthでソート
    //------------------------------------------------------------------------
    void SortByDepth(World& world) {
        sortedActors_ = dirtyActors_;

        std::sort(sortedActors_.begin(), sortedActors_.end(),
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

        // StaticTransformの場合は初期化済みマーク
        if (world.HasComponent<StaticTransform>(actor)) {
            world.AddComponent<TransformInitialized>(actor);
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
    int Priority() const override { return 0; }
    const char* Name() const override { return "TransformSystem"; }

private:
    std::vector<Actor> dirtyActors_;   //!< ダーティなActor
    std::vector<Actor> sortedActors_;  //!< ソート済みActor
};

} // namespace ECS
