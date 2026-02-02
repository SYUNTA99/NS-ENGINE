//----------------------------------------------------------------------------
//! @file   skinned_mesh_loader.cpp
//! @brief  SkinnedMeshLoader 実装
//----------------------------------------------------------------------------
#include "skinned_mesh_loader.h"
#include "mesh_loader.h"
#include "engine/fs/file_system_manager.h"
#include "common/logging/logging.h"

#define HAS_ASSIMP 1

#if HAS_ASSIMP

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#pragma warning(pop)

#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace
{

//============================================================================
// 変換ユーティリティ
//============================================================================

inline Vector3 ToVector3(const aiVector3D& v) {
    return Vector3(v.x, v.y, v.z);
}

inline Quaternion ToQuaternion(const aiQuaternion& q) {
    return Quaternion(q.x, q.y, q.z, q.w);
}

inline Matrix ToMatrix(const aiMatrix4x4& m) {
    // Assimp: a=row1, b=row2, c=row3, d=row4
    // 1,2,3,4 = column
    // DirectXMath Matrix(m11,m12,m13,m14, m21,...) = row-major
    return Matrix(
        m.a1, m.a2, m.a3, m.a4,  // Row 1
        m.b1, m.b2, m.b3, m.b4,  // Row 2
        m.c1, m.c2, m.c3, m.c4,  // Row 3
        m.d1, m.d2, m.d3, m.d4   // Row 4
    );
}

inline Color ToColor(const aiColor4D& c) {
    return Color(c.r, c.g, c.b, c.a);
}

//============================================================================
// ボーン抽出
//============================================================================

//! @brief ボーン名のファジーマッチング（inverseBindMatrix検索用）
Matrix* FindInverseBindMatrixFuzzy(
    std::unordered_map<std::string, Matrix>& matrices,
    const std::string& nodeName)
{
    // 1. 完全一致
    auto it = matrices.find(nodeName);
    if (it != matrices.end()) {
        return &it->second;
    }

    // 2. Character1_プレフィックスを除去して検索
    std::string searchName = nodeName;
    if (nodeName.find("Character1_") == 0) {
        searchName = nodeName.substr(11);  // "Character1_" を除去
        it = matrices.find(searchName);
        if (it != matrices.end()) {
            return &it->second;
        }
    }

    // 3. ノード名がメッシュボーン名を含む、または逆
    for (auto& [meshBoneName, matrix] : matrices) {
        // メッシュボーン名がノード名で終わる
        if (meshBoneName.length() <= nodeName.length()) {
            if (nodeName.find(meshBoneName) != std::string::npos) {
                return &matrix;
            }
        }
        // ノード名がメッシュボーン名を含む
        if (nodeName.length() <= meshBoneName.length()) {
            if (meshBoneName.find(searchName) != std::string::npos) {
                return &matrix;
            }
        }
    }

    return nullptr;
}

//! @brief メッシュからボーン情報を抽出してSkeletonを構築
SkeletonPtr ExtractSkeleton(const aiScene* scene)
{
    auto skeleton = std::make_shared<Skeleton>();
    std::unordered_map<std::string, int> boneNameToIndex;

    // InverseBindMatrixのキャッシュ（メッシュのボーンから取得）
    std::unordered_map<std::string, Matrix> inverseBindMatrices;

    // まず全メッシュからボーンのInverseBindMatrixを収集
    LOG_INFO("[SkinnedMeshLoader] Collecting inverse bind matrices from mesh bones:");
    for (uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx) {
        const aiMesh* mesh = scene->mMeshes[meshIdx];
        if (!mesh->HasBones()) continue;

        for (uint32_t boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
            const aiBone* aiBone = mesh->mBones[boneIdx];
            std::string boneName = aiBone->mName.C_Str();
            Matrix ibm = ToMatrix(aiBone->mOffsetMatrix);
            inverseBindMatrices[boneName] = ibm;
            if (boneIdx < 10) {
                LOG_INFO("  MeshBone[" + std::to_string(boneIdx) + "]: '" + boneName + "'");
            }
            // 重要なボーンのinverseBindMatrix値を出力
            if (boneName.find("Hips") != std::string::npos ||
                boneName.find("Head") != std::string::npos ||
                boneName.find("LeftArm") != std::string::npos) {
                LOG_INFO("    IBM[" + boneName + "] row3: (" +
                    std::to_string(ibm._41) + ", " +
                    std::to_string(ibm._42) + ", " +
                    std::to_string(ibm._43) + ")");
            }
        }
        LOG_INFO("  ... total " + std::to_string(mesh->mNumBones) + " mesh bones");
    }

    // ノード階層を走査して、ボーン候補となるノードを全て追加
    // （Character1_で始まるノード、Skirtノード、J_ノード、髪・リボンボーン等）
    std::function<void(const aiNode*, int, bool)> collectBones;
    collectBones = [&](const aiNode* node, int parentIndex, bool inBoneHierarchy) {
        std::string nodeName = node->mName.C_Str();

        // ボーン階層の開始点を検出（Character1_Reference や _Hips など）
        bool isBoneNode = inBoneHierarchy;
        if (!isBoneNode) {
            // ボーン階層の開始判定
            if (nodeName.find("Character1_") != std::string::npos ||
                nodeName.find("Skirt") != std::string::npos ||
                nodeName.find("J_") != std::string::npos ||
                nodeName.find("bone_") != std::string::npos ||
                nodeName.find("toko_") != std::string::npos ||
                nodeName.find("Collar") != std::string::npos ||
                nodeName.find("Ribbon") != std::string::npos) {
                isBoneNode = true;
            }
        }

        int currentIndex = -1;

        if (isBoneNode && boneNameToIndex.find(nodeName) == boneNameToIndex.end()) {
            // 新しいボーンを追加
            Bone bone;
            bone.name = nodeName;
            bone.parentIndex = parentIndex;
            bone.localBindPose = ToMatrix(node->mTransformation);

            // InverseBindMatrixがあれば使用（ファジーマッチング）
            Matrix* ibm = FindInverseBindMatrixFuzzy(inverseBindMatrices, nodeName);
            if (ibm != nullptr) {
                bone.inverseBindMatrix = *ibm;
            } else {
                // 見つからない場合はIdentity（頂点に影響しないボーン）
                bone.inverseBindMatrix = Matrix::Identity;
                // 重要なボーンでIdentityの場合は警告
                if (nodeName.find("Hips") != std::string::npos ||
                    nodeName.find("Spine") != std::string::npos ||
                    nodeName.find("Arm") != std::string::npos ||
                    nodeName.find("Leg") != std::string::npos ||
                    nodeName.find("Head") != std::string::npos) {
                    LOG_WARN("[SkinnedMeshLoader] No inverseBindMatrix for bone: " + nodeName);
                }
            }

            currentIndex = skeleton->AddBone(bone);
            boneNameToIndex[nodeName] = currentIndex;
        } else if (boneNameToIndex.find(nodeName) != boneNameToIndex.end()) {
            currentIndex = boneNameToIndex[nodeName];
            // 既存ボーンの親を更新
            Bone& bone = skeleton->GetBone(currentIndex);
            if (bone.parentIndex < 0) {
                bone.parentIndex = parentIndex;
            }
            bone.localBindPose = ToMatrix(node->mTransformation);
        }

        // 子ノードを再帰処理
        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            int nextParent = (currentIndex >= 0) ? currentIndex : parentIndex;
            collectBones(node->mChildren[i], nextParent, isBoneNode);
        }
    };

    if (scene->mRootNode) {
        collectBones(scene->mRootNode, -1, false);
    }

    if (skeleton->GetBoneCount() == 0) {
        return nullptr;  // ボーンなし
    }

    LOG_INFO("[SkinnedMeshLoader] Extracted skeleton with " +
             std::to_string(skeleton->GetBoneCount()) + " bones");

    // デバッグ: inverseBindMatrix再計算を一時的に無効化
    // TODO: 問題が解決したら再度有効にする
#if 0
    // InverseBindMatrixがIdentityのボーンに対して、グローバルバインドポーズから再計算
    // Unity FBXではmOffsetMatrixがIdentityになっている場合がある
    {
        // まずグローバルバインドポーズを計算
        std::vector<Matrix> globalBindPoses(skeleton->GetBoneCount());
        for (size_t i = 0; i < skeleton->GetBoneCount(); ++i) {
            const Bone& bone = skeleton->GetBone(static_cast<int>(i));
            if (bone.parentIndex < 0) {
                globalBindPoses[i] = bone.localBindPose;
            } else {
                globalBindPoses[i] = bone.localBindPose * globalBindPoses[bone.parentIndex];
            }
        }

        // InverseBindMatrixがIdentityの場合、グローバルバインドポーズの逆行列で補完
        int recalculatedCount = 0;
        for (size_t i = 0; i < skeleton->GetBoneCount(); ++i) {
            Bone& bone = skeleton->GetBone(static_cast<int>(i));

            // Identityかどうかチェック（対角成分が1で他が0）
            bool isIdentity = (
                std::abs(bone.inverseBindMatrix._11 - 1.0f) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._22 - 1.0f) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._33 - 1.0f) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._44 - 1.0f) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._41) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._42) < 0.0001f &&
                std::abs(bone.inverseBindMatrix._43) < 0.0001f
            );

            if (isIdentity) {
                // グローバルバインドポーズの逆行列を使用
                bone.inverseBindMatrix = globalBindPoses[i].Invert();
                recalculatedCount++;
            }
        }

        if (recalculatedCount > 0) {
            LOG_INFO("[SkinnedMeshLoader] Recalculated inverseBindMatrix for " +
                     std::to_string(recalculatedCount) + " bones from global bind pose");
        }
    }
