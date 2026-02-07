//----------------------------------------------------------------------------
//! @file   physics_gravity_factor_data.h
//! @brief  ECS PhysicsGravityFactorData - 個別重力スケール
//! @ref    Unity DOTS PhysicsGravityFactor
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief 重力スケールデータ（POD構造体）
//!
//! オブジェクトごとの重力影響度を定義。
//! 浮遊オブジェクト、宇宙空間、水中浮力などをシミュレート。
//!
//! @note メモリレイアウト: 4 bytes
//!
//! @code
//! // 風船（重力なし）
//! world.AddComponent<PhysicsGravityFactorData>(balloon, PhysicsGravityFactorData::Zero());
//!
//! // 重い隕石（2倍の重力）
//! world.AddComponent<PhysicsGravityFactorData>(meteor, PhysicsGravityFactorData{2.0f});
//!
//! // システムでの適用
//! Vector3 gravity = world.GetGravity() * gravityFactor.value;
//! velocity += gravity * dt;
//! @endcode
//============================================================================
struct PhysicsGravityFactorData : public IComponentData {
    float value = 1.0f;  //!< 重力スケール (1.0 = 通常, 0 = 無重力, 負数 = 上昇)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PhysicsGravityFactorData() = default;

    explicit PhysicsGravityFactorData(float factor) noexcept
        : value(factor) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 通常の重力
    [[nodiscard]] static PhysicsGravityFactorData Normal() noexcept {
        return PhysicsGravityFactorData(1.0f);
    }

    //! @brief 重力なし（宇宙、浮遊）
    [[nodiscard]] static PhysicsGravityFactorData Zero() noexcept {
        return PhysicsGravityFactorData(0.0f);
    }

    //! @brief 軽い重力（月面、水中浮力）
    [[nodiscard]] static PhysicsGravityFactorData Light() noexcept {
        return PhysicsGravityFactorData(0.16f);  // 月面重力 ≈ 1/6
    }

    //! @brief 強い重力（木星、重いオブジェクト）
    [[nodiscard]] static PhysicsGravityFactorData Heavy() noexcept {
        return PhysicsGravityFactorData(2.5f);
    }

    //! @brief 反転重力（上昇）
    [[nodiscard]] static PhysicsGravityFactorData Inverted() noexcept {
        return PhysicsGravityFactorData(-1.0f);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 重力を適用
    //! @param worldGravity ワールドの重力ベクトル
    //! @return スケールされた重力
    template<typename T>
    [[nodiscard]] T ApplyGravity(const T& worldGravity) const noexcept {
        return worldGravity * value;
    }

    //! @brief 重力の影響を受けるか
    [[nodiscard]] bool IsAffectedByGravity() const noexcept {
        return value != 0.0f;
    }

    //! @brief 浮遊オブジェクトか（重力なしまたは上昇）
    [[nodiscard]] bool IsFloating() const noexcept {
        return value <= 0.0f;
    }
};

// コンパイル時検証
ECS_COMPONENT(PhysicsGravityFactorData);
static_assert(sizeof(PhysicsGravityFactorData) == 4, "PhysicsGravityFactorData must be 4 bytes");

} // namespace ECS
