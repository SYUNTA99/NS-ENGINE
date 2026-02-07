//----------------------------------------------------------------------------
//! @file   render_bounds_update_system.h
//! @brief  ECS RenderBoundsUpdateSystem - AABB更新システム
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/render_components.h"

namespace ECS {

//============================================================================
//! @brief AABB更新システム（更新システム）
//!
//! 入力: RenderBoundsData, LocalToWorld
//! 出力: WorldRenderBoundsData
//!
//! ローカル空間のAABB（RenderBoundsData）をワールド変換し、
//! ワールド空間のAABB（WorldRenderBoundsData）を更新する。
//! 視錐台カリングの準備として使用。
//!
//! @note 優先度12（LocalToWorldSystemの後）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<RenderBoundsUpdateSystem>();
//!
//! // カリング対応メッシュを作成
//! auto mesh = world.CreateActor();
//! world.AddComponent<LocalTransform>(mesh, position);
//! world.AddComponent<LocalToWorld>(mesh);
//! world.AddComponent<MeshData>(mesh, meshHandle);
//! world.AddComponent<RenderBoundsData>(mesh, RenderBoundsData::UnitCube());
//! world.AddComponent<WorldRenderBoundsData>(mesh);
//!
//! // WorldRenderBoundsDataが毎フレーム自動更新される
//! @endcode
//============================================================================
class RenderBoundsUpdateSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        world.ForEach<RenderBoundsData, LocalToWorld, WorldRenderBoundsData>(
            [](Actor, const RenderBoundsData& local, const LocalToWorld& ltw,
               WorldRenderBoundsData& worldBounds) {
                TransformAABB(local, ltw.value, worldBounds);
            });
    }

    int Priority() const override { return 12; }
    const char* Name() const override { return "RenderBoundsUpdateSystem"; }

private:
    //------------------------------------------------------------------------
    //! @brief ローカルAABBをワールド空間に変換
    //! @param local ローカル空間AABB
    //! @param worldMatrix ワールド変換行列
    //! @param out 出力先ワールド空間AABB
    //------------------------------------------------------------------------
    static void TransformAABB(const RenderBoundsData& local, const Matrix& worldMatrix,
                              WorldRenderBoundsData& out) {
        Vector3 center = local.center;
        Vector3 extents = local.extents;

        // ローカルAABBの8頂点を生成
        Vector3 corners[8] = {
            center + Vector3(-extents.x, -extents.y, -extents.z),
            center + Vector3(+extents.x, -extents.y, -extents.z),
            center + Vector3(-extents.x, +extents.y, -extents.z),
            center + Vector3(+extents.x, +extents.y, -extents.z),
            center + Vector3(-extents.x, -extents.y, +extents.z),
            center + Vector3(+extents.x, -extents.y, +extents.z),
            center + Vector3(-extents.x, +extents.y, +extents.z),
            center + Vector3(+extents.x, +extents.y, +extents.z),
        };

        // 最初の頂点をワールド変換して初期化
        Vector3 worldCorner = Vector3::Transform(corners[0], worldMatrix);
        out.minPoint = out.maxPoint = worldCorner;

        // 残りの頂点をワールド変換してmin/maxを更新
        for (int i = 1; i < 8; ++i) {
            worldCorner = Vector3::Transform(corners[i], worldMatrix);
            out.minPoint = Vector3::Min(out.minPoint, worldCorner);
            out.maxPoint = Vector3::Max(out.maxPoint, worldCorner);
        }
    }
};

} // namespace ECS