#endif

    return skeleton;
}

//============================================================================
// アニメーション抽出
//============================================================================

//! @brief ボーン名マッピングテーブル（異なるスケルトン間の変換用）
//! key = アニメーション側のボーン名, value = ターゲットスケルトンのボーン名候補
static const std::unordered_map<std::string, std::vector<std::string>> kBoneNameMapping = {
    // ルートオブジェクト
    {"Toko_sum", {"Character1_Hips", "Hips", "mixamorig:Hips"}},
    {"Cube", {"Character1_Hips", "Hips", "mixamorig:Hips"}},
    {"Armature", {"Character1_Hips", "Hips", "mixamorig:Hips"}},
    {"Root_M", {"Character1_Hips", "Hips"}},

    // Mixamo/Sprint形式 → Character1形式
    {"Hips_M", {"Character1_Hips"}},
    {"Hip_L", {"Character1_LeftUpLeg"}},
    {"Hip_R", {"Character1_RightUpLeg"}},
    {"Knee_L", {"Character1_LeftLeg"}},
    {"Knee_R", {"Character1_RightLeg"}},
    {"Ankle_L", {"Character1_LeftFoot"}},
    {"Ankle_R", {"Character1_RightFoot"}},
    {"Toes_L", {"Character1_LeftToeBase"}},
    {"Toes_R", {"Character1_RightToeBase"}},
    {"Spine_M", {"Character1_Spine"}},
    {"Spine1_M", {"Character1_Spine1"}},
    {"Chest_M", {"Character1_Spine2"}},
    {"Neck_M", {"Character1_Neck"}},
    {"Head_M", {"Character1_Head"}},
    {"Scapula_L", {"Character1_LeftShoulder"}},
    {"Scapula_R", {"Character1_RightShoulder"}},
    {"Shoulder_L", {"Character1_LeftArm"}},
    {"Shoulder_R", {"Character1_RightArm"}},
    {"Elbow_L", {"Character1_LeftForeArm"}},
    {"Elbow_R", {"Character1_RightForeArm"}},
    {"Wrist_L", {"Character1_LeftHand"}},
    {"Wrist_R", {"Character1_RightHand"}},

    // 指（Mixamo形式）
    {"IndexFinger1_L", {"Character1_LeftHandIndex1"}},
    {"IndexFinger2_L", {"Character1_LeftHandIndex2"}},
    {"IndexFinger3_L", {"Character1_LeftHandIndex3"}},
    {"IndexFinger1_R", {"Character1_RightHandIndex1"}},
    {"IndexFinger2_R", {"Character1_RightHandIndex2"}},
    {"IndexFinger3_R", {"Character1_RightHandIndex3"}},
    {"MiddleFinger1_L", {"Character1_LeftHandMiddle1"}},
    {"MiddleFinger2_L", {"Character1_LeftHandMiddle2"}},
    {"MiddleFinger3_L", {"Character1_LeftHandMiddle3"}},
    {"MiddleFinger1_R", {"Character1_RightHandMiddle1"}},
    {"MiddleFinger2_R", {"Character1_RightHandMiddle2"}},
    {"MiddleFinger3_R", {"Character1_RightHandMiddle3"}},
    {"PinkyFinger1_L", {"Character1_LeftHandPinky1"}},
    {"PinkyFinger2_L", {"Character1_LeftHandPinky2"}},
    {"PinkyFinger3_L", {"Character1_LeftHandPinky3"}},
    {"PinkyFinger1_R", {"Character1_RightHandPinky1"}},
    {"PinkyFinger2_R", {"Character1_RightHandPinky2"}},
    {"PinkyFinger3_R", {"Character1_RightHandPinky3"}},
    {"RingFinger1_L", {"Character1_LeftHandRing1"}},
    {"RingFinger2_L", {"Character1_LeftHandRing2"}},
    {"RingFinger3_L", {"Character1_LeftHandRing3"}},
    {"RingFinger1_R", {"Character1_RightHandRing1"}},
    {"RingFinger2_R", {"Character1_RightHandRing2"}},
    {"RingFinger3_R", {"Character1_RightHandRing3"}},
    {"ThumbFinger1_L", {"Character1_LeftHandThumb1"}},
    {"ThumbFinger2_L", {"Character1_LeftHandThumb2"}},
    {"ThumbFinger3_L", {"Character1_LeftHandThumb3"}},
    {"ThumbFinger1_R", {"Character1_RightHandThumb1"}},
    {"ThumbFinger2_R", {"Character1_RightHandThumb2"}},
    {"ThumbFinger3_R", {"Character1_RightHandThumb3"}},
};

