//----------------------------------------------------------------------------
//! @file   physics_mass_override_data.h
//! @brief  ECS PhysicsMassOverrideData - キネマティック設定
//! @ref    Unity DOTS PhysicsMassOverride
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief 質量オーバーライドデータ（POD構造体）
//!
//! 物理挙動のオーバーライド設定。
//! キネマティックボディ（スクリプト制御で動くが物理影響を受けない）などに使用。
//!
//! @note メモリレイアウト: 4 bytes
//!
//! @code
//! // 動くプラットフォーム（キネマティック）
//! world.AddComponent<PhysicsMassOverrideData>(platform, PhysicsMassOverrideData::Kinematic());
//!
//! // システムでの適用
//! if (override.IsKinematic()) {
//!     // 力やインパルスを無視
//!     return;
//! }
//! if (override.ShouldSetVelocityToZero()) {
//!     velocity = Vector3::Zero;
//! }
//! @endcode
//============================================================================
struct PhysicsMassOverrideData : public IComponentData {
    uint8_t isKinematic = 0;        //!< キネマティックフラグ (1 = 力を受けない)
    uint8_t setVelocityToZero = 0;  //!< 速度ゼロ強制フラグ (1 = 毎フレーム速度リセット)
    uint16_t _pad = 0;              //!< パディング

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    PhysicsMassOverrideData() = default;

    PhysicsMassOverrideData(bool kinematic, bool zeroVel = false) noexcept
        : isKinematic(kinematic ? 1 : 0)
        , setVelocityToZero(zeroVel ? 1 : 0) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 動的オブジェクト（オーバーライドなし）
    [[nodiscard]] static PhysicsMassOverrideData Dynamic() noexcept {
        return PhysicsMassOverrideData(false, false);
    }

    //! @brief キネマティック（力を受けない、スクリプト制御）
    [[nodiscard]] static PhysicsMassOverrideData Kinematic() noexcept {
        return PhysicsMassOverrideData(true, false);
    }

    //! @brief 完全に固定（速度もゼロに強制）
    [[nodiscard]] static PhysicsMassOverrideData Frozen() noexcept {
        return PhysicsMassOverrideData(true, true);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief キネマティックか
    [[nodiscard]] bool IsKinematic() const noexcept {
        return isKinematic != 0;
    }

    //! @brief 速度をゼロに強制するか
    [[nodiscard]] bool ShouldSetVelocityToZero() const noexcept {
        return setVelocityToZero != 0;
    }

    //! @brief キネマティック設定
    void SetKinematic(bool value) noexcept {
        isKinematic = value ? 1 : 0;
    }

    //! @brief 速度ゼロ強制設定
    void SetVelocityToZero(bool value) noexcept {
        setVelocityToZero = value ? 1 : 0;
    }

    //! @brief 動的オブジェクトか（力を受ける）
    [[nodiscard]] bool IsDynamic() const noexcept {
        return isKinematic == 0;
    }
};

// コンパイル時検証
ECS_COMPONENT(PhysicsMassOverrideData);
static_assert(sizeof(PhysicsMassOverrideData) == 4, "PhysicsMassOverrideData must be 4 bytes");

} // namespace ECS
