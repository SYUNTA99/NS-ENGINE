//----------------------------------------------------------------------------
//! @file   local_to_world.h
//! @brief  ECS LocalToWorld - ワールド変換行列
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief ワールド変換行列コンポーネント
//!
//! 64バイト。LocalToWorldSystemによって計算される最終的なワールド行列。
//! レンダリングシステムはこれを直接使用。
//!
//! @note LocalTransformから毎フレーム計算される。
//!       親子階層がある場合は親のLocalToWorldも乗算。
//!
//! @code
//! // レンダリングシステム
//! world.ForEach<LocalToWorld, SpriteData>([&](Actor e, auto& ltw, auto& spr) {
//!     renderer.Draw(spr.texture, ltw.value);
//! });
//! @endcode
//============================================================================
struct LocalToWorld : public IComponentData {
    Matrix value = Matrix::Identity;          //!< ワールド変換行列 (64 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    LocalToWorld() = default;

    explicit LocalToWorld(const Matrix& mat) noexcept
        : value(mat) {}

    //------------------------------------------------------------------------
    // 位置抽出ヘルパー
    //------------------------------------------------------------------------

    //! @brief ワールド位置を取得
    [[nodiscard]] Vector3 GetPosition() const noexcept {
        return value.Translation();
    }

    //! @brief ワールド位置（2D）を取得
    [[nodiscard]] Vector2 GetPosition2D() const noexcept {
        Vector3 pos = value.Translation();
        return Vector2(pos.x, pos.y);
    }

    //------------------------------------------------------------------------
    // スケール抽出ヘルパー
    //------------------------------------------------------------------------

    //! @brief ワールドスケールを取得（近似値）
    [[nodiscard]] Vector3 GetScale() const noexcept {
        Vector3 scaleX(value._11, value._12, value._13);
        Vector3 scaleY(value._21, value._22, value._23);
        Vector3 scaleZ(value._31, value._32, value._33);
        return Vector3(scaleX.Length(), scaleY.Length(), scaleZ.Length());
    }

    //------------------------------------------------------------------------
    // 方向ベクトル抽出
    //------------------------------------------------------------------------

    //! @brief 前方向を取得
    [[nodiscard]] Vector3 GetForward() const noexcept {
        return Vector3(value._31, value._32, value._33);
    }

    //! @brief 右方向を取得
    [[nodiscard]] Vector3 GetRight() const noexcept {
        return Vector3(value._11, value._12, value._13);
    }

    //! @brief 上方向を取得
    [[nodiscard]] Vector3 GetUp() const noexcept {
        return Vector3(value._21, value._22, value._23);
    }
};

// コンパイル時検証
ECS_COMPONENT(LocalToWorld);
static_assert(sizeof(LocalToWorld) == 64, "LocalToWorld must be 64 bytes");

} // namespace ECS
