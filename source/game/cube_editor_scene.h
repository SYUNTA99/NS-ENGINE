//----------------------------------------------------------------------------
//! @file   cube_editor_scene.h
//! @brief  オブジェクト配置エディターシーン（ecs_model_sceneベース）
//!
//! 操作方法:
//!   【オブジェクト】
//!     Tab/1-9: オブジェクト選択
//!     N: Cube追加, M: Diamond追加, G: Goal追加
//!     Delete: 選択オブジェクト削除
//!     左ドラッグ: XZ平面移動
//!     右ドラッグ: Y回転, X+右ドラッグ: X回転, Z+右ドラッグ: Z回転
//!     Shift+ホイール: Y軸移動
//!     Ctrl+ホイール: スケール
//!     R: 選択オブジェクトリセット
//!   【カメラ】
//!     中ボタンドラッグ: オービット
//!     Ctrl+中ボタンドラッグ: パン
//!     ホイール: ズーム
//!     F: フォーカス
//!   【出力】
//!     Insert/Ctrl+C: クリップボードにコピー
//!     P: 全オブジェクト情報をログ出力
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
#include "engine/graphics/mesh_batch.h"
#include "engine/platform/renderer.h"
#include "engine/platform/application.h"
#include "engine/input/input_manager.h"
#include "engine/physics/mesh_collider.h"
#include "engine/physics/raycast.h"
#include "engine/texture/texture_manager.h"
#include "dx11/graphics_context.h"
#include "dx11/graphics_device.h"
#include "common/logging/logging.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vector>

//============================================================================
//! @brief 編集可能オブジェクト
//============================================================================
struct EditableObject
{
    enum class Type { Cube, Diamond, Goal };

    Type type = Type::Cube;
    ECS::Actor actor;
    Vector3 position = Vector3(0, 1.0f, 0);
    Vector3 rotation = Vector3::Zero;  // degrees
    Vector3 scale = Vector3(1, 1, 1);
    std::string name;
};

//============================================================================
//! @brief オブジェクト配置エディターシーン
//============================================================================
class CubeEditorScene : public Scene
{
public:
    void OnEnter() override
    {
        LOG_INFO("[CubeEditorScene] Initializing...");
        LOG_INFO("  [Object] Tab/1-9: Select, N: Cube, M: Diamond, G: Goal");
        LOG_INFO("  [Object] Delete: Remove, Left: Move XZ, Right: Y rot, X+Right: X rot, Z+Right: Z rot");
        LOG_INFO("  [Object] Shift+Wheel: Move Y, Ctrl+Wheel: Scale, R: Reset");
        LOG_INFO("  [Camera] Middle: Orbit, Ctrl+Middle: Pan, Wheel: Zoom, F: Focus");
        LOG_INFO("  [Output] Insert/Ctrl+C: Copy, P: Print all");

        // 1. ECS World作成
        world_ = std::make_unique<ECS::World>();
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
            LOG_INFO("[CubeEditorScene] Stage loaded!");
        } else {
            LOG_ERROR("[CubeEditorScene] Stage load FAILED! Using box.");
            stageMesh = MeshManager::Get().CreateBox(Vector3(10, 1, 10));
            stageMaterials.push_back(MaterialManager::Get().CreateDefault());
        }

        // 3. ステージEntity作成
        stageActor_ = world_->CreateActor();

        auto* transform = world_->AddComponent<ECS::LocalTransform>(stageActor_);
        transform->position = Vector3(0.0f, 0.0f, 0.0f);
        stageRotation_ = Quaternion::CreateFromAxisAngle(Vector3::Right, DirectX::XM_PIDIV2);
        transform->rotation = stageRotation_;
        transform->scale = Vector3(5.0f, 5.0f, 5.0f);
        world_->AddComponent<ECS::LocalToWorld>(stageActor_);
        world_->AddComponent<ECS::TransformDirty>(stageActor_);

