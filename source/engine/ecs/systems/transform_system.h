//----------------------------------------------------------------------------
//! @file   transform_system.h
//! @brief  ECS TransformSystem - トランスフォーム更新システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform_data.h"

namespace ECS {

//============================================================================
//! @brief トランスフォームシステム（クラスベース）
//!
//! ダーティフラグのあるTransformDataのワールド行列を更新する。
//! 親子関係を考慮してローカル→ワールド変換を計算。
//!
//! @note 優先度0（最初に実行）
//============================================================================
class TransformSystem final : public ISystem {
public:
    //------------------------------------------------------------------------
    //! @brief システム実行
    //------------------------------------------------------------------------
    void Execute(World& world, [[maybe_unused]] float dt) override {
        // パス1: ダーティなTransformのローカル行列を計算
        world.ForEach<TransformData>([](Actor, TransformData& t) {
            if (t.dirty) {
                ComputeLocalMatrix(t);
            }
        });

        // パス2: ワールド行列を計算（親子関係考慮）
        world.ForEach<TransformData>([&world](Actor, TransformData& t) {
            if (!t.dirty) return;

            if (t.parent.IsValid() && world.IsAlive(t.parent)) {
                auto* parentT = world.GetComponent<TransformData>(t.parent);
                if (parentT) {
                    t.worldMatrix = t.localMatrix * parentT->worldMatrix;
                } else {
                    t.worldMatrix = t.localMatrix;
                }
            } else {
                t.worldMatrix = t.localMatrix;
            }

            t.dirty = false;
        });
    }

    int Priority() const override { return 0; }
    const char* Name() const override { return "TransformSystem"; }

    //------------------------------------------------------------------------
    //! @brief ローカル行列を計算（静的ヘルパー）
    //------------------------------------------------------------------------
    static void ComputeLocalMatrix(TransformData& t) {
        // 変換順序: -pivot → scale → rotate → +pivot → translate
        Matrix pivotMat = Matrix::CreateTranslation(-t.pivot.x, -t.pivot.y, 0.0f);
        Matrix scaleMat = Matrix::CreateScale(t.scale);
        Matrix rotMat = Matrix::CreateFromQuaternion(t.rotation);
        Matrix pivotBackMat = Matrix::CreateTranslation(t.pivot.x, t.pivot.y, 0.0f);
        Matrix transMat = Matrix::CreateTranslation(t.position);

        t.localMatrix = pivotMat * scaleMat * rotMat * pivotBackMat * transMat;
    }

    //------------------------------------------------------------------------
    //! @brief 単一TransformDataのワールド行列を強制更新（静的ヘルパー）
    //------------------------------------------------------------------------
    static void UpdateSingle(World& world, TransformData& t) {
        ComputeLocalMatrix(t);

        if (t.parent.IsValid() && world.IsAlive(t.parent)) {
            auto* parentT = world.GetComponent<TransformData>(t.parent);
            if (parentT) {
                t.worldMatrix = t.localMatrix * parentT->worldMatrix;
            } else {
                t.worldMatrix = t.localMatrix;
            }
        } else {
            t.worldMatrix = t.localMatrix;
        }

        t.dirty = false;
    }
};

} // namespace ECS
