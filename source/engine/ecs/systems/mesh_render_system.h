//----------------------------------------------------------------------------
//! @file   mesh_render_system.h
//! @brief  ECS MeshRenderSystem - メッシュ描画システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/components/mesh_data.h"
#include "engine/c_systems/mesh_batch.h"

namespace ECS {

//============================================================================
//! @brief メッシュ描画システム（クラスベース）
//!
//! TransformDataとMeshDataを持つエンティティをMeshBatchで描画する。
//!
//! @note 優先度10（スプライトより後）
//============================================================================
class MeshRenderSystem final : public IRenderSystem {
public:
    //------------------------------------------------------------------------
    //! @brief 描画実行
    //------------------------------------------------------------------------
    void Render(World& world, [[maybe_unused]] float alpha) override {
        auto& batch = MeshBatch::Get();

        batch.Begin();

        // TransformDataとMeshDataを持つ全エンティティを描画
        world.ForEach<TransformData, MeshData>(
            [&batch](Actor, TransformData& transform, MeshData& mesh) {
                // 非表示はスキップ
                if (!mesh.visible) {
                    return;
                }

                // 無効なメッシュはスキップ
                if (!mesh.mesh.IsValid()) {
                    return;
                }

                // ワールド行列を使用して描画
                if (mesh.materials.empty()) {
                    batch.Draw(mesh.mesh, MaterialHandle::Invalid(), transform.worldMatrix);
                } else if (mesh.materials.size() == 1) {
                    batch.Draw(mesh.mesh, mesh.materials[0], transform.worldMatrix);
                } else {
                    batch.Draw(mesh.mesh, mesh.materials, transform.worldMatrix);
                }
            });

        batch.End();
    }

    int Priority() const override { return 10; }
    const char* Name() const override { return "MeshRenderSystem"; }
};

//============================================================================
//! @brief シャドウキャスター描画システム（クラスベース）
//!
//! シャドウマップ生成用の描画システム。
//!
//! @note 優先度5
//============================================================================
class ShadowCasterRenderSystem final : public IRenderSystem {
public:
    //------------------------------------------------------------------------
    //! @brief 描画実行
    //------------------------------------------------------------------------
    void Render(World& world, [[maybe_unused]] float alpha) override {
        auto& batch = MeshBatch::Get();

        batch.Begin();

        // シャドウをキャストするMeshDataのみ
        world.ForEach<TransformData, MeshData>(
            [&batch](Actor, TransformData& transform, MeshData& mesh) {
                if (!mesh.visible || !mesh.castShadow) {
                    return;
                }

                if (!mesh.mesh.IsValid()) {
                    return;
                }

                if (mesh.materials.empty()) {
                    batch.Draw(mesh.mesh, MaterialHandle::Invalid(), transform.worldMatrix);
                } else if (mesh.materials.size() == 1) {
                    batch.Draw(mesh.mesh, mesh.materials[0], transform.worldMatrix);
                } else {
                    batch.Draw(mesh.mesh, mesh.materials, transform.worldMatrix);
                }
            });

        batch.RenderShadowPass();
    }

    int Priority() const override { return 5; }
    const char* Name() const override { return "ShadowCasterRenderSystem"; }
};

} // namespace ECS
