//----------------------------------------------------------------------------
//! @file   mesh_renderer.h
//! @brief  MeshRenderer - 純粋OOP 3D描画コンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"
#include <vector>
#include <string>

//============================================================================
//! @brief シャドウモード
//============================================================================
enum class ShadowCastingMode {
    Off,            //!< 影を落とさない
    On,             //!< 影を落とす
    TwoSided,       //!< 両面で影を落とす
    ShadowsOnly     //!< 影のみ（本体は描画しない）
};

//============================================================================
//! @brief MeshRenderer - 3D描画コンポーネント
//!
//! Unity MonoBehaviour風の設計。3Dメッシュ描画の設定を管理。
//!
//! @code
//! auto* go = world.CreateGameObject("Cube");
//! go->AddComponent<Transform>();
//! auto* mr = go->AddComponent<MeshRenderer>();
//!
//! mr->SetMesh(cubeMeshHandle);
//! mr->SetMaterial(defaultMaterial);
//! mr->SetCastShadows(ShadowCastingMode::On);
//! mr->SetReceiveShadows(true);
//! @endcode
//============================================================================
class MeshRenderer final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    MeshRenderer() = default;

    explicit MeshRenderer(uint32_t meshHandle)
        : meshHandle_(meshHandle) {}

    MeshRenderer(uint32_t meshHandle, uint32_t materialHandle)
        : meshHandle_(meshHandle) {
        materials_.push_back(materialHandle);
    }

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
    }

    //========================================================================
    // メッシュ
    //========================================================================

    [[nodiscard]] uint32_t GetMesh() const noexcept { return meshHandle_; }
    void SetMesh(uint32_t handle) noexcept { meshHandle_ = handle; }

    //========================================================================
    // マテリアル
    //========================================================================

    //! @brief マテリアル数を取得
    [[nodiscard]] size_t GetMaterialCount() const noexcept { return materials_.size(); }

    //! @brief マテリアルを取得
    [[nodiscard]] uint32_t GetMaterial(size_t index = 0) const {
        return index < materials_.size() ? materials_[index] : 0;
    }

    //! @brief 全マテリアルを取得
    [[nodiscard]] const std::vector<uint32_t>& GetMaterials() const noexcept {
        return materials_;
    }

    //! @brief マテリアルを設定
    void SetMaterial(uint32_t handle, size_t index = 0) {
        if (index >= materials_.size()) {
            materials_.resize(index + 1, 0);
        }
        materials_[index] = handle;
    }

    //! @brief 全マテリアルを設定
    void SetMaterials(const std::vector<uint32_t>& materials) {
        materials_ = materials;
    }

    //! @brief マテリアルを追加
    void AddMaterial(uint32_t handle) {
        materials_.push_back(handle);
    }

    //! @brief マテリアルをクリア
    void ClearMaterials() {
        materials_.clear();
    }

    //========================================================================
    // 影
    //========================================================================

    [[nodiscard]] ShadowCastingMode GetShadowCastingMode() const noexcept {
        return shadowCastingMode_;
    }
    [[nodiscard]] bool GetReceiveShadows() const noexcept { return receiveShadows_; }

    void SetShadowCastingMode(ShadowCastingMode mode) noexcept {
        shadowCastingMode_ = mode;
    }
    void SetCastShadows(ShadowCastingMode mode) noexcept {
        shadowCastingMode_ = mode;
    }
    void SetReceiveShadows(bool receive) noexcept { receiveShadows_ = receive; }

    //========================================================================
    // 表示
    //========================================================================

    [[nodiscard]] bool IsVisible() const noexcept { return isVisible_; }
    void SetVisible(bool visible) noexcept { isVisible_ = visible; }

    //========================================================================
    // ライティング
    //========================================================================

    [[nodiscard]] bool GetUseLighting() const noexcept { return useLighting_; }
    void SetUseLighting(bool use) noexcept { useLighting_ = use; }

    //========================================================================
    // バウンディングボックス
    //========================================================================

    //! @brief ローカル空間でのバウンディングボックスを設定
    void SetLocalBounds(const Vector3& min, const Vector3& max) {
        localBoundsMin_ = min;
        localBoundsMax_ = max;
    }

    //! @brief ローカル空間でのバウンディングボックスを取得
    void GetLocalBounds(Vector3& min, Vector3& max) const {
        min = localBoundsMin_;
        max = localBoundsMax_;
    }

    //! @brief ワールド空間でのバウンディングボックスを取得
    void GetWorldBounds(Vector3& min, Vector3& max) const {
        if (!transform_) {
            min = localBoundsMin_;
            max = localBoundsMax_;
            return;
        }

        // 8頂点を変換してAABBを再計算
        Matrix world = transform_->GetLocalMatrix();

        Vector3 corners[8] = {
            Vector3(localBoundsMin_.x, localBoundsMin_.y, localBoundsMin_.z),
            Vector3(localBoundsMax_.x, localBoundsMin_.y, localBoundsMin_.z),
            Vector3(localBoundsMin_.x, localBoundsMax_.y, localBoundsMin_.z),
            Vector3(localBoundsMax_.x, localBoundsMax_.y, localBoundsMin_.z),
            Vector3(localBoundsMin_.x, localBoundsMin_.y, localBoundsMax_.z),
            Vector3(localBoundsMax_.x, localBoundsMin_.y, localBoundsMax_.z),
            Vector3(localBoundsMin_.x, localBoundsMax_.y, localBoundsMax_.z),
            Vector3(localBoundsMax_.x, localBoundsMax_.y, localBoundsMax_.z)
        };

        min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
        max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (int i = 0; i < 8; ++i) {
            Vector3 worldCorner = Vector3::Transform(corners[i], world);
            min = Vector3::Min(min, worldCorner);
            max = Vector3::Max(max, worldCorner);
        }
    }

    //========================================================================
    // LOD
    //========================================================================

    [[nodiscard]] int GetLODLevel() const noexcept { return lodLevel_; }
    void SetLODLevel(int level) noexcept { lodLevel_ = level; }

    //========================================================================
    // レイヤー
    //========================================================================

    [[nodiscard]] uint32_t GetRenderLayer() const noexcept { return renderLayer_; }
    void SetRenderLayer(uint32_t layer) noexcept { renderLayer_ = layer; }

    //========================================================================
    // 描画情報取得（レンダラー用）
    //========================================================================

    //! @brief 描画用のワールド行列を取得
    [[nodiscard]] Matrix GetRenderMatrix() const {
        if (!transform_) return Matrix::Identity;
        return transform_->GetLocalMatrix();
    }

private:
    Transform* transform_ = nullptr;

    // メッシュ・マテリアル
    uint32_t meshHandle_ = 0;
    std::vector<uint32_t> materials_;

    // 影
    ShadowCastingMode shadowCastingMode_ = ShadowCastingMode::On;
    bool receiveShadows_ = true;

    // 表示
    bool isVisible_ = true;
    bool useLighting_ = true;

    // バウンディングボックス
    Vector3 localBoundsMin_ = Vector3(-0.5f, -0.5f, -0.5f);
    Vector3 localBoundsMax_ = Vector3(0.5f, 0.5f, 0.5f);

    // LOD
    int lodLevel_ = 0;

    // レイヤー
    uint32_t renderLayer_ = 1;  // デフォルトレイヤー
};

OOP_COMPONENT(MeshRenderer);
