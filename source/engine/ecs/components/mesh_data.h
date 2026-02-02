//----------------------------------------------------------------------------
//! @file   mesh_data.h
//! @brief  ECS MeshData - メッシュデータ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/mesh/mesh_handle.h"
#include "engine/material/material_handle.h"
#include <vector>

namespace ECS {

//============================================================================
//! @brief メッシュデータ（POD構造体）
//!
//! 3Dメッシュ描画に必要なデータを保持する。
//! MeshRenderSystemによってMeshBatchに送られる。
//!
//! @note メモリレイアウト最適化:
//!       - boolフラグをパッキングして無駄を削減
//!       - std::vectorのため厳密なalignasは適用しない
//============================================================================
struct MeshData : public IComponentData {
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
    // マテリアル配列（vectorはヒープ割り当て）
    //------------------------------------------------------------------------
    std::vector<MaterialHandle> materials;         //!< マテリアル配列（サブメッシュ対応）

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    MeshData() = default;

    explicit MeshData(MeshHandle msh)
        : mesh(msh) {}

    MeshData(MeshHandle msh, MaterialHandle mat)
        : mesh(msh) {
        if (mat.IsValid()) {
            materials.push_back(mat);
        }
    }

    MeshData(MeshHandle msh, const std::vector<MaterialHandle>& mats)
        : mesh(msh), materials(mats) {}

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief マテリアル数を取得
    [[nodiscard]] size_t GetMaterialCount() const noexcept {
        return materials.size();
    }

    //! @brief マテリアルを取得
    //! @param index マテリアルインデックス
    //! @return マテリアルハンドル（範囲外の場合Invalid）
    [[nodiscard]] MaterialHandle GetMaterial(size_t index = 0) const noexcept {
        return index < materials.size() ? materials[index] : MaterialHandle::Invalid();
    }

    //! @brief マテリアルを設定（単一）
    void SetMaterial(MaterialHandle mat) noexcept {
        materials.clear();
        if (mat.IsValid()) {
            materials.push_back(mat);
        }
    }

    //! @brief マテリアルを設定（インデックス指定）
    void SetMaterial(size_t index, MaterialHandle mat) noexcept {
        if (index >= materials.size()) {
            materials.resize(index + 1, MaterialHandle::Invalid());
        }
        materials[index] = mat;
    }

    //! @brief 有効なメッシュを持っているか
    [[nodiscard]] bool HasValidMesh() const noexcept {
        return mesh.IsValid();
    }
};

} // namespace ECS
