//----------------------------------------------------------------------------
//! @file   math_types.h
//! @brief  数学型定義（DirectXTK SimpleMath使用）
//----------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>

//===========================================================================
//! DirectXTK SimpleMath型のエイリアス
//===========================================================================
using Vector2    = DirectX::SimpleMath::Vector2;
using Vector3    = DirectX::SimpleMath::Vector3;
using Vector4    = DirectX::SimpleMath::Vector4;
using Matrix     = DirectX::SimpleMath::Matrix;
using Matrix4x4  = DirectX::SimpleMath::Matrix;  // エイリアス
using Quaternion = DirectX::SimpleMath::Quaternion;
using Plane      = DirectX::SimpleMath::Plane;
using Ray        = DirectX::SimpleMath::Ray;
// Rectangle/Rect は Windows API と衝突するため定義しない
// 必要な場合は DirectX::SimpleMath::Rectangle を直接使用
using Viewport   = DirectX::SimpleMath::Viewport;

//===========================================================================
//! 左手座標系関数（DirectX標準）
//===========================================================================
namespace LH {

//! 左手系のビュー行列を作成
[[nodiscard]] inline Matrix CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up) noexcept {
    return DirectX::XMMatrixLookAtLH(position, target, up);
}

//! 左手系の透視投影行列を作成
[[nodiscard]] inline Matrix CreatePerspectiveFov(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept {
    return DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
}

//! 左手系の正射影行列を作成
[[nodiscard]] inline Matrix CreateOrthographic(float width, float height, float nearPlane, float farPlane) noexcept {
    return DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
}

//! 左手系の正射影行列を作成（オフセンター）
[[nodiscard]] inline Matrix CreateOrthographicOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept {
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearPlane, farPlane);
}

//! 左手系の前方ベクトル（+Z方向）
[[nodiscard]] constexpr Vector3 Forward() noexcept {
    return Vector3(0.0f, 0.0f, 1.0f);
}

//! 左手系の後方ベクトル（-Z方向）
[[nodiscard]] constexpr Vector3 Backward() noexcept {
    return Vector3(0.0f, 0.0f, -1.0f);
}

} // namespace LH

//===========================================================================
//! 右手座標系関数（非推奨 - 左手系を使用してください）
//===========================================================================
namespace RH {

//! @deprecated LH::CreateLookAt を使用してください
[[deprecated("LH::CreateLookAt を使用してください")]]
[[nodiscard]] inline Matrix CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up) noexcept {
    return DirectX::XMMatrixLookAtRH(position, target, up);
}

//! @deprecated LH::CreatePerspectiveFov を使用してください
[[deprecated("LH::CreatePerspectiveFov を使用してください")]]
[[nodiscard]] inline Matrix CreatePerspectiveFov(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept {
    return DirectX::XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
}

//! @deprecated LH::Forward を使用してください
[[deprecated("LH::Forward を使用してください")]]
[[nodiscard]] constexpr Vector3 Forward() noexcept {
    return Vector3(0.0f, 0.0f, -1.0f);
}

} // namespace RH

//===========================================================================
//! 追加の便利関数
//===========================================================================

//! 度をラジアンに変換
[[nodiscard]] constexpr float ToRadians(float degrees) noexcept {
    return degrees * (DirectX::XM_PI / 180.0f);
}

//! ラジアンを度に変換
[[nodiscard]] constexpr float ToDegrees(float radians) noexcept {
    return radians * (180.0f / DirectX::XM_PI);
}

//! 値をクランプ
template<typename T>
[[nodiscard]] constexpr T Clamp(T value, T min, T max) noexcept {
    return (value < min) ? min : (value > max) ? max : value;
}

//! 線形補間
template<typename T>
[[nodiscard]] constexpr T Lerp(const T& a, const T& b, float t) noexcept {
    return a + (b - a) * t;
}

//! 0〜1にクランプした線形補間
template<typename T>
[[nodiscard]] constexpr T LerpClamped(const T& a, const T& b, float t) noexcept {
    return Lerp(a, b, Clamp(t, 0.0f, 1.0f));
}

//===========================================================================
//! @brief 線分（2D）
//!
//! 始点から終点への線分を表す。
//! 主に「切」の縁切断判定で使用。
//===========================================================================
struct LineSegment {
    Vector2 start;  //!< 始点
    Vector2 end;    //!< 終点

    LineSegment() = default;
    LineSegment(const Vector2& s, const Vector2& e) : start(s), end(e) {}
    LineSegment(float x1, float y1, float x2, float y2)
        : start(x1, y1), end(x2, y2) {}

    //! @brief 線分の方向ベクトルを取得
    [[nodiscard]] Vector2 Direction() const noexcept {
        return end - start;
    }

    //! @brief 線分の長さを取得
    [[nodiscard]] float Length() const noexcept {
        return Direction().Length();
    }

    //! @brief 線分の長さの2乗を取得
    [[nodiscard]] float LengthSquared() const noexcept {
        return Direction().LengthSquared();
    }

    //! @brief 他の線分と交差するか判定
    //! @param other 判定対象の線分
    //! @return 交差する場合true
    [[nodiscard]] bool Intersects(const LineSegment& other) const noexcept {
        Vector2 intersection;
        return Intersects(other, intersection);
    }

    //! @brief 他の線分と交差するか判定（交点取得版）
    //! @param other 判定対象の線分
    //! @param[out] intersection 交点（交差する場合のみ有効）
    //! @return 交差する場合true
    [[nodiscard]] bool Intersects(const LineSegment& other, Vector2& intersection) const noexcept {
        const Vector2 ab = end - start;
        const Vector2 cd = other.end - other.start;
        const Vector2 ac = other.start - start;

        const float cross_ab_cd = ab.x * cd.y - ab.y * cd.x;

        constexpr float kEpsilon = 1e-6f;
        if (std::abs(cross_ab_cd) < kEpsilon) {
            return false;
        }

        const float t = (ac.x * cd.y - ac.y * cd.x) / cross_ab_cd;
        const float u = (ac.x * ab.y - ac.y * ab.x) / cross_ab_cd;

        if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
            intersection = start + ab * t;
            return true;
        }

        return false;
    }

    //! @brief 点までの最短距離を計算
    //! @param point 点
    //! @return 線分上の最近点までの距離
    [[nodiscard]] float DistanceToPoint(const Vector2& point) const noexcept {
        const Vector2 ab = end - start;
        const Vector2 ap = point - start;

        const float lengthSq = ab.LengthSquared();
        if (lengthSq < 1e-8f) {
            return ap.Length();
        }

        float t = Clamp(ap.Dot(ab) / lengthSq, 0.0f, 1.0f);

        const Vector2 closest = start + ab * t;
        return (point - closest).Length();
    }
};