//! @brief ボーン名からマッチするインデックスを探す（部分一致対応）
int FindBoneIndexFuzzy(const SkeletonPtr& skeleton, const std::string& nodeName)
{
    // 1. マッピングテーブルで変換を試みる
    auto it = kBoneNameMapping.find(nodeName);
    if (it != kBoneNameMapping.end()) {
        for (const auto& targetName : it->second) {
            int index = skeleton->FindBoneIndex(targetName);
            if (index >= 0) {
                return index;
            }
        }
    }

    // 2. 完全一致を試す
    int index = skeleton->FindBoneIndex(nodeName);
    if (index >= 0) return index;

    // 3. mixamorig: プレフィックスを除去して試す
    std::string searchName = nodeName;
    if (nodeName.find("mixamorig:") == 0) {
        searchName = nodeName.substr(10);  // "mixamorig:" を除去
        index = skeleton->FindBoneIndex(searchName);
        if (index >= 0) return index;
        // Character1_ プレフィックスを付けて試す
        index = skeleton->FindBoneIndex("Character1_" + searchName);
        if (index >= 0) return index;
    }

    // 4. ノード名がボーン名を含む、またはボーン名がノード名を含む場合
    for (int i = 0; i < skeleton->GetBoneCount(); ++i) {
        const Bone& bone = skeleton->GetBone(i);
        // ノード名がボーン名で終わる（例: "Armature|mixamorig:Hips" -> "Hips"）
        if (searchName.length() > bone.name.length()) {
            size_t pos = searchName.rfind(bone.name);
            if (pos != std::string::npos && pos + bone.name.length() == searchName.length()) {
                return i;
            }
        }
        // ボーン名がノード名で終わる
        if (bone.name.length() > searchName.length()) {
            size_t pos = bone.name.rfind(searchName);
            if (pos != std::string::npos && pos + searchName.length() == bone.name.length()) {
                return i;
            }
        }
        // 部分文字列マッチ（"Hips" が "Character1_Hips" に含まれる）
        if (bone.name.find(searchName) != std::string::npos) {
            return i;
        }
    }
    return -1;
}

