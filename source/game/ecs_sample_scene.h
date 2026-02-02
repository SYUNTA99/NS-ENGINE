//----------------------------------------------------------------------------
//! @file   ecs_sample_scene.h
//! @brief  ECSシステムを使用したサンプルシーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include "engine/ecs/systems/transform_system.h"
#include "engine/ecs/systems/mesh_render_system.h"
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/components/sprite_data.h"
#include "engine/ecs/components/mesh_data.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/material/material_manager.h"
#include "engine/component/camera3d.h"
#include "engine/component/game_object.h"
#include "engine/c_systems/mesh_batch.h"
#include "engine/platform/renderer.h"
#include "engine/input/input_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"
#include <vector>

//============================================================================
// シーン固有のECSシステム
//============================================================================
namespace {

//! @brief 回転システム - 全TransformDataをY軸回転させる
class RotationSystem final : public ECS::ISystem {
public:
    void Execute(ECS::World& world, float dt) override {
        world.ForEach<ECS::TransformData>([dt](ECS::Actor, ECS::TransformData& t) {
            float rotationSpeed = 0.5f;  // rad/s
            Quaternion deltaRot = Quaternion::CreateFromAxisAngle(
                Vector3::UnitY, rotationSpeed * dt);
            t.rotation = t.rotation * deltaRot;
            t.dirty = true;
        });
    }
    int Priority() const override { return 50; }  // Transform更新後
    const char* Name() const override { return "RotationSystem"; }
};

} // namespace

//============================================================================
//! @brief ECSサンプルシーン
//!
//! ハイブリッドECSアーキテクチャのデモンストレーション。
//! - Entity/Component/System パターン
//! - GameObjectAdapter による互換API
//! - 固定タイムステップ更新
//============================================================================
class ECSSampleScene : public Scene
{
public:
    void OnEnter() override
    {
        LOG_INFO("[ECSSampleScene] Initializing ECS World...");

        // World初期化
        InitializeWorld();

        // クラスベースSystem登録
        GetWorldRef().RegisterSystem<ECS::TransformSystem>();
        GetWorldRef().RegisterSystem<RotationSystem>();
        GetWorldRef().RegisterRenderSystem<ECS::MeshRenderSystem>();

        // メッシュ読み込み
        LoadMesh();

        // ECSエンティティ作成
        CreateEntities();

        // カメラ設定
        SetupCamera();

        LOG_INFO("[ECSSampleScene] ECS World ready! Entities: " +
                 std::to_string(GetWorld()->ActorCount()));
    }

    void OnExit() override
    {
        cameraObj_.reset();
        entities_.clear();
        LOG_INFO("[ECSSampleScene] Scene cleanup complete.");
    }

    void Update() override
    {
        // 従来のUpdate（マウス入力など即時性が必要なもの）
        HandleInput();
    }

    void FixedUpdate(float dt) override
    {
        // 基底クラスを呼び出し（World::FixedUpdateを実行）
        Scene::FixedUpdate(dt);
    }

    void Render([[maybe_unused]] float alpha) override
    {
        // レンダーターゲットとカメラのセットアップ
        GraphicsContext& ctx = GraphicsContext::Get();
        Renderer& renderer = Renderer::Get();

        Texture* backBuffer = renderer.GetBackBuffer();
        Texture* depthBuffer = renderer.GetDepthBuffer();
        if (!backBuffer || !depthBuffer) return;

        float width = static_cast<float>(backBuffer->Width());
        float height = static_cast<float>(backBuffer->Height());

        ctx.SetRenderTarget(backBuffer, depthBuffer);
        ctx.SetViewport(0, 0, width, height);

        float clearColor[4] = { 0.1f, 0.15f, 0.2f, 1.0f };
        ctx.ClearRenderTarget(backBuffer, clearColor);
        ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

        if (camera_) {
            camera_->SetAspectRatio(width / height);

            MeshBatch& mb = MeshBatch::Get();
            mb.SetCamera(*camera_);
            mb.SetAmbientLight(Color(0.3f, 0.3f, 0.3f, 1.0f));

            Vector3 lightDir(0.5f, -1.0f, 0.5f);
            lightDir.Normalize();
            mb.AddLight(LightBuilder::Directional(lightDir, Colors::White, 1.0f));

            // 基底クラスを呼び出し（登録されたRenderSystemsを実行）
            Scene::Render(alpha);

            mb.ClearLights();
        }
    }

