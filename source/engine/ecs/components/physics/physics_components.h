//----------------------------------------------------------------------------
//! @file   physics_components.h
//! @brief  Physics Components - 全物理コンポーネント
//! @ref    Unity DOTS Unity.Physics
//----------------------------------------------------------------------------
#pragma once

// Physics コンポーネント
#include "physics_mass_data.h"
#include "physics_damping_data.h"
#include "physics_gravity_factor_data.h"
#include "physics_mass_override_data.h"

namespace ECS {

//============================================================================
//! @brief Physics関連のメモリレイアウト情報
//!
//! 各コンポーネントサイズ:
//!
//! | コンポーネント             | サイズ | 用途                       |
//! |---------------------------|--------|---------------------------|
//! | PhysicsMassData           | 64B    | 質量・慣性テンソル          |
//! | PhysicsDampingData        | 8B     | 減衰（空気抵抗）            |
//! | PhysicsGravityFactorData  | 4B     | 個別重力スケール            |
//! | PhysicsMassOverrideData   | 4B     | キネマティック設定          |
//!
//! 典型的な構成例:
//!
//! 1. 動的物理オブジェクト:
//!    LocalTransform + LocalToWorld + VelocityData + PhysicsMassData + PhysicsDampingData
//!    = 48 + 64 + 16 + 64 + 8 = 200B
//!
//! 2. キネマティックオブジェクト（プラットフォーム等）:
//!    LocalTransform + LocalToWorld + VelocityData + PhysicsMassOverrideData
//!    = 48 + 64 + 16 + 4 = 132B
//!
//! 3. 浮遊オブジェクト:
//!    LocalTransform + LocalToWorld + VelocityData + PhysicsGravityFactorData
//!    = 48 + 64 + 16 + 4 = 132B
//!
//============================================================================

} // namespace ECS