        auto* mesh = world_->AddComponent<ECS::MeshData>(stageActor_);
        mesh->mesh = stageMesh;
        mesh->SetMaterials(stageMaterials);
        mesh->visible = true;

        // 4. コリジョンメッシュ作成
        CreateStageCollider(stagePath);

        // 5. 編集用オブジェクト作成
        CreateEditObjects();

        // 6. カメラ設定
        cameraActor_ = world_->CreateActor();
        camera_ = world_->AddComponent<ECS::Camera3DData>(cameraActor_, 60.0f, 16.0f / 9.0f);

        UpdateWindowTitle();
        LOG_INFO("[CubeEditorScene] Scene setup complete!");
    }

    void OnExit() override
    {
        LOG_INFO("[CubeEditorScene] Shutting down...");
        SetWindowTextW(Application::Get().GetHWND(), L"NS-ENGINE");
        world_.reset();
    }

    void FixedUpdate(float dt) override
    {
        auto& keyboard = InputManager::Get().GetKeyboard();
        auto& mouse = InputManager::Get().GetMouse();

        float dx = static_cast<float>(mouse.GetDeltaX());
        float dy = static_cast<float>(mouse.GetDeltaY());
        float wheel = mouse.GetWheelDelta();

        //====================================================================
        // オブジェクト選択・追加・削除
        //====================================================================
        // Tab: 次のオブジェクト
        if (keyboard.IsKeyDown(Key::Tab)) {
            if (!objects_.empty()) {
                selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(objects_.size());
                positionChanged_ = true;
            }
        }
        // 1-9: 直接選択
        for (int i = 0; i < 9 && i < static_cast<int>(objects_.size()); ++i) {
            if (keyboard.IsKeyDown(static_cast<Key>(static_cast<int>(Key::Num1) + i))) {
                selectedIndex_ = i;
                positionChanged_ = true;
            }
        }
        // N: 新規Cube追加
        if (keyboard.IsKeyDown(Key::N)) {
            Vector3 spawnPos = objects_.empty() ? Vector3(0, 1, 0) : objects_[selectedIndex_].position + Vector3(2, 0, 0);
            AddObject(EditableObject::Type::Cube, spawnPos);
            positionChanged_ = true;
        }
        // M: 新規Diamond追加
        if (keyboard.IsKeyDown(Key::M)) {
            Vector3 spawnPos = objects_.empty() ? Vector3(0, 1, 0) : objects_[selectedIndex_].position + Vector3(2, 0, 0);
            AddObject(EditableObject::Type::Diamond, spawnPos);
            positionChanged_ = true;
        }
        // G: 新規Goal追加
        if (keyboard.IsKeyDown(Key::G)) {
            Vector3 spawnPos = objects_.empty() ? Vector3(0, 1, 0) : objects_[selectedIndex_].position + Vector3(2, 0, 0);
            AddObject(EditableObject::Type::Goal, spawnPos);
            positionChanged_ = true;
        }
        // Delete: 削除
        if (keyboard.IsKeyDown(Key::Delete)) {
            RemoveSelectedObject();
            positionChanged_ = true;
        }

        // 現在選択中のオブジェクト（なければスキップ）
        if (objects_.empty() || selectedIndex_ < 0) {
            UpdateCamera();
            world_->FixedUpdate(dt);
            return;
        }
        auto& obj = objects_[selectedIndex_];
        auto& pos = obj.position;
        auto& rot = obj.rotation;
        auto& scale = obj.scale;

        //====================================================================
        // カメラ操作
        //====================================================================

        // 中ボタンドラッグ: カメラオービット or パン
        if (mouse.IsButtonPressed(MouseButton::Middle)) {
            if (dx != 0.0f || dy != 0.0f) {
                if (keyboard.IsControlPressed()) {
                    // Ctrl+中ボタン: パン（平行移動）
                    float panSpeed = 0.02f * cameraDistance_;

                    float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
                    float cosYaw = std::cos(yawRad);
                    float sinYaw = std::sin(yawRad);

                    Vector3 right(-cosYaw, 0, sinYaw);
                    Vector3 up(0, 1, 0);

                    cameraPivot_ += right * (-dx) * panSpeed;
                    cameraPivot_ += up * dy * panSpeed;
                } else {
                    // 中ボタン: オービット（回転）
                    cameraYaw_ += dx * 0.3f;
                    cameraPitch_ += dy * 0.3f;
                    cameraPitch_ = std::clamp(cameraPitch_, -89.0f, 89.0f);
                }
            }
        }

        // ホイール: ズーム / オブジェクト操作
        if (wheel != 0.0f) {
            if (keyboard.IsControlPressed()) {
                // Ctrl+ホイール: オブジェクトスケール
                float scaleSpeed = 0.1f;
                float scaleDelta = wheel * scaleSpeed;
                scale += Vector3(scaleDelta, scaleDelta, scaleDelta);
                scale.x = (std::max)(0.1f, scale.x);
                scale.y = (std::max)(0.1f, scale.y);
                scale.z = (std::max)(0.1f, scale.z);
                positionChanged_ = true;
            } else if (keyboard.IsShiftPressed()) {
                // Shift+ホイール: オブジェクトY移動
                float moveSpeed = 0.3f;
                pos.y += wheel * moveSpeed;
                positionChanged_ = true;
            } else {
                // ホイール: カメラズーム
                float zoomSpeed = 0.8f;
                cameraDistance_ -= wheel * zoomSpeed;
                cameraDistance_ = std::clamp(cameraDistance_, 2.0f, 100.0f);
            }
        }

        // F: 選択オブジェクトにフォーカス
        if (keyboard.IsKeyDown(Key::F) && !objects_.empty()) {
            cameraPivot_ = objects_[selectedIndex_].position;
            cameraDistance_ = 10.0f;
            LOG_INFO("[CubeEditor] Focus on " + objects_[selectedIndex_].name);
        }

        //====================================================================
        // オブジェクト操作
        //====================================================================

        // 左ドラッグ: XZ平面移動
        if (mouse.IsButtonPressed(MouseButton::Left)) {
            if (dx != 0.0f || dy != 0.0f) {
                float moveSpeed = 0.01f * cameraDistance_;

                float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
                float cosYaw = std::cos(yawRad);
                float sinYaw = std::sin(yawRad);

                Vector3 right(-cosYaw, 0, sinYaw);
                Vector3 forward(-sinYaw, 0, -cosYaw);

                pos += right * dx * moveSpeed;
                pos += forward * (-dy) * moveSpeed;
                positionChanged_ = true;
            }
        }

        // 右ドラッグ: 回転（X/Y/Zキーで軸指定）
        if (mouse.IsButtonPressed(MouseButton::Right)) {
            if (dx != 0.0f || dy != 0.0f) {
                float rotSpeed = 0.5f;
                if (keyboard.IsKeyPressed(Key::X)) {
                    // X+右ドラッグ: X軸回転
                    rot.x += dy * rotSpeed;
                } else if (keyboard.IsKeyPressed(Key::Z)) {
                    // Z+右ドラッグ: Z軸回転
                    rot.z += dx * rotSpeed;
                } else {
                    // 右ドラッグのみ: Y軸回転
                    rot.y += dx * rotSpeed;
                }
                WrapRotation(rot);
                positionChanged_ = true;
            }
        }

        //====================================================================
        // キーボード操作
        //====================================================================

        // リセット（R）
        if (keyboard.IsKeyDown(Key::R)) {
            pos = Vector3(0, 1.0f, 0);
            rot = Vector3::Zero;
            scale = Vector3(1, 1, 1);
            positionChanged_ = true;
            LOG_INFO("[CubeEditor] Reset");
        }

        // 座標コピー（Insert）- クリップボードにコピー（AddObject互換形式）
        if (keyboard.IsKeyDown(Key::Insert)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3);

            // タイプ名を取得
            const char* typeName = "Cube";
            if (obj.type == EditableObject::Type::Diamond) typeName = "Diamond";
            else if (obj.type == EditableObject::Type::Goal) typeName = "Goal";

            oss << "// " << obj.name << "\r\n";
            oss << "AddObject(EditableObject::Type::" << typeName << ", ";
            oss << "Vector3(" << pos.x << "f, " << pos.y << "f, " << pos.z << "f), ";
            oss << "Vector3(" << rot.x << "f, " << rot.y << "f, " << rot.z << "f), ";
            oss << "Vector3(" << scale.x << "f, " << scale.y << "f, " << scale.z << "f));";

            std::string text = oss.str();

            if (OpenClipboard(Application::Get().GetHWND())) {
                EmptyClipboard();
                HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
                if (hGlobal) {
                    char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
                    memcpy(pGlobal, text.c_str(), text.size() + 1);
                    GlobalUnlock(hGlobal);
                    SetClipboardData(CF_TEXT, hGlobal);
                }
                CloseClipboard();
                LOG_INFO("[CubeEditor] Copied to clipboard");
            }
        }

        // P: 全オブジェクト情報をログ出力
        if (keyboard.IsKeyDown(Key::P)) {
            PrintAllObjects();
        }

        // Transform更新（全オブジェクト）
        for (auto& o : objects_) {
            UpdateObjectTransform(o.actor, o.position, o.rotation, o.scale);
        }

        // カメラ更新（選択中のオブジェクトを注視）
        UpdateCamera();

        if (positionChanged_) {
            UpdateWindowTitle();
            positionChanged_ = false;
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

        mb.SetAmbientLight(Color(0.4f, 0.4f, 0.5f, 1.0f));

        Vector3 lightDir(0.3f, -1.0f, 0.5f);
        lightDir.Normalize();
        mb.AddLight(LightBuilder::Directional(lightDir, Colors::White, 1.2f));

        world_->Render(alpha);

        mb.ClearLights();
    }

