//----------------------------------------------------------------------------
//! @file   skeleton.h
//! @brief  Skeleton - 骨格（ボーン階層）クラス
//----------------------------------------------------------------------------
#pragma once

#include "engine/math/math_types.h"
#include <vector>
#include <string>
#include <unordered_map>

//============================================================================
//! @brief ボーン情報
//============================================================================
struct Bone {
    std::string name;               //!< ボーン名
    int parentIndex = -1;           //!< 親ボーンインデックス（-1 = ルート）
    Matrix localBindPose;           //!< ローカルバインドポーズ
    Matrix inverseBindMatrix;       //!< 逆バインド行列（スキニング用）

    Bone() = default;

    Bone(const std::string& boneName, int parent = -1)
        : name(boneName)
        , parentIndex(parent)
        , localBindPose(Matrix::Identity)
        , inverseBindMatrix(Matrix::Identity) {}

    Bone(const std::string& boneName, int parent, const Matrix& bindPose)
        : name(boneName)
        , parentIndex(parent)
        , localBindPose(bindPose)
        , inverseBindMatrix(Matrix::Identity) {}
};

//============================================================================
//! @brief Skeleton - 骨格（ボーン階層）
//!
//! スキンメッシュアニメーションのためのボーン階層を管理。
//! AnimatorコンポーネントがSkeletonを参照し、ボーンTransformを更新する。
//!
//! @code
//! auto skeleton = std::make_shared<Skeleton>();
//!
//! // ボーン追加（親子関係を定義）
//! int root = skeleton->AddBone(Bone("Root"));
//! int spine = skeleton->AddBone(Bone("Spine", root));
//! int head = skeleton->AddBone(Bone("Head", spine));
//! int leftArm = skeleton->AddBone(Bone("LeftArm", spine));
//! int rightArm = skeleton->AddBone(Bone("RightArm", spine));
//!
//! // 逆バインド行列を計算（メッシュのバインドポーズから）
//! skeleton->ComputeInverseBindMatrices();
//!
//! // アニメーション適用後、スキニング行列を計算
//! std::vector<Matrix> localTransforms = ...;  // アニメーションからサンプリング
//! std::vector<Matrix> globalTransforms;
//! std::vector<Matrix> skinningMatrices;
//!
//! skeleton->ComputeGlobalTransforms(localTransforms, globalTransforms);
//! skeleton->ComputeSkinningMatrices(globalTransforms, skinningMatrices);
//! // skinningMatricesをGPUに送信
//! @endcode
//============================================================================
class Skeleton {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Skeleton() = default;

    //========================================================================
    // ボーン管理
    //========================================================================

    //! @brief ボーンを追加
    //! @param bone ボーン情報
    //! @return 追加されたボーンのインデックス
    int AddBone(const Bone& bone) {
        int index = static_cast<int>(bones_.size());
        boneNameToIndex_[bone.name] = index;
        bones_.push_back(bone);
        return index;
    }

    //! @brief ボーン名からインデックスを検索
    //! @param name ボーン名
    //! @return ボーンインデックス（見つからない場合-1）
    [[nodiscard]] int FindBoneIndex(const std::string& name) const {
        auto it = boneNameToIndex_.find(name);
        return (it != boneNameToIndex_.end()) ? it->second : -1;
    }

    //! @brief ボーンを取得
    //! @param index ボーンインデックス
    //! @return ボーン参照
    [[nodiscard]] const Bone& GetBone(int index) const {
        return bones_[index];
    }

    //! @brief ボーンを取得（変更可能）
    [[nodiscard]] Bone& GetBone(int index) {
        return bones_[index];
    }

    //! @brief ボーン数を取得
    [[nodiscard]] size_t GetBoneCount() const noexcept {
        return bones_.size();
    }

    //! @brief 全ボーンを取得
    [[nodiscard]] const std::vector<Bone>& GetBones() const noexcept {
        return bones_;
    }

    //========================================================================
    // バインドポーズ計算
    //========================================================================