//! @brief ノード階層をダンプ（デバッグ用）
void DumpNodeHierarchy(const aiNode* node, int depth, int maxDepth = 5)
{
    if (depth > maxDepth) return;
    std::string indent(depth * 2, ' ');
    LOG_INFO(indent + "Node: " + std::string(node->mName.C_Str()) +
             " (meshes:" + std::to_string(node->mNumMeshes) +
             ", children:" + std::to_string(node->mNumChildren) + ")");
    for (uint32_t i = 0; i < node->mNumChildren && i < 10; ++i) {
        DumpNodeHierarchy(node->mChildren[i], depth + 1, maxDepth);
    }
}

//! @brief シーンからアニメーションクリップを抽出
std::vector<AnimationClipPtr> ExtractAnimations(const aiScene* scene, const SkeletonPtr& skeleton)
{
    std::vector<AnimationClipPtr> clips;

    if (!scene->HasAnimations() || !skeleton) {
        return clips;
    }

    // デバッグ: ノード階層を出力
    LOG_INFO("[SkinnedMeshLoader] Node hierarchy:");
    DumpNodeHierarchy(scene->mRootNode, 0, 4);

    // デバッグ: スケルトンの全ボーン名を出力
    LOG_INFO("[SkinnedMeshLoader] Skeleton bones (" + std::to_string(skeleton->GetBoneCount()) + " total):");
    for (int i = 0; i < skeleton->GetBoneCount(); ++i) {
        LOG_INFO("  Bone[" + std::to_string(i) + "]: " + skeleton->GetBone(i).name);
    }

    for (uint32_t animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx) {
        const aiAnimation* anim = scene->mAnimations[animIdx];

        auto clip = std::make_shared<AnimationClip>();
        clip->name = anim->mName.C_Str();
        if (clip->name.empty()) {
            clip->name = "Animation_" + std::to_string(animIdx);
        }

        // 再生時間（Assimpはticks単位）
        double ticksPerSecond = (anim->mTicksPerSecond > 0) ? anim->mTicksPerSecond : 25.0;
        clip->duration = static_cast<float>(anim->mDuration / ticksPerSecond);
        clip->frameRate = static_cast<float>(ticksPerSecond);
        clip->wrapMode = WrapMode::Loop;

        // デバッグ: アニメーションチャンネル数を出力（全チャンネル）
        LOG_INFO("[SkinnedMeshLoader] Animation '" + clip->name + "' has " +
                 std::to_string(anim->mNumChannels) + " channels");
        for (uint32_t c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* na = anim->mChannels[c];
            LOG_INFO("  Channel[" + std::to_string(c) + "]: " +
                     std::string(na->mNodeName.C_Str()) +
                     " (pos:" + std::to_string(na->mNumPositionKeys) +
                     ", rot:" + std::to_string(na->mNumRotationKeys) +
                     ", scl:" + std::to_string(na->mNumScalingKeys) + ")");
        }
        // モーフアニメーションも確認
        if (anim->mNumMorphMeshChannels > 0) {
            LOG_INFO("[SkinnedMeshLoader] Animation has " +
                     std::to_string(anim->mNumMorphMeshChannels) + " morph channels");
        }

        // 各チャンネル（ボーン）のキーフレームを抽出
        for (uint32_t chIdx = 0; chIdx < anim->mNumChannels; ++chIdx) {
            const aiNodeAnim* nodeAnim = anim->mChannels[chIdx];
            std::string nodeName = nodeAnim->mNodeName.C_Str();

            int boneIndex = FindBoneIndexFuzzy(skeleton, nodeName);
            if (boneIndex < 0) {
                // 最初の10チャンネルのみログ出力（デバッグ用）
                if (chIdx < 10) {
                    LOG_INFO("[SkinnedMeshLoader] Channel '" + nodeName + "' not found in skeleton");
                }
                continue;  // スケルトンに含まれないボーン
            }

            // マッチしたボーンをログ出力（最初の10チャンネルのみ）
            if (chIdx < 10) {
                LOG_INFO("[SkinnedMeshLoader] Channel '" + nodeName + "' -> Bone[" +
                         std::to_string(boneIndex) + "] " + skeleton->GetBone(boneIndex).name);
            }

            BoneChannel& channel = clip->AddChannel(boneIndex, nodeName);

            // 位置キーフレーム
            for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; ++k) {
                const aiVectorKey& key = nodeAnim->mPositionKeys[k];
                float time = static_cast<float>(key.mTime / ticksPerSecond);
                channel.positionKeys.emplace_back(time, ToVector3(key.mValue));
            }

            // 回転キーフレーム
            for (uint32_t k = 0; k < nodeAnim->mNumRotationKeys; ++k) {
                const aiQuatKey& key = nodeAnim->mRotationKeys[k];
                float time = static_cast<float>(key.mTime / ticksPerSecond);
                channel.rotationKeys.emplace_back(time, ToQuaternion(key.mValue));
            }

            // スケールキーフレーム
            for (uint32_t k = 0; k < nodeAnim->mNumScalingKeys; ++k) {
                const aiVectorKey& key = nodeAnim->mScalingKeys[k];
                float time = static_cast<float>(key.mTime / ticksPerSecond);
                channel.scaleKeys.emplace_back(time, ToVector3(key.mValue));
            }
        }

        clips.push_back(clip);

        LOG_INFO("[SkinnedMeshLoader] Extracted animation '" + clip->name +
                 "' duration=" + std::to_string(clip->duration) + "s, " +
                 std::to_string(clip->channels.size()) + " channels");
    }

    return clips;
}

