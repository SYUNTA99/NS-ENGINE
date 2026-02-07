//----------------------------------------------------------------------------
//! @file   camera3d_data.h
//! @brief  ECS Camera3DData - 3Dカメラデータ構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 3Dカメラデータ（POD構造体）
//!
//! 3D描画用のビュー・プロジェクション行列を管理する。
//! LookAtベースまたはTransformDataとの連携で位置を設定可能。
//!
//! @note メモリレイアウト最適化:
//!       - alignas(16)でSIMD命令に最適化
//!       - Matrixは64バイト、16バイト境界に配置
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) Camera3DData : public IComponentData {
    //------------------------------------------------------------------------
    // キャッシュ（頻繁にアクセス、16バイト境界）
    //------------------------------------------------------------------------
    Matrix viewMatrix = Matrix::Identity;           //!< ビュー変換行列 (64 bytes)
    Matrix projectionMatrix = Matrix::Identity;     //!< プロジェクション行列 (64 bytes)

    //------------------------------------------------------------------------
    // ビュー設定
    //------------------------------------------------------------------------
    Vector3 position = Vector3::Zero;               //!< カメラ位置 (12 bytes)
    float _pad0 = 0.0f;                             //!< パディング (4 bytes)
    Vector3 target = Vector3(0.0f, 0.0f, 1.0f);    //!< 注視点 (12 bytes) - LH座標系で前方
    float _pad1 = 0.0f;                             //!< パディング (4 bytes)
    Vector3 up = Vector3::Up;                       //!< 上方向ベクトル (12 bytes)
    float _pad2 = 0.0f;                             //!< パディング (4 bytes)

    //------------------------------------------------------------------------
    // プロジェクション設定
    //------------------------------------------------------------------------
    float fovY = 60.0f;                             //!< 視野角Y（度）
    float aspectRatio = 16.0f / 9.0f;               //!< アスペクト比
    float nearPlane = 0.1f;                         //!< ニアクリップ面
    float farPlane = 1000.0f;                       //!< ファークリップ面

    //------------------------------------------------------------------------
    // フラグ
    //------------------------------------------------------------------------
    bool dirty = true;                              //!< ダーティフラグ
    bool _pad3[3] = {false, false, false};          //!< パディング

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Camera3DData() = default;

    Camera3DData(float fov, float aspect)
        : fovY(fov), aspectRatio(aspect) {}

    Camera3DData(float fov, float aspect, float nearP, float farP)
        : fovY(fov), aspectRatio(aspect), nearPlane(nearP), farPlane(farP) {}

    //------------------------------------------------------------------------
    // 位置・注視点設定
    //------------------------------------------------------------------------

    //! @brief 位置を設定
    void SetPosition(float x, float y, float z) noexcept {
        position.x = x;
        position.y = y;
        position.z = z;
        dirty = true;
    }

    //! @brief 位置を設定
    void SetPosition(const Vector3& pos) noexcept {
        position = pos;
        dirty = true;
    }

    //! @brief 注視点を設定
    void LookAt(const Vector3& t) noexcept {
        target = t;
        dirty = true;
    }

    //! @brief 注視点を設定
    void LookAt(const Vector3& t, const Vector3& upVec) noexcept {
        target = t;
        up = upVec;
        dirty = true;
    }

    //! @brief 移動
    void Translate(const Vector3& delta) noexcept {
        position += delta;
        target += delta;  // 注視点も同時に移動
        dirty = true;
    }

    //------------------------------------------------------------------------
    // 行列計算
    //------------------------------------------------------------------------

    //! @brief 行列を更新（dirtyの場合）
    void UpdateMatrices() noexcept {
        if (!dirty) return;

        // ビュー行列（左手座標系）
        viewMatrix = LH::CreateLookAt(position, target, up);

        // プロジェクション行列（左手座標系）
        float fovRadians = fovY * (3.14159265358979323846f / 180.0f);
        projectionMatrix = LH::CreatePerspectiveFov(
            fovRadians, aspectRatio, nearPlane, farPlane);

        dirty = false;
    }

    //! @brief ビュー行列を取得
    [[nodiscard]] Matrix GetViewMatrix() const noexcept {
        if (dirty) {
            const_cast<Camera3DData*>(this)->UpdateMatrices();
        }
        return viewMatrix;
    }

    //! @brief プロジェクション行列を取得
    [[nodiscard]] Matrix GetProjectionMatrix() const noexcept {
        if (dirty) {
            const_cast<Camera3DData*>(this)->UpdateMatrices();
        }
        return projectionMatrix;
    }

    //! @brief ビュープロジェクション行列を取得
    [[nodiscard]] Matrix GetViewProjectionMatrix() const noexcept {
        if (dirty) {
            const_cast<Camera3DData*>(this)->UpdateMatrices();
        }
        return viewMatrix * projectionMatrix;
    }

    //------------------------------------------------------------------------
    // 方向ベクトル
    //------------------------------------------------------------------------

    //! @brief カメラの前方向を取得
    [[nodiscard]] Vector3 GetForward() const noexcept {
        Vector3 forward = target - position;
        forward.Normalize();
        return forward;
    }

    //! @brief カメラの右方向を取得
    [[nodiscard]] Vector3 GetRight() const noexcept {
        Vector3 forward = GetForward();
        Vector3 right = up.Cross(forward);
        // upとforwardが平行な場合のフォールバック
        if (right.LengthSquared() < 0.0001f) {
            return Vector3::Right;
        }
        right.Normalize();
        return right;
    }

    //! @brief カメラの上方向を取得
    [[nodiscard]] Vector3 GetUp() const noexcept {
        return up;
    }
};
#pragma warning(pop)

ECS_COMPONENT(Camera3DData);

} // namespace ECS
