//----------------------------------------------------------------------------
//! @file   animation_test_scene.h
//! @brief  アニメーションテストシーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/scene/scene_manager.h"
#include "engine/input/input_manager.h"
#include "engine/platform/renderer.h"
#include "engine/mesh/skinned_mesh.h"
#include "engine/mesh/skinned_mesh_loader.h"
#include "engine/mesh/vertex_format.h"
#include "engine/material/material_manager.h"
#include "engine/texture/texture_manager.h"
#include "engine/shader/shader_manager.h"
#include "engine/game_object/components/animation/skeleton.h"
#include "engine/game_object/components/animation/animation_clip.h"
#include "dx11/graphics_context.h"
#include "dx11/gpu/gpu.h"
#include "dx11/state/sampler_state.h"
#include "engine/graphics/render_state_manager.h"
#include "common/logging/logging.h"
#include <cmath>

// 前方宣言
class TitleScene;

//============================================================================
//! @brief アニメーションテストシーン
//============================================================================
class AnimationTestScene : public Scene
{
public:
    static constexpr size_t MAX_BONES = 256;
    static constexpr uint32_t MAX_LIGHTS = 8;

    // 定数バッファ構造体（VS用）
    struct alignas(16) PerFrameCB {
        Matrix viewProjection;
        Vector4 cameraPosition;
    };

    struct alignas(16) PerObjectCB {
        Matrix world;
        Matrix worldInvTranspose;
    };

    struct alignas(16) BoneMatricesCB {
        Matrix bones[MAX_BONES];
    };

    // 定数バッファ構造体（PS用）
    struct alignas(16) MaterialCB {
        Vector4 albedoColor;      // rgb = albedo, a = alpha
        float metallic;
        float roughness;
        float ao;
        float emissiveStrength;
        Vector4 emissiveColor;
        uint32_t useAlbedoMap;
        uint32_t useNormalMap;
        uint32_t useMetallicMap;
        uint32_t useRoughnessMap;
    };

    struct LightData {
        Vector4 position;     // xyz = pos, w = type
        Vector4 direction;    // xyz = dir, w = range
        Vector4 color;        // rgb = color, a = intensity
        Vector4 spotParams;   // x = innerCos, y = outerCos, z = falloff
    };

    struct alignas(16) LightingCB {
        Vector4 lightCameraPosition;
        Vector4 ambientColor;
        uint32_t numLights;
        uint32_t pad[3];
        LightData lights[MAX_LIGHTS];
    };

    struct alignas(16) ShadowCB {
        Matrix lightViewProjection;
        Vector4 shadowParams;  // x = depthBias, y = normalBias, z = shadowStrength, w = enabled
    };

    [[nodiscard]] const char* GetName() const override { return "AnimationTestScene"; }

