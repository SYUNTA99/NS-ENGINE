//----------------------------------------------------------------------------
//! @file   ecs_model_scene.h
//! @brief  ECSを使った3Dモデル表示シーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/ecs/world.h"
#include "engine/ecs/actor.h"
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/components/mesh_data.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/material/material_manager.h"
#include "engine/component/transform.h"
#include "engine/component/camera3d.h"
#include "engine/component/game_object.h"
#include "engine/c_systems/mesh_batch.h"
#include "engine/platform/renderer.h"
#include "engine/input/input_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"
#include <vector>

//============================================================================
//! @brief ECSを使用した3Dモデル表示シーン
//!
//! 従来のGameObject方式ではなく、ECS方式でモデルを描画するデモ。
//! - Actor: 軽量ID
//! - MeshData: メッシュ・マテリアル情報
//! - TransformData: 位置・回転・スケール
//============================================================================
class ECSModelScene : public Scene
{
public:
    void OnEnter() override
    {
        LOG_INFO("[ECSModelScene] Initializing ECS World...");

        // 1. ECS World作成
        world_ = std::make_unique<ECS::World>();

        // 2. モデルをロード
        const std::string modelPath = "model:/characters/pipib/ppb.pmx";
        auto result = MeshManager::Get().LoadWithMaterials(modelPath);

        MeshHandle meshHandle;
        std::vector<MaterialHandle> materials;

        if (result.success) {
            meshHandle = result.mesh;
            materials = std::move(result.materials);

            // 表情メッシュを非表示（インデックス21-27）
            for (size_t i = 21; i <= 27 && i < materials.size(); ++i) {
                materials[i] = MaterialHandle{};
            }

            LOG_INFO("[ECSModelScene] Model loaded! SubMeshes: " + std::to_string(materials.size()));
        } else {
            LOG_ERROR("[ECSModelScene] Model load FAILED! Using box.");
            meshHandle = MeshManager::Get().CreateBox(Vector3(1, 1, 1));
            materials.push_back(MaterialManager::Get().CreateDefault());
        }

        // 3. メインキャラクターEntity作成
        mainCharacter_ = world_->CreateActor();

        // TransformData追加
        auto* transform = world_->AddComponent<ECS::TransformData>(mainCharacter_);
        transform->position = Vector3(0.0f, 0.0f, 0.0f);
        transform->rotation = Quaternion::Identity;
        transform->scale = Vector3(1.0f, 1.0f, 1.0f);
        transform->dirty = true;

        // MeshData追加
        auto* mesh = world_->AddComponent<ECS::MeshData>(mainCharacter_);
        mesh->mesh = meshHandle;
        mesh->materials = materials;
        mesh->visible = true;
        mesh->castShadow = true;
        mesh->receiveShadow = true;

        LOG_INFO("[ECSModelScene] Actor created: index=" + std::to_string(mainCharacter_.Index()));

        // 4. 追加のエンティティを作成（複数オブジェクトデモ）
        CreateAdditionalEntities();

        // 5. カメラ設定（従来方式 - カメラはUIなので）
        cameraObj_ = std::make_unique<GameObject>("Camera");
        cameraObj_->AddComponent<Transform>();
        camera_ = cameraObj_->AddComponent<Camera3D>(45.0f, 16.0f / 9.0f);

        angle_ = 0.0f;
        pitch_ = 15.0f;
        distance_ = 8.0f;
        UpdateCameraPosition();

        LOG_INFO("[ECSModelScene] ECS setup complete! Total entities: " +
                 std::to_string(world_->ActorCount()));
    }

    void OnExit() override
    {
        LOG_INFO("[ECSModelScene] Shutting down ECS World...");
        world_.reset();
        cameraObj_.reset();
    }