    //! @brief 逆バインド行列を計算
    //!
    //! 各ボーンのlocalBindPoseからグローバルバインドポーズを計算し、
    //! その逆行列をinverseBindMatrixに格納する。
    //! メッシュロード後に1回呼び出す。
    void ComputeInverseBindMatrices() {
        if (bones_.empty()) return;

        std::vector<Matrix> globalBindPose(bones_.size());

        for (size_t i = 0; i < bones_.size(); ++i) {
            const Bone& bone = bones_[i];
            if (bone.parentIndex < 0) {
                // ルートボーン
                globalBindPose[i] = bone.localBindPose;
            } else {
                // 親のグローバル行列 × ローカル行列
                globalBindPose[i] = bone.localBindPose * globalBindPose[bone.parentIndex];
            }
            // 逆行列を計算
            globalBindPose[i].Invert(bones_[i].inverseBindMatrix);
        }
    }

    //! @brief 逆バインド行列を直接設定（メッシュローダーから）
    //! @param index ボーンインデックス
    //! @param inverseBindMatrix 逆バインド行列
    void SetInverseBindMatrix(int index, const Matrix& inverseBindMatrix) {
        if (index >= 0 && index < static_cast<int>(bones_.size())) {
            bones_[index].inverseBindMatrix = inverseBindMatrix;
        }
    }

    //========================================================================
    // Transform計算
    //========================================================================

    //! @brief ローカルTransformからグローバルTransformを計算
    //!
    //! 各ボーンのローカル変換行列から、階層を考慮した
    //! グローバル（ワールド空間）変換行列を計算する。
    //!
    //! @param localTransforms ローカル変換行列配列（アニメーションから取得）
    //! @param globalOut グローバル変換行列配列（出力）
    void ComputeGlobalTransforms(
        const std::vector<Matrix>& localTransforms,
        std::vector<Matrix>& globalOut) const {

        if (bones_.empty()) return;
        if (localTransforms.size() != bones_.size()) return;

        globalOut.resize(bones_.size());

        for (size_t i = 0; i < bones_.size(); ++i) {
            const Bone& bone = bones_[i];
            if (bone.parentIndex < 0) {
                // ルートボーン: ローカル = グローバル
                globalOut[i] = localTransforms[i];
            } else {
                // 子ボーン: ローカル × 親のグローバル
                globalOut[i] = localTransforms[i] * globalOut[bone.parentIndex];
            }
        }
    }

    //! @brief グローバルTransformからスキニング行列を計算
    //!
    //! スキニング行列 = 逆バインド行列 × グローバル変換行列
    //! この行列をGPUシェーダーに送信してスキンメッシュを変形する。
    //!
    //! @param globalTransforms グローバル変換行列配列
    //! @param skinningOut スキニング行列配列（出力）
    void ComputeSkinningMatrices(
        const std::vector<Matrix>& globalTransforms,
        std::vector<Matrix>& skinningOut) const {

        if (bones_.empty()) return;
        if (globalTransforms.size() != bones_.size()) return;

        skinningOut.resize(bones_.size());

        for (size_t i = 0; i < bones_.size(); ++i) {
            // スキニング行列 = 逆バインド × グローバル
            skinningOut[i] = bones_[i].inverseBindMatrix * globalTransforms[i];
        }
    }

    //========================================================================
    // ユーティリティ
    //========================================================================

    //! @brief ボーンの子インデックスリストを取得
    //! @param parentIndex 親ボーンインデックス
    //! @return 子ボーンインデックスのリスト
    [[nodiscard]] std::vector<int> GetChildBoneIndices(int parentIndex) const {
        std::vector<int> children;
        for (size_t i = 0; i < bones_.size(); ++i) {
            if (bones_[i].parentIndex == parentIndex) {
                children.push_back(static_cast<int>(i));
            }
        }
        return children;
    }

    //! @brief ルートボーンのインデックスを取得
    //! @return ルートボーンインデックス（見つからない場合-1）
    [[nodiscard]] int GetRootBoneIndex() const {
        for (size_t i = 0; i < bones_.size(); ++i) {
            if (bones_[i].parentIndex < 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //! @brief スケルトンをクリア
    void Clear() {
        bones_.clear();
        boneNameToIndex_.clear();
    }

private:
    std::vector<Bone> bones_;                           //!< ボーン配列
    std::unordered_map<std::string, int> boneNameToIndex_;  //!< 名前→インデックスマップ
};

using SkeletonPtr = std::shared_ptr<Skeleton>;