    void OnEnter() override
    {
        LOG_INFO("[AnimationTestScene] Enter");

        // カメラ初期設定（スケール0.01なので近くから見る）
        cameraPos_ = Vector3(0, 1.0f, -3.0f);
        cameraYaw_ = 0.0f;
        cameraPitch_ = 0.0f;

        // シェーダーをロード
        vertexShader_ = ShaderManager::Get().LoadVertexShader("skinned_mesh_vs.hlsl");
        pixelShader_ = ShaderManager::Get().LoadPixelShader("mesh_ps.hlsl");

        if (vertexShader_) {
            LOG_INFO("[AnimationTestScene] Vertex shader loaded successfully");
        } else {
            LOG_ERROR("[AnimationTestScene] Failed to load vertex shader!");
        }
        if (pixelShader_) {
            LOG_INFO("[AnimationTestScene] Pixel shader loaded successfully");
        } else {
            LOG_ERROR("[AnimationTestScene] Failed to load pixel shader!");
        }

        // 入力レイアウト作成
        if (vertexShader_) {
            inputLayout_ = ShaderManager::Get().CreateInputLayout(
                vertexShader_.get(),
                MeshInputLayouts::SkinnedMeshVertexLayout,
                MeshInputLayouts::SkinnedMeshVertexLayoutCount);
            if (inputLayout_) {
                LOG_INFO("[AnimationTestScene] InputLayout created successfully");
            } else {
                LOG_ERROR("[AnimationTestScene] Failed to create InputLayout!");
            }
        } else {
            LOG_ERROR("[AnimationTestScene] vertexShader_ is null, cannot create InputLayout");
        }

        // 定数バッファ作成（VS用）
        perFrameBuffer_ = Buffer::CreateConstant(sizeof(PerFrameCB));
        perObjectBuffer_ = Buffer::CreateConstant(sizeof(PerObjectCB));
        boneMatricesBuffer_ = Buffer::CreateConstant(sizeof(BoneMatricesCB));

        // 定数バッファ作成（PS用）
        materialBuffer_ = Buffer::CreateConstant(sizeof(MaterialCB));
        lightingBuffer_ = Buffer::CreateConstant(sizeof(LightingCB));
        shadowBuffer_ = Buffer::CreateConstant(sizeof(ShadowCB));

        // サンプラーステート作成
        linearSampler_ = SamplerState::CreateDefault();
        shadowSampler_ = SamplerState::CreateComparison();

        // Tokoモデルをスキンメッシュとしてロード
        const std::string modelPath = "model:/characters/Toko/Toko_sum.fbx";
        const std::string animPath = "model:/characters/Toko/Cube.fbx";  // アニメーションのみ
        LOG_INFO("[AnimationTestScene] Loading skinned mesh: " + modelPath);

        auto result = SkinnedMeshLoader::Load(modelPath);

        if (result.IsValid()) {
            skinnedMesh_ = result.mesh;
            skeleton_ = skinnedMesh_->GetSkeleton();

            LOG_INFO("[AnimationTestScene] Mesh loaded successfully!");
            LOG_INFO("[AnimationTestScene] - Bones: " + std::to_string(skinnedMesh_->GetBoneCount()));

            // バウンディングボックスを確認
            const auto& bounds = skinnedMesh_->GetBounds();
            LOG_INFO("[AnimationTestScene] - Bounds Min: (" +
                std::to_string(bounds.min.x) + ", " +
                std::to_string(bounds.min.y) + ", " +
                std::to_string(bounds.min.z) + ")");
            LOG_INFO("[AnimationTestScene] - Bounds Max: (" +
                std::to_string(bounds.max.x) + ", " +
                std::to_string(bounds.max.y) + ", " +
                std::to_string(bounds.max.z) + ")");
            LOG_INFO("[AnimationTestScene] - SubMeshes: " + std::to_string(skinnedMesh_->GetSubMeshCount()));
            LOG_INFO("[AnimationTestScene] - VertexCount: " + std::to_string(skinnedMesh_->GetVertexCount()));
            LOG_INFO("[AnimationTestScene] - IndexCount: " + std::to_string(skinnedMesh_->GetIndexCount()));

            // 別ファイルからアニメーションをロード（アニメーション専用ローダーを使用）
            std::vector<std::string> animFiles = {
                "model:/characters/Toko/unitychan_RUN00_F.fbx",
                "model:/characters/Toko/unitychan_JUMP01.fbx",
                "model:/characters/Toko/Sprint.fbx"
            };
            for (const auto& animFilePath : animFiles) {
                LOG_INFO("[AnimationTestScene] Loading animations from: " + animFilePath);
                auto animResult = SkinnedMeshLoader::LoadAnimationsOnly(animFilePath, skinnedMesh_->GetSkeleton());
                if (animResult.IsValid()) {
                    LOG_INFO("[AnimationTestScene] - Loaded " + std::to_string(animResult.animations.size()) + " animations");
                    for (size_t i = 0; i < animResult.animations.size(); ++i) {
                        const auto& clip = animResult.animations[i];
                        LOG_INFO("[AnimationTestScene] - Clip: " + clip->name +
                                 " (duration: " + std::to_string(clip->duration) + "s, channels: " +
                                 std::to_string(clip->channels.size()) + ")");
                        skinnedMesh_->AddAnimation(clip);
                    }
                } else {
                    LOG_WARN("[AnimationTestScene] Failed to load: " + animResult.errorMessage);
                }
            }

            // アニメーションクリップを取得
            const auto& clips = skinnedMesh_->GetAnimations();
            LOG_INFO("[AnimationTestScene] - Total Animations: " + std::to_string(clips.size()));

            if (!clips.empty()) {
                // 外部ファイルからロードした完全なアニメーション（110チャンネル）を優先
                // Toko_sum.fbx内のアニメーションは1チャンネルしかないので動かない
                // 外部アニメーションはインデックス22以降に追加されている
                size_t preferredIndex = 0;
                for (size_t i = 0; i < clips.size(); ++i) {
                    // 完全なアニメーション（チャンネル数が多い）を探す
                    if (clips[i]->channels.size() > 10) {
                        preferredIndex = i;
                        LOG_INFO("[AnimationTestScene] Found full animation at index " +
                                 std::to_string(i) + ": " + clips[i]->name +
                                 " (channels: " + std::to_string(clips[i]->channels.size()) + ")");
                        break;
                    }
                }
                currentClipIndex_ = preferredIndex;
                currentClip_ = clips[currentClipIndex_];
                LOG_INFO("[AnimationTestScene] Playing: " + currentClip_->name +
                         " (index: " + std::to_string(currentClipIndex_) +
                         ", channels: " + std::to_string(currentClip_->channels.size()) + ")");
            }

            // マテリアル作成（テクスチャをロード）
            const std::string textureDir = "texture:/Toko_Textures/";
            LOG_INFO("[AnimationTestScene] Processing " + std::to_string(result.materialDescs.size()) + " materials");
            for (size_t matIdx = 0; matIdx < result.materialDescs.size(); ++matIdx) {
                auto& matDesc = result.materialDescs[matIdx];
                LOG_INFO("[AnimationTestScene] Material[" + std::to_string(matIdx) + "]: " + matDesc.name);

                // diffuseTexturePathからテクスチャをロード
                if (!matDesc.diffuseTexturePath.empty()) {
                    LOG_INFO("[AnimationTestScene]   FBX texture ref: " + matDesc.diffuseTexturePath);

                    // パスからファイル名を抽出
                    std::string texPath = matDesc.diffuseTexturePath;
                    size_t lastSlash = texPath.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        texPath = texPath.substr(lastSlash + 1);
                    }

                    // 拡張子を.tgaに変更してみる（モデルがpngを参照していてもtgaで提供されている場合）
                    std::string baseName = texPath;
                    size_t dotPos = baseName.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        baseName = baseName.substr(0, dotPos);
                    }

                    LOG_INFO("[AnimationTestScene]   Trying base name: " + baseName);

                    // 複数の拡張子を試す
                    std::vector<std::string> extensions = {".tga", ".png", ".tif", ".jpg"};
                    TextureHandle texHandle;
                    for (const auto& ext : extensions) {
                        std::string fullPath = textureDir + baseName + ext;
                        texHandle = TextureManager::Get().Load(fullPath);
                        if (texHandle.IsValid()) {
                            LOG_INFO("[AnimationTestScene]   Loaded: " + fullPath);
                            break;
                        }
                    }

                    if (texHandle.IsValid()) {
                        matDesc.textures[static_cast<size_t>(MaterialTextureSlot::Albedo)] = texHandle;
                    } else {
                        LOG_WARN("[AnimationTestScene]   Failed to load texture: " + baseName + " (tried .tga/.png/.tif/.jpg)");
                    }
                } else {
                    LOG_INFO("[AnimationTestScene]   No diffuse texture path");
                }
                auto mat = MaterialManager::Get().Create(matDesc);
                materials_.push_back(mat);
            }

