//----------------------------------------------------------------------------
//! @file   raycast.h
//! @brief  レイキャスト関連の構造体とユーティリティ
//----------------------------------------------------------------------------
#pragma once

#include "engine/math/math_types.h"
#include <cmath>
#include <limits>

namespace Physics {

//============================================================================
//! @brief レイキャストヒット情報
//============================================================================
struct RaycastHit {
    Vector3 point;          //!< ヒット座標
    Vector3 normal;         //!< ヒット面の法線
    float distance = 0.0f;  //!< レイ原点からの距離
    bool hit = false;       //!< ヒットしたか

    //! @brief ヒット情報をリセット
    void Reset() noexcept {
        point = Vector3::Zero;
        normal = Vector3::Up;
        distance = (std::numeric_limits<float>::max)();
        hit = false;
    }
};

//============================================================================
//! @brief レイ（光線）
//============================================================================
struct Ray {
    Vector3 origin;         //!< 原点
    Vector3 direction;      //!< 方向（正規化）

    Ray() = default;
    Ray(const Vector3& o, const Vector3& d)
        : origin(o), direction(d) {
        direction.Normalize();
    }

    //! @brief レイ上の点を取得
    [[nodiscard]] Vector3 GetPoint(float t) const noexcept {
        return origin + direction * t;
    }
};

//============================================================================
//! @brief レイ-三角形交差判定（Möller-Trumbore法）
//!
//! @param ray        レイ
//! @param v0, v1, v2 三角形の頂点（反時計回り）
//! @param maxDistance 最大距離
//! @param outT       [out] 交差点までの距離
//! @param outU, outV [out] 重心座標（オプション）
//! @return 交差したらtrue
//============================================================================
inline bool RayTriangleIntersect(
    const Ray& ray,
    const Vector3& v0, const Vector3& v1, const Vector3& v2,
    float maxDistance,
    float& outT,
    float* outU = nullptr,
    float* outV = nullptr)
{
    constexpr float EPSILON = 1e-8f;

    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;

    Vector3 h = ray.direction.Cross(edge2);
    float a = edge1.Dot(h);

    // レイが三角形と平行
    if (std::abs(a) < EPSILON) {
        return false;
    }

    float f = 1.0f / a;
    Vector3 s = ray.origin - v0;
    float u = f * s.Dot(h);

    // 交点が三角形の外
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    Vector3 q = s.Cross(edge1);
    float v = f * ray.direction.Dot(q);

    // 交点が三角形の外
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    float t = f * edge2.Dot(q);

    // 交点がレイの後ろまたは最大距離を超える
    if (t < EPSILON || t > maxDistance) {
        return false;
    }

    outT = t;
    if (outU) *outU = u;
    if (outV) *outV = v;
    return true;
}

//============================================================================
//! @brief 三角形の法線を計算
//============================================================================
inline Vector3 CalculateTriangleNormal(
    const Vector3& v0, const Vector3& v1, const Vector3& v2)
{
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 normal = edge1.Cross(edge2);
    normal.Normalize();
    return normal;
}

//============================================================================
//! @brief AABB-レイ交差判定
//============================================================================
inline bool RayAABBIntersect(
    const Ray& ray,
    const Vector3& aabbMin,
    const Vector3& aabbMax,
    float maxDistance)
{
    float tmin = 0.0f;
    float tmax = maxDistance;

    for (int i = 0; i < 3; ++i) {
        float origin = (&ray.origin.x)[i];
        float dir = (&ray.direction.x)[i];
        float bmin = (&aabbMin.x)[i];
        float bmax = (&aabbMax.x)[i];

        if (std::abs(dir) < 1e-8f) {
            // レイがこの軸に平行
            if (origin < bmin || origin > bmax) {
                return false;
            }
        } else {
            float t1 = (bmin - origin) / dir;
            float t2 = (bmax - origin) / dir;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) {
                return false;
            }
        }
    }
    return true;
}

} // namespace Physics
