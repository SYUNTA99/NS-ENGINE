//----------------------------------------------------------------------------
//! @file   mesh_data.h
//! @brief  ECS MeshData - メッシュデータ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/mesh/mesh_handle.h"
#include "engine/material/material_handle.h"
#include <cstdint>
#include <algorithm>

namespace ECS {

//============================================================================
//! @brief メッシュデータ（POD構造体）
//!
//! 3Dメッシュ描画に必要なデータを保持する。
//! MeshRenderSystemによってMeshBatchに送られる。
//!
//! @note trivially copyable維持のため固定サイズ配列を使用。
//!       32サブメッシュまで対応（PMXモデルで十分）。
//============================================================================
struct MeshData : public IComponentData {
    //! 最大マテリアル数（PMXモデルで通常20-30程度）
    static constexpr size_t kMaxMaterials = 32;

    //------------------------------------------------------------------------
    // メッシュ・マテリアル
    //------------------------------------------------------------------------
    MeshHandle mesh;                               //!< メッシュハンドル (4 bytes)
    uint32_t renderLayer = 0;                      //!< レンダリングレイヤー（ビットマスク）(4 bytes)

    //------------------------------------------------------------------------
    // 描画設定（boolパッキング）
    //------------------------------------------------------------------------
    bool visible = true;                           //!< 表示フラグ
    bool castShadow = true;                        //!< シャドウキャスト
    bool receiveShadow = true;                     //!< シャドウレシーブ
    bool _pad0 = false;                            //!< パディング (合計4bytes)

    //------------------------------------------------------------------------
    // マテリアル配列（固定サイズでtrivially copyable維持）
    //------------------------------------------------------------------------
    MaterialHandle materials[kMaxMaterials]{};     //!< マテリアル配列（サブメッシュ対応）
    uint8_t materialCount = 0;                     //!< 有効なマテリアル数

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    MeshData() = default;

    explicit MeshData(MeshHandle msh)
        : mesh(msh) {}

    MeshData(MeshHandle msh, MaterialHandle mat)
        : mesh(msh) {
        if (mat.IsValid()) {
            materials[0] = mat;
            materialCount = 1;
        }
    }

    MeshData(MeshHandle msh, const std::vector<MaterialHandle>& mats)
        : mesh(msh) {
        SetMaterials(mats);
    }

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief マテリアル数を取得
    [[nodiscard]] size_t GetMaterialCount() const noexcept {
        return materialCount;
    }

    //! @brief マテリアルを取得
    //! @param index マテリアルインデックス
    //! @return マテリアルハンドル（範囲外の場合Invalid）
    [[nodiscard]] MaterialHandle GetMaterial(size_t index = 0) const noexcept {
        return index < materialCount ? materials[index] : MaterialHandle::Invalid();
    }

    //! @brief マテリアルを設定（単一）
    void SetMaterial(MaterialHandle mat) noexcept {
        materials[0] = mat;
        materialCount = mat.IsValid() ? 1 : 0;
    }

    //! @brief マテリアルを設定（インデックス指定）
    void SetMaterial(size_t index, MaterialHandle mat) noexcept {
        if (index >= kMaxMaterials) return;
        materials[index] = mat;
        if (index >= materialCount) {
            materialCount = static_cast<uint8_t>(index + 1);
        }
    }

    //! @brief マテリアル配列を設定
    void SetMaterials(const std::vector<MaterialHandle>& mats) noexcept {
        size_t count = (std::min)(mats.size(), kMaxMaterials);
        for (size_t i = 0; i < count; ++i) {
            materials[i] = mats[i];
        }
        materialCount = static_cast<uint8_t>(count);
    }

    //! @brief 有効なメッシュを持っているか
    [[nodiscard]] bool HasValidMesh() const noexcept {
        return mesh.IsValid();
    }
};

ECS_COMPONENT(MeshData);

} // namespace ECS
