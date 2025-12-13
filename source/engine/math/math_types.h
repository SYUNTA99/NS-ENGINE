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
