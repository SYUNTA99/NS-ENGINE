//----------------------------------------------------------------------------
//! @file   game_scene.h
//! @brief  ゲームシーン（メインゲーム）
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/ecs/world.h"
#include "engine/ecs/actor.h"
#include "engine/ecs/systems/transform/transform_system.h"
#include "engine/ecs/systems/rendering/mesh_render_system.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/mesh_data.h"
#include "engine/ecs/components/camera/camera3d_data.h"
#include "engine/mesh/mesh_manager.h"
#include "engine/mesh/mesh_loader.h"
#include "engine/material/material_manager.h"
#include "engine/texture/texture_manager.h"
#include "engine/graphics/mesh_batch.h"
#include "engine/platform/renderer.h"
#include "engine/input/input_manager.h"
#include "engine/physics/mesh_collider.h"
#include "engine/physics/raycast.h"
#include "dx11/graphics_context.h"
#include "dx11/graphics_device.h"
#include "dx11/gpu/buffer.h"
#include "dx11/gpu/shader.h"
#include "dx11/state/blend_state.h"
#include "dx11/state/depth_stencil_state.h"
#include "dx11/compile/shader_compiler.h"
#include "common/logging/logging.h"
#include "engine/scene/scene_manager.h"
#include <vector>
#include <cmath>

// 前方宣言（循環参照回避）
class ResultScene;

//============================================================================
//! @brief デバッグライン（3D）
//============================================================================
struct DebugLine3D {
    Vector3 start;
    Vector3 end;
    Color color;
};

//============================================================================
//! @brief ゲームシーン（メインゲーム）
//============================================================================
class GameScene : public Scene
{
public:
    void OnEnter() override
    {
        LOG_INFO("[GameScene] Initializing Stage Scene...");

        // 1. ECS World作成
        world_ = std::make_unique<ECS::World>();

        // Systems登録
        world_->RegisterSystem<ECS::TransformSystem>();
        world_->RegisterRenderSystem<ECS::MeshRenderSystem>();

        // 2. ステージモデルをロード
        const std::string stagePath = "model:/stage/Meshy_AI__0116015212_texture.fbx";
        auto result = MeshManager::Get().LoadWithMaterials(stagePath);

        MeshHandle stageMesh;
        std::vector<MaterialHandle> stageMaterials;

        if (result.success) {
            stageMesh = result.mesh;
            stageMaterials = std::move(result.materials);
            LOG_INFO("[GameScene] Stage loaded! SubMeshes: " + std::to_string(stageMaterials.size()));
        } else {
            LOG_ERROR("[GameScene] Stage load FAILED! Using box.");
            stageMesh = MeshManager::Get().CreateBox(Vector3(10, 1, 10));
            stageMaterials.push_back(MaterialManager::Get().CreateDefault());
        }

        // 3. ステージEntity作成
        stageActor_ = world_->CreateActor();

        // Transform Components追加
        auto* transform = world_->AddComponent<ECS::LocalTransform>(stageActor_);
        transform->position = Vector3(0.0f, 0.0f, 0.0f);
        // X軸で+90度回転（Y-up に変換）
        stageRotation_ = Quaternion::CreateFromAxisAngle(Vector3::Right, DirectX::XM_PIDIV2);
        transform->rotation = stageRotation_;
        transform->scale = Vector3(5.0f, 5.0f, 5.0f);  // ステージを大きく
        world_->AddComponent<ECS::LocalToWorld>(stageActor_);
        world_->AddComponent<ECS::TransformDirty>(stageActor_);

        // MeshData追加
        auto* mesh = world_->AddComponent<ECS::MeshData>(stageActor_);
        mesh->mesh = stageMesh;
        mesh->SetMaterials(stageMaterials);
        mesh->visible = true;
        mesh->castShadow = true;
        mesh->receiveShadow = true;

        LOG_INFO("[GameScene] Stage Actor created: index=" + std::to_string(stageActor_.Index()));

        // 4. コリジョンメッシュ作成
        CreateStageCollider(stagePath);

        // 5. プレイヤー作成
        CreatePlayer();

        // 6. Cube追加
        CreateCube(Vector3(1.152f, 1.000f, 2.767f), Vector3(91.000f, 271.000f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));
        CreateCube(Vector3(2.269f, 1.600f, 2.334f), Vector3(90.500f, 267.500f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));
        CreateCube(Vector3(0.734f, 2.500f, 2.177f), Vector3(273.000f, 356.500f, 337.000f), Vector3(1.000f, 1.000f, 1.000f));
        CreateCube(Vector3(1.272f, 4.300f, 2.007f), Vector3(90.500f, 0.500f, 2.500f), Vector3(0.300f, 0.300f, 0.300f));

        // 7. Goal追加
        CreateGoal(Vector3(0.603f, 5.500f, 1.765f), Vector3(88.500f, 323.000f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));

        // 8. プレイヤー追従カメラ設定
        cameraActor_ = world_->CreateActor();
        camera_ = world_->AddComponent<ECS::Camera3DData>(cameraActor_, 60.0f, 16.0f / 9.0f);
        // カメラ初期位置はUpdatePlayerCameraで計算される

        LOG_INFO("[GameScene] Scene setup complete!");
    }

