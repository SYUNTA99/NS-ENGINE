//----------------------------------------------------------------------------
//! @file   physics_damping_data.h
//! @brief  ECS PhysicsDampingData - 減衰（空気抵抗）データ
//! @ref    Unity DOTS PhysicsDamping
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief 減衰データ（POD構造体）
//!
//! 速度の自然減衰を定義。空気抵抗、摩擦、水中抵抗などをシミュレート。
//! VelocityData / AngularVelocityData と組み合わせて使用。
//!
//! @note メモリレイアウト: 8 bytes
//!
//! @code
//! // 空気抵抗を追加
//! world.AddComponent<PhysicsDampingData>(actor, PhysicsDampingData{0.01f, 0.05f});
//!
//! // システムでの適用
//! vel.value *= std::max(0.0f, 1.0f - damping.linear * dt);
//! angVel.value *= std::max(0.0f, 1.0f - damping.angular * dt);
//! @endcode
//============================================================================
struct PhysicsDampingData : public IComponentData {
    float linear = 0.0f;    //!< 線形減衰係数 (0 = 減衰なし)
    float angular = 0.05f;  //!< 角度減衰係数 (0 = 減衰なし)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PhysicsDampingData() = default;

    PhysicsDampingData(float lin, float ang) noexcept
        : linear(lin), angular(ang) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 減衰なし
    [[nodiscard]] static PhysicsDampingData None() noexcept {
        return PhysicsDampingData(0.0f, 0.0f);
    }

    //! @brief 空気中（軽い減衰）
    [[nodiscard]] static PhysicsDampingData Air() noexcept {
        return PhysicsDampingData(0.01f, 0.05f);
    }

    //! @brief 水中（強い減衰）
    [[nodiscard]] static PhysicsDampingData Water() noexcept {
        return PhysicsDampingData(0.5f, 0.8f);
    }

    //! @brief 高摩擦（地面との接触など）
    [[nodiscard]] static PhysicsDampingData HighFriction() noexcept {
        return PhysicsDampingData(0.3f, 0.5f);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 線形速度に減衰を適用
    //! @param velocity 現在の速度
    //! @param dt デルタタイム
    //! @return 減衰後の速度
    template<typename T>
    [[nodiscard]] T ApplyLinear(const T& velocity, float dt) const noexcept {
        float factor = 1.0f - linear * dt;
        return velocity * ((factor > 0.0f) ? factor : 0.0f);
    }

    //! @brief 角速度に減衰を適用
    //! @param angularVelocity 現在の角速度
    //! @param dt デルタタイム
    //! @return 減衰後の角速度
    template<typename T>
    [[nodiscard]] T ApplyAngular(const T& angularVelocity, float dt) const noexcept {
        float factor = 1.0f - angular * dt;
        return angularVelocity * ((factor > 0.0f) ? factor : 0.0f);
    }
};

// コンパイル時検証
ECS_COMPONENT(PhysicsDampingData);
static_assert(sizeof(PhysicsDampingData) == 8, "PhysicsDampingData must be 8 bytes");

} // namespace ECS
