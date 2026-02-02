//----------------------------------------------------------------------------
//! @file   camera2d_system.h
//! @brief  ECS Camera2DSystem - 2Dカメラ更新システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/camera/camera2d_data.h"
#include "engine/ecs/components/transform/transform_components.h"

namespace ECS {

//============================================================================
//! @brief 2Dカメラ更新システム（更新システム）
//!
//! 入力: Camera2DData, LocalTransform（オプション）
//! 出力: Camera2DData.viewProjectionMatrix
//!
//! LocalTransformと連携してカメラ位置を同期し、
//! ビュープロジェクション行列を更新する。
//!
//! @note 優先度8（LocalToWorldSystemの前）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<Camera2DSystem>();
//!
//! // 2Dカメラを作成
//! auto camera = world.CreateActor();
//! world.AddComponent<Camera2DData>(camera, 1280.0f, 720.0f);
//! world.AddComponent<LocalTransform>(camera);  // 位置同期用
//! world.AddComponent<ActiveCameraTag>(camera); // アクティブマーク
//!
//! // カメラ位置はLocalTransformから自動同期される
//! @endcode
//============================================================================
class Camera2DSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // LocalTransformと連携するカメラの位置を同期
        world.ForEach<Camera2DData, LocalTransform>(
            [](Actor, Camera2DData& cam, const LocalTransform& transform) {
                cam.position.x = transform.position.x;
                cam.position.y = transform.position.y;
                cam.rotation = transform.GetRotationZ();
                cam.dirty = true;
            });

        // 全カメラの行列更新（dirty時のみ）
        world.ForEach<Camera2DData>([](Actor, Camera2DData& cam) {
            cam.UpdateMatrix();
        });
    }

    int Priority() const override { return 8; }
    const char* Name() const override { return "Camera2DSystem"; }
};

} // namespace ECS
