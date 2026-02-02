//----------------------------------------------------------------------------
//! @file   local_transform.h
//! @brief  ECS LocalTransform - ローカル変換データ（TRS統合）
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief ローカル変換データ（Position + Rotation + Scale 統合）
//!
//! 48バイト。Position, Rotation, Scale を統合したコンポーネント。
//! 階層構造での変換計算に使用。
//!
//! @code
//! // エンティティ作成
//! auto actor = world.CreateActor();
//! world.AddComponent<LocalTransform>(actor, Vector3(1, 2, 3));
//!
//! // 変換操作
//! auto* transform = world.GetComponent<LocalTransform>(actor);
//! transform->position += Vector3(1, 0, 0);
//! transform->rotation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.5f);
//! @endcode
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) LocalTransform : public IComponentData {
    Vector3 position = Vector3::Zero;           //!< 位置 (12 bytes)
    float _pad0 = 0.0f;                         //!< パディング (4 bytes)
    Quaternion rotation = Quaternion::Identity; //!< 回転 (16 bytes)
    Vector3 scale = Vector3::One;               //!< スケール (12 bytes)
    float _pad1 = 0.0f;                         //!< パディング (4 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    LocalTransform() = default;

    explicit LocalTransform(const Vector3& pos) noexcept
        : position(pos) {}

    LocalTransform(const Vector3& pos, const Quaternion& rot) noexcept
        : position(pos), rotation(rot) {}

    LocalTransform(const Vector3& pos, const Quaternion& rot, const Vector3& scl) noexcept
        : position(pos), rotation(rot), scale(scl) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 単位変換を作成
    [[nodiscard]] static LocalTransform Identity() noexcept {
        return LocalTransform();
    }

    //! @brief 位置のみから作成
    [[nodiscard]] static LocalTransform FromPosition(const Vector3& pos) noexcept {
        return LocalTransform(pos);
    }

    //! @brief 位置のみから作成（2D）
    [[nodiscard]] static LocalTransform FromPosition(float x, float y, float z = 0.0f) noexcept {
        return LocalTransform(Vector3(x, y, z));
    }

    //------------------------------------------------------------------------
    // 2D互換ヘルパー
    //------------------------------------------------------------------------

    //! @brief 2D位置を取得
    [[nodiscard]] Vector2 GetPosition2D() const noexcept {
        return Vector2(position.x, position.y);
    }

    //! @brief 2D位置を設定（Z保持）
    void SetPosition2D(const Vector2& pos) noexcept {
        position.x = pos.x;
        position.y = pos.y;
    }

    //! @brief 2D位置を設定（Z保持）
    void SetPosition2D(float x, float y) noexcept {
        position.x = x;
        position.y = y;
    }

    //! @brief Z軸回転を取得（ラジアン）
    [[nodiscard]] float GetRotationZ() const noexcept {
        return 2.0f * std::atan2(rotation.z, rotation.w);
    }

    //! @brief Z軸回転を設定（ラジアン）
    void SetRotationZ(float radians) noexcept {
        rotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, radians);
    }

    //! @brief 2Dスケールを取得
    [[nodiscard]] Vector2 GetScale2D() const noexcept {
        return Vector2(scale.x, scale.y);
    }

    //! @brief 2Dスケールを設定（Z保持）
    void SetScale2D(const Vector2& scl) noexcept {
        scale.x = scl.x;
        scale.y = scl.y;
    }

    //------------------------------------------------------------------------
    // 回転操作
    //------------------------------------------------------------------------

    //! @brief 軸周りに回転を追加
    void Rotate(const Vector3& axis, float angle) noexcept {
        Quaternion delta = Quaternion::CreateFromAxisAngle(axis, angle);
        rotation = rotation * delta;
    }

    //! @brief Z軸周りに回転（2D用）
    void RotateZ(float radians) noexcept {
        Quaternion delta = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, radians);
        rotation = rotation * delta;
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

    //------------------------------------------------------------------------
    // 行列計算
    //------------------------------------------------------------------------

    //! @brief ローカル変換行列を計算
    [[nodiscard]] Matrix ToMatrix() const noexcept {
        // Scale -> Rotate -> Translate
        Matrix s = Matrix::CreateScale(scale);
        Matrix r = Matrix::CreateFromQuaternion(rotation);
        Matrix t = Matrix::CreateTranslation(position);
        return s * r * t;
    }
};
#pragma warning(pop)

// コンパイル時検証
ECS_COMPONENT(LocalTransform);
static_assert(sizeof(LocalTransform) == 48, "LocalTransform must be 48 bytes");

} // namespace ECS
