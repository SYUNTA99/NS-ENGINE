//----------------------------------------------------------------------------
//! @file   model_scene.h
//! @brief  3Dモデル表示シーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/material/material_manager.h"
#include "engine/component/camera3d.h"
#include "engine/component/transform.h"
#include "engine/component/game_object.h"
#include "engine/c_systems/mesh_batch.h"
#include "engine/platform/renderer.h"
#include "engine/input/input_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"
#include <vector>

class ModelScene : public Scene
{
public:
    void OnEnter() override
    {
        LOG_INFO("[ModelScene] Loading model...");

        const std::string modelPath = "model:/characters/pipib/ppb.pmx";
        auto result = MeshManager::Get().LoadWithMaterials(modelPath);

        if (result.success) {
            meshHandle_ = result.mesh;
            materials_ = std::move(result.materials);

            // 表情メッシュを非表示にする（インデックス21-27）
            // これらは opacity=0 の表情用サブメッシュなのでデフォルトで非表示
            // 非表示にするにはマテリアルを無効にする
            for (size_t i = 21; i <= 27 && i < materials_.size(); ++i) {
                materials_[i] = MaterialHandle{};  // 無効化して描画スキップ
            }

            LOG_INFO("[ModelScene] Model loaded! SubMeshes: " + std::to_string(materials_.size()));
        } else {
            LOG_ERROR("[ModelScene] Model load FAILED!");
            meshHandle_ = MeshManager::Get().CreateBox(Vector3(1, 1, 1));
            materials_.push_back(MaterialManager::Get().CreateDefault());
        }

        // カメラ設定
        cameraObj_ = std::make_unique<GameObject>("Camera");
        cameraObj_->AddComponent<Transform>();
        camera_ = cameraObj_->AddComponent<Camera3D>(45.0f, 16.0f / 9.0f);

        angle_ = 0.0f;
        distance_ = 5.0f;
        height_ = 2.0f;
        UpdateCameraPosition();
    }

    void OnExit() override
    {
        cameraObj_.reset();
    }

    void Update() override
    {
        auto& mouse = InputManager::Get().GetMouse();
        bool changed = false;

        // 左ドラッグ: 回転
        if (mouse.IsButtonPressed(MouseButton::Left)) {
            angle_ += static_cast<float>(mouse.GetDeltaX()) * 0.5f;  // 左右で水平回転
            height_ -= static_cast<float>(mouse.GetDeltaY()) * 0.02f;  // 上下で高さ変更
            changed = true;
        }

        // 右ドラッグ: ターゲット移動
        if (mouse.IsButtonPressed(MouseButton::Right)) {
            float dx = static_cast<float>(mouse.GetDeltaX());
            float dy = static_cast<float>(mouse.GetDeltaY());
            float rad = DirectX::XMConvertToRadians(angle_);
            targetX_ += (-std::cos(rad) * dx) * 0.01f;
            targetZ_ += (std::sin(rad) * dx) * 0.01f;
            targetY_ += dy * 0.01f;
            changed = true;
        }

        // ホイール: ズーム
        float wheel = mouse.GetWheelDelta();
        if (wheel != 0.0f) {
            distance_ -= wheel * 0.5f;
            distance_ = (std::max)(0.5f, (std::min)(50.0f, distance_));
            changed = true;
        }

        if (changed) {
            UpdateCameraPosition();
        }
    }

    void Render() override
    {
        GraphicsContext& ctx = GraphicsContext::Get();
        Renderer& renderer = Renderer::Get();

        Texture* backBuffer = renderer.GetBackBuffer();
        Texture* depthBuffer = renderer.GetDepthBuffer();
        if (!backBuffer || !depthBuffer) return;

        float width = static_cast<float>(backBuffer->Width());
        float height = static_cast<float>(backBuffer->Height());

        ctx.SetRenderTarget(backBuffer, depthBuffer);
        ctx.SetViewport(0, 0, width, height);

        float clearColor[4] = { 0.2f, 0.2f, 0.25f, 1.0f };
        ctx.ClearRenderTarget(backBuffer, clearColor);
        ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

        camera_->SetAspectRatio(width / height);

        MeshBatch& mb = MeshBatch::Get();
        mb.SetCamera(*camera_);
        mb.SetAmbientLight(Color(0.3f, 0.3f, 0.3f, 1.0f));

        Vector3 lightDir(0.5f, -1.0f, 0.5f);
        lightDir.Normalize();
        mb.AddLight(LightBuilder::Directional(lightDir, Colors::White, 1.0f));

        mb.Begin();
        mb.Draw(meshHandle_, materials_, Matrix::Identity);
        mb.End();
        mb.ClearLights();
    }

private:
    void UpdateCameraPosition()
    {
        float rad = DirectX::XMConvertToRadians(angle_);
        // 左手座標系: Z+が前方
        float x = targetX_ + std::sin(rad) * distance_;
        float z = targetZ_ + std::cos(rad) * distance_;
        camera_->SetPosition(x, height_, z);
        camera_->LookAt(Vector3(targetX_, targetY_, targetZ_));
    }

    MeshHandle meshHandle_;
    std::vector<MaterialHandle> materials_;
    std::unique_ptr<GameObject> cameraObj_;
    Camera3D* camera_ = nullptr;
    float angle_ = 0.0f;
    float distance_ = 5.0f;
    float height_ = 2.0f;
    float targetX_ = 0.0f;
    float targetY_ = 1.0f;
    float targetZ_ = 0.0f;
};

