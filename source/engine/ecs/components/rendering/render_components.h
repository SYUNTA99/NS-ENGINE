//----------------------------------------------------------------------------
//! @file   render_components.h
//! @brief  Rendering Components - 全レンダリングコンポーネント
//! @ref    Unity DOTS Unity.Rendering
//----------------------------------------------------------------------------
#pragma once

// Rendering コンポーネント
#include "sprite_data.h"
#include "mesh_data.h"
#include "light_component_data.h"
#include "render_bounds_data.h"
#include "world_render_bounds_data.h"
#include "lod_range_data.h"

namespace ECS {

//============================================================================
//! @brief Rendering関連のメモリレイアウト情報
//!
//! 各コンポーネントサイズ:
//!
//! | コンポーネント           | サイズ | 用途                        |
//! |-------------------------|--------|----------------------------|
//! | RenderBoundsData        | 32B    | ローカル空間AABB            |
//! | WorldRenderBoundsData   | 32B    | ワールド空間AABB（カリング用）|
//! | LODRangeData            | 8B     | LOD距離設定                 |
//!
//! 典型的な構成例:
//!
//! 1. カリング対応メッシュ:
//!    LocalTransform + LocalToWorld + MeshData + RenderBoundsData + WorldRenderBoundsData
//!    = 48 + 64 + 140 + 32 + 32 = 316B
//!
//! 2. LOD対応オブジェクト:
//!    上記 + LODRangeData = 324B
//!
//! 3. スプライト（カリング対応）:
//!    LocalTransform + LocalToWorld + SpriteData + WorldRenderBoundsData
//!    = 48 + 64 + 80 + 32 = 224B
//!
//============================================================================

} // namespace ECS