    void FixedUpdate(float dt) override
    {
        auto& mouse = InputManager::Get().GetMouse();
        auto& keyboard = InputManager::Get().GetKeyboard();
        bool changed = false;

        const bool shiftPressed = keyboard.IsShiftPressed();

        //--------------------------------------------------------------------
        // Maya/Unity スタイル カメラ操作
        //--------------------------------------------------------------------

        // Shift + 左ドラッグ: オービット（ターゲットを中心に回転）
        if (shiftPressed && mouse.IsButtonPressed(MouseButton::Left)) {
            angle_ += static_cast<float>(mouse.GetDeltaX()) * 0.5f;
            pitch_ += static_cast<float>(mouse.GetDeltaY()) * 0.5f;
            pitch_ = (std::max)(-89.0f, (std::min)(89.0f, pitch_));
            changed = true;
        }

        // Shift + 中ドラッグ: パン（ターゲットを移動）
        if (shiftPressed && mouse.IsButtonPressed(MouseButton::Middle)) {
            float dx = static_cast<float>(mouse.GetDeltaX());
            float dy = static_cast<float>(mouse.GetDeltaY());
            float rad = DirectX::XMConvertToRadians(angle_);
            float speed = distance_ * 0.002f;  // 距離に比例
            targetX_ += (-std::cos(rad) * dx + std::sin(rad) * 0.0f) * speed;
            targetZ_ += (std::sin(rad) * dx + std::cos(rad) * 0.0f) * speed;
            targetY_ += dy * speed;
            changed = true;
        }

        // Shift + 右ドラッグ: ズーム（ドリー）
        if (shiftPressed && mouse.IsButtonPressed(MouseButton::Right)) {
            float dy = static_cast<float>(mouse.GetDeltaY());
            distance_ += dy * 0.05f;
            distance_ = (std::max)(0.5f, (std::min)(100.0f, distance_));
            changed = true;
        }

        // 左ドラッグ（Shift無し）: FPSスタイル視点回転
        if (!shiftPressed && mouse.IsButtonPressed(MouseButton::Left)) {
            angle_ += static_cast<float>(mouse.GetDeltaX()) * 0.3f;
            pitch_ += static_cast<float>(mouse.GetDeltaY()) * 0.3f;
            pitch_ = (std::max)(-89.0f, (std::min)(89.0f, pitch_));
            changed = true;
        }

        // ホイール: ズーム
        float wheel = mouse.GetWheelDelta();
        if (wheel != 0.0f) {
            distance_ -= wheel * 0.5f;
            distance_ = (std::max)(0.5f, (std::min)(100.0f, distance_));
            changed = true;
        }

        //--------------------------------------------------------------------
        // WASD + QE キーボード移動
        //--------------------------------------------------------------------
        float moveSpeed = 5.0f * dt;
        if (keyboard.IsControlPressed()) {
            moveSpeed *= 3.0f;  // Ctrl押下で高速移動
        }

        float rad = DirectX::XMConvertToRadians(angle_);
        float forwardX = std::sin(rad);
        float forwardZ = std::cos(rad);
        float rightX = std::cos(rad);
        float rightZ = -std::sin(rad);

        // W/S: 前後移動
        if (keyboard.IsKeyPressed(Key::S)) {
            targetX_ += forwardX * moveSpeed;
            targetZ_ += forwardZ * moveSpeed;
            changed = true;
            LOG_DEBUG("[Camera] W pressed: target=(" + std::to_string(targetX_) + "," + std::to_string(targetZ_) + ")");
        }
        if (keyboard.IsKeyPressed(Key::W)) {
            targetX_ -= forwardX * moveSpeed;
            targetZ_ -= forwardZ * moveSpeed;
            changed = true;
        }

        // A/D: 左右移動
        if (keyboard.IsKeyPressed(Key::D)) {
            targetX_ -= rightX * moveSpeed;
            targetZ_ -= rightZ * moveSpeed;
            changed = true;
        }
        if (keyboard.IsKeyPressed(Key::A)) {
            targetX_ += rightX * moveSpeed;
            targetZ_ += rightZ * moveSpeed;
            changed = true;
        }

        // Q/E: 上下移動
        if (keyboard.IsKeyPressed(Key::Q)) {
            targetY_ -= moveSpeed;
            changed = true;
        }
        if (keyboard.IsKeyPressed(Key::E)) {
            targetY_ += moveSpeed;
            changed = true;
        }

        if (changed) {
            UpdateCameraPosition();
        }

        // ECS: 全TransformDataの行列を更新
        UpdateTransformSystem();

        // キャラクター回転アニメーション
        static float rotationTime = 0.0f;
        rotationTime += dt;

        // メインキャラクターを回転
        if (auto* transform = world_->GetComponent<ECS::TransformData>(mainCharacter_)) {
          //  transform->SetRotationZ(rotationTime * 0.5f);
        }
    }

    void Render([[maybe_unused]] float alpha) override
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

        float clearColor[4] = { 0.15f, 0.15f, 0.2f, 1.0f };
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

        // ECS: 全MeshDataエンティティを描画
        RenderMeshSystem(mb);

