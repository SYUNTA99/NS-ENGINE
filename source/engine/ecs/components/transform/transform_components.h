//----------------------------------------------------------------------------
//! @file   transform_components.h
//! @brief  Transform Components - 全Transformコンポーネント
//----------------------------------------------------------------------------
#pragma once

// Transform コンポーネント
#include "local_transform.h"
#include "local_to_world.h"
#include "post_transform_matrix.h"
#include "parent.h"
#include "previous_parent.h"
#include "hierarchy_depth_data.h"
#include "transform_tags.h"

namespace ECS {

//============================================================================
//! @brief Transform関連のメモリレイアウト情報
//!
//! 各コンポーネントサイズ:
//!
//! | コンポーネント        | サイズ | 用途                    |
//! |-----------------------|--------|-------------------------|
//! | LocalTransform        | 48B    | TRS統合                 |
//! | LocalToWorld          | 64B    | ワールド行列            |
//! | PostTransformMatrix   | 64B    | シアー/非均一スケール   |
//! | Parent                | 4B     | 親参照                  |
//! | PreviousParent        | 4B     | 前フレーム親            |
//! | HierarchyDepthData    | 2B     | 階層深度                |
//! | TransformDirty        | 0B     | 変更フラグ（タグ）      |
//! | StaticTransform       | 0B     | 静的フラグ（タグ）      |
//! | HierarchyRoot         | 0B     | ルートフラグ（タグ）    |
//! | TransformInitialized  | 0B     | 初期化済みフラグ（タグ）|
//!
//! 典型的な構成例:
//!
//! 1. ルートエンティティ（LocalTransform + LocalToWorld）: 112B
//!
//! 2. 子エンティティ（LocalTransform + LocalToWorld + Parent + PreviousParent + HierarchyDepthData）: 122B
//!
//! 3. シアー付き（LocalTransform + LocalToWorld + PostTransformMatrix）: 176B
//!
//============================================================================

} // namespace ECS