//============================================================================
// スキンメッシュ頂点抽出
//============================================================================

//! @brief メッシュからスキン頂点データを抽出
void ExtractSkinnedVertices(
    const aiMesh* mesh,
    const SkeletonPtr& skeleton,
    std::vector<SkinnedMeshVertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<SubMesh>& subMeshes,
    BoundingBox& bounds,
    float scale)
{
    if (!mesh->HasPositions()) return;

    uint32_t startVertex = static_cast<uint32_t>(vertices.size());
    uint32_t startIndex = static_cast<uint32_t>(indices.size());
    uint32_t vertexCount = mesh->mNumVertices;

    // 頂点ごとのボーンデータを一時保存
    struct VertexBoneData {
        int indices[4] = {0, 0, 0, 0};
        float weights[4] = {0, 0, 0, 0};
        int count = 0;
    };
    std::vector<VertexBoneData> vertexBoneData(vertexCount);

    // ボーンデータを収集
    if (mesh->HasBones() && skeleton) {
        int unmappedBones = 0;
        for (uint32_t boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
            const aiBone* bone = mesh->mBones[boneIdx];
            std::string boneName = bone->mName.C_Str();
            int skeletonBoneIdx = skeleton->FindBoneIndex(boneName);

            // 完全一致で見つからない場合、ファジーマッチング
            if (skeletonBoneIdx < 0) {
                // Character1_プレフィックスを付けて検索
                skeletonBoneIdx = skeleton->FindBoneIndex("Character1_" + boneName);
            }
            if (skeletonBoneIdx < 0) {
                // 部分一致で検索
                for (int i = 0; i < static_cast<int>(skeleton->GetBoneCount()); ++i) {
                    const Bone& skelBone = skeleton->GetBone(i);
                    if (skelBone.name.find(boneName) != std::string::npos ||
                        boneName.find(skelBone.name) != std::string::npos) {
                        skeletonBoneIdx = i;
                        break;
                    }
                }
            }

            if (skeletonBoneIdx < 0) {
                unmappedBones++;
                if (unmappedBones <= 5) {
                    LOG_WARN("[SkinnedMeshLoader] Mesh bone '" + boneName + "' not found in skeleton");
                }
                continue;
            }

            for (uint32_t w = 0; w < bone->mNumWeights; ++w) {
                const aiVertexWeight& weight = bone->mWeights[w];
                uint32_t vertexId = weight.mVertexId;

                if (vertexId >= vertexCount) continue;

                auto& vbd = vertexBoneData[vertexId];
                if (vbd.count < 4) {
                    vbd.indices[vbd.count] = skeletonBoneIdx;
                    vbd.weights[vbd.count] = weight.mWeight;
                    vbd.count++;
                }
            }
        }
    }

    // 頂点データを構築
    for (uint32_t i = 0; i < vertexCount; ++i) {
        SkinnedMeshVertex vertex;

        // 位置
        vertex.position = ToVector3(mesh->mVertices[i]) * scale;
        bounds.Expand(vertex.position);

        // 法線
        if (mesh->HasNormals()) {
            vertex.normal = ToVector3(mesh->mNormals[i]);
        } else {
            vertex.normal = Vector3(0, 1, 0);
        }

        // タンジェント
        if (mesh->HasTangentsAndBitangents()) {
            const aiVector3D& t = mesh->mTangents[i];
            const aiVector3D& b = mesh->mBitangents[i];
            const aiVector3D& n = mesh->mNormals[i];

            aiVector3D cross;
            cross.x = n.y * t.z - n.z * t.y;
            cross.y = n.z * t.x - n.x * t.z;
            cross.z = n.x * t.y - n.y * t.x;

            float dot = cross.x * b.x + cross.y * b.y + cross.z * b.z;
            float w = (dot < 0.0f) ? -1.0f : 1.0f;

            vertex.tangent = Vector4(t.x, t.y, t.z, w);
        } else {
            vertex.tangent = Vector4(1, 0, 0, 1);
        }

        // テクスチャ座標
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord = Vector2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        } else {
            vertex.texCoord = Vector2(0, 0);
        }

        // 頂点カラー
        if (mesh->HasVertexColors(0)) {
            vertex.color = ToColor(mesh->mColors[0][i]);
        } else {
            vertex.color = Colors::White;
        }

        // ボーンインデックス（4つを1つのuint32にパック）
        const auto& vbd = vertexBoneData[i];
        vertex.boneIndices =
            (static_cast<uint8_t>(vbd.indices[0])) |
            (static_cast<uint8_t>(vbd.indices[1]) << 8) |
            (static_cast<uint8_t>(vbd.indices[2]) << 16) |
            (static_cast<uint8_t>(vbd.indices[3]) << 24);

        // ボーンウェイト（正規化）
        float totalWeight = vbd.weights[0] + vbd.weights[1] + vbd.weights[2] + vbd.weights[3];
        if (totalWeight > 0.0001f) {
            vertex.boneWeights = Vector4(
                vbd.weights[0] / totalWeight,
                vbd.weights[1] / totalWeight,
                vbd.weights[2] / totalWeight,
                vbd.weights[3] / totalWeight
            );
        } else {
            // ウェイトがない場合はボーン0に100%
            vertex.boneWeights = Vector4(1, 0, 0, 0);
        }

        vertices.push_back(vertex);
    }

    // インデックスデータ
    for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (uint32_t j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(startVertex + face.mIndices[j]);
        }
    }

    // サブメッシュ
    SubMesh subMesh;
    subMesh.indexOffset = startIndex;
    subMesh.indexCount = static_cast<uint32_t>(indices.size()) - startIndex;
    subMesh.materialIndex = mesh->mMaterialIndex;
    subMesh.name = mesh->mName.C_Str();
    subMeshes.push_back(subMesh);
}