        mb.End();
        mb.ClearLights();
    }

private:
    //------------------------------------------------------------------------
    // ECS Systems
    //------------------------------------------------------------------------

    //! @brief TransformSystem - ダーティなTransformの行列を更新
    void UpdateTransformSystem()
    {
        world_->ForEach<ECS::TransformData>([](ECS::Actor /*e*/, ECS::TransformData& t) {
            if (!t.dirty) return;

            // ローカル行列計算
            t.localMatrix = Matrix::CreateScale(t.scale) *
                           Matrix::CreateFromQuaternion(t.rotation) *
                           Matrix::CreateTranslation(t.position);

            // 親がなければローカル = ワールド
            if (!t.parent.IsValid()) {
                t.worldMatrix = t.localMatrix;
            }
            // Note: 親子関係がある場合は親の行列を掛ける（今回は未使用）

            t.dirty = false;
        });
    }

    //! @brief MeshRenderSystem - 全MeshDataをMeshBatchで描画
    void RenderMeshSystem(MeshBatch& mb)
    {
        world_->ForEach<ECS::TransformData, ECS::MeshData>(
            [&mb](ECS::Actor /*e*/, ECS::TransformData& t, ECS::MeshData& m) {
                // ECS版Draw呼び出し
                mb.Draw(m, t);
            });
    }

    //------------------------------------------------------------------------
    // 追加エンティティ作成（デモ用）
    //------------------------------------------------------------------------
    void CreateAdditionalEntities()
    {
        // ボックスメッシュを作成
        MeshHandle boxMesh = MeshManager::Get().CreateBox(Vector3(0.5f, 0.5f, 0.5f));
        MaterialHandle defaultMat = MaterialManager::Get().CreateDefault();

        // 周囲にボックスを配置
        const int numBoxes = 8;
        for (int i = 0; i < numBoxes; ++i) {
            ECS::Actor box = world_->CreateActor();

            float angle = static_cast<float>(i) / numBoxes * DirectX::XM_2PI;
            float radius = 3.0f;

            auto* transform = world_->AddComponent<ECS::TransformData>(box);
            transform->position = Vector3(
                std::cos(angle) * radius,
                0.5f + std::sin(angle * 2.0f) * 0.5f,  // 高さに変化
                std::sin(angle) * radius
            );
            transform->rotation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, angle);
            transform->scale = Vector3(1.0f, 1.0f, 1.0f);
            transform->dirty = true;

            auto* mesh = world_->AddComponent<ECS::MeshData>(box);
            mesh->mesh = boxMesh;
            mesh->SetMaterial(defaultMat);
            mesh->visible = true;

            additionalEntities_.push_back(box);
        }

        LOG_INFO("[ECSModelScene] Created " + std::to_string(numBoxes) + " additional box entities");
    }

    //------------------------------------------------------------------------
    // カメラ操作
    //------------------------------------------------------------------------
    void UpdateCameraPosition()
    {
        float yawRad = DirectX::XMConvertToRadians(angle_);
        float pitchRad = DirectX::XMConvertToRadians(pitch_);

        // 球面座標からカメラ位置を計算
        float horizontalDist = distance_ * std::cos(pitchRad);
        float verticalOffset = distance_ * std::sin(pitchRad);

        float x = targetX_ + std::sin(yawRad) * horizontalDist;
        float y = targetY_ + verticalOffset;
        float z = targetZ_ + std::cos(yawRad) * horizontalDist;

        camera_->SetPosition(x, y, z);
        camera_->LookAt(Vector3(targetX_, targetY_, targetZ_));
    }

    //------------------------------------------------------------------------
    // メンバー変数
    //------------------------------------------------------------------------

    // ECS World
    std::unique_ptr<ECS::World> world_;

    // エンティティ参照
    ECS::Actor mainCharacter_;
    std::vector<ECS::Actor> additionalEntities_;

    // カメラ（従来方式）
    std::unique_ptr<GameObject> cameraObj_;
    Camera3D* camera_ = nullptr;

    // カメラパラメータ
    float angle_ = 0.0f;       // 水平角度（Yaw）
    float pitch_ = 15.0f;      // 垂直角度（Pitch）
    float distance_ = 8.0f;    // ターゲットからの距離
    float targetX_ = 0.0f;     // 注視点X
    float targetY_ = 1.0f;     // 注視点Y
    float targetZ_ = 0.0f;     // 注視点Z
};
