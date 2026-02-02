//----------------------------------------------------------------------------
//! @file   hierarchy_depth_data.h
//! @brief  ECS HierarchyDepthData - 階層深度（Fine-grained Transform）
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief 階層深度データ（Fine-grained）
//!
//! 親子階層での深度を保持。TransformSystemがソートに使用。
//! 深度0 = ルート、深度1 = ルートの子、...
//!
//! @note HierarchyRegistryがSetParent時に自動計算。
//!       深度順にソートすることで親→子の順で行列計算可能。
//!
//! @code
//! // TransformSystemでの使用
//! // 深度でソートしてから処理
//! world.Query<HierarchyDepthData, LocalToWorldData>()
//!     .SortBy<HierarchyDepthData>([](auto& a, auto& b) { return a.depth < b.depth; })
//!     .ForEach([](Actor e, auto& depth, auto& ltw) { ... });
//! @endcode
//============================================================================
struct HierarchyDepthData : public IComponentData {
    uint16_t depth = 0;                       //!< 階層深度 (2 bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    HierarchyDepthData() = default;

    explicit HierarchyDepthData(uint16_t d) noexcept
        : depth(d) {}

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief ルートかどうか
    [[nodiscard]] bool IsRoot() const noexcept {
        return depth == 0;
    }

    //! @brief 深度を設定
    void SetDepth(uint16_t d) noexcept {
        depth = d;
    }

    //! @brief 深度を1増加
    void IncrementDepth() noexcept {
        ++depth;
    }
};

// コンパイル時検証
ECS_COMPONENT(HierarchyDepthData);
static_assert(sizeof(HierarchyDepthData) == 2, "HierarchyDepthData must be 2 bytes");

} // namespace ECS