    void OnExit() override
    {
        LOG_INFO("[GameScene] Shutting down...");
        world_.reset();
    }

    void FixedUpdate(float dt) override
    {
        auto& mouse = InputManager::Get().GetMouse();

        // マウスでカメラ回転（左or右クリック中のみ）
        bool rotating = mouse.IsButtonPressed(MouseButton::Left) ||
                        mouse.IsButtonPressed(MouseButton::Right);

        if (rotating) {
            float dx = static_cast<float>(mouse.GetDeltaX());
            float dy = static_cast<float>(mouse.GetDeltaY());

            if (dx != 0.0f || dy != 0.0f) {
                cameraYaw_ += dx * 0.2f;
                cameraPitch_ += dy * 0.2f;  // 上下反転
                cameraPitch_ = std::clamp(cameraPitch_, -60.0f, 60.0f);
            }
        }

        // プレイヤー更新
        UpdatePlayer(dt);

        // カメラ更新
        UpdatePlayerCamera(dt);

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

        // 空色の背景
        float clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
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

        // ライティング
        mb.SetAmbientLight(Color(0.4f, 0.4f, 0.5f, 1.0f));

        Vector3 lightDir(0.3f, -1.0f, 0.5f);
        lightDir.Normalize();
        mb.AddLight(LightBuilder::Directional(lightDir, Colors::White, 1.2f));

        // ECS描画
        world_->Render(alpha);

        mb.ClearLights();

        // デバッグライン描画
        if (showDebugRays_ && !debugLines_.empty() && camera_) {
            DrawDebugLines(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
        }
    }

private:
    //------------------------------------------------------------------------
    // デバッグライン描画（dx11レイヤー使用）
    //------------------------------------------------------------------------
    void DrawDebugLines(const Matrix& view, const Matrix& projection)
    {
        if (debugLines_.empty()) return;

        GraphicsContext& ctx = GraphicsContext::Get();

        // 頂点データ構造
        struct DebugVertex {
            Vector3 position;
            Color color;
        };

        // 頂点データ作成（長すぎるラインをフィルタ）
        std::vector<DebugVertex> vertices;
        vertices.reserve(debugLines_.size() * 2);
        static int logCounter = 0;
        for (size_t idx = 0; idx < debugLines_.size(); ++idx) {
            const auto& line = debugLines_[idx];

            // NaN/Inf チェック
            if (!std::isfinite(line.start.x) || !std::isfinite(line.start.y) || !std::isfinite(line.start.z) ||
                !std::isfinite(line.end.x) || !std::isfinite(line.end.y) || !std::isfinite(line.end.z)) {
                if (logCounter++ < 10) {
                    LOG_WARN("[DebugLine] NaN/Inf detected at index " + std::to_string(idx) +
                             " start=(" + std::to_string(line.start.x) + "," + std::to_string(line.start.y) + "," + std::to_string(line.start.z) + ")" +
                             " end=(" + std::to_string(line.end.x) + "," + std::to_string(line.end.y) + "," + std::to_string(line.end.z) + ")");
                }
                continue;
            }

            // 異常に長いラインをスキップ
            float length = Vector3::Distance(line.start, line.end);
            if (length > 50.0f) {
                if (logCounter++ < 10) {
                    LOG_WARN("[DebugLine] Too long line (" + std::to_string(length) + "m) at index " + std::to_string(idx) +
                             " start=(" + std::to_string(line.start.x) + "," + std::to_string(line.start.y) + "," + std::to_string(line.start.z) + ")" +
                             " end=(" + std::to_string(line.end.x) + "," + std::to_string(line.end.y) + "," + std::to_string(line.end.z) + ")");
                }
                continue;
            }

            vertices.push_back({line.start, line.color});
            vertices.push_back({line.end, line.color});
        }

        if (vertices.empty()) return;

        // シェーダー作成（初回のみ）
        if (!debugLineVS_) {
            CreateDebugLineShaders();
        }
        if (!debugLineVS_ || !debugLinePS_) return;

        // 頂点バッファ作成（毎フレーム作り直し）
        auto vertexBuffer = Buffer::CreateVertex(
            static_cast<uint32_t>(sizeof(DebugVertex) * vertices.size()),
            sizeof(DebugVertex),
            false,
            vertices.data()
        );
        if (!vertexBuffer) return;

        // 定数バッファ更新
        Matrix viewProj = view * projection;
        ctx.UpdateConstantBuffer(debugLineCB_.get(), viewProj);

        // パイプライン設定
        ctx.SetVertexShader(debugLineVS_.get());
        ctx.SetPixelShader(debugLinePS_.get());
        ctx.SetVSConstantBuffer(0, debugLineCB_.get());
        ctx.SetInputLayout(debugLineIL_.Get());
        ctx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx.SetVertexBuffer(0, vertexBuffer.get(), sizeof(DebugVertex), 0);

        // ステート設定
        ctx.SetBlendState(debugBlendState_.get());
        ctx.SetDepthStencilState(debugDepthState_.get());

        // 描画
        ctx.Draw(static_cast<uint32_t>(vertices.size()), 0);

        // ステートを戻す
        ctx.SetBlendState(nullptr);
        ctx.SetDepthStencilState(nullptr);
    }

    //------------------------------------------------------------------------
    // デバッグラインシェーダー作成（dx11レイヤー使用）
    //------------------------------------------------------------------------
    void CreateDebugLineShaders()
    {
        auto* device = GraphicsDevice::Get().Device();
        D3DShaderCompiler compiler;

        // 頂点シェーダー（row_majorでDirectXMath行列と一致させる）
        const char* vsCode = R"(
            cbuffer CB : register(b0) { row_major matrix viewProj; };
            struct VS_IN { float3 pos : POSITION; float4 col : COLOR; };
            struct VS_OUT { float4 pos : SV_Position; float4 col : COLOR; };
            VS_OUT main(VS_IN i) {
                VS_OUT o;
                o.pos = mul(float4(i.pos, 1), viewProj);
                o.col = i.col;
                return o;
            }
        )";

        std::vector<char> vsSource(vsCode, vsCode + strlen(vsCode));
        auto vsResult = compiler.compile(vsSource, "DebugLineVS", "vs_5_0", "main");
        if (!vsResult.success) {
            LOG_ERROR("[DebugLine] VS compile failed: " + vsResult.errorMessage);
            return;
        }
        debugLineVS_ = Shader::CreateVertexShader(vsResult.bytecode);

        // 入力レイアウト
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        HRESULT hr = device->CreateInputLayout(
            layout, 2,
            debugLineVS_->Bytecode(), debugLineVS_->BytecodeSize(),
            &debugLineIL_
        );
        if (FAILED(hr)) {
            LOG_ERROR("[DebugLine] Input layout creation failed");
            return;
        }

        // ピクセルシェーダー
        const char* psCode = R"(
            struct PS_IN { float4 pos : SV_Position; float4 col : COLOR; };
            float4 main(PS_IN i) : SV_Target { return i.col; }
        )";