    const char* GetName() const override { return "ECSSampleScene"; }

private:
    //------------------------------------------------------------------------
    // メッシュ読み込み
    //------------------------------------------------------------------------
    void LoadMesh()
    {
        // ボックスメッシュを作成
        boxMesh_ = MeshManager::Get().CreateBox(Vector3(1.0f, 1.0f, 1.0f));
        defaultMaterial_ = MaterialManager::Get().CreateDefault();
    }

    //------------------------------------------------------------------------
    // エンティティ作成
    //------------------------------------------------------------------------
    void CreateEntities()
    {
        // 複数のボックスを配置
        const int gridSize = 3;
        const float spacing = 2.5f;

        for (int x = -gridSize; x <= gridSize; ++x) {
            for (int z = -gridSize; z <= gridSize; ++z) {
                // GameObjectAdapterを使用してエンティティ作成
                ECS::GameObjectAdapter obj = ECS::GameObjectFactory::CreateMesh(
                    GetWorldRef(),
                    boxMesh_,
                    defaultMaterial_,
                    Vector3(x * spacing, 0.0f, z * spacing),
                    "Box_" + std::to_string(x) + "_" + std::to_string(z)
                );

                entities_.push_back(obj);
            }
        }

        LOG_INFO("[ECSSampleScene] Created " + std::to_string(entities_.size()) + " entities");
    }

    //------------------------------------------------------------------------
    // カメラ設定
    //------------------------------------------------------------------------
    void SetupCamera()
    {
        cameraObj_ = std::make_unique<GameObject>("Camera");
        cameraObj_->AddComponent<Transform>();
        camera_ = cameraObj_->AddComponent<Camera3D>(45.0f, 16.0f / 9.0f);

        angle_ = 45.0f;
        distance_ = 15.0f;
        height_ = 10.0f;
        UpdateCameraPosition();
    }

    //------------------------------------------------------------------------
    // 入力処理
    //------------------------------------------------------------------------
    void HandleInput()
    {
        auto& mouse = InputManager::Get().GetMouse();
        bool changed = false;

        // 左ドラッグ: 回転
        if (mouse.IsButtonPressed(MouseButton::Left)) {
            angle_ += static_cast<float>(mouse.GetDeltaX()) * 0.5f;
            height_ -= static_cast<float>(mouse.GetDeltaY()) * 0.05f;
            height_ = (std::max)(1.0f, (std::min)(30.0f, height_));
            changed = true;
        }

        // ホイール: ズーム
        float wheel = mouse.GetWheelDelta();
        if (wheel != 0.0f) {
            distance_ -= wheel * 1.0f;
            distance_ = (std::max)(5.0f, (std::min)(50.0f, distance_));
            changed = true;
        }

        if (changed) {
            UpdateCameraPosition();
        }
    }

    //------------------------------------------------------------------------
    // カメラ位置更新
    //------------------------------------------------------------------------
    void UpdateCameraPosition()
    {
        float rad = DirectX::XMConvertToRadians(angle_);
        float x = std::sin(rad) * distance_;
        float z = std::cos(rad) * distance_;
        camera_->SetPosition(x, height_, z);
        camera_->LookAt(Vector3::Zero);
    }

    //------------------------------------------------------------------------
    // メンバ変数
    //------------------------------------------------------------------------
    std::vector<ECS::GameObjectAdapter> entities_;
    MeshHandle boxMesh_;
    MaterialHandle defaultMaterial_;

    std::unique_ptr<GameObject> cameraObj_;
    Camera3D* camera_ = nullptr;



    float angle_ = 0.0f;
    float distance_ = 15.0f;
    float height_ = 10.0f;
};
