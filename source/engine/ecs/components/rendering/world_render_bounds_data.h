//----------------------------------------------------------------------------
//! @file   world_render_bounds_data.h
//! @brief  ECS WorldRenderBoundsData - ワールド空間AABB
//! @ref    Unity DOTS WorldRenderBounds
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"
#include <limits>

namespace ECS {

//============================================================================
//! @brief ワールド空間AABBデータ（POD構造体）
//!
//! フラスタムカリング用のワールド空間バウンディングボックス。
//! RenderBoundsData + LocalToWorld から毎フレーム計算される。
//!
//! @note メモリレイアウト: 32 bytes, 16B境界アライン
//! @note システムが自動更新するコンポーネント
//!
//! @code
//! // カリングシステム
//! world.ForEach<WorldRenderBoundsData, MeshData>(
//!     [&](Actor e, auto& bounds, auto& mesh) {
//!         if (frustum.Intersects(bounds)) {
//!             // 描画対象
//!             visibleEntities.push_back(e);
//!         }
//!     });
//! @endcode
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) WorldRenderBoundsData : public IComponentData {
    Vector3 minPoint = Vector3::Zero;    //!< ワールド空間最小点 (12B)
    float _pad0 = 0.0f;                  //!< パディング (4B)
    Vector3 maxPoint = Vector3::Zero;    //!< ワールド空間最大点 (12B)
    float _pad1 = 0.0f;                  //!< パディング (4B)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    WorldRenderBoundsData() = default;

    WorldRenderBoundsData(const Vector3& minPt, const Vector3& maxPt) noexcept
        : minPoint(minPt), maxPoint(maxPt) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 無効なバウンズ（カリングで必ず除外）
    [[nodiscard]] static WorldRenderBoundsData Invalid() noexcept {
        return WorldRenderBoundsData(
            Vector3((std::numeric_limits<float>::max)()),
            Vector3((std::numeric_limits<float>::lowest)())
        );
    }

    //! @brief Center/Extentsから作成
    [[nodiscard]] static WorldRenderBoundsData FromCenterExtents(
        const Vector3& center,
        const Vector3& extents) noexcept
    {
        return WorldRenderBoundsData(center - extents, center + extents);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 中心を取得
    [[nodiscard]] Vector3 GetCenter() const noexcept {
        return (minPoint + maxPoint) * 0.5f;
    }

    //! @brief 半サイズを取得
    [[nodiscard]] Vector3 GetExtents() const noexcept {
        return (maxPoint - minPoint) * 0.5f;
    }

    //! @brief サイズを取得
    [[nodiscard]] Vector3 GetSize() const noexcept {
        return maxPoint - minPoint;
    }

    //! @brief 有効なバウンズか
    [[nodiscard]] bool IsValid() const noexcept {
        return minPoint.x <= maxPoint.x && minPoint.y <= maxPoint.y && minPoint.z <= maxPoint.z;
    }

    //! @brief 点がAABB内にあるか
    [[nodiscard]] bool Contains(const Vector3& point) const noexcept {
        return point.x >= minPoint.x && point.x <= maxPoint.x
            && point.y >= minPoint.y && point.y <= maxPoint.y
            && point.z >= minPoint.z && point.z <= maxPoint.z;
    }

    //! @brief 他のAABBと交差するか
    [[nodiscard]] bool Intersects(const WorldRenderBoundsData& other) const noexcept {
        return minPoint.x <= other.maxPoint.x && maxPoint.x >= other.minPoint.x
            && minPoint.y <= other.maxPoint.y && maxPoint.y >= other.minPoint.y
            && minPoint.z <= other.maxPoint.z && maxPoint.z >= other.minPoint.z;
    }

    //! @brief 球と交差するか
    [[nodiscard]] bool IntersectsSphere(const Vector3& center, float radius) const noexcept {
        // 最近接点を計算
        Vector3 closest(
            (std::max)(minPoint.x, (std::min)(center.x, maxPoint.x)),
            (std::max)(minPoint.y, (std::min)(center.y, maxPoint.y)),
            (std::max)(minPoint.z, (std::min)(center.z, maxPoint.z))
        );
        // 距離をチェック
        Vector3 diff = closest - center;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        return distSq <= radius * radius;
    }

    //! @brief 平面の前側にあるか（フラスタムカリング用）
    //! @param planeNormal 平面の法線（単位ベクトル）
    //! @param planeDistance 平面の距離（原点からの符号付き距離）
    //! @return 前側なら正、後ろ側なら負、交差なら0
    [[nodiscard]] int ClassifyAgainstPlane(const Vector3& planeNormal, float planeDistance) const noexcept {
        // P-vertex（平面に最も近い頂点）とN-vertex（最も遠い頂点）を計算
        Vector3 pVertex(
            (planeNormal.x >= 0) ? maxPoint.x : minPoint.x,
            (planeNormal.y >= 0) ? maxPoint.y : minPoint.y,
            (planeNormal.z >= 0) ? maxPoint.z : minPoint.z
        );
        Vector3 nVertex(
            (planeNormal.x >= 0) ? minPoint.x : maxPoint.x,
            (planeNormal.y >= 0) ? minPoint.y : maxPoint.y,
            (planeNormal.z >= 0) ? minPoint.z : maxPoint.z
        );

        float pDist = planeNormal.x * pVertex.x + planeNormal.y * pVertex.y + planeNormal.z * pVertex.z + planeDistance;
        if (pDist < 0) return -1;  // 完全に後ろ側

        float nDist = planeNormal.x * nVertex.x + planeNormal.y * nVertex.y + planeNormal.z * nVertex.z + planeDistance;
        if (nDist > 0) return 1;   // 完全に前側

        return 0;  // 交差
    }
};
#pragma warning(pop)

// コンパイル時検証
ECS_COMPONENT(WorldRenderBoundsData);
static_assert(sizeof(WorldRenderBoundsData) == 32, "WorldRenderBoundsData must be 32 bytes");

} // namespace ECS