        std::vector<char> psSource(psCode, psCode + strlen(psCode));
        auto psResult = compiler.compile(psSource, "DebugLinePS", "ps_5_0", "main");
        if (!psResult.success) {
            LOG_ERROR("[DebugLine] PS compile failed: " + psResult.errorMessage);
            return;
        }
        debugLinePS_ = Shader::CreatePixelShader(psResult.bytecode);

        // 定数バッファ
        debugLineCB_ = Buffer::CreateConstant(sizeof(Matrix));

        // ブレンドステート（アルファブレンド）
        debugBlendState_ = BlendState::CreateAlphaBlend();

        // 深度ステート（深度テスト無効）
        debugDepthState_ = DepthStencilState::CreateDisabled();

        LOG_INFO("[DebugLine] Shaders created successfully");
    }
    //------------------------------------------------------------------------
    // マウス操作カメラ更新
    //------------------------------------------------------------------------
    void UpdatePlayerCamera(float dt)
    {
        if (!camera_) return;

        // カメラのターゲット位置（プレイヤーの腰あたり）
        Vector3 targetPos = playerPos_ + Vector3(0, 1.0f, 0);

        // マウスのYaw/Pitchからカメラ位置を計算
        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        float pitchRad = DirectX::XMConvertToRadians(cameraPitch_);

        const float distance = 5.0f;
        float cosPitch = std::cos(pitchRad);
        float sinPitch = std::sin(pitchRad);

        Vector3 offset(
            -std::sin(yawRad) * cosPitch * distance,
            sinPitch * distance,
            -std::cos(yawRad) * cosPitch * distance
        );

        Vector3 desiredCameraPos = targetPos + offset;

        // カメラ衝突判定
        if (stageCollider_) {
            Vector3 rayDir = desiredCameraPos - targetPos;
            float rayLength = rayDir.Length();
            if (rayLength > 0.1f) {
                rayDir.Normalize();
                Physics::Ray ray(targetPos, rayDir);
                Physics::RaycastHit hit;

                if (stageCollider_->Raycast(ray, rayLength, hit)) {
                    desiredCameraPos = targetPos + rayDir * (hit.distance - 0.3f);
                }
            }
        }

        // スムーズ追従
        float smoothSpeed = 12.0f * dt;
        cameraPos_ = Vector3::Lerp(cameraPos_, desiredCameraPos, (std::min)(1.0f, smoothSpeed));

        camera_->SetPosition(cameraPos_.x, cameraPos_.y, cameraPos_.z);
        camera_->LookAt(targetPos, Vector3::Up);
    }

    //------------------------------------------------------------------------
    // カメラ方向取得（移動用）
    //------------------------------------------------------------------------
    [[nodiscard]] Vector3 GetCameraForwardXZ() const
    {
        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        return Vector3(std::sin(yawRad), 0, std::cos(yawRad));
    }

    [[nodiscard]] Vector3 GetCameraRightXZ() const
    {
        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        return Vector3(std::cos(yawRad), 0, -std::sin(yawRad));
    }

    //------------------------------------------------------------------------
    // コリジョンメッシュ作成
    //------------------------------------------------------------------------
    void CreateStageCollider(const std::string& path)
    {
        // MeshLoaderRegistryで頂点データを取得
        MeshLoadResult loadResult = MeshLoaderRegistry::Get().Load(path, {});
        if (!loadResult.IsValid() || loadResult.meshDescs.empty()) {
            LOG_WARN("[GameScene] Failed to load collision mesh, using plane");
            CreateFlatPlaneCollider();
            return;
        }

        // MeshDescからBVH付きMeshColliderを作成
        const MeshDesc& desc = loadResult.meshDescs[0];
        stageCollider_ = Physics::MeshCollider::CreateFromMeshDesc(desc);

        // ステージのワールド変換を適用
        Matrix stageWorld = Matrix::CreateScale(5.0f) *
                           Matrix::CreateFromQuaternion(stageRotation_);
        stageCollider_->SetWorldMatrix(stageWorld);

        LOG_INFO("[GameScene] Created BVH mesh collider with " +
                 std::to_string(stageCollider_->GetTriangleCount()) + " triangles");
    }

    void CreateFlatPlaneCollider()
    {
        // 大きな平面を作成（Y=0の平面）
        std::vector<Vector3> positions = {
            {-500, 0, -500},
            { 500, 0, -500},
            { 500, 0,  500},
            {-500, 0,  500}
        };
        std::vector<uint32_t> indices = {
            0, 1, 2,
            0, 2, 3
        };

        stageCollider_ = Physics::MeshCollider::Create(positions, indices);
        stageCollider_->SetWorldMatrix(Matrix::Identity);
        LOG_INFO("[GameScene] Created flat plane collider");
    }

    //------------------------------------------------------------------------
    // プレイヤー作成
    //------------------------------------------------------------------------
    void CreatePlayer()
    {
        // pipibモデルをロード
        const std::string playerPath = "model:/characters/pipib/ppb.pmx";
        auto result = MeshManager::Get().LoadWithMaterials(playerPath);

        MeshHandle playerMesh;
        std::vector<MaterialHandle> playerMaterials;

        if (result.success) {
            playerMesh = result.mesh;
            playerMaterials = std::move(result.materials);
            LOG_INFO("[GameScene] Player model loaded! SubMeshes: " + std::to_string(playerMaterials.size()));
        } else {
            LOG_ERROR("[GameScene] Player model load FAILED! Using box.");
            playerMesh = MeshManager::Get().CreateBox(Vector3(0.5f, 1.0f, 0.5f));
            playerMaterials.push_back(MaterialManager::Get().CreateDefault());
        }

        playerActor_ = world_->CreateActor();

        // Transform
        auto* transform = world_->AddComponent<ECS::LocalTransform>(playerActor_);
        transform->position = Vector3(0.0f, 5.0f, 0.0f);  // 上から落下開始
        transform->rotation = Quaternion::Identity;
        transform->scale = Vector3(0.02f, 0.02f, 0.02f);  // PMXモデルは大きいのでスケール調整
        world_->AddComponent<ECS::LocalToWorld>(playerActor_);
        world_->AddComponent<ECS::TransformDirty>(playerActor_);

        // MeshData
        auto* mesh = world_->AddComponent<ECS::MeshData>(playerActor_);
        mesh->mesh = playerMesh;
        mesh->SetMaterials(playerMaterials);
        mesh->visible = true;

        // 表情関連のサブメッシュを無効化（基本表情のみ表示）
        // 21: face_anger, 23: drool, 24: eye_anger, 25: eye_happy, 26: cheek
        const std::vector<size_t> expressionIndices = {21, 23, 24, 25, 26};
        for (size_t idx : expressionIndices) {
            if (idx < mesh->GetMaterialCount()) {
                mesh->SetMaterial(idx, MaterialHandle::Invalid());
            }
        }

        // プレイヤー状態初期化
        playerPos_ = transform->position;
        playerVelocity_ = Vector3::Zero;
        isPlayerGrounded_ = false;

        LOG_INFO("[GameScene] Player created at " +
            std::to_string(playerPos_.x) + ", " +
            std::to_string(playerPos_.y) + ", " +
            std::to_string(playerPos_.z));
    }

    //------------------------------------------------------------------------
    // Cubeアセットをロード（初回のみ）
    //------------------------------------------------------------------------
    void LoadCubeAssets()
    {
        if (cubeMesh_.IsValid()) return;  // 既にロード済み

        auto cubeResult = MeshManager::Get().LoadWithMaterials("model:/cube/Meshy_AI_cube.fbx");
        if (cubeResult.success) {
            cubeMesh_ = cubeResult.mesh;
            cubeMaterials_ = std::move(cubeResult.materials);

            // テクスチャを手動で設定（cube.png）
            auto cubeTex = TextureManager::Get().Load("texture:/cube/cube.png");
            if (cubeTex) {
                for (auto& mat : cubeMaterials_) {
                    if (auto* matPtr = MaterialManager::Get().Get(mat)) {
                        matPtr->SetTexture(MaterialTextureSlot::Albedo, cubeTex);
                    }
                }
            }
            LOG_INFO("[GameScene] Cube assets loaded with texture");
        } else {
            cubeMesh_ = MeshManager::Get().CreateBox(Vector3(1, 1, 1));
            cubeMaterials_.push_back(MaterialManager::Get().CreateDefault());
        }
    }

    //------------------------------------------------------------------------
    // Cube作成（引数対応）
    //------------------------------------------------------------------------
    ECS::Actor CreateCube(const Vector3& position, const Vector3& rotationDegrees, const Vector3& scale)
    {
        LoadCubeAssets();

        ECS::Actor cubeActor = world_->CreateActor();

        // Transform
        auto* transform = world_->AddComponent<ECS::LocalTransform>(cubeActor);
        transform->position = position;
        // degrees to radians (XYZ順序 - エディターと同じ)
        float rotX = DirectX::XMConvertToRadians(rotationDegrees.x);
        float rotY = DirectX::XMConvertToRadians(rotationDegrees.y);
        float rotZ = DirectX::XMConvertToRadians(rotationDegrees.z);
        Quaternion qx = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rotX);
        Quaternion qy = Quaternion::CreateFromAxisAngle(Vector3::UnitY, rotY);
        Quaternion qz = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, rotZ);
        transform->rotation = qx * qy * qz;
        transform->scale = scale;
        world_->AddComponent<ECS::LocalToWorld>(cubeActor);
        world_->AddComponent<ECS::TransformDirty>(cubeActor);

        // MeshData
        auto* mesh = world_->AddComponent<ECS::MeshData>(cubeActor);
        mesh->mesh = cubeMesh_;
        mesh->SetMaterials(cubeMaterials_);
        mesh->visible = true;

        // 当たり判定用に位置とスケールを保存
        cubeActors_.push_back(cubeActor);
        cubePositions_.push_back(position);
        cubeScales_.push_back(scale);

        LOG_INFO("[GameScene] Cube created at (" +
            std::to_string(position.x) + ", " +
            std::to_string(position.y) + ", " +
            std::to_string(position.z) + ")");

        return cubeActor;
    }

    //------------------------------------------------------------------------
    // Goal作成
    //------------------------------------------------------------------------
    ECS::Actor CreateGoal(const Vector3& position, const Vector3& rotationDegrees, const Vector3& scale)
    {
        // Goalモデルをロード
        auto result = MeshManager::Get().LoadWithMaterials("model:/goal/goal.fbx");

        MeshHandle goalMesh;
        std::vector<MaterialHandle> goalMaterials;

        if (result.success) {
            goalMesh = result.mesh;
            goalMaterials = std::move(result.materials);

            // テクスチャを手動で設定（goal1.png〜goal4.png）
            const char* goalTexNames[] = { "goal1.png", "goal2.png", "goal3.png", "goal4.png" };
            for (size_t i = 0; i < goalMaterials.size() && i < 4; ++i) {
                std::string texPath = "texture:/goal/" + std::string(goalTexNames[i]);
                auto goalTex = TextureManager::Get().Load(texPath);
                if (goalTex) {
                    if (auto* matPtr = MaterialManager::Get().Get(goalMaterials[i])) {
                        matPtr->SetTexture(MaterialTextureSlot::Albedo, goalTex);
                    }
                }
            }
        } else {
            // フォールバック: 緑の球
            goalMesh = MeshManager::Get().CreateSphere(0.5f, 16);
            MaterialDesc matDesc;
            matDesc.params.albedoColor = Color(0.2f, 1.0f, 0.3f, 1.0f);
            goalMaterials.push_back(MaterialManager::Get().Create(matDesc));
        }

        ECS::Actor goalActor = world_->CreateActor();

        // Transform
        auto* transform = world_->AddComponent<ECS::LocalTransform>(goalActor);
        transform->position = position;
        float rotX = DirectX::XMConvertToRadians(rotationDegrees.x);
        float rotY = DirectX::XMConvertToRadians(rotationDegrees.y);
        float rotZ = DirectX::XMConvertToRadians(rotationDegrees.z);
        Quaternion qx = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rotX);
        Quaternion qy = Quaternion::CreateFromAxisAngle(Vector3::UnitY, rotY);
        Quaternion qz = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, rotZ);
        transform->rotation = qx * qy * qz;
        transform->scale = scale;
        world_->AddComponent<ECS::LocalToWorld>(goalActor);
        world_->AddComponent<ECS::TransformDirty>(goalActor);

        // MeshData
        auto* mesh = world_->AddComponent<ECS::MeshData>(goalActor);
        mesh->mesh = goalMesh;
        mesh->SetMaterials(goalMaterials);
        mesh->visible = true;

        goalActor_ = goalActor;
        goalPos_ = position;

        LOG_INFO("[GameScene] Goal created at (" +
            std::to_string(position.x) + ", " +
            std::to_string(position.y) + ", " +
            std::to_string(position.z) + ")");

        return goalActor;
    }

    //------------------------------------------------------------------------
    // プレイヤー更新（スムーズ移動＆ジャンプ）
    //------------------------------------------------------------------------
    void UpdatePlayer(float dt)
    {
        auto& keyboard = InputManager::Get().GetKeyboard();

        //====================================================================
        // 入力取得
        //====================================================================
        float inputForward = 0.0f;
        float inputRight = 0.0f;
        if (keyboard.IsKeyPressed(Key::Up) || keyboard.IsKeyPressed(Key::W))    inputForward += 1.0f;
        if (keyboard.IsKeyPressed(Key::Down) || keyboard.IsKeyPressed(Key::S))  inputForward -= 1.0f;
        if (keyboard.IsKeyPressed(Key::Left) || keyboard.IsKeyPressed(Key::A))  inputRight -= 1.0f;
        if (keyboard.IsKeyPressed(Key::Right) || keyboard.IsKeyPressed(Key::D)) inputRight += 1.0f;

        // カメラ方向を基準に移動
        Vector3 forward = GetCameraForwardXZ();
        Vector3 right = GetCameraRightXZ();

        //====================================================================
        // スムーズ移動（加速度ベース）
        //====================================================================
        const float maxSpeed = 3.5f;        // 最大速度
        const float acceleration = 25.0f;   // 加速度
        const float deceleration = 20.0f;   // 減速度（地上）
        const float airDeceleration = 3.0f; // 減速度（空中）

        bool hasInput = std::abs(inputForward) > 0.01f || std::abs(inputRight) > 0.01f;
        Vector3 targetVelocityXZ = Vector3::Zero;

        if (hasInput) {
            Vector3 moveDir = forward * inputForward + right * inputRight;
            moveDir.Normalize();
            targetVelocityXZ = moveDir * maxSpeed;

            // スムーズな回転（Lerp）
            if (auto* transform = world_->GetComponent<ECS::LocalTransform>(playerActor_)) {
                float targetYaw = std::atan2(moveDir.x, moveDir.z) + DirectX::XM_PI;  // 180度反転

                // 現在の回転からYawを抽出
                Vector3 currentForward = Vector3::Transform(LH::Forward(), transform->rotation);
                float currentYaw = std::atan2(currentForward.x, currentForward.z);

                // 角度差を-π～πに正規化
                float yawDiff = targetYaw - currentYaw;
                while (yawDiff > DirectX::XM_PI) yawDiff -= DirectX::XM_2PI;
                while (yawDiff < -DirectX::XM_PI) yawDiff += DirectX::XM_2PI;

                // スムーズ回転（回転速度: 10 rad/s）
                float rotationSpeed = 10.0f;
                float maxRotation = rotationSpeed * dt;
                float newYaw = currentYaw + std::clamp(yawDiff, -maxRotation, maxRotation);

                transform->rotation = Quaternion::CreateFromYawPitchRoll(newYaw, 0, 0);
            }
        }

        // 加速度による速度更新
        float accelRate = hasInput ? acceleration : (isPlayerGrounded_ ? deceleration : airDeceleration);

        // X方向
        if (std::abs(targetVelocityXZ.x - playerVelocity_.x) < accelRate * dt) {
            playerVelocity_.x = targetVelocityXZ.x;
        } else {
            playerVelocity_.x += std::copysign(accelRate * dt, targetVelocityXZ.x - playerVelocity_.x);
        }

        // Z方向
        if (std::abs(targetVelocityXZ.z - playerVelocity_.z) < accelRate * dt) {
            playerVelocity_.z = targetVelocityXZ.z;
        } else {
            playerVelocity_.z += std::copysign(accelRate * dt, targetVelocityXZ.z - playerVelocity_.z);
        }

        //====================================================================
        // シンプルジャンプ
        //====================================================================
        const float jumpForce = 6.0f;
        const float gravity = 18.0f;

        // スペースキーでジャンプ（押している間受付）
        if (keyboard.IsKeyPressed(Key::Space) && isPlayerGrounded_) {
            playerVelocity_.y = jumpForce;
            isPlayerGrounded_ = false;
        }

        // 重力
        if (!isPlayerGrounded_) {
            playerVelocity_.y -= gravity * dt;
        }

        // 落下速度制限（ふわっと落下）
        const float maxFallSpeed = -12.0f;
        playerVelocity_.y = (std::max)(playerVelocity_.y, maxFallSpeed);

        // 位置更新
        playerPos_ += playerVelocity_ * dt;

        // デバッグライン クリア
        debugLines_.clear();

        // プレイヤー位置マーカー（白い十字）
        if (showDebugRays_) {
            Color markerColor(1.0f, 1.0f, 1.0f, 1.0f);
            float markerSize = 0.5f;
            Vector3 p = playerPos_ + Vector3(0, 0.5f, 0);
            debugLines_.push_back({p - Vector3(markerSize, 0, 0), p + Vector3(markerSize, 0, 0), markerColor});
            debugLines_.push_back({p - Vector3(0, 0, markerSize), p + Vector3(0, 0, markerSize), markerColor});
            debugLines_.push_back({p - Vector3(0, markerSize, 0), p + Vector3(0, markerSize, 0), markerColor});
        }

        // コリジョンパラメータ（ぎりぎりまで近づける設定）
        const float playerRadius = 0.15f;  // プレイヤーの当たり判定半径（小さめ）
        const float skinWidth = 0.02f;     // 壁からの最小距離（薄め）

        // 壁との衝突判定（BVHで高速化）
        if (stageCollider_) {
            const float wallCheckDist = playerRadius + 0.15f;  // 0.3m先までチェック

            // 4方向チェック（基本方向のみ、斜めは省略して軽量化）
            const Vector3 directions[4] = {
                Vector3(1, 0, 0), Vector3(-1, 0, 0),
                Vector3(0, 0, 1), Vector3(0, 0, -1)
            };

            // 複数の高さでチェック（足元、腰）- タイトに
            const float heights[2] = { 0.1f, 0.4f };

            // 壁レイの色（赤）
            Color wallHitColor(1.0f, 0.0f, 0.0f, 1.0f);
            Color wallPushColor(1.0f, 0.5f, 0.0f, 1.0f);  // 押し戻し発生時はオレンジ

            for (float h : heights) {
                Vector3 rayOrigin = playerPos_ + Vector3(0, h, 0);
                for (const auto& dir : directions) {
                    Physics::Ray wallRay(rayOrigin, dir);
                    Physics::RaycastHit wallHit;

                    // 長めにレイキャスト（スケール考慮）
                    if (stageCollider_->Raycast(wallRay, wallCheckDist * 10.0f, wallHit)) {
                        // ワールド空間の距離でチェック
                        if (wallHit.distance <= wallCheckDist) {
                            // エンドポイントを方向×距離で計算（wallHit.pointは使わない）
                            Vector3 endPos = rayOrigin + dir * wallHit.distance;

                            // 壁に近すぎる場合は押し戻す
                            float penetration = playerRadius + skinWidth - wallHit.distance;
                            if (penetration > 0) {
                                playerPos_ -= dir * penetration;
                                // デバッグライン（押し戻し発生 = オレンジ）
                                if (showDebugRays_) {
                                    debugLines_.push_back({rayOrigin, endPos, wallPushColor});
                                }
                            } else {
                                // デバッグライン（ヒットしたが押し戻しなし = 赤）
                                if (showDebugRays_) {
                                    debugLines_.push_back({rayOrigin, endPos, wallHitColor});
                                }
                            }
                        }
                    }
                    // 遠すぎるヒット or ミスは表示しない
                }
            }
        }

        // Cubeとの当たり判定
        // Cubeモデルは90度X回転で配置されるため、薄い足場として扱う
        bool onCube = false;
        for (size_t i = 0; i < cubePositions_.size(); ++i) {
            const Vector3& cubePos = cubePositions_[i];
            const Vector3& cubeScale = cubeScales_[i];

            // 薄いAABB（足場）
            float halfW = cubeScale.x * 0.5f;  // XZ方向の半サイズ
            float halfH = 0.1f;                // 高さは薄く固定（0.1m）

            // AABB判定
            float minX = cubePos.x - halfW;
            float maxX = cubePos.x + halfW;
            float minZ = cubePos.z - halfW;
            float maxZ = cubePos.z + halfW;
            float cubeTop = cubePos.y + halfH;
            float cubeBottom = cubePos.y - halfH;

            // XZ範囲内チェック
            bool inXZ = playerPos_.x >= minX - playerRadius && playerPos_.x <= maxX + playerRadius &&
                        playerPos_.z >= minZ - playerRadius && playerPos_.z <= maxZ + playerRadius;

            if (inXZ) {
                float playerFeet = playerPos_.y;

                // 上から乗る判定（足元がcubeTop付近で落下中）
                if (playerFeet >= cubeTop - 0.3f && playerFeet <= cubeTop + 0.5f && playerVelocity_.y <= 0.1f) {
                    // XZ中心からの距離チェック（完全に乗っている場合のみ）
                    if (playerPos_.x >= minX && playerPos_.x <= maxX &&
                        playerPos_.z >= minZ && playerPos_.z <= maxZ) {
                        playerPos_.y = cubeTop;
                        playerVelocity_.y = 0;
                        isPlayerGrounded_ = true;
                        onCube = true;
                    }
                }
                // 横から押し出し（乗っていない場合）
                else if (!onCube && playerFeet < cubeTop - 0.1f && playerFeet + 1.0f > cubeBottom) {
                    // 最も近い面から押し出す
                    float overlapX = (std::min)(maxX + playerRadius - playerPos_.x, playerPos_.x - (minX - playerRadius));
                    float overlapZ = (std::min)(maxZ + playerRadius - playerPos_.z, playerPos_.z - (minZ - playerRadius));

                    if (overlapX < overlapZ) {
                        // X方向に押し出す
                        if (playerPos_.x < cubePos.x) {
                            playerPos_.x = minX - playerRadius;
                        } else {
                            playerPos_.x = maxX + playerRadius;
                        }
                    } else {
                        // Z方向に押し出す
                        if (playerPos_.z < cubePos.z) {
                            playerPos_.z = minZ - playerRadius;
                        } else {
                            playerPos_.z = maxZ + playerRadius;
                        }
                    }
                }
            }
        }

        // 地面レイキャスト（Cube上にいない場合のみ通常の地面判定）
        if (stageCollider_ && !onCube) {
            // プレイヤー足元より少し上からレイを飛ばす
            const float groundRayOffset = 0.15f;  // タイトに
            Vector3 rayOrigin = playerPos_ + Vector3(0, groundRayOffset, 0);
            Physics::Ray ray(rayOrigin, Vector3(0, -1, 0));
            Physics::RaycastHit hit;

            // 床レイの色（緑）
            Color groundHitColor(0.0f, 1.0f, 0.0f, 1.0f);
            Color groundMissColor(0.0f, 0.5f, 0.0f, 0.8f);

            // 長距離でレイキャスト
            const float groundRayLength = 3.0f;  // デバッグ表示用の最大長さ
            if (stageCollider_->Raycast(ray, 100.0f, hit)) {
                // デバッグライン（ヒット地点まで、最大3m）
                if (showDebugRays_) {
                    float drawDist = (std::min)(hit.distance, groundRayLength);
                    debugLines_.push_back({rayOrigin, rayOrigin + Vector3(0, -drawDist, 0), groundHitColor});
                }

                // 地面との距離（レイ開始点からの距離 - オフセット = 足元からの距離）
                float feetToGround = hit.distance - groundRayOffset;

                // 着地判定（緩めに設定）
                const float groundCheckDist = 0.3f;  // この距離以内なら接地

                if (feetToGround <= groundCheckDist) {
                    // 地面に近い
                    if (playerVelocity_.y <= 0.1f) {
                        // 落下中または静止中は地面に吸着
                        playerPos_.y = hit.point.y;
                        playerVelocity_.y = 0;
                    }
                    isPlayerGrounded_ = true;
                } else if (feetToGround < 0) {
                    // 地面にめり込んでいる場合
                    playerPos_.y = hit.point.y;
                    playerVelocity_.y = 0;
                    isPlayerGrounded_ = true;
                } else {
                    isPlayerGrounded_ = false;
                }
            } else {
                // デバッグライン（ヒットなし = 暗い緑で短く表示）
                if (showDebugRays_) {
                    debugLines_.push_back({rayOrigin, rayOrigin + Vector3(0, -groundRayLength, 0), groundMissColor});
                }
                isPlayerGrounded_ = false;
            }
        }

        // 穴に落ちたらリスタート
        if (playerPos_.y < -3.0f) {
            LOG_INFO("[Player] Fell into hole! Restarting...");
            playerPos_ = Vector3(0, 3, 0);  // スタート地点に戻る
            playerVelocity_ = Vector3::Zero;
            isPlayerGrounded_ = false;
        }

        // ゴール判定（Goalに触れたらクリア）
        float distToGoal = Vector3::Distance(playerPos_, goalPos_);
        const float goalRadius = 0.5f;  // ゴール判定の半径
        if (distToGoal < goalRadius) {
            LOG_INFO("[Player] Goal reached! Game Clear!");
            SceneManager::Get().Load<ResultScene>();
        }

        // Transformに反映
        if (auto* transform = world_->GetComponent<ECS::LocalTransform>(playerActor_)) {
            transform->position = playerPos_;
        }
        // Mark transform as dirty (tag component - just add it)
        if (!world_->HasComponent<ECS::TransformDirty>(playerActor_)) {
            world_->AddComponent<ECS::TransformDirty>(playerActor_);
        }
    }

    // ECS World
    std::unique_ptr<ECS::World> world_;

    // エンティティ
    ECS::Actor stageActor_;
    ECS::Actor playerActor_;
    std::vector<ECS::Actor> cubeActors_;
    std::vector<Vector3> cubePositions_;  // 当たり判定用
    std::vector<Vector3> cubeScales_;     // 当たり判定用
    ECS::Actor goalActor_;
    Vector3 goalPos_ = Vector3::Zero;
    ECS::Actor cameraActor_;
    ECS::Camera3DData* camera_ = nullptr;

    // Cubeアセット（共有）
    MeshHandle cubeMesh_;
    std::vector<MaterialHandle> cubeMaterials_;

    // ステージ
    Quaternion stageRotation_ = Quaternion::Identity;
    Physics::MeshColliderPtr stageCollider_;

    // プレイヤー状態
    Vector3 playerPos_ = Vector3::Zero;
    Vector3 playerVelocity_ = Vector3::Zero;
    bool isPlayerGrounded_ = false;
    float coyoteTimeCounter_ = 0.0f;   // コヨーテタイム残り
    float jumpBufferCounter_ = 0.0f;   // ジャンプバッファ残り

    //------------------------------------------------------------------------
    // カメラパラメータ
    //------------------------------------------------------------------------
    Vector3 cameraPos_ = Vector3(0.0f, 5.0f, -10.0f);
    float cameraYaw_ = 0.0f;      // Y軸回転（度）
    float cameraPitch_ = 20.0f;   // X軸回転（度）

    //------------------------------------------------------------------------
    // デバッグ描画
    //------------------------------------------------------------------------
    std::vector<DebugLine3D> debugLines_;
    bool showDebugRays_ = false;

    // デバッグラインシェーダー（dx11レイヤー使用）
    ShaderPtr debugLineVS_;
    ShaderPtr debugLinePS_;
    BufferPtr debugLineCB_;
    std::unique_ptr<BlendState> debugBlendState_;
    std::unique_ptr<DepthStencilState> debugDepthState_;
    ComPtr<ID3D11InputLayout> debugLineIL_;
};