//! @brief ノードを再帰的に処理
void ProcessNodeSkinned(
    const aiNode* node,
    const aiScene* scene,
    const SkeletonPtr& skeleton,
    std::vector<SkinnedMeshVertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<SubMesh>& subMeshes,
    BoundingBox& bounds,
    float scale)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ExtractSkinnedVertices(mesh, skeleton, vertices, indices, subMeshes, bounds, scale);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        ProcessNodeSkinned(node->mChildren[i], scene, skeleton, vertices, indices, subMeshes, bounds, scale);
    }
}

//! @brief マテリアル変換
MaterialDesc ConvertMaterialSkinned(const aiMaterial* aiMat, std::vector<std::string>& texturePaths)
{
    MaterialDesc desc;

    aiString name;
    if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        desc.name = name.C_Str();
    }

    aiColor4D diffuse;
    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
        desc.params.albedoColor = ToColor(diffuse);
    }

    float metallic = 0.0f;
    if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        desc.params.metallic = metallic;
    }

    float roughness = 0.5f;
    if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        desc.params.roughness = roughness;
    } else {
        float shininess = 0.0f;
        if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            desc.params.roughness = 1.0f - std::min(shininess / 128.0f, 1.0f);
        }
    }

    if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString texPath;
        if (aiMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texPath) == AI_SUCCESS) {
            desc.diffuseTexturePath = texPath.C_Str();
            texturePaths.push_back(desc.diffuseTexturePath);
        }
    } else if (aiMat->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
        aiString texPath;
        if (aiMat->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), texPath) == AI_SUCCESS) {
            desc.diffuseTexturePath = texPath.C_Str();
            texturePaths.push_back(desc.diffuseTexturePath);
        }
    }

    return desc;
}

