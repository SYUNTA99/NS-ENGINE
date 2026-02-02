//----------------------------------------------------------------------------
//! @file   physics_mass_data.h
//! @brief  ECS PhysicsMassData - 質量・慣性テンソルデータ
//! @ref    Unity DOTS PhysicsMass
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 質量・慣性テンソルデータ（POD構造体）
//!
//! 物理シミュレーションにおける質量特性を定義。
//! VelocityData と組み合わせて使用し、力/インパルス適用時の応答を計算。
//!
//! @note メモリレイアウト: 64 bytes, 16B境界アライン
//!
//! @code
//! // 質量1kgの動的オブジェクト
//! world.AddComponent<PhysicsMassData>(actor, PhysicsMassData::CreateDynamic(1.0f));
//!
//! // キネマティック（力を受けない）
//! world.AddComponent<PhysicsMassData>(actor, PhysicsMassData::CreateKinematic());
//!
//! // 力の適用
//! auto* mass = world.GetComponent<PhysicsMassData>(actor);
//! auto* vel = world.GetComponent<VelocityData>(actor);
//! Vector3 acceleration = force * mass->inverseMass;
//! vel->value += acceleration * dt;
//! @endcode
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) PhysicsMassData : public IComponentData {
    //------------------------------------------------------------------------
    // 重心トランスフォーム（慣性空間への変換）- 32 bytes
    //------------------------------------------------------------------------
    Vector3 centerOfMass = Vector3::Zero;              //!< 重心オフセット (12B)
    float _pad0 = 0.0f;                                //!< パディング (4B)
    Quaternion inertiaOrientation = Quaternion::Identity; //!< 慣性主軸の回転 (16B)

    //------------------------------------------------------------------------
    // 質量プロパティ - 32 bytes
    //------------------------------------------------------------------------
    float inverseMass = 1.0f;                          //!< 質量の逆数 (0 = 無限質量/キネマティック)
    float angularExpansionFactor = 0.0f;               //!< CCD用角度拡張係数
    float _pad1[2] = {0.0f, 0.0f};                     //!< パディング (8B)
    Vector3 inverseInertia = Vector3::One;             //!< 慣性テンソル主軸の逆数 (12B)
    float _pad2 = 0.0f;                                //!< パディング (4B)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PhysicsMassData() = default;

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 動的オブジェクト用の質量データを作成
    //! @param mass 質量（kg）
    //! @return PhysicsMassData
    [[nodiscard]] static PhysicsMassData CreateDynamic(float mass) noexcept {
        PhysicsMassData data;
        data.inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
        // 球近似の慣性テンソル（簡易計算）
        float inertia = 0.4f * mass; // 球の慣性モーメント I = 2/5 * m * r^2 (r=1と仮定)
        data.inverseInertia = Vector3(1.0f / inertia);
        return data;
    }

    //! @brief キネマティック（力を受けない）オブジェクト用
    [[nodiscard]] static PhysicsMassData CreateKinematic() noexcept {
        PhysicsMassData data;
        data.inverseMass = 0.0f;  // 無限質量
        data.inverseInertia = Vector3::Zero;
        return data;
    }

    //! @brief 詳細指定で作成
    //! @param mass 質量
    //! @param inertia 慣性テンソル主軸
    //! @param com 重心オフセット
    [[nodiscard]] static PhysicsMassData Create(
        float mass,
        const Vector3& inertia,
        const Vector3& com = Vector3::Zero) noexcept
    {
        PhysicsMassData data;
        data.inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
        data.inverseInertia = Vector3(
            (inertia.x > 0.0f) ? (1.0f / inertia.x) : 0.0f,
            (inertia.y > 0.0f) ? (1.0f / inertia.y) : 0.0f,
            (inertia.z > 0.0f) ? (1.0f / inertia.z) : 0.0f
        );
        data.centerOfMass = com;
        return data;
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 質量を取得
    [[nodiscard]] float GetMass() const noexcept {
        return (inverseMass > 0.0f) ? (1.0f / inverseMass) : 0.0f;
    }

    //! @brief キネマティックかどうか
    [[nodiscard]] bool IsKinematic() const noexcept {
        return inverseMass == 0.0f;
    }

    //! @brief 動的オブジェクトかどうか
    [[nodiscard]] bool IsDynamic() const noexcept {
        return inverseMass > 0.0f;
    }

    //! @brief 線形インパルスを速度に変換
    //! @param impulse インパルス（N・s）
    //! @return 速度変化
    [[nodiscard]] Vector3 ApplyLinearImpulse(const Vector3& impulse) const noexcept {
        return impulse * inverseMass;
    }

    //! @brief 角インパルスを角速度に変換
    //! @param angularImpulse 角インパルス
    //! @return 角速度変化
    [[nodiscard]] Vector3 ApplyAngularImpulse(const Vector3& angularImpulse) const noexcept {
        return angularImpulse * inverseInertia;
    }
};
#pragma warning(pop)

// コンパイル時検証
ECS_COMPONENT(PhysicsMassData);
static_assert(sizeof(PhysicsMassData) == 64, "PhysicsMassData must be 64 bytes");

} // namespace ECS
