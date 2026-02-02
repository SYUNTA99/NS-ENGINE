//----------------------------------------------------------------------------
//! @file   skinned_mesh_renderer.h
//! @brief  SkinnedMeshRenderer - スキンメッシュ描画コンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "engine/mesh/skinned_mesh.h"
#include "transform.h"
#include "animator.h"
#include <vector>
#include <memory>

//============================================================================
//! @brief SkinnedMeshRenderer - スキンメッシュ描画コンポーネント
//!
//! Animatorコンポーネントと連携してボーンアニメーション付き
//! メッシュを描画する。Animatorからスキニング行列を取得し、
//! GPUの定数バッファに送信する。
//!
//! @code
//! // スキンメッシュを読み込み
//! auto result = SkinnedMeshLoader::Load("model:/character.fbx");
//!
//! // GameObjectに追加
//! auto* go = world.CreateGameObject("Character");
//! go->AddComponent<Transform>();
//!
//! // Animatorを追加
//! auto* animator = go->AddComponent<Animator>();
//! animator->SetSkeleton(result.mesh->GetSkeleton());
//! animator->SetController(characterController);
//!
//! // SkinnedMeshRendererを追加
//! auto* renderer = go->AddComponent<SkinnedMeshRenderer>();
//! renderer->SetSkinnedMesh(result.mesh);
//! renderer->SetMaterial(characterMaterial);
//! @endcode
//============================================================================
class SkinnedMeshRenderer final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    SkinnedMeshRenderer() = default;

    explicit SkinnedMeshRenderer(SkinnedMeshPtr mesh)
        : skinnedMesh_(std::move(mesh)) {}

    SkinnedMeshRenderer(SkinnedMeshPtr mesh, uint32_t materialHandle)
        : skinnedMesh_(std::move(mesh)) {
        materials_.push_back(materialHandle);
    }

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
        animator_ = GetComponent<Animator>();

        // Animatorがあればスケルトンを自動設定
        if (animator_ && skinnedMesh_ && skinnedMesh_->GetSkeleton()) {
            if (!animator_->GetSkeleton()) {
                animator_->SetSkeleton(skinnedMesh_->GetSkeleton());
            }
        }
    }

    void LateUpdate([[maybe_unused]] float dt) override {
        // スキニング行列を更新
        UpdateBoneMatrices();
    }

    //========================================================================
    // メッシュ
    //========================================================================

    [[nodiscard]] SkinnedMeshPtr GetSkinnedMesh() const noexcept { return skinnedMesh_; }

    void SetSkinnedMesh(SkinnedMeshPtr mesh) {
        skinnedMesh_ = std::move(mesh);

        // スケルトンをAnimatorに設定
        if (animator_ && skinnedMesh_ && skinnedMesh_->GetSkeleton()) {
            if (!animator_->GetSkeleton()) {
                animator_->SetSkeleton(skinnedMesh_->GetSkeleton());
            }
        }
    }

    //========================================================================
    // マテリアル
    //========================================================================

    [[nodiscard]] size_t GetMaterialCount() const noexcept { return materials_.size(); }

    [[nodiscard]] uint32_t GetMaterial(size_t index = 0) const {
        return index < materials_.size() ? materials_[index] : 0;
    }

    [[nodiscard]] const std::vector<uint32_t>& GetMaterials() const noexcept {
        return materials_;
    }

    void SetMaterial(uint32_t handle, size_t index = 0) {
        if (index >= materials_.size()) {
            materials_.resize(index + 1, 0);
        }
        materials_[index] = handle;
    }

    void SetMaterials(const std::vector<uint32_t>& materials) {
        materials_ = materials;
    }

    void AddMaterial(uint32_t handle) {
        materials_.push_back(handle);
    }

    void ClearMaterials() {
        materials_.clear();
    }

    //========================================================================
    // 影
    //========================================================================

    [[nodiscard]] bool GetCastShadows() const noexcept { return castShadows_; }
    [[nodiscard]] bool GetReceiveShadows() const noexcept { return receiveShadows_; }

    void SetCastShadows(bool cast) noexcept { castShadows_ = cast; }
    void SetReceiveShadows(bool receive) noexcept { receiveShadows_ = receive; }

    //========================================================================
    // 表示
    //========================================================================

    [[nodiscard]] bool IsVisible() const noexcept { return isVisible_; }
    void SetVisible(bool visible) noexcept { isVisible_ = visible; }

    //========================================================================
    // バウンディングボックス
    //========================================================================

    void GetWorldBounds(Vector3& min, Vector3& max) const {
        if (!skinnedMesh_ || !transform_) {
            min = Vector3::Zero;
            max = Vector3::Zero;
            return;
        }

        // ローカルバウンディングボックスを取得
        const auto& localBounds = skinnedMesh_->GetBounds();

        // 8頂点を変換してAABBを再計算
        Matrix worldMatrix = transform_->GetLocalMatrix();

        Vector3 corners[8] = {
            Vector3(localBounds.min.x, localBounds.min.y, localBounds.min.z),
            Vector3(localBounds.max.x, localBounds.min.y, localBounds.min.z),
            Vector3(localBounds.min.x, localBounds.max.y, localBounds.min.z),
            Vector3(localBounds.max.x, localBounds.max.y, localBounds.min.z),
            Vector3(localBounds.min.x, localBounds.min.y, localBounds.max.z),
            Vector3(localBounds.max.x, localBounds.min.y, localBounds.max.z),
            Vector3(localBounds.min.x, localBounds.max.y, localBounds.max.z),
            Vector3(localBounds.max.x, localBounds.max.y, localBounds.max.z)
        };

        min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
        max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (int i = 0; i < 8; ++i) {
            Vector3 worldCorner = Vector3::Transform(corners[i], worldMatrix);
            min = Vector3::Min(min, worldCorner);
            max = Vector3::Max(max, worldCorner);
        }
    }

    //========================================================================
    // レイヤー
    //========================================================================

    [[nodiscard]] uint32_t GetRenderLayer() const noexcept { return renderLayer_; }
    void SetRenderLayer(uint32_t layer) noexcept { renderLayer_ = layer; }

    //========================================================================
    // スキニング行列（レンダラー用）
    //========================================================================

    //! @brief スキニング行列を取得（GPU送信用）
    [[nodiscard]] const std::vector<Matrix>& GetBoneMatrices() const noexcept {
        return boneMatrices_;
    }

    //! @brief ボーン数を取得
    [[nodiscard]] size_t GetBoneCount() const noexcept {
        return boneMatrices_.size();
    }

    //========================================================================
    // 描画情報取得（レンダラー用）
    //========================================================================

    //! @brief 描画用のワールド行列を取得
    [[nodiscard]] Matrix GetRenderMatrix() const {
        if (!transform_) return Matrix::Identity;
        return transform_->GetLocalMatrix();
    }

    //! @brief Animatorを取得
    [[nodiscard]] Animator* GetAnimator() const noexcept { return animator_; }

    //! @brief Animatorを設定
    void SetAnimator(Animator* animator) { animator_ = animator; }

private:
    void UpdateBoneMatrices() {
        if (!animator_) {
            // Animatorがない場合は単位行列
            if (skinnedMesh_) {
                size_t boneCount = skinnedMesh_->GetBoneCount();
                if (boneMatrices_.size() != boneCount) {
                    boneMatrices_.resize(boneCount, Matrix::Identity);
                }
            }
            return;
        }

        // Animatorからスキニング行列を取得
        const auto& skinningMatrices = animator_->GetSkinningMatrices();
        boneMatrices_ = skinningMatrices;
    }

    Transform* transform_ = nullptr;
    Animator* animator_ = nullptr;

    // メッシュ・マテリアル
    SkinnedMeshPtr skinnedMesh_;
    std::vector<uint32_t> materials_;

    // 影
    bool castShadows_ = true;
    bool receiveShadows_ = true;

    // 表示
    bool isVisible_ = true;

    // レイヤー
    uint32_t renderLayer_ = 1;

    // スキニング行列キャッシュ
    std::vector<Matrix> boneMatrices_;
};

OOP_COMPONENT(SkinnedMeshRenderer);