unsigned int GetPostProcessFlagsSkinned(const MeshLoadOptions& options)
{
    unsigned int flags =
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_LimitBoneWeights |  // ボーンウェイトを4つまでに制限
        aiProcess_PopulateArmatureData;  // ボーン階層を詳細に取得

    if (options.calculateNormals) {
        flags |= aiProcess_GenSmoothNormals;
    } else {
        flags |= aiProcess_GenNormals;
    }

    if (options.calculateTangents) {
        flags |= aiProcess_CalcTangentSpace;
    }

    if (options.flipUVs) {
        flags |= aiProcess_FlipUVs;
    }

    if (options.flipWindingOrder) {
        flags |= aiProcess_FlipWindingOrder;
    }

    return flags;
}

} // anonymous namespace

//============================================================================
// SkinnedMeshLoader 実装
//============================================================================

SkinnedMeshLoadResult SkinnedMeshLoader::Load(const std::string& filePath, const MeshLoadOptions& options)
{
    SkinnedMeshLoadResult result;

    auto& fsm = FileSystemManager::Get();
    auto fileResult = fsm.ReadFile(filePath);

    if (!fileResult.success) {
        result.errorMessage = "Failed to read file: " + filePath;
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    std::string ext = MeshLoaderUtils::GetExtension(filePath);
    return LoadFromMemory(fileResult.bytes.data(), fileResult.bytes.size(), ext, options);
}

SkinnedMeshLoadResult SkinnedMeshLoader::LoadFromMemory(
    const void* data,
    size_t size,
    const std::string& hint,
    const MeshLoadOptions& options)
{
    SkinnedMeshLoadResult result;

    Assimp::Importer importer;

    // FBXのピボット保持を無効化（ボーン階層がクリーンになる）
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    // すべてのアニメーションレイヤーを読み込む
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, true);

    unsigned int flags = GetPostProcessFlagsSkinned(options);

    const aiScene* scene = importer.ReadFileFromMemory(data, size, flags, hint.c_str());

    if (!scene) {
        result.errorMessage = "Assimp error (scene null): " + std::string(importer.GetErrorString());
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    LOG_INFO("[SkinnedMeshLoader] Scene loaded - Meshes: " + std::to_string(scene->mNumMeshes) +
             ", Animations: " + std::to_string(scene->mNumAnimations) +
             ", Flags: " + std::to_string(scene->mFlags));

    if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        result.errorMessage = "Assimp error (incomplete/no root): flags=" + std::to_string(scene->mFlags) +
                              ", hasRoot=" + std::to_string(scene->mRootNode != nullptr);
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    // スケルトン抽出
    SkeletonPtr skeleton = ExtractSkeleton(scene);

    // アニメーション抽出
    std::vector<AnimationClipPtr> animations;
    if (skeleton) {
        animations = ExtractAnimations(scene, skeleton);
    }

    // マテリアル変換
    if (options.loadMaterials) {
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
            result.materialDescs.push_back(
                ConvertMaterialSkinned(scene->mMaterials[i], result.texturePathsToLoad));
        }
    }

    // スキンメッシュ頂点抽出
    SkinnedMeshDesc desc;
    desc.name = "SkinnedMesh";
    desc.skeleton = skeleton;
    desc.animations = std::move(animations);

    ProcessNodeSkinned(
        scene->mRootNode, scene, skeleton,
        desc.vertices, desc.indices, desc.subMeshes, desc.bounds,
        options.scale);

    if (desc.vertices.empty()) {
        result.errorMessage = "No valid mesh data found";
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    // メッシュ作成
    result.mesh = SkinnedMesh::Create(desc);
    if (result.mesh) {
        result.success = true;
    } else {
        result.errorMessage = "Failed to create skinned mesh";
    }

    return result;
}

AnimationLoadResult SkinnedMeshLoader::LoadAnimationsOnly(
    const std::string& filePath,
    const SkeletonPtr& targetSkeleton)
{
    AnimationLoadResult result;

    // ファイル読み込み
    auto& fsm = FileSystemManager::Get();
    auto fileResult = fsm.ReadFile(filePath);

    if (!fileResult.success) {
        result.errorMessage = "Failed to read file: " + filePath;
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    std::string ext = MeshLoaderUtils::GetExtension(filePath);

    Assimp::Importer importer;

    // FBXのピボット保持を無効化
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    // すべてのアニメーションレイヤーを読み込む
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, true);

    // アニメーション専用ファイルはメッシュ処理不要なのでフラグを最小限に
    unsigned int flags = aiProcess_PopulateArmatureData;

    const aiScene* scene = importer.ReadFileFromMemory(
        fileResult.bytes.data(),
        fileResult.bytes.size(),
        flags,
        ext.c_str());

    // アニメーション専用ファイルはメッシュがなくてもOK
    // AI_SCENE_FLAGS_INCOMPLETEは無視する
    if (!scene || !scene->mRootNode) {
        result.errorMessage = "Assimp error: " + std::string(importer.GetErrorString());
        LOG_ERROR("[SkinnedMeshLoader] " + result.errorMessage);
        return result;
    }

    LOG_INFO("[SkinnedMeshLoader] LoadAnimationsOnly: " + filePath);
    LOG_INFO("[SkinnedMeshLoader]   NumAnimations: " + std::to_string(scene->mNumAnimations));
    LOG_INFO("[SkinnedMeshLoader]   NumMeshes: " + std::to_string(scene->mNumMeshes));
    LOG_INFO("[SkinnedMeshLoader]   SceneFlags: " + std::to_string(scene->mFlags));

    // スケルトンを決定（指定されていなければファイルから抽出）
    SkeletonPtr skeleton = targetSkeleton;
    if (!skeleton) {
        skeleton = ExtractSkeleton(scene);
        result.skeleton = skeleton;
    }

    // アニメーション抽出
    if (skeleton) {
        result.animations = ExtractAnimations(scene, skeleton);
        LOG_INFO("[SkinnedMeshLoader]   Extracted " + std::to_string(result.animations.size()) + " animations");

        // 詳細ログ
        for (size_t i = 0; i < result.animations.size() && i < 5; ++i) {
            const auto& clip = result.animations[i];
            size_t keyCount = 0;
            for (const auto& ch : clip->channels) {
                keyCount += ch.positionKeys.size() + ch.rotationKeys.size() + ch.scaleKeys.size();
            }
            LOG_INFO("[SkinnedMeshLoader]     [" + std::to_string(i) + "] " + clip->name +
                     " - " + std::to_string(clip->channels.size()) + " channels, " +
                     std::to_string(keyCount) + " keys");
        }
    } else {
        LOG_WARN("[SkinnedMeshLoader] No skeleton available for animation extraction");
    }

    result.success = !result.animations.empty();
    return result;
}

#else // !HAS_ASSIMP

SkinnedMeshLoadResult SkinnedMeshLoader::Load(const std::string& filePath, const MeshLoadOptions& options)
{
    (void)filePath;
    (void)options;
    SkinnedMeshLoadResult result;
    result.errorMessage = "Assimp not available";
    return result;
}

SkinnedMeshLoadResult SkinnedMeshLoader::LoadFromMemory(
    const void* data, size_t size, const std::string& hint, const MeshLoadOptions& options)
{
    (void)data; (void)size; (void)hint; (void)options;
    SkinnedMeshLoadResult result;
    result.errorMessage = "Assimp not available";
    return result;
}

AnimationLoadResult SkinnedMeshLoader::LoadAnimationsOnly(
    const std::string& filePath,
    const SkeletonPtr& targetSkeleton)
{
    (void)filePath; (void)targetSkeleton;
    AnimationLoadResult result;
    result.errorMessage = "Assimp not available";
    return result;
}

#endif // HAS_ASSIMP
