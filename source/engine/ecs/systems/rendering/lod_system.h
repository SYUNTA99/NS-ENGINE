//----------------------------------------------------------------------------
//! @file   lod_system.h
//! @brief  ECS LODSystem - LOD切り替えシステム
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/render_components.h"
#include "engine/ecs/components/rendering/mesh_data.h"
#include "engine/ecs/components/rendering/sprite_data.h"
#include "engine/ecs/components/camera/camera2d_data.h"
#include "engine/ecs/components/camera/camera3d_data.h"
#include "engine/ecs/components/common/entity_tags.h"

namespace ECS {

//============================================================================
//! @brief LOD切り替えシステム（更新システム）
//!
//! 入力: LODRangeData, LocalToWorld, Camera2DData/Camera3DData, ActiveCameraTag
//! 出力: MeshData.visible, SpriteData.visible
//!
//! カメラからの距離に基づいてvisibleフラグを制御する。
//! LODRangeDataの範囲外のオブジェクトは非表示になる。
//!
//! @note 優先度14（RenderBoundsUpdateSystemの後）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<LODSystem>();
//!
//! // LOD対応メッシュを作成
//! auto mesh = world.CreateActor();
//! world.AddComponent<LocalTransform>(mesh, position);
//! world.AddComponent<LocalToWorld>(mesh);
//! world.AddComponent<MeshData>(mesh, meshHandle);
//! world.AddComponent<LODRangeData>(mesh, LODRangeData::Medium());  // 50-200m
//!
//! // アクティブカメラを設定
//! auto camera = world.CreateActor();
//! world.AddComponent<Camera3DData>(camera, 60.0f, 16.0f/9.0f);
//! world.AddComponent<ActiveCameraTag>(camera);
//!
//! // カメラから50-200mの範囲内のみvisible=true
//! @endcode
//============================================================================
class LODSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // 3Dアクティブカメラの位置を取得（最初のカメラのみ使用）
        Vector3 cameraPos3D = Vector3::Zero;
        bool hasCamera3D = false;
        world.ForEach<Camera3DData, ActiveCameraTag>(
            [&cameraPos3D, &hasCamera3D]([[maybe_unused]] Actor actor,
                                          const Camera3DData& cam,
                                          [[maybe_unused]] ActiveCameraTag& tag) {
                if (!hasCamera3D) {  // 最初のカメラのみ
                    cameraPos3D = cam.position;
                    hasCamera3D = true;
                }
            });

        // 2Dアクティブカメラの位置を取得（最初のカメラのみ使用）
        Vector2 cameraPos2D = Vector2::Zero;
        bool hasCamera2D = false;
        world.ForEach<Camera2DData, ActiveCameraTag>(
            [&cameraPos2D, &hasCamera2D]([[maybe_unused]] Actor actor,
                                          const Camera2DData& cam,
                                          [[maybe_unused]] ActiveCameraTag& tag) {
                if (!hasCamera2D) {  // 最初のカメラのみ
                    cameraPos2D = cam.position;
                    hasCamera2D = true;
                }
            });

        // 3Dメッシュの距離判定
        if (hasCamera3D) {
            world.ForEach<LODRangeData, LocalToWorld, MeshData>(
                [&cameraPos3D](Actor, const LODRangeData& lod, const LocalToWorld& ltw, MeshData& mesh) {
                    float distance = Vector3::Distance(ltw.GetPosition(), cameraPos3D);
                    mesh.visible = lod.IsInRange(distance);
                });
        }

        // 2Dスプライトの距離判定
        if (hasCamera2D) {
            world.ForEach<LODRangeData, LocalToWorld, SpriteData>(
                [&cameraPos2D](Actor, const LODRangeData& lod, const LocalToWorld& ltw, SpriteData& sprite) {
                    Vector2 pos2D = ltw.GetPosition2D();
                    float distance = Vector2::Distance(pos2D, cameraPos2D);
                    sprite.visible = lod.IsInRange(distance);
                });
        }
    }

    int Priority() const override { return 14; }
    const char* Name() const override { return "LODSystem"; }
};

} // namespace ECS