            // デフォルトマテリアル（足りない場合）
            if (materials_.empty()) {
                materials_.push_back(MaterialManager::Get().CreateDefault());
            }

            // ボーン行列配列を初期化
            if (skeleton_) {
                size_t boneCount = skeleton_->GetBoneCount();
                localTransforms_.resize(boneCount, Matrix::Identity);
                globalTransforms_.resize(boneCount, Matrix::Identity);
                skinningMatrices_.resize(boneCount, Matrix::Identity);

                // 初期ポーズを設定
                for (size_t i = 0; i < boneCount; ++i) {
                    localTransforms_[i] = skeleton_->GetBone(static_cast<int>(i)).localBindPose;
                }

                // 初期状態でグローバル変換とスキニング行列を計算
                skeleton_->ComputeGlobalTransforms(localTransforms_, globalTransforms_);
                skeleton_->ComputeSkinningMatrices(globalTransforms_, skinningMatrices_);
                LOG_INFO("[AnimationTestScene] Bone matrices initialized");
            }
        } else {
            LOG_ERROR("[AnimationTestScene] Failed to load: " + result.errorMessage);
        }
    }

    void OnExit() override
    {
        LOG_INFO("[AnimationTestScene] Exit");
        skinnedMesh_.reset();
        skeleton_.reset();
        currentClip_.reset();
        materials_.clear();
        vertexShader_.reset();
        pixelShader_.reset();
        inputLayout_.Reset();
        perFrameBuffer_.reset();
        perObjectBuffer_.reset();
        boneMatricesBuffer_.reset();
        materialBuffer_.reset();
        lightingBuffer_.reset();
        shadowBuffer_.reset();
        linearSampler_.reset();
        shadowSampler_.reset();
    }

    void FixedUpdate(float dt) override
    {
        auto& keyboard = InputManager::Get().GetKeyboard();
        auto& mouse = InputManager::Get().GetMouse();

        // ESCでタイトルに戻る
        if (keyboard.IsKeyDown(Key::Escape)) {
            LOG_INFO("[AnimationTestScene] Returning to title...");
            SceneManager::Get().Load<TitleScene>();
            return;
        }

        // マウスでカメラ回転（右クリック中）
        if (mouse.IsButtonPressed(MouseButton::Right)) {
            float dx = static_cast<float>(mouse.GetDeltaX());
            float dy = static_cast<float>(mouse.GetDeltaY());
            cameraYaw_ += dx * 0.2f;
            cameraPitch_ += dy * 0.2f;
            cameraPitch_ = std::clamp(cameraPitch_, -60.0f, 60.0f);
        }

        // WASDでカメラ移動
        float moveSpeed = 3.0f * dt;
        Vector3 forward = GetCameraForward();
        Vector3 right = GetCameraRight();

        if (keyboard.IsKeyPressed(Key::W)) cameraPos_ += forward * moveSpeed;
        if (keyboard.IsKeyPressed(Key::S)) cameraPos_ -= forward * moveSpeed;
        if (keyboard.IsKeyPressed(Key::A)) cameraPos_ -= right * moveSpeed;
        if (keyboard.IsKeyPressed(Key::D)) cameraPos_ += right * moveSpeed;
        if (keyboard.IsKeyPressed(Key::Q)) cameraPos_.y -= moveSpeed;
        if (keyboard.IsKeyPressed(Key::E)) cameraPos_.y += moveSpeed;

        // アニメーション再生/停止
        if (keyboard.IsKeyDown(Key::Space)) {
            isPlaying_ = !isPlaying_;
            LOG_INFO(isPlaying_ ? "[AnimationTestScene] Play" : "[AnimationTestScene] Pause");
        }

        // Bキー: バインドポーズにリセット（アニメーションなしの状態を確認）
        if (keyboard.IsKeyDown(Key::B) && skeleton_) {
            LOG_INFO("[AnimationTestScene] Reset to BIND POSE (no animation)");
            for (size_t i = 0; i < localTransforms_.size(); ++i) {
                localTransforms_[i] = skeleton_->GetBone(static_cast<int>(i)).localBindPose;
            }
            skeleton_->ComputeGlobalTransforms(localTransforms_, globalTransforms_);
            skeleton_->ComputeSkinningMatrices(globalTransforms_, skinningMatrices_);
            isPlaying_ = false;
        }

        // Tキー: スキニング行列をすべてIdentityにする（メッシュが元の位置に表示されるはず）
        if (keyboard.IsKeyDown(Key::T) && skeleton_) {
            LOG_INFO("[AnimationTestScene] Set all SKINNING matrices to IDENTITY (mesh should appear at origin)");
            for (size_t i = 0; i < skinningMatrices_.size(); ++i) {
                skinningMatrices_[i] = Matrix::Identity;
            }
            isPlaying_ = false;
        }

        // Rキー: 左腕を90度回転（スキニングが機能しているかテスト）
        if (keyboard.IsKeyDown(Key::R) && skeleton_) {
            int leftArmIdx = skeleton_->FindBoneIndex("Character1_LeftArm");
            if (leftArmIdx >= 0) {
                LOG_INFO("[AnimationTestScene] Rotating LeftArm by 90 degrees (test)");
                // まずバインドポーズにリセット
                for (size_t i = 0; i < localTransforms_.size(); ++i) {
                    localTransforms_[i] = skeleton_->GetBone(static_cast<int>(i)).localBindPose;
                }
                // 左腕に90度回転を適用
                Matrix rotation = Matrix::CreateRotationZ(DirectX::XM_PIDIV2);  // 90度
                localTransforms_[leftArmIdx] = rotation * localTransforms_[leftArmIdx];

                skeleton_->ComputeGlobalTransforms(localTransforms_, globalTransforms_);
                skeleton_->ComputeSkinningMatrices(globalTransforms_, skinningMatrices_);
                isPlaying_ = false;
                LOG_INFO("[AnimationTestScene] LeftArm rotated. If arm doesn't move, skinning is broken!");
            } else {
                LOG_WARN("[AnimationTestScene] LeftArm bone not found!");
            }
        }

        // Pキー: 現在の状態を詳細出力
        if (keyboard.IsKeyDown(Key::P) && skeleton_) {
            LOG_INFO("[AnimationTestScene] === DETAILED STATE DUMP ===");
            LOG_INFO("Total animations: " + std::to_string(skinnedMesh_->GetAnimationCount()));
            LOG_INFO("Current animation index: " + std::to_string(currentClipIndex_));
            if (currentClip_) {
                LOG_INFO("Current clip: " + currentClip_->name);
                LOG_INFO("  Duration: " + std::to_string(currentClip_->duration) + "s");
                LOG_INFO("  Channels: " + std::to_string(currentClip_->channels.size()));

                // サンプリングして最初の5チャンネルの値を出力
                float testTime = 0.0f;
                LOG_INFO("  Keyframe values at time 0:");
                for (size_t i = 0; i < currentClip_->channels.size() && i < 5; ++i) {
                    const auto& ch = currentClip_->channels[i];
                    if (!ch.positionKeys.empty()) {
                        const auto& pk = ch.positionKeys[0];
                        LOG_INFO("    [" + std::to_string(i) + "] " + ch.boneName +
                                 " pos=(" + std::to_string(pk.value.x) + "," +
                                 std::to_string(pk.value.y) + "," +
                                 std::to_string(pk.value.z) + ")");
                    }
                    if (!ch.rotationKeys.empty()) {
                        const auto& rk = ch.rotationKeys[0];
                        LOG_INFO("    [" + std::to_string(i) + "] " + ch.boneName +
                                 " rot=(" + std::to_string(rk.value.x) + "," +
                                 std::to_string(rk.value.y) + "," +
                                 std::to_string(rk.value.z) + "," +
                                 std::to_string(rk.value.w) + ")");
                    }
                }
            }

            // スケルトンの最初の5ボーン
            LOG_INFO("Skeleton bones (first 5):");
            int maxBones = (skeleton_->GetBoneCount() < 5) ? static_cast<int>(skeleton_->GetBoneCount()) : 5;
            for (int i = 0; i < maxBones; ++i) {
                const Bone& bone = skeleton_->GetBone(i);
                LOG_INFO("  [" + std::to_string(i) + "] " + bone.name +
                         " parent=" + std::to_string(bone.parentIndex));
            }
        }

        // アニメーション切り替え
        if (keyboard.IsKeyDown(Key::N) && skinnedMesh_) {
            const auto& clips = skinnedMesh_->GetAnimations();
            if (!clips.empty()) {
                currentClipIndex_ = (currentClipIndex_ + 1) % clips.size();
                currentClip_ = clips[currentClipIndex_];
                animationTime_ = 0.0f;
                LOG_INFO("[AnimationTestScene] === Switched to animation " + std::to_string(currentClipIndex_) + " ===");
                LOG_INFO("[AnimationTestScene] Name: " + currentClip_->name);
                LOG_INFO("[AnimationTestScene] Duration: " + std::to_string(currentClip_->duration) + "s");
                LOG_INFO("[AnimationTestScene] Channels: " + std::to_string(currentClip_->channels.size()));
                LOG_INFO("[AnimationTestScene] Skeleton bones: " + std::to_string(skeleton_->GetBoneCount()));

                // 有効なチャンネル数をカウント
                int validChannels = 0;
                int invalidChannels = 0;
                for (const auto& ch : currentClip_->channels) {
                    if (ch.boneIndex >= 0 && ch.boneIndex < static_cast<int>(skeleton_->GetBoneCount())) {
                        validChannels++;
                    } else {
                        invalidChannels++;
                    }
                }
                LOG_INFO("[AnimationTestScene] Valid channels: " + std::to_string(validChannels) +
                         ", Invalid: " + std::to_string(invalidChannels));

                // 部分的なキーを持つチャンネルをチェック
                int partialKeyChannels = 0;
                for (const auto& ch : currentClip_->channels) {
                    bool hasPos = !ch.positionKeys.empty();
                    bool hasRot = !ch.rotationKeys.empty();
                    bool hasScl = !ch.scaleKeys.empty();
                    if ((hasPos || hasRot || hasScl) && !(hasPos && hasRot && hasScl)) {
                        partialKeyChannels++;
                        if (partialKeyChannels <= 3) {
                            LOG_WARN("[AnimationTestScene] Partial keys: " + ch.boneName +
                                     " (pos:" + std::string(hasPos ? "Y" : "N") +
                                     " rot:" + std::string(hasRot ? "Y" : "N") +
                                     " scl:" + std::string(hasScl ? "Y" : "N") + ")");
                        }
                    }
                }
                if (partialKeyChannels > 0) {
                    LOG_WARN("[AnimationTestScene] WARNING: " + std::to_string(partialKeyChannels) +
                             " channels have partial keys - this may cause bones to collapse!");
                }

                // 最初の10チャンネルの詳細
                for (size_t i = 0; i < currentClip_->channels.size() && i < 10; ++i) {
                    const auto& ch = currentClip_->channels[i];
                    std::string boneName = (ch.boneIndex >= 0 && ch.boneIndex < static_cast<int>(skeleton_->GetBoneCount()))
                        ? skeleton_->GetBone(ch.boneIndex).name
                        : "INVALID";
                    LOG_INFO("  [" + std::to_string(i) + "] " + ch.boneName +
                             " -> bone[" + std::to_string(ch.boneIndex) + "] " + boneName +
                             " (pos:" + std::to_string(ch.positionKeys.size()) +
                             " rot:" + std::to_string(ch.rotationKeys.size()) +
                             " scl:" + std::to_string(ch.scaleKeys.size()) + ")");
                }
            }
        }

        // アニメーション更新
        if (isPlaying_ && currentClip_ && skeleton_) {
            animationTime_ += dt * playbackSpeed_;

            // ループ
            if (animationTime_ > currentClip_->duration) {
                animationTime_ = std::fmod(animationTime_, currentClip_->duration);
            }

            // 重要: サンプリング前にバインドポーズにリセット
            // アニメーションチャンネルがないボーンは元のポーズを維持する必要がある
            for (size_t i = 0; i < localTransforms_.size(); ++i) {
                localTransforms_[i] = skeleton_->GetBone(static_cast<int>(i)).localBindPose;
            }

            // アニメーションをサンプリング（チャンネルがあるボーンのみ上書き）
            size_t channelsBefore = currentClip_->channels.size();
            currentClip_->SamplePose(animationTime_, localTransforms_);

            // 毎秒1回、実際に更新されているか確認
            static int frameCount = 0;
            frameCount++;
            if (frameCount % 60 == 0) {
                // Hipsボーンの位置を確認（アニメーションで変化しているはず）
                int hipsIdx = skeleton_->FindBoneIndex("Character1_Hips");
                if (hipsIdx >= 0) {
                    const Matrix& hipsPose = localTransforms_[hipsIdx];
                    LOG_INFO("[AnimationTestScene] Frame " + std::to_string(frameCount) +
                             ": Hips pos=(" + std::to_string(hipsPose._41) + "," +
                             std::to_string(hipsPose._42) + "," + std::to_string(hipsPose._43) +
                             ") time=" + std::to_string(animationTime_) +
                             " clip=" + currentClip_->name +
                             " channels=" + std::to_string(channelsBefore));
                }
            }

            // グローバル変換を計算
            skeleton_->ComputeGlobalTransforms(localTransforms_, globalTransforms_);

            // スキニング行列を計算
            skeleton_->ComputeSkinningMatrices(globalTransforms_, skinningMatrices_);

            // デバッグ: 1秒ごとにアニメーション情報を出力
            static float debugTimer = 0.0f;
            debugTimer += dt;
            if (debugTimer >= 2.0f) {
                debugTimer = 0.0f;
                LOG_INFO("[AnimationTestScene] === Animation Debug ===");
                LOG_INFO("[AnimationTestScene] Clip: " + currentClip_->name +
                         " time=" + std::to_string(animationTime_) + "/" + std::to_string(currentClip_->duration));

                // 複数のボーンの変換を比較（バインドポーズ vs アニメーション後）
                std::vector<int> debugBones;
                // Hips (root)
                int hipsIdx = skeleton_->FindBoneIndex("Character1_Hips");
                if (hipsIdx >= 0) debugBones.push_back(hipsIdx);
                // LeftArm
                int leftArmIdx = skeleton_->FindBoneIndex("Character1_LeftArm");
                if (leftArmIdx >= 0) debugBones.push_back(leftArmIdx);
                // Head
                int headIdx = skeleton_->FindBoneIndex("Character1_Head");
                if (headIdx >= 0) debugBones.push_back(headIdx);

                for (int boneIdx : debugBones) {
                    const Bone& bone = skeleton_->GetBone(boneIdx);
                    const Matrix& bindPose = bone.localBindPose;
                    const Matrix& animPose = localTransforms_[boneIdx];
                    const Matrix& globalPose = globalTransforms_[boneIdx];
                    const Matrix& skinMat = skinningMatrices_[boneIdx];

                    // バインドポーズとアニメーションポーズの差を計算
                    float posDiff = std::sqrt(
                        std::pow(animPose._41 - bindPose._41, 2) +
                        std::pow(animPose._42 - bindPose._42, 2) +
                        std::pow(animPose._43 - bindPose._43, 2));

                    LOG_INFO("  [" + bone.name + "] idx=" + std::to_string(boneIdx) +
                             " parent=" + std::to_string(bone.parentIndex));
                    LOG_INFO("    BindPose pos: (" + std::to_string(bindPose._41) + ", " +
                             std::to_string(bindPose._42) + ", " + std::to_string(bindPose._43) + ")");
                    LOG_INFO("    AnimPose pos: (" + std::to_string(animPose._41) + ", " +
                             std::to_string(animPose._42) + ", " + std::to_string(animPose._43) + ")");
                    LOG_INFO("    Position diff: " + std::to_string(posDiff));
                    LOG_INFO("    GlobalPose pos: (" + std::to_string(globalPose._41) + ", " +
                             std::to_string(globalPose._42) + ", " + std::to_string(globalPose._43) + ")");

                    // inverseBindMatrixがIdentityかどうかチェック
                    bool isIdentity = (bone.inverseBindMatrix._11 == 1.0f &&
                                       bone.inverseBindMatrix._22 == 1.0f &&
                                       bone.inverseBindMatrix._33 == 1.0f &&
                                       bone.inverseBindMatrix._44 == 1.0f &&
                                       bone.inverseBindMatrix._41 == 0.0f &&
                                       bone.inverseBindMatrix._42 == 0.0f &&
                                       bone.inverseBindMatrix._43 == 0.0f);
                    LOG_INFO("    InverseBindMatrix is Identity: " + std::string(isIdentity ? "YES (PROBLEM!)" : "no"));
                }

                // アニメーションチャンネルのキーフレーム値を確認
                if (currentClip_->channels.size() > 1) {
                    const auto& ch = currentClip_->channels[1];  // 2番目のチャンネル
                    if (!ch.rotationKeys.empty()) {
                        const auto& firstKey = ch.rotationKeys.front();
                        const auto& lastKey = ch.rotationKeys.back();
                        LOG_INFO("  Channel[1] '" + ch.boneName + "' rotation range:");
                        LOG_INFO("    First: (" + std::to_string(firstKey.value.x) + ", " +
                                 std::to_string(firstKey.value.y) + ", " +
                                 std::to_string(firstKey.value.z) + ", " +
                                 std::to_string(firstKey.value.w) + ")");
                        LOG_INFO("    Last:  (" + std::to_string(lastKey.value.x) + ", " +
                                 std::to_string(lastKey.value.y) + ", " +
                                 std::to_string(lastKey.value.z) + ", " +
                                 std::to_string(lastKey.value.w) + ")");
                    }
                }
            }
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

        // 明るい青の背景（デバッグ用）
        float clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
        ctx.ClearRenderTarget(backBuffer, clearColor);
        ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

        // ビュー・プロジェクション行列
        Matrix view = LH::CreateLookAt(cameraPos_, cameraPos_ + GetCameraForward(), Vector3::Up);
        Matrix proj = LH::CreatePerspectiveFov(
            DirectX::XMConvertToRadians(60.0f),
            width / height,
            0.1f, 1000.0f);
        Matrix viewProj = view * proj;

        // スキンメッシュ描画
        if (skinnedMesh_ && vertexShader_ && pixelShader_ && inputLayout_) {
            RenderSkinnedMesh(ctx, viewProj);
        } else {
            static bool logged = false;
            if (!logged) {
                LOG_ERROR("[AnimationTestScene] Render skipped: mesh=" + std::to_string(skinnedMesh_ != nullptr)
                    + " vs=" + std::to_string(vertexShader_ != nullptr)
                    + " ps=" + std::to_string(pixelShader_ != nullptr)
                    + " il=" + std::to_string(inputLayout_ != nullptr));
                logged = true;
            }
        }
    }

private:
    void RenderSkinnedMesh(GraphicsContext& ctx, const Matrix& viewProj)
    {
        auto* d3dCtx = ctx.GetContext();
        if (!d3dCtx) return;

        // レンダーステート設定
        RenderStateManager& rsm = RenderStateManager::Get();
        ctx.SetDepthStencilState(rsm.GetDepthDefault());
        ctx.SetRasterizerState(rsm.GetNoCull());  // 両面描画でデバッグ
        ctx.SetBlendState(rsm.GetOpaque());

        // ワールド行列（モデル位置調整）
        // Unity FBXは座標系が異なるので回転で調整
        // X軸で180度回転して上下反転を修正
        // FBXはcm単位なので0.01倍にスケールダウン
        Matrix scale = Matrix::CreateScale(0.01f);
        Matrix rotation = Matrix::CreateRotationX(DirectX::XM_PI);
        Matrix world = scale * rotation;
        Matrix worldInvTranspose = world.Invert().Transpose();

        // PerFrame定数バッファ更新
        PerFrameCB perFrame;
        perFrame.viewProjection = viewProj.Transpose();  // HLSL row_major
        perFrame.cameraPosition = Vector4(cameraPos_.x, cameraPos_.y, cameraPos_.z, 1.0f);
        ctx.UpdateBuffer(perFrameBuffer_.get(), &perFrame);

        // PerObject定数バッファ更新
        PerObjectCB perObject;
        perObject.world = world.Transpose();
        perObject.worldInvTranspose = worldInvTranspose.Transpose();
        ctx.UpdateBuffer(perObjectBuffer_.get(), &perObject);

        // BoneMatrices定数バッファ更新
        BoneMatricesCB bonesCB;
        std::memset(&bonesCB, 0, sizeof(bonesCB));
        size_t boneCount = (std::min)(skinningMatrices_.size(), MAX_BONES);
        for (size_t i = 0; i < boneCount; ++i) {
            bonesCB.bones[i] = skinningMatrices_[i].Transpose();  // HLSL row_major
        }
        ctx.UpdateBuffer(boneMatricesBuffer_.get(), &bonesCB);

        // シェーダー設定
        ctx.SetVertexShader(vertexShader_.get());
        ctx.SetPixelShader(pixelShader_.get());
        ctx.SetInputLayout(inputLayout_.Get());

        // 頂点シェーダー定数バッファをバインド
        ID3D11Buffer* vsBuffers[] = {
            perFrameBuffer_->Get(),
            perObjectBuffer_->Get(),
            boneMatricesBuffer_->Get()
        };
        d3dCtx->VSSetConstantBuffers(0, 3, vsBuffers);

        // Lighting定数バッファ更新（b3）
        LightingCB lighting;
        std::memset(&lighting, 0, sizeof(lighting));
        lighting.lightCameraPosition = Vector4(cameraPos_.x, cameraPos_.y, cameraPos_.z, 1.0f);
        lighting.ambientColor = Vector4(0.4f, 0.4f, 0.5f, 1.0f);
        lighting.numLights = 1;
        // ディレクショナルライト
        Vector3 lightDir(0.3f, -1.0f, 0.5f);
        lightDir.Normalize();
        lighting.lights[0].position = Vector4(0.0f, 0.0f, 0.0f, 0.0f);  // w=0 = directional
        lighting.lights[0].direction = Vector4(lightDir.x, lightDir.y, lightDir.z, 100.0f);
        lighting.lights[0].color = Vector4(1.0f, 1.0f, 1.0f, 1.2f);  // a = intensity
        lighting.lights[0].spotParams = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        ctx.UpdateBuffer(lightingBuffer_.get(), &lighting);

        // Shadow定数バッファ更新（b4）- シャドウ無効
        ShadowCB shadow;
        shadow.lightViewProjection = Matrix::Identity;
        shadow.shadowParams = Vector4(0.001f, 0.01f, 0.5f, 0.0f);  // w=0 = シャドウ無効
        ctx.UpdateBuffer(shadowBuffer_.get(), &shadow);

        // ピクセルシェーダー定数バッファをバインド
        ID3D11Buffer* psBuffers[] = {
            perFrameBuffer_->Get(),    // b0: PerFrame
            nullptr,                    // b1: 未使用
            materialBuffer_->Get(),     // b2: Material
            lightingBuffer_->Get(),     // b3: Lighting
            shadowBuffer_->Get()        // b4: Shadow
        };
        d3dCtx->PSSetConstantBuffers(0, 5, psBuffers);

        // サンプラーステートをバインド
        ID3D11SamplerState* samplers[] = {
            linearSampler_ ? linearSampler_->GetD3DSamplerState() : nullptr,  // s0
            shadowSampler_ ? shadowSampler_->GetD3DSamplerState() : nullptr   // s1
        };
        d3dCtx->PSSetSamplers(0, 2, samplers);

        // 頂点/インデックスバッファ設定
        auto* vb = skinnedMesh_->GetVertexBuffer();
        auto* ib = skinnedMesh_->GetIndexBuffer();
        if (!vb || !ib) {
            static bool logged = false;
            if (!logged) {
                LOG_ERROR("[AnimationTestScene] Missing VB or IB: vb=" + std::to_string(vb != nullptr) + " ib=" + std::to_string(ib != nullptr));
                logged = true;
            }
            return;
        }

        UINT stride = GetSkinnedMeshVertexStride();
        UINT offset = 0;
        ID3D11Buffer* vbPtr = vb->Get();
        d3dCtx->IASetVertexBuffers(0, 1, &vbPtr, &stride, &offset);
        d3dCtx->IASetIndexBuffer(ib->Get(), DXGI_FORMAT_R32_UINT, 0);
        d3dCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // サブメッシュごとに描画
        const auto& subMeshes = skinnedMesh_->GetSubMeshes();
        static bool drawLogged = false;
        if (!drawLogged) {
            LOG_INFO("[AnimationTestScene] Drawing " + std::to_string(subMeshes.size()) + " submeshes");
            drawLogged = true;
        }

        for (size_t i = 0; i < subMeshes.size(); ++i) {
            const auto& subMesh = subMeshes[i];

            // マテリアル取得
            MaterialHandle matHandle = (i < materials_.size()) ? materials_[i] : materials_[0];

            // テクスチャをバインド
            TextureHandle albedoHandle = MaterialManager::Get().GetTexture(matHandle, MaterialTextureSlot::Albedo);
            Texture* albedoTex = TextureManager::Get().Get(albedoHandle);

            // Material定数バッファ更新（テクスチャフラグ付き）
            MaterialCB mat;
            mat.albedoColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            mat.metallic = 0.0f;
            mat.roughness = 0.5f;
            mat.ao = 1.0f;
            mat.emissiveStrength = 0.0f;
            mat.emissiveColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            mat.useAlbedoMap = (albedoTex && albedoTex->HasSrv()) ? 1 : 0;
            mat.useNormalMap = 0;
            mat.useMetallicMap = 0;
            mat.useRoughnessMap = 0;
            ctx.UpdateBuffer(materialBuffer_.get(), &mat);

            // テクスチャSRVをバインド (t0)
            if (albedoTex && albedoTex->HasSrv()) {
                ID3D11ShaderResourceView* srvs[] = { albedoTex->Srv() };
                d3dCtx->PSSetShaderResources(0, 1, srvs);
            } else {
                ID3D11ShaderResourceView* nullSrv[] = { nullptr };
                d3dCtx->PSSetShaderResources(0, 1, nullSrv);
            }

            // 描画
            d3dCtx->DrawIndexed(subMesh.indexCount, subMesh.indexOffset, 0);
        }
    }

    [[nodiscard]] Vector3 GetCameraForward() const {
        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        float pitchRad = DirectX::XMConvertToRadians(cameraPitch_);
        return Vector3(
            std::sin(yawRad) * std::cos(pitchRad),
            -std::sin(pitchRad),
            std::cos(yawRad) * std::cos(pitchRad)
        );
    }

    [[nodiscard]] Vector3 GetCameraRight() const {
        float yawRad = DirectX::XMConvertToRadians(cameraYaw_);
        return Vector3(std::cos(yawRad), 0, -std::sin(yawRad));
    }

private:
    // モデルデータ
    SkinnedMeshPtr skinnedMesh_;
    SkeletonPtr skeleton_;
    std::vector<MaterialHandle> materials_;

    // シェーダー
    ShaderPtr vertexShader_;
    ShaderPtr pixelShader_;
    ComPtr<ID3D11InputLayout> inputLayout_;

    // 定数バッファ（VS用）
    BufferPtr perFrameBuffer_;
    BufferPtr perObjectBuffer_;
    BufferPtr boneMatricesBuffer_;

    // 定数バッファ（PS用）
    BufferPtr materialBuffer_;
    BufferPtr lightingBuffer_;
    BufferPtr shadowBuffer_;

    // サンプラーステート
    std::unique_ptr<SamplerState> linearSampler_;
    std::unique_ptr<SamplerState> shadowSampler_;

    // アニメーション
    AnimationClipPtr currentClip_;
    size_t currentClipIndex_ = 0;
    float animationTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool isPlaying_ = true;

    // ボーン行列
    std::vector<Matrix> localTransforms_;
    std::vector<Matrix> globalTransforms_;
    std::vector<Matrix> skinningMatrices_;

    // カメラ
    Vector3 cameraPos_;
    float cameraYaw_ = 0.0f;
    float cameraPitch_ = 0.0f;
};
