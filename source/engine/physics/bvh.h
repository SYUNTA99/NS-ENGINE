//----------------------------------------------------------------------------
//! @file   bvh.h
//! @brief  BVH (Bounding Volume Hierarchy) - 高速レイキャスト用空間分割
//----------------------------------------------------------------------------
#pragma once


#include "engine/math/math_types.h"
#include <vector>
#include <algorithm>
#include <cstdint>

namespace Physics {

//============================================================================
//! @brief AABB (Axis-Aligned Bounding Box)
//============================================================================
struct AABB {
    Vector3 min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    //! @brief 点を含むように拡張
    void Expand(const Vector3& point) noexcept {
        min = Vector3::Min(min, point);
        max = Vector3::Max(max, point);
    }

    //! @brief 別のAABBを含むように拡張
    void Expand(const AABB& other) noexcept {
        min = Vector3::Min(min, other.min);
        max = Vector3::Max(max, other.max);
    }

    //! @brief 中心を取得
    [[nodiscard]] Vector3 Center() const noexcept {
        return (min + max) * 0.5f;
    }

    //! @brief 表面積を取得（SAHで使用）
    [[nodiscard]] float SurfaceArea() const noexcept {
        Vector3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    //! @brief 最長軸を取得（0=X, 1=Y, 2=Z）
    [[nodiscard]] int LongestAxis() const noexcept {
        Vector3 d = max - min;
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }

    //! @brief 軸インデックスで成分を取得
    [[nodiscard]] static float GetAxis(const Vector3& v, int axis) noexcept {
        switch (axis) {
            case 0: return v.x;
            case 1: return v.y;
            default: return v.z;
        }
    }

    //! @brief レイとの交差判定
    [[nodiscard]] bool Intersect(const Vector3& origin, const Vector3& invDir, float tMax) const noexcept {
        float t1 = (min.x - origin.x) * invDir.x;
        float t2 = (max.x - origin.x) * invDir.x;
        float tmin = (std::min)(t1, t2);
        float tmax = (std::max)(t1, t2);

        t1 = (min.y - origin.y) * invDir.y;
        t2 = (max.y - origin.y) * invDir.y;
        tmin = (std::max)(tmin, (std::min)(t1, t2));
        tmax = (std::min)(tmax, (std::max)(t1, t2));

        t1 = (min.z - origin.z) * invDir.z;
        t2 = (max.z - origin.z) * invDir.z;
        tmin = (std::max)(tmin, (std::min)(t1, t2));
        tmax = (std::min)(tmax, (std::max)(t1, t2));

        return tmax >= (std::max)(0.0f, tmin) && tmin < tMax;
    }
};

//============================================================================
//! @brief 三角形データ
//============================================================================
struct Triangle {
    Vector3 v0, v1, v2;
    uint32_t index;  // 元のインデックス

    [[nodiscard]] AABB GetAABB() const noexcept {
        AABB aabb;
        aabb.Expand(v0);
        aabb.Expand(v1);
        aabb.Expand(v2);
        return aabb;
    }

    [[nodiscard]] Vector3 Centroid() const noexcept {
        return (v0 + v1 + v2) / 3.0f;
    }
};

//============================================================================
//! @brief BVHノード
//============================================================================
struct BVHNode {
    AABB bounds;
    uint32_t leftFirst;   // 内部ノード: 左子のインデックス, 葉: 三角形開始インデックス
    uint32_t triCount;    // 0なら内部ノード、それ以外は葉の三角形数

    [[nodiscard]] bool IsLeaf() const noexcept { return triCount > 0; }
};

//============================================================================
//! @brief BVH (Bounding Volume Hierarchy)
//!
//! 三角形メッシュに対する高速レイキャスト用の空間分割構造。
//! 構築: O(n log n)、レイキャスト: O(log n)
//!
//! @code
//! BVH bvh;
//! bvh.Build(triangles);
//!
//! float t;
//! uint32_t triIndex;
//! if (bvh.Intersect(rayOrigin, rayDir, maxDist, t, triIndex)) {
//!     // ヒット処理
//! }
//! @endcode
//============================================================================
class BVH {
public:
    //! @brief BVHを構築
    void Build(std::vector<Triangle> triangles) {
        if (triangles.empty()) return;

        triangles_ = std::move(triangles);
        nodes_.clear();
        nodes_.reserve(triangles_.size() * 2);

        // ルートノード作成
        nodes_.push_back({});
        rootIndex_ = 0;
        nodesUsed_ = 1;

        // 全三角形のAABBを計算
        BVHNode& root = nodes_[rootIndex_];
        root.leftFirst = 0;
        root.triCount = static_cast<uint32_t>(triangles_.size());

        UpdateNodeBounds(rootIndex_);

        // 再帰的に分割
        Subdivide(rootIndex_);

        LOG_INFO("[BVH] Built with " + std::to_string(triangles_.size()) +
                 " triangles, " + std::to_string(nodesUsed_) + " nodes");
    }

