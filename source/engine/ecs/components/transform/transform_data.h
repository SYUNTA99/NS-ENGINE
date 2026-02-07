//----------------------------------------------------------------------------
//! @file   transform_data.h
//! @brief  ECS TransformData - トランスフォームデータ構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/actor.h"
#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief トランスフォームデータ（POD構造体）
//!
//! 位置・回転・スケールを保持するデータ構造。
//! TransformSystemによってワールド行列が計算される。
//!
//! @note メモリレイアウト最適化:
//!       - alignas(16)でSIMD命令に最適化
//!       - Matrix/Quaternionは16バイト境界に配置
//!       - キャッシュライン(64B)効率を考慮したメンバー配置
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) TransformData : public IComponentData {
    //------------------------------------------------------------------------
    // キャッシュ（頻繁にアクセス、16バイト境界）
    //------------------------------------------------------------------------
    Matrix worldMatrix = Matrix::Identity;         //!< ワールド変換行列 (64 bytes)
    Matrix localMatrix = Matrix::Identity;         //!< ローカル変換行列 (64 bytes)

    //------------------------------------------------------------------------
    // ローカル変換（16バイト境界）
    //------------------------------------------------------------------------
    Quaternion rotation = Quaternion::Identity;    //!< 3D回転 (16 bytes)
    Vector3 position = Vector3::Zero;              //!< 位置（XYZ）(12 bytes)
    float _pad0 = 0.0f;                            //!< パディング (4 bytes) → position+pad=16bytes
    Vector3 scale = Vector3::One;                  //!< スケール (12 bytes)
    float _pad1 = 0.0f;                            //!< パディング (4 bytes) → scale+pad=16bytes

    //------------------------------------------------------------------------
    // 小さいメンバー（パッキング）
    //------------------------------------------------------------------------
    Vector2 pivot = Vector2::Zero;                 //!< 回転・スケールの中心点 (8 bytes)
    Actor parent = Actor::Invalid();             //!< 親エンティティ (4 bytes)
    bool dirty = true;                             //!< ダーティフラグ (1 byte)
    bool _pad2[3] = {false, false, false};         //!< パディング (3 bytes) → 合計16bytes

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    TransformData() = default;

    TransformData(const Vector3& pos)
        : position(pos) {}

    TransformData(const Vector3& pos, const Quaternion& rot)
        : position(pos), rotation(rot) {}

    TransformData(const Vector3& pos, const Quaternion& rot, const Vector3& scl)
        : position(pos), rotation(rot), scale(scl) {}

    //------------------------------------------------------------------------
    // 2D互換用ヘルパー
    //------------------------------------------------------------------------

    //! @brief 2D位置を取得
    [[nodiscard]] Vector2 GetPosition2D() const noexcept {
        return Vector2(position.x, position.y);
    }

    //! @brief 2D位置を設定
    void SetPosition2D(const Vector2& pos) noexcept {
        position.x = pos.x;
        position.y = pos.y;
        dirty = true;
    }

    //! @brief 2D位置を設定（Z保持）
    void SetPosition2D(float x, float y) noexcept {
        position.x = x;
        position.y = y;
        dirty = true;
    }

    //! @brief Z軸回転を取得（ラジアン）
    [[nodiscard]] float GetRotationZ() const noexcept {
        // クォータニオンからZ軸回転を抽出
        return 2.0f * std::atan2(rotation.z, rotation.w);
    }

    //! @brief Z軸回転を設定（ラジアン）
    void SetRotationZ(float radians) noexcept {
        rotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, radians);
        dirty = true;
    }

    //! @brief 2Dスケールを取得
    [[nodiscard]] Vector2 GetScale2D() const noexcept {
        return Vector2(scale.x, scale.y);
    }

    //! @brief 2Dスケールを設定
    void SetScale2D(const Vector2& scl) noexcept {
        scale.x = scl.x;
        scale.y = scl.y;
        dirty = true;
    }

    //! @brief 均一スケールを設定
    void SetUniformScale(float s) noexcept {
        scale.x = s;
        scale.y = s;
        scale.z = s;
        dirty = true;
    }

    //------------------------------------------------------------------------
    // 移動・回転ヘルパー
    //------------------------------------------------------------------------

    //! @brief 移動
    void Translate(const Vector3& delta) noexcept {
        position += delta;
        dirty = true;
    }

    //! @brief 移動（2D）
    void Translate(float dx, float dy) noexcept {
        position.x += dx;
        position.y += dy;
        dirty = true;
    }

    //! @brief 軸周りに回転を追加
    void Rotate(const Vector3& axis, float angle) noexcept {
        Quaternion delta = Quaternion::CreateFromAxisAngle(axis, angle);
        rotation = rotation * delta;
        dirty = true;
    }

    //! @brief Z軸周りに回転（2D用）
    void RotateZ(float radians) noexcept {
        Quaternion delta = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, radians);
        rotation = rotation * delta;
        dirty = true;
    }

    //------------------------------------------------------------------------
    // 方向ベクトル
    //------------------------------------------------------------------------

    //! @brief 前方向を取得
    [[nodiscard]] Vector3 GetForward() const noexcept {
        return Vector3::Transform(LH::Forward(), rotation);
    }

    //! @brief 右方向を取得
    [[nodiscard]] Vector3 GetRight() const noexcept {
        return Vector3::Transform(Vector3::Right, rotation);
    }

    //! @brief 上方向を取得
    [[nodiscard]] Vector3 GetUp() const noexcept {
        return Vector3::Transform(Vector3::Up, rotation);
    }
};
#pragma warning(pop)

} // namespace ECS
