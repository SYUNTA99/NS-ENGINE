//----------------------------------------------------------------------------
//! @file   angular_velocity_data.h
//! @brief  ECS AngularVelocityData - 角速度データ構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 角速度データ（POD構造体）
//!
//! RotationUpdateSystemによってRotationDataを更新するための入力。
//! 軸角度表現（axis * angle/sec）。
//!
//! @note メモリレイアウト: 16 bytes, 16B境界アライン
//============================================================================
struct alignas(16) AngularVelocityData : public IComponentData {
    Vector3 value = Vector3::Zero;   //!< 角速度（rad/s、軸方向×角速度）(12 bytes)
    float _pad0 = 0.0f;              //!< パディング (4 bytes)

    AngularVelocityData() = default;
    explicit AngularVelocityData(const Vector3& v) : value(v) {}
    AngularVelocityData(float x, float y, float z) : value(x, y, z) {}

    //! @brief Y軸回転（ヨー）速度を設定
    void SetYawSpeed(float radPerSec) noexcept {
        value = Vector3(0.0f, radPerSec, 0.0f);
    }

    //! @brief X軸回転（ピッチ）速度を設定
    void SetPitchSpeed(float radPerSec) noexcept {
        value = Vector3(radPerSec, 0.0f, 0.0f);
    }

    //! @brief Z軸回転（ロール）速度を設定
    void SetRollSpeed(float radPerSec) noexcept {
        value = Vector3(0.0f, 0.0f, radPerSec);
    }
};

ECS_COMPONENT(AngularVelocityData);

} // namespace ECS
