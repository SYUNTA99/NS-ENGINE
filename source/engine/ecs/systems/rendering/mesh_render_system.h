//----------------------------------------------------------------------------
//! @file   mesh_render_system.h
//! @brief  ECS MeshRenderSystem - メッシュ描画システム（Fine-grained版）
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/mesh_data.h"
#include "engine/graphics/mesh_batch.h"

namespace ECS {

//============================================================================
//! @brief メッシュ描画システム（描画システム）
//!
//! 入力: LocalToWorld, MeshData（読み取り専用）
//! 出力: GPU (MeshBatch)
//!
//! @note 優先度10（スプライトより後）
//============================================================================
class MeshRenderSystem final : public IRenderSystem {
public:
    void OnRender(World& world, [[maybe_unused]] float alpha) override {
        auto& batch = MeshBatch::Get();

        batch.Begin();

        // LocalToWorldとMeshDataを持つ全エンティティを描画
        world.ForEach<LocalToWorld, MeshData>(
            [&batch](Actor, LocalToWorld& ltw, MeshData& mesh) {
                // 非表示はスキップ
                if (!mesh.visible) {
                    return;
                }

                // 無効なメッシュはスキップ
                if (!mesh.mesh.IsValid()) {
                    return;
                }

                // マテリアルを取得して描画
                if (mesh.materialCount > 1) {
                    // 複数マテリアル
                    std::vector<MaterialHandle> mats;
                    mats.reserve(mesh.materialCount);
                    for (uint8_t i = 0; i < mesh.materialCount; ++i) {
                        mats.push_back(mesh.materials[i]);
                    }
                    batch.Draw(mesh.mesh, mats, ltw.value);
                } else {
                    // 単一マテリアル
                    batch.Draw(mesh.mesh, mesh.GetMaterial(), ltw.value);
                }
            });

        batch.End();
    }

    int Priority() const override { return 10; }
    const char* Name() const override { return "MeshRenderSystem"; }
};

//============================================================================
//! @brief シャドウ描画システム（描画システム）
//!
//! 入力: LocalToWorld, MeshData（読み取り専用）
//! 出力: GPU (Shadow Pass)
//!
//! @note 優先度5
//============================================================================
class ShadowRenderSystem final : public IRenderSystem {
public:
    void OnRender(World& world, [[maybe_unused]] float alpha) override {
        auto& batch = MeshBatch::Get();

        batch.Begin();

        // シャドウをキャストするMeshDataのみ
        world.ForEach<LocalToWorld, MeshData>(
            [&batch](Actor, LocalToWorld& ltw, MeshData& mesh) {
                if (!mesh.visible || !mesh.castShadow) {
                    return;
                }

                if (!mesh.mesh.IsValid()) {
                    return;
                }

                // マテリアルを取得して描画
                if (mesh.materialCount > 1) {
                    std::vector<MaterialHandle> mats;
                    mats.reserve(mesh.materialCount);
                    for (uint8_t i = 0; i < mesh.materialCount; ++i) {
                        mats.push_back(mesh.materials[i]);
                    }
                    batch.Draw(mesh.mesh, mats, ltw.value);
                } else {
                    batch.Draw(mesh.mesh, mesh.GetMaterial(), ltw.value);
                }
            });

        batch.RenderShadowPass();
    }

    int Priority() const override { return 5; }
    const char* Name() const override { return "ShadowRenderSystem"; }
};

} // namespace ECS
