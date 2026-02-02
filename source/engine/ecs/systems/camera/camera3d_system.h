//----------------------------------------------------------------------------
//! @file   camera3d_system.h
//! @brief  ECS Camera3DSystem - 3Dカメラ更新システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/camera/camera3d_data.h"
#include "engine/ecs/components/transform/transform_components.h"

namespace ECS {

//============================================================================
//! @brief 3Dカメラ更新システム（更新システム）
//!
//! 入力: Camera3DData, LocalTransform（オプション）
//! 出力: Camera3DData.viewMatrix, Camera3DData.projectionMatrix
//!
//! LocalTransformと連携してカメラ位置・注視点を同期し、
//! ビュー・プロジェクション行列を更新する。
//!
//! @note 優先度8（LocalToWorldSystemの前）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<Camera3DSystem>();
//!
//! // 3Dカメラを作成
//! auto camera = world.CreateActor();
//! world.AddComponent<Camera3DData>(camera, 60.0f, 16.0f/9.0f);
//! world.AddComponent<LocalTransform>(camera);  // 位置・回転同期用
//! world.AddComponent<ActiveCameraTag>(camera); // アクティブマーク
//!
//! // カメラ位置と注視点はLocalTransformから自動計算される
//! @endcode
//============================================================================
class Camera3DSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // LocalTransformと連携するカメラの位置・注視点を同期
        world.ForEach<Camera3DData, LocalTransform>(
            [](Actor, Camera3DData& cam, const LocalTransform& transform) {
                cam.position = transform.position;
                // 注視点は回転から計算（前方向）
                Vector3 forward = transform.GetForward();
                cam.target = cam.position + forward;
                cam.dirty = true;
            });

        // 全カメラの行列更新（dirty時のみ）
        world.ForEach<Camera3DData>([](Actor, Camera3DData& cam) {
            cam.UpdateMatrices();
        });
    }

    int Priority() const override { return 8; }
    const char* Name() const override { return "Camera3DSystem"; }
};

} // namespace ECS
