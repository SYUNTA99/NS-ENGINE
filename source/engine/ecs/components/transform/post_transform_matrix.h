//----------------------------------------------------------------------------
//! @file   post_transform_matrix.h
//! @brief  ECS PostTransformMatrix - 追加変換行列（シアー/非均一スケール用）
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 追加変換行列コンポーネント（オプション）
//!
//! 64バイト。LocalTransformの後に適用される追加の変換行列。
//! シアー変換や複雑な非均一スケールに使用。
//!
//! @note LocalToWorld計算時:
//!       result = LocalTransform行列 * PostTransformMatrix * 親のLocalToWorld
//!
//! @code
//! // シアー変換を追加
//! Matrix shear = Matrix::Identity;
//! shear._21 = 0.5f;  // X方向にY軸シアー
//! world.AddComponent<PostTransformMatrix>(actor, shear);
//! @endcode
//============================================================================
struct PostTransformMatrix : public IComponentData {
    Matrix value = Matrix::Identity;          //!< 追加変換行列 (64 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PostTransformMatrix() = default;

    explicit PostTransformMatrix(const Matrix& mat) noexcept
        : value(mat) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief シアー行列を作成（XY平面）
    [[nodiscard]] static PostTransformMatrix CreateShearXY(float shearX, float shearY) noexcept {
        Matrix mat = Matrix::Identity;
        mat._21 = shearX;  // Y軸方向のX成分
        mat._12 = shearY;  // X軸方向のY成分
        return PostTransformMatrix(mat);
    }

    //! @brief 軸ごとの非均一スケールを作成
    [[nodiscard]] static PostTransformMatrix CreateNonUniformScale(
        const Vector3& scaleX, const Vector3& scaleY, const Vector3& scaleZ) noexcept
    {
        Matrix mat;
        mat._11 = scaleX.x; mat._12 = scaleX.y; mat._13 = scaleX.z; mat._14 = 0.0f;
        mat._21 = scaleY.x; mat._22 = scaleY.y; mat._23 = scaleY.z; mat._24 = 0.0f;
        mat._31 = scaleZ.x; mat._32 = scaleZ.y; mat._33 = scaleZ.z; mat._34 = 0.0f;
        mat._41 = 0.0f;     mat._42 = 0.0f;     mat._43 = 0.0f;     mat._44 = 1.0f;
        return PostTransformMatrix(mat);
    }
};

// コンパイル時検証
ECS_COMPONENT(PostTransformMatrix);
static_assert(sizeof(PostTransformMatrix) == 64, "PostTransformMatrix must be 64 bytes");

} // namespace ECS
