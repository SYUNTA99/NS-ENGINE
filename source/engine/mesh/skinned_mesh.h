//----------------------------------------------------------------------------
//! @file   skinned_mesh.h
//! @brief  SkinnedMesh - スキンメッシュクラス（ボーンアニメーション対応）
//----------------------------------------------------------------------------
#pragma once


#include "mesh.h"
#include "vertex_format.h"
#include "engine/game_object/components/animation/skeleton.h"
#include "engine/game_object/components/animation/animation_clip.h"
#include "engine/material/material.h"
#include "dx11/gpu/buffer.h"
#include <vector>
#include <string>
#include <memory>

//============================================================================
//! @brief スキンメッシュ記述子
//============================================================================
struct SkinnedMeshDesc
{
    std::vector<SkinnedMeshVertex> vertices;    //!< スキン頂点データ
    std::vector<uint32_t> indices;              //!< インデックスデータ
    std::vector<SubMesh> subMeshes;             //!< サブメッシュ配列
    BoundingBox bounds;                         //!< バウンディングボックス
    std::string name;                           //!< メッシュ名

    SkeletonPtr skeleton;                       //!< スケルトン（ボーン階層）
    std::vector<AnimationClipPtr> animations;   //!< アニメーションクリップ
};

//============================================================================
//! @brief SkinnedMesh - スキンメッシュクラス
//!
//! ボーンアニメーション対応のメッシュ。
//! 頂点にボーンインデックスとウェイトが含まれ、
//! GPU上でスキニング変換を行う。
//!
//! @code
//! // ロード
//! auto result = SkinnedMeshLoader::Load("model:/character.fbx");
//! if (result.success) {
//!     auto skinnedMesh = result.mesh;
//!     auto skeleton = skinnedMesh->GetSkeleton();
//!     auto& clips = skinnedMesh->GetAnimations();
//!
//!     // Animatorに設定
//!     animator->SetSkeleton(skeleton);
//!     // クリップをコントローラーに登録
//! }
//! @endcode
//============================================================================
class SkinnedMesh final : private NonCopyable
{
public:
    //! @brief スキンメッシュ生成
    //! @param desc メッシュ記述子
    //! @return 生成されたメッシュ（失敗時nullptr）
    [[nodiscard]] static std::shared_ptr<SkinnedMesh> Create(const SkinnedMeshDesc& desc);

    ~SkinnedMesh() = default;

    //----------------------------------------------------------
    //! @name バッファアクセス
    //----------------------------------------------------------
    //!@{

    //! @brief 頂点バッファ取得
    [[nodiscard]] Buffer* GetVertexBuffer() const noexcept { return vertexBuffer_.get(); }

    //! @brief インデックスバッファ取得
    [[nodiscard]] Buffer* GetIndexBuffer() const noexcept { return indexBuffer_.get(); }

    //!@}
    //----------------------------------------------------------
    //! @name メッシュ情報
    //----------------------------------------------------------
    //!@{

    //! @brief 頂点数取得
    [[nodiscard]] uint32_t GetVertexCount() const noexcept { return vertexCount_; }

    //! @brief インデックス数取得
    [[nodiscard]] uint32_t GetIndexCount() const noexcept { return indexCount_; }

    //! @brief サブメッシュ数取得
    [[nodiscard]] uint32_t GetSubMeshCount() const noexcept {
        return static_cast<uint32_t>(subMeshes_.size());
    }

    //! @brief サブメッシュ取得
    [[nodiscard]] const SubMesh& GetSubMesh(uint32_t index) const {
        return subMeshes_[index];
    }

    //! @brief 全サブメッシュ取得
    [[nodiscard]] const std::vector<SubMesh>& GetSubMeshes() const noexcept {
        return subMeshes_;
    }

    //! @brief バウンディングボックス取得
    [[nodiscard]] const BoundingBox& GetBounds() const noexcept { return bounds_; }

    //! @brief メッシュ名取得
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }

    //!@}
    //----------------------------------------------------------
    //! @name スケルトン・アニメーション
    //----------------------------------------------------------
    //!@{

    //! @brief スケルトン取得
    [[nodiscard]] SkeletonPtr GetSkeleton() const noexcept { return skeleton_; }

    //! @brief ボーン数取得
    [[nodiscard]] size_t GetBoneCount() const noexcept {
        return skeleton_ ? skeleton_->GetBoneCount() : 0;
    }

    //! @brief アニメーションクリップ配列を取得
    [[nodiscard]] const std::vector<AnimationClipPtr>& GetAnimations() const noexcept {
        return animations_;
    }

    //! @brief アニメーションクリップ数取得
    [[nodiscard]] size_t GetAnimationCount() const noexcept {
        return animations_.size();
    }

    //! @brief 名前でアニメーションクリップを検索
    [[nodiscard]] AnimationClipPtr FindAnimation(const std::string& name) const {
        for (const auto& clip : animations_) {
            if (clip && clip->name == name) {
                return clip;
            }
        }
        return nullptr;
    }

    //! @brief アニメーションクリップを追加
    void AddAnimation(const AnimationClipPtr& clip) {
        if (clip) {
            animations_.push_back(clip);
        }
    }

    //!@}

private:
    SkinnedMesh() = default;

    BufferPtr vertexBuffer_;                    //!< 頂点バッファ
    BufferPtr indexBuffer_;                     //!< インデックスバッファ
    uint32_t vertexCount_ = 0;                  //!< 頂点数
    uint32_t indexCount_ = 0;                   //!< インデックス数
    std::vector<SubMesh> subMeshes_;            //!< サブメッシュ配列
    BoundingBox bounds_;                        //!< バウンディングボックス
    std::string name_;                          //!< メッシュ名

    SkeletonPtr skeleton_;                      //!< スケルトン
    std::vector<AnimationClipPtr> animations_;  //!< アニメーションクリップ
};

using SkinnedMeshPtr = std::shared_ptr<SkinnedMesh>;

//============================================================================
//! @brief スキンメッシュロード結果
//============================================================================
struct SkinnedMeshLoadResult
{
    SkinnedMeshPtr mesh;                        //!< 読み込んだメッシュ
    std::vector<MaterialDesc> materialDescs;   //!< マテリアル記述子
    std::vector<std::string> texturePathsToLoad; //!< 読み込むべきテクスチャパス
    bool success = false;                       //!< 成功フラグ
    std::string errorMessage;                   //!< エラーメッセージ

    [[nodiscard]] bool IsValid() const noexcept { return success && mesh != nullptr; }
};
