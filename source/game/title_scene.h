//----------------------------------------------------------------------------
//! @file   title_scene.h
//! @brief  タイトルシーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/scene/scene_manager.h"
#include "engine/input/input_manager.h"
#include "engine/platform/renderer.h"
#include "engine/ecs/world.h"
#include "engine/ecs/actor.h"
#include "engine/ecs/systems/transform/transform_system.h"
#include "engine/ecs/systems/rendering/mesh_render_system.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/mesh_data.h"
#include "engine/ecs/components/camera/camera3d_data.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/material/material_manager.h"
#include "engine/graphics/mesh_batch.h"
#include "engine/graphics/sprite_batch.h"
#include "engine/texture/texture_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"
#include <cmath>

// 前方宣言
class GameScene;

//============================================================================
//! @brief タイトルシーン
//============================================================================
class TitleScene : public Scene
{
public:
    [[nodiscard]] const char* GetName() const override { return "TitleScene"; }

    void OnEnter() override
    {
        LOG_INFO("[TitleScene] Enter");

        // ECS World作成
        world_ = std::make_unique<ECS::World>();
        world_->RegisterSystem<ECS::TransformSystem>();
        world_->RegisterRenderSystem<ECS::MeshRenderSystem>();

        // ステージモデルをロード
        const std::string stagePath = "model:/stage/Meshy_AI__0116015212_texture.fbx";
        auto result = MeshManager::Get().LoadWithMaterials(stagePath);

        MeshHandle stageMesh;
        std::vector<MaterialHandle> stageMaterials;

        if (result.success) {
            stageMesh = result.mesh;
            stageMaterials = std::move(result.materials);
            LOG_INFO("[TitleScene] Stage model loaded!");
        } else {
            LOG_ERROR("[TitleScene] Stage load FAILED! Using box.");
            stageMesh = MeshManager::Get().CreateBox(Vector3(2, 2, 2));
            stageMaterials.push_back(MaterialManager::Get().CreateDefault());
        }

        // ステージActor作成
        stageActor_ = world_->CreateActor();

        auto* transform = world_->AddComponent<ECS::LocalTransform>(stageActor_);
        transform->position = Vector3(0.0f, 0.0f, 0.0f);
        // X軸で+90度回転（Y-up に変換）
        baseRotation_ = Quaternion::CreateFromAxisAngle(Vector3::Right, DirectX::XM_PIDIV2);
        transform->rotation = baseRotation_;
        transform->scale = Vector3(5.0f, 5.0f, 5.0f);  // 大きめに表示
        world_->AddComponent<ECS::LocalToWorld>(stageActor_);
        world_->AddComponent<ECS::TransformDirty>(stageActor_);

        auto* mesh = world_->AddComponent<ECS::MeshData>(stageActor_);
        mesh->mesh = stageMesh;
        mesh->SetMaterials(stageMaterials);
        mesh->visible = true;
        mesh->castShadow = true;
        mesh->receiveShadow = true;

        // カメラ作成
        cameraActor_ = world_->CreateActor();
        camera_ = world_->AddComponent<ECS::Camera3DData>(cameraActor_, 60.0f, 16.0f / 9.0f);
        camera_->SetPosition(0.0f, 10.0f, -18.0f);
        camera_->LookAt(Vector3(0, 3, 0), Vector3::Up);

        rotationAngle_ = 0.0f;

        // タイトルロゴをロード
        titleLogo_ = TextureManager::Get().Load("texture:/titlelog.png");
        if (titleLogo_) {
            LOG_INFO("[TitleScene] Title logo loaded");
        }
    }

    void OnExit() override
    {
        LOG_INFO("[TitleScene] Exit");
        world_.reset();
    }

    void FixedUpdate(float dt) override
    {
        auto& keyboard = InputManager::Get().GetKeyboard();

        // SpaceまたはEnterでゲーム開始
        if (keyboard.IsKeyPressed(Key::Space) || keyboard.IsKeyPressed(Key::Enter)) {
            LOG_INFO("[TitleScene] Starting game...");
            SceneManager::Get().Load<GameScene>();
            return;
        }

        // ステージを回転
        rotationAngle_ += dt * 30.0f;  // 30度/秒で回転
        if (rotationAngle_ >= 360.0f) {
            rotationAngle_ -= 360.0f;
        }

        if (auto* transform = world_->GetComponent<ECS::LocalTransform>(stageActor_)) {
            // Y軸回転を追加（baseRotation適用後にY軸回転）
            Quaternion yRotation = Quaternion::CreateFromAxisAngle(
                Vector3::Up, DirectX::XMConvertToRadians(rotationAngle_));
            transform->rotation = baseRotation_ * yRotation;
        }

        // TransformDirtyを設定
        if (!world_->HasComponent<ECS::TransformDirty>(stageActor_)) {
            world_->AddComponent<ECS::TransformDirty>(stageActor_);
        }

        world_->FixedUpdate(dt);
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

        // 暗めの青い背景
        float clearColor[4] = { 0.05f, 0.1f, 0.2f, 1.0f };
        ctx.ClearRenderTarget(backBuffer, clearColor);
        ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

        if (camera_) {
            camera_->aspectRatio = width / height;
            camera_->dirty = true;
        }

        MeshBatch& mb = MeshBatch::Get();

        if (camera_) {
            mb.SetViewProjection(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
        }

        // ライティング設定
        mb.SetAmbientLight(Color(0.3f, 0.3f, 0.4f, 1.0f));

        Vector3 lightDir(0.3f, -1.0f, 0.5f);
        lightDir.Normalize();
        mb.AddLight(LightBuilder::Directional(lightDir, Colors::White, 1.2f));

        // ECS描画
        world_->Render(alpha);

        mb.ClearLights();

        // タイトルロゴを描画（中央上部）
        Texture* logoTex = TextureManager::Get().Get(titleLogo_);
        if (logoTex) {
            // 深度バッファをクリア（2D UIが3Dシーンの前面に描画されるように）
            ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

            SpriteBatch& sb = SpriteBatch::Get();
            sb.Begin();

            float logoWidth = static_cast<float>(logoTex->Width());
            float logoHeight = static_cast<float>(logoTex->Height());

            // スケール調整（画面幅の60%程度に収める）
            float scale = (width * 0.6f) / logoWidth;
            float scaledWidth = logoWidth * scale;
            float scaledHeight = logoHeight * scale;
            (void)scaledHeight;  // unused

            // 中央上部に配置
            float x = (width - scaledWidth) * 0.5f;
            float y = height * 0.05f;  // 上から5%の位置

            sb.Draw(logoTex, Vector2(x, y), Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));
            sb.End();
        }
    }

private:
    std::unique_ptr<ECS::World> world_;
    ECS::Actor stageActor_;
    ECS::Actor cameraActor_;
    ECS::Camera3DData* camera_ = nullptr;
    Quaternion baseRotation_ = Quaternion::Identity;
    float rotationAngle_ = 0.0f;
    TextureHandle titleLogo_;
};
