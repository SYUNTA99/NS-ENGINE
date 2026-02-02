//----------------------------------------------------------------------------
//! @file   scale_velocity_data.h
//! @brief  ECS ScaleVelocityData - スケール変化速度データ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief スケール変化速度データ（POD構造体）
//!
//! ScaleUpdateSystemによってScaleDataを更新するための入力。
//!
//! @note メモリレイアウト: 16 bytes, 16B境界アライン
//============================================================================
struct alignas(16) ScaleVelocityData : public IComponentData {
    Vector3 value = Vector3::Zero;   //!< スケール変化速度/秒 (12 bytes)
    float _pad0 = 0.0f;              //!< パディング (4 bytes)

    ScaleVelocityData() = default;
    explicit ScaleVelocityData(const Vector3& v) : value(v) {}
    ScaleVelocityData(float x, float y, float z) : value(x, y, z) {}

    //! @brief 均一スケール変化を設定
    void SetUniform(float scalePerSec) noexcept {
        value = Vector3(scalePerSec, scalePerSec, scalePerSec);
    }
};

ECS_COMPONENT(ScaleVelocityData);

} // namespace ECS
