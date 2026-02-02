//----------------------------------------------------------------------------
//! @file   mesh_collider.h
//! @brief  メッシュコライダー - BVHによる高速レイキャスト
//----------------------------------------------------------------------------
#pragma once

#include "raycast.h"
#include "bvh.h"
#include "engine/mesh/mesh.h"
#include "engine/mesh/vertex_format.h"
#include <vector>
#include <memory>

namespace Physics {

//============================================================================
//! @brief メッシュコライダー
//!
//! BVH（Bounding Volume Hierarchy）を使用した高速レイキャスト。
//! 構築時にBVHを生成し、レイキャストをO(log n)で実行。
//!
//! @code
//! auto collider = MeshCollider::CreateFromMeshDesc(meshDesc);
//! collider->SetWorldMatrix(worldMatrix);
//!
//! RaycastHit hit;
//! if (collider->Raycast(ray, 100.0f, hit)) {
//!     // ヒット処理
//! }
//! @endcode
//============================================================================
class MeshCollider {
public:
    //------------------------------------------------------------------------
    // 生成
    //------------------------------------------------------------------------

    //! @brief MeshDescからコライダーを生成
    [[nodiscard]] static std::shared_ptr<MeshCollider> CreateFromMeshDesc(
        const MeshDesc& desc)
    {
        auto collider = std::make_shared<MeshCollider>();

        // 頂点位置のみ抽出
        collider->positions_.reserve(desc.vertices.size());
        for (const auto& v : desc.vertices) {
            collider->positions_.push_back(v.position);
        }

        // インデックスをコピー
        collider->indices_ = desc.indices;

        // バウンディングボックス
        collider->localBounds_ = desc.bounds;

        // BVH構築
        collider->BuildBVH();

        return collider;
    }

    //! @brief 頂点・インデックスから直接生成
    [[nodiscard]] static std::shared_ptr<MeshCollider> Create(
        const std::vector<Vector3>& positions,
        const std::vector<uint32_t>& indices)
    {
        auto collider = std::make_shared<MeshCollider>();
        collider->positions_ = positions;
        collider->indices_ = indices;

        // バウンディングボックス計算
        for (const auto& p : positions) {
            collider->localBounds_.Expand(p);
        }

        // BVH構築
        collider->BuildBVH();

        return collider;
    }

    //------------------------------------------------------------------------
    // ワールド変換
    //------------------------------------------------------------------------

    //! @brief ワールド変換行列を設定
    void SetWorldMatrix(const Matrix& world) noexcept {
        worldMatrix_ = world;
        worldMatrixInverse_ = world.Invert();
        UpdateWorldBounds();
    }

    //! @brief ワールド変換行列を取得
    [[nodiscard]] const Matrix& GetWorldMatrix() const noexcept {
        return worldMatrix_;
    }

    //------------------------------------------------------------------------
    // レイキャスト
    //------------------------------------------------------------------------

    //! @brief レイキャスト（ワールド空間）- BVH使用
    [[nodiscard]] bool Raycast(
        const Ray& ray,
        float maxDistance,
        RaycastHit& outHit) const
    {
        outHit.Reset();

        // まずワールドAABBでカリング
        if (!RayAABBIntersect(ray, worldBounds_.min, worldBounds_.max, maxDistance)) {
            return false;
        }

        // レイをローカル空間に変換
        Vector3 localOrigin = Vector3::Transform(ray.origin, worldMatrixInverse_);
        Vector3 localDir = Vector3::TransformNormal(ray.direction, worldMatrixInverse_);
        localDir.Normalize();

        // BVHでレイキャスト
        float t;
        uint32_t triIndex;
        if (!bvh_.Intersect(localOrigin, localDir, maxDistance, t, triIndex)) {
            return false;
        }

        // ヒット情報を構築
        Vector3 localPoint = localOrigin + localDir * t;

        const Vector3& v0 = positions_[indices_[triIndex * 3 + 0]];
        const Vector3& v1 = positions_[indices_[triIndex * 3 + 1]];
        const Vector3& v2 = positions_[indices_[triIndex * 3 + 2]];
        Vector3 localNormal = CalculateTriangleNormal(v0, v1, v2);

        // ワールド空間に変換
        outHit.point = Vector3::Transform(localPoint, worldMatrix_);
        outHit.normal = Vector3::TransformNormal(localNormal, worldMatrix_);
        outHit.normal.Normalize();
        outHit.distance = Vector3::Distance(ray.origin, outHit.point);
        outHit.hit = true;

        return true;
    }

    //------------------------------------------------------------------------
    // バウンディングボックス
    //------------------------------------------------------------------------

    [[nodiscard]] const BoundingBox& GetLocalBounds() const noexcept {
        return localBounds_;
    }

    [[nodiscard]] const BoundingBox& GetWorldBounds() const noexcept {
        return worldBounds_;
    }

    //------------------------------------------------------------------------
    // 情報取得
    //------------------------------------------------------------------------

    [[nodiscard]] size_t GetTriangleCount() const noexcept {
        return indices_.size() / 3;
    }

    [[nodiscard]] size_t GetVertexCount() const noexcept {
        return positions_.size();
    }

    [[nodiscard]] bool HasBVH() const noexcept {
        return bvh_.IsBuilt();
    }

public:
    MeshCollider() = default;

private:
    //! @brief BVHを構築
    void BuildBVH() {
        const size_t triCount = indices_.size() / 3;
        if (triCount == 0) return;

        std::vector<Triangle> triangles;
        triangles.reserve(triCount);

        for (size_t i = 0; i < triCount; ++i) {
            Triangle tri;
            tri.v0 = positions_[indices_[i * 3 + 0]];
            tri.v1 = positions_[indices_[i * 3 + 1]];
            tri.v2 = positions_[indices_[i * 3 + 2]];
            tri.index = static_cast<uint32_t>(i);
            triangles.push_back(tri);
        }

        bvh_.Build(std::move(triangles));
    }

    void UpdateWorldBounds() {
        worldBounds_.min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
        worldBounds_.max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        Vector3 corners[8] = {
            {localBounds_.min.x, localBounds_.min.y, localBounds_.min.z},
            {localBounds_.max.x, localBounds_.min.y, localBounds_.min.z},
            {localBounds_.min.x, localBounds_.max.y, localBounds_.min.z},
            {localBounds_.max.x, localBounds_.max.y, localBounds_.min.z},
            {localBounds_.min.x, localBounds_.min.y, localBounds_.max.z},
            {localBounds_.max.x, localBounds_.min.y, localBounds_.max.z},
            {localBounds_.min.x, localBounds_.max.y, localBounds_.max.z},
            {localBounds_.max.x, localBounds_.max.y, localBounds_.max.z},
        };

        for (const auto& corner : corners) {
            Vector3 worldCorner = Vector3::Transform(corner, worldMatrix_);
            worldBounds_.Expand(worldCorner);
        }
    }

    std::vector<Vector3> positions_;
    std::vector<uint32_t> indices_;
    BoundingBox localBounds_;
    BoundingBox worldBounds_;
    Matrix worldMatrix_ = Matrix::Identity;
    Matrix worldMatrixInverse_ = Matrix::Identity;
    BVH bvh_;  // 空間分割構造
};

using MeshColliderPtr = std::shared_ptr<MeshCollider>;

} // namespace Physics
