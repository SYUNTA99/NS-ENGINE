//----------------------------------------------------------------------------
//! @file   velocity_data.h
//! @brief  ECS VelocityData - 速度データ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 速度データ（POD構造体）
//!
//! MovementSystemによってPositionDataを更新するための入力。
//!
//! @note メモリレイアウト: 16 bytes, 16B境界アライン
//============================================================================
struct alignas(16) VelocityData : public IComponentData {
    Vector3 value = Vector3::Zero;   //!< 速度 (12 bytes)
    float _pad0 = 0.0f;              //!< パディング (4 bytes)

    VelocityData() = default;
    explicit VelocityData(const Vector3& v) : value(v) {}
    VelocityData(float x, float y, float z) : value(x, y, z) {}
};

ECS_COMPONENT(VelocityData);

} // namespace ECS