    //! @brief レイとの交差判定
    //! @param origin レイの原点
    //! @param dir レイの方向（正規化）
    //! @param tMax 最大距離
    //! @param outT [out] 交差距離
    //! @param outTriIndex [out] 交差した三角形のインデックス
    //! @return 交差したらtrue
    [[nodiscard]] bool Intersect(
        const Vector3& origin,
        const Vector3& dir,
        float tMax,
        float& outT,
        uint32_t& outTriIndex) const
    {
        if (nodes_.empty()) return false;

        // 逆方向を事前計算（除算を避ける）
        Vector3 invDir(
            std::abs(dir.x) > 1e-8f ? 1.0f / dir.x : 1e8f,
            std::abs(dir.y) > 1e-8f ? 1.0f / dir.y : 1e8f,
            std::abs(dir.z) > 1e-8f ? 1.0f / dir.z : 1e8f
        );

        float closestT = tMax;
        uint32_t closestTri = UINT32_MAX;

        // スタックベースのトラバーサル
        uint32_t stack[64];
        int stackPtr = 0;
        stack[stackPtr++] = rootIndex_;

        while (stackPtr > 0) {
            uint32_t nodeIdx = stack[--stackPtr];
            const BVHNode& node = nodes_[nodeIdx];

            // AABBテスト
            if (!node.bounds.Intersect(origin, invDir, closestT)) {
                continue;
            }

            if (node.IsLeaf()) {
                // 葉ノード：三角形をテスト
                for (uint32_t i = 0; i < node.triCount; ++i) {
                    const Triangle& tri = triangles_[node.leftFirst + i];
                    float t;
                    if (IntersectTriangle(origin, dir, tri, closestT, t)) {
                        closestT = t;
                        closestTri = tri.index;
                    }
                }
            } else {
                // 内部ノード：子をスタックに追加
                stack[stackPtr++] = node.leftFirst;
                stack[stackPtr++] = node.leftFirst + 1;
            }
        }

        if (closestTri != UINT32_MAX) {
            outT = closestT;
            outTriIndex = closestTri;
            return true;
        }
        return false;
    }

    //! @brief 構築済みか
    [[nodiscard]] bool IsBuilt() const noexcept { return !nodes_.empty(); }

    //! @brief 三角形数を取得
    [[nodiscard]] size_t GetTriangleCount() const noexcept { return triangles_.size(); }

private:
    //! @brief ノードのAABBを更新
    void UpdateNodeBounds(uint32_t nodeIdx) {
        BVHNode& node = nodes_[nodeIdx];
        node.bounds = AABB();
        for (uint32_t i = 0; i < node.triCount; ++i) {
            node.bounds.Expand(triangles_[node.leftFirst + i].GetAABB());
        }
    }

    //! @brief ノードを分割
    void Subdivide(uint32_t nodeIdx) {
        BVHNode& node = nodes_[nodeIdx];

        // 三角形が少なければ葉のまま
        if (node.triCount <= 4) return;

        // 最長軸で分割
        int axis = node.bounds.LongestAxis();
        float splitPos = AABB::GetAxis(node.bounds.Center(), axis);

        // 三角形を分割位置でパーティション
        uint32_t i = node.leftFirst;
        uint32_t j = i + node.triCount - 1;

        while (i <= j) {
            if (AABB::GetAxis(triangles_[i].Centroid(), axis) < splitPos) {
                ++i;
            } else {
                std::swap(triangles_[i], triangles_[j]);
                --j;
            }
        }

        // 分割が有効かチェック
        uint32_t leftCount = i - node.leftFirst;
        if (leftCount == 0 || leftCount == node.triCount) {
            return;  // 分割できない
        }

        // 子ノードを作成
        uint32_t leftChildIdx = nodesUsed_++;
        uint32_t rightChildIdx = nodesUsed_++;

        // ノード配列の拡張
        if (nodes_.size() <= rightChildIdx) {
            nodes_.resize(rightChildIdx + 1);
        }

        nodes_[leftChildIdx].leftFirst = node.leftFirst;
        nodes_[leftChildIdx].triCount = leftCount;

        nodes_[rightChildIdx].leftFirst = i;
        nodes_[rightChildIdx].triCount = node.triCount - leftCount;

        // 現在のノードを内部ノードに変換
        node.leftFirst = leftChildIdx;
        node.triCount = 0;

        // 子のAABBを更新して再帰
        UpdateNodeBounds(leftChildIdx);
        UpdateNodeBounds(rightChildIdx);
        Subdivide(leftChildIdx);
        Subdivide(rightChildIdx);
    }

    //! @brief レイ-三角形交差判定（Möller-Trumbore法）
    [[nodiscard]] static bool IntersectTriangle(
        const Vector3& origin,
        const Vector3& dir,
        const Triangle& tri,
        float tMax,
        float& outT)
    {
        constexpr float EPSILON = 1e-8f;

        Vector3 edge1 = tri.v1 - tri.v0;
        Vector3 edge2 = tri.v2 - tri.v0;

        Vector3 h = dir.Cross(edge2);
        float a = edge1.Dot(h);

        if (std::abs(a) < EPSILON) return false;

        float f = 1.0f / a;
        Vector3 s = origin - tri.v0;
        float u = f * s.Dot(h);

        if (u < 0.0f || u > 1.0f) return false;

        Vector3 q = s.Cross(edge1);
        float v = f * dir.Dot(q);

        if (v < 0.0f || u + v > 1.0f) return false;

        float t = f * edge2.Dot(q);

        if (t > EPSILON && t < tMax) {
            outT = t;
            return true;
        }
        return false;
    }

    std::vector<Triangle> triangles_;
    std::vector<BVHNode> nodes_;
    uint32_t rootIndex_ = 0;
    uint32_t nodesUsed_ = 0;
};

} // namespace Physics
