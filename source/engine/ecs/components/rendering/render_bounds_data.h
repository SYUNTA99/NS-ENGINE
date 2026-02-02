//----------------------------------------------------------------------------
//! @file   render_bounds_data.h
//! @brief  ECS RenderBoundsData - ローカル空間AABB
//! @ref    Unity DOTS RenderBounds
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief ローカル空間AABBデータ（POD構造体）
//!
//! メッシュのバウンディングボックス。カリング計算の入力として使用。
//! メッシュロード時に設定され、LocalToWorldと組み合わせてワールドAABBを計算。
//!
//! @note メモリレイアウト: 32 bytes, 16B境界アライン
//!
//! @code
//! // メッシュロード時に設定
//! auto bounds = mesh->GetBounds();
//! world.AddComponent<RenderBoundsData>(actor, bounds.center, bounds.extents);
//!
//! // WorldRenderBoundsData を計算（システム）
//! world.ForEach<RenderBoundsData, LocalToWorld, WorldRenderBoundsData>(
//!     [](Actor e, auto& local, auto& ltw, auto& world) {
//!         world = TransformAABB(local, ltw.value);
//!     });
//! @endcode
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) RenderBoundsData : public IComponentData {
    Vector3 center = Vector3::Zero;      //!< ローカル空間中心 (12B)
    float _pad0 = 0.0f;                  //!< パディング (4B)
    Vector3 extents = Vector3(0.5f);     //!< 半サイズ（各軸の半径）(12B)
    float _pad1 = 0.0f;                  //!< パディング (4B)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    RenderBoundsData() = default;

    RenderBoundsData(const Vector3& c, const Vector3& e) noexcept
        : center(c), extents(e) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 単位キューブ（中心0、半径0.5）
    [[nodiscard]] static RenderBoundsData UnitCube() noexcept {
        return RenderBoundsData(Vector3::Zero, Vector3(0.5f));
    }

    //! @brief 単位球（半径1）
    [[nodiscard]] static RenderBoundsData UnitSphere() noexcept {
        return RenderBoundsData(Vector3::Zero, Vector3(1.0f));
    }

    //! @brief Min/Maxから作成
    [[nodiscard]] static RenderBoundsData FromMinMax(const Vector3& min, const Vector3& max) noexcept {
        Vector3 center = (min + max) * 0.5f;
        Vector3 extents = (max - min) * 0.5f;
        return RenderBoundsData(center, extents);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 最小点を取得
    [[nodiscard]] Vector3 GetMin() const noexcept {
        return center - extents;
    }

    //! @brief 最大点を取得
    [[nodiscard]] Vector3 GetMax() const noexcept {
        return center + extents;
    }

    //! @brief サイズを取得
    [[nodiscard]] Vector3 GetSize() const noexcept {
        return extents * 2.0f;
    }

    //! @brief 点がAABB内にあるか
    [[nodiscard]] bool Contains(const Vector3& point) const noexcept {
        Vector3 min = GetMin();
        Vector3 max = GetMax();
        return point.x >= min.x && point.x <= max.x
            && point.y >= min.y && point.y <= max.y
            && point.z >= min.z && point.z <= max.z;
    }

    //! @brief 他のAABBと交差するか
    [[nodiscard]] bool Intersects(const RenderBoundsData& other) const noexcept {
        Vector3 minA = GetMin(), maxA = GetMax();
        Vector3 minB = other.GetMin(), maxB = other.GetMax();
        return minA.x <= maxB.x && maxA.x >= minB.x
            && minA.y <= maxB.y && maxA.y >= minB.y
            && minA.z <= maxB.z && maxA.z >= minB.z;
    }

    //! @brief 他のAABBを包含するように拡大
    void Encapsulate(const RenderBoundsData& other) noexcept {
        Vector3 minA = GetMin(), maxA = GetMax();
        Vector3 minB = other.GetMin(), maxB = other.GetMax();
        Vector3 newMin(
            (std::min)(minA.x, minB.x),
            (std::min)(minA.y, minB.y),
            (std::min)(minA.z, minB.z)
        );
        Vector3 newMax(
            (std::max)(maxA.x, maxB.x),
            (std::max)(maxA.y, maxB.y),
            (std::max)(maxA.z, maxB.z)
        );
        center = (newMin + newMax) * 0.5f;
        extents = (newMax - newMin) * 0.5f;
    }
};
#pragma warning(pop)

// コンパイル時検証
ECS_COMPONENT(RenderBoundsData);
static_assert(sizeof(RenderBoundsData) == 32, "RenderBoundsData must be 32 bytes");

} // namespace ECS