private:
    //------------------------------------------------------------------------
    // 回転を0-360にラップ
    //------------------------------------------------------------------------
    void WrapRotation(Vector3& rot)
    {
        while (rot.x >= 360.0f) rot.x -= 360.0f;
        while (rot.x < 0.0f) rot.x += 360.0f;
        while (rot.y >= 360.0f) rot.y -= 360.0f;
        while (rot.y < 0.0f) rot.y += 360.0f;
        while (rot.z >= 360.0f) rot.z -= 360.0f;
        while (rot.z < 0.0f) rot.z += 360.0f;
    }

    //------------------------------------------------------------------------
    // オブジェクトTransform更新
    //------------------------------------------------------------------------
    void UpdateObjectTransform(ECS::Actor actor, const Vector3& pos, const Vector3& rot, const Vector3& scale)
    {
        if (auto* t = world_->GetComponent<ECS::LocalTransform>(actor)) {
            t->position = pos;
            // XYZ順序（ローカル軸回転）: Pitch(X) → Yaw(Y) → Roll(Z)
            // これにより Z回転で斜めに傾く
            Quaternion qx = Quaternion::CreateFromAxisAngle(Vector3::UnitX, DirectX::XMConvertToRadians(rot.x));
            Quaternion qy = Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XMConvertToRadians(rot.y));
            Quaternion qz = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, DirectX::XMConvertToRadians(rot.z));
            t->rotation = qx * qy * qz;
            t->scale = scale;
        }
        if (!world_->HasComponent<ECS::TransformDirty>(actor)) {
            world_->AddComponent<ECS::TransformDirty>(actor);
        }
    }

    //------------------------------------------------------------------------
    // コリジョンメッシュ作成
    //------------------------------------------------------------------------
    void CreateStageCollider(const std::string& path)
    {
        MeshLoadResult loadResult = MeshLoaderRegistry::Get().Load(path, {});
        if (!loadResult.IsValid() || loadResult.meshDescs.empty()) {
            LOG_WARN("[CubeEditorScene] Failed to load collision mesh");
            return;
        }

        const MeshDesc& desc = loadResult.meshDescs[0];
        stageCollider_ = Physics::MeshCollider::CreateFromMeshDesc(desc);

        Matrix stageWorld = Matrix::CreateScale(5.0f) *
                           Matrix::CreateFromQuaternion(stageRotation_);
        stageCollider_->SetWorldMatrix(stageWorld);

        LOG_INFO("[CubeEditorScene] Collider created");
    }

    //------------------------------------------------------------------------
    // メッシュ・マテリアルをロード（初回のみ）
    //------------------------------------------------------------------------
    void LoadAssets()
    {
        // Cubeモデルをロード
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
            LOG_INFO("[CubeEditorScene] Cube assets loaded");
        } else {
            cubeMesh_ = MeshManager::Get().CreateBox(Vector3(1, 1, 1));
            MaterialDesc matDesc;
            matDesc.params.albedoColor = Color(1.0f, 0.3f, 0.1f, 1.0f);
            cubeMaterials_.push_back(MaterialManager::Get().Create(matDesc));
        }

        // Diamondモデルをロード
        auto diamondResult = MeshManager::Get().LoadWithMaterials("model:/cube/Diamond.fbx");
        if (diamondResult.success) {
            diamondMesh_ = diamondResult.mesh;
        } else {
            diamondMesh_ = MeshManager::Get().CreateBox(Vector3(0.5f, 1.0f, 0.5f));
        }

        // Diamond用の透明赤マテリアル
        MaterialDesc diamondMatDesc;
        diamondMatDesc.params.albedoColor = Color(1.0f, 0.1f, 0.1f, 0.5f);
        diamondMatDesc.params.metallic = 0.8f;
        diamondMatDesc.params.roughness = 0.1f;
        diamondMaterial_ = MaterialManager::Get().Create(diamondMatDesc);
        LOG_INFO("[CubeEditorScene] Diamond assets loaded");

        // Goalモデルをロード
        auto goalResult = MeshManager::Get().LoadWithMaterials("model:/goal/goal.fbx");
        if (goalResult.success) {
            goalMesh_ = goalResult.mesh;
            goalMaterials_ = std::move(goalResult.materials);

            // テクスチャを手動で設定（goal1.png〜goal4.png）
            const char* goalTexNames[] = { "goal1.png", "goal2.png", "goal3.png", "goal4.png" };
            for (size_t i = 0; i < goalMaterials_.size() && i < 4; ++i) {
                std::string texPath = "texture:/goal/" + std::string(goalTexNames[i]);
                auto goalTex = TextureManager::Get().Load(texPath);
                if (goalTex) {
                    if (auto* matPtr = MaterialManager::Get().Get(goalMaterials_[i])) {
                        matPtr->SetTexture(MaterialTextureSlot::Albedo, goalTex);
                    }
                }
            }
            LOG_INFO("[CubeEditorScene] Goal assets loaded with textures");
        } else {
            // フォールバック: 緑の球
            goalMesh_ = MeshManager::Get().CreateSphere(0.5f, 16);
            MaterialDesc matDesc;
            matDesc.params.albedoColor = Color(0.2f, 1.0f, 0.3f, 1.0f);
            goalMaterials_.push_back(MaterialManager::Get().Create(matDesc));
        }
    }

    //------------------------------------------------------------------------
    // 新規オブジェクト追加（フル引数版）
    //------------------------------------------------------------------------
    void AddObject(EditableObject::Type type, const Vector3& position, const Vector3& rotationDegrees, const Vector3& scale)
    {
        EditableObject obj;
        obj.type = type;
        obj.position = position;
        obj.rotation = rotationDegrees;
        obj.scale = scale;
        switch (type) {
            case EditableObject::Type::Cube:
                obj.name = "Cube_" + std::to_string(cubeCount_++);
                break;
            case EditableObject::Type::Diamond:
                obj.name = "Diamond_" + std::to_string(diamondCount_++);
                break;
            case EditableObject::Type::Goal:
                obj.name = "Goal_" + std::to_string(goalCount_++);
                break;
        }

        // Actor作成
        obj.actor = world_->CreateActor();

        auto* transform = world_->AddComponent<ECS::LocalTransform>(obj.actor);
        transform->position = obj.position;
        // XYZ順序（ローカル軸回転）- UpdateObjectTransformと同じ
        Quaternion qx = Quaternion::CreateFromAxisAngle(Vector3::UnitX, DirectX::XMConvertToRadians(obj.rotation.x));
        Quaternion qy = Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XMConvertToRadians(obj.rotation.y));
        Quaternion qz = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, DirectX::XMConvertToRadians(obj.rotation.z));
        transform->rotation = qx * qy * qz;
        transform->scale = obj.scale;
        world_->AddComponent<ECS::LocalToWorld>(obj.actor);
        world_->AddComponent<ECS::TransformDirty>(obj.actor);

        auto* mesh = world_->AddComponent<ECS::MeshData>(obj.actor);
        switch (type) {
            case EditableObject::Type::Cube:
                mesh->mesh = cubeMesh_;
                mesh->SetMaterials(cubeMaterials_);
                break;
            case EditableObject::Type::Diamond:
                mesh->mesh = diamondMesh_;
                mesh->SetMaterial(0, diamondMaterial_);
                break;
            case EditableObject::Type::Goal:
                mesh->mesh = goalMesh_;
                mesh->SetMaterials(goalMaterials_);
                break;
        }
        mesh->visible = true;

        objects_.push_back(obj);
        selectedIndex_ = static_cast<int>(objects_.size()) - 1;

        LOG_INFO("[CubeEditorScene] Added: " + obj.name);
    }

    //------------------------------------------------------------------------
    // 新規オブジェクト追加（簡易版 - 位置のみ）
    //------------------------------------------------------------------------
    void AddObject(EditableObject::Type type, const Vector3& position)
    {
        AddObject(type, position, Vector3::Zero, Vector3(1, 1, 1));
    }

    //------------------------------------------------------------------------
    // 選択オブジェクト削除
    //------------------------------------------------------------------------
    void RemoveSelectedObject()
    {
        if (objects_.empty() || selectedIndex_ < 0) return;

        auto& obj = objects_[selectedIndex_];
        world_->DestroyActor(obj.actor);
        LOG_INFO("[CubeEditorScene] Removed: " + obj.name);

        objects_.erase(objects_.begin() + selectedIndex_);

        if (selectedIndex_ >= static_cast<int>(objects_.size())) {
            selectedIndex_ = static_cast<int>(objects_.size()) - 1;
        }
    }

    //------------------------------------------------------------------------
    // 初期オブジェクト作成
    //------------------------------------------------------------------------
    void CreateEditObjects()
    {
        LoadAssets();
        // Cube_0
        AddObject(EditableObject::Type::Cube, Vector3(1.152f, 1.000f, 2.767f), Vector3(91.000f, 271.000f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));
        // Cube_1
        AddObject(EditableObject::Type::Cube, Vector3(2.269f, 1.600f, 2.334f), Vector3(90.500f, 267.500f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));
        // Cube_2
        AddObject(EditableObject::Type::Cube, Vector3(0.734f, 2.500f, 2.177f), Vector3(273.000f, 356.500f, 337.000f), Vector3(1.000f, 1.000f, 1.000f));
        // Cube_3
        AddObject(EditableObject::Type::Cube, Vector3(1.272f, 4.300f, 2.007f), Vector3(90.500f, 0.500f, 2.500f), Vector3(0.300f, 0.300f, 0.300f));
        // Goal_0
        AddObject(EditableObject::Type::Goal, Vector3(0.603f, 5.500f, 1.765f), Vector3(88.500f, 323.000f, 0.000f), Vector3(1.000f, 1.000f, 1.000f));
    }

    //------------------------------------------------------------------------
    // カメラ更新
    //------------------------------------------------------------------------
    void UpdateCamera()
    {
        if (!camera_) return;

        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        float pitchRad = DirectX::XMConvertToRadians(cameraPitch_);

        float cosPitch = std::cos(pitchRad);
        float sinPitch = std::sin(pitchRad);

        // カメラ位置を球面座標で計算
        Vector3 offset(
            -std::sin(yawRad) * cosPitch * cameraDistance_,
            sinPitch * cameraDistance_,
            -std::cos(yawRad) * cosPitch * cameraDistance_
        );

        Vector3 cameraPos = cameraPivot_ + offset;

        camera_->SetPosition(cameraPos.x, cameraPos.y, cameraPos.z);
        camera_->LookAt(cameraPivot_, Vector3::Up);
    }

    //------------------------------------------------------------------------
    // ウィンドウタイトル更新
    //------------------------------------------------------------------------
    void UpdateWindowTitle()
    {
        if (objects_.empty() || selectedIndex_ < 0) {
            SetWindowTextW(Application::Get().GetHWND(), L"[No Object]");
            return;
        }

        const auto& obj = objects_[selectedIndex_];
        std::wostringstream woss;
        woss << std::fixed << std::setprecision(1);
        woss << L"[" << selectedIndex_ + 1 << L"/" << objects_.size() << L" ";

        // ASCII文字列をwstringに変換
        std::wstring wname(obj.name.begin(), obj.name.end());
        woss << wname << L"] Pos: ("
             << obj.position.x << L", " << obj.position.y << L", " << obj.position.z
             << L") Rot: (" << obj.rotation.x << L", " << obj.rotation.y << L", " << obj.rotation.z
             << L") Scale: (" << obj.scale.x << L", " << obj.scale.y << L", " << obj.scale.z << L")";

        SetWindowTextW(Application::Get().GetHWND(), woss.str().c_str());
    }

    //------------------------------------------------------------------------
    // 全オブジェクト情報をログ出力
    //------------------------------------------------------------------------
    void PrintAllObjects()
    {
        LOG_INFO("//========================================");
        LOG_INFO("// CubeEditorScene Objects (" + std::to_string(objects_.size()) + ")");
        LOG_INFO("//========================================");

        for (size_t i = 0; i < objects_.size(); ++i) {
            const auto& obj = objects_[i];
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3);
            oss << "// [" << (i + 1) << "] " << obj.name;
            LOG_INFO(oss.str());

            oss.str(""); oss.clear();
            oss << std::fixed << std::setprecision(3);
            oss << "  pos=(" << obj.position.x << ", " << obj.position.y << ", " << obj.position.z << ")";
            oss << " rot=(" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << ")";
            oss << " scale=(" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << ")";
            LOG_INFO(oss.str());
        }
        LOG_INFO("//========================================");
    }

    // ECS World
    std::unique_ptr<ECS::World> world_;

    // エンティティ
    ECS::Actor stageActor_;
    ECS::Actor cameraActor_;
    ECS::Camera3DData* camera_ = nullptr;

    // ステージ
    Quaternion stageRotation_ = Quaternion::Identity;
    Physics::MeshColliderPtr stageCollider_;

    // アセット（共有）
    MeshHandle cubeMesh_;
    std::vector<MaterialHandle> cubeMaterials_;
    MeshHandle diamondMesh_;
    MaterialHandle diamondMaterial_;
    MeshHandle goalMesh_;
    std::vector<MaterialHandle> goalMaterials_;
    int cubeCount_ = 0;
    int diamondCount_ = 0;
    int goalCount_ = 0;

    // オブジェクト管理
    std::vector<EditableObject> objects_;
    int selectedIndex_ = 0;
    bool positionChanged_ = true;

    // カメラ
    Vector3 cameraPivot_ = Vector3(0, 1.0f, 0);
    float cameraDistance_ = 15.0f;
    float cameraYaw_ = 0.0f;
    float cameraPitch_ = 25.0f;
};
