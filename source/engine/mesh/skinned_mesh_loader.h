//----------------------------------------------------------------------------
//! @file   skinned_mesh_loader.h
//! @brief  SkinnedMeshLoader - スキンメッシュローダー
//----------------------------------------------------------------------------
#pragma once

#include "skinned_mesh.h"
#include "mesh_loader.h"
#include <string>

//============================================================================
//! @brief アニメーション専用ロード結果
//============================================================================
struct AnimationLoadResult
{
    std::vector<AnimationClipPtr> animations;   //!< アニメーションクリップ配列
    SkeletonPtr skeleton;                       //!< スケルトン（ボーン階層）
    bool success = false;                       //!< 成功フラグ
    std::string errorMessage;                   //!< エラーメッセージ

    [[nodiscard]] bool IsValid() const noexcept { return success && !animations.empty(); }
};

//============================================================================
//! @brief スキンメッシュローダー
//!
//! FBX/glTF等からスケルトン、アニメーション、スキンメッシュを読み込む。
//!
//! @code
//! auto result = SkinnedMeshLoader::Load("model:/character.fbx");
//! if (result.IsValid()) {
//!     auto mesh = result.mesh;
//!     auto skeleton = mesh->GetSkeleton();
//!
//!     // Animatorに設定
//!     animator->SetSkeleton(skeleton);
//!
//!     // アニメーションクリップを取得
//!     for (const auto& clip : mesh->GetAnimations()) {
//!         // AnimatorControllerに登録
//!     }
//! }
//! @endcode
//============================================================================
class SkinnedMeshLoader
{
public:
    //! @brief ファイルからスキンメッシュを読み込む
    //! @param filePath ファイルパス
    //! @param options ロードオプション
    //! @return ロード結果
    [[nodiscard]] static SkinnedMeshLoadResult Load(
        const std::string& filePath,
        const MeshLoadOptions& options = {});

    //! @brief メモリからスキンメッシュを読み込む
    //! @param data バイナリデータ
    //! @param size データサイズ
    //! @param hint ファイル名ヒント（拡張子判定用）
    //! @param options ロードオプション
    //! @return ロード結果
    [[nodiscard]] static SkinnedMeshLoadResult LoadFromMemory(
        const void* data,
        size_t size,
        const std::string& hint,
        const MeshLoadOptions& options = {});

    //! @brief ファイルからアニメーションのみを読み込む
    //! @param filePath ファイルパス（アニメーション専用FBX等）
    //! @param targetSkeleton 対象スケルトン（ボーン名マッピング用）
    //! @return アニメーションロード結果
    //! @note メッシュを含まないアニメーション専用ファイル用
    [[nodiscard]] static AnimationLoadResult LoadAnimationsOnly(
        const std::string& filePath,
        const SkeletonPtr& targetSkeleton = nullptr);
};
