//----------------------------------------------------------------------------
//! @file   camera2d_data.h
//! @brief  ECS Camera2DData - 2Dカメラデータ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief 2Dカメラデータ（POD構造体）
//!
//! 2D描画用のビュープロジェクション行列を管理する。
//! 正射影投影を使用。
//!
//! @note メモリレイアウト最適化:
//!       - alignas(16)でSIMD命令に最適化
//!       - Matrixは64バイト、16バイト境界に配置
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) Camera2DData : public IComponentData {
    //------------------------------------------------------------------------
    // キャッシュ（頻繁にアクセス、16バイト境界）
    //------------------------------------------------------------------------
    Matrix viewProjectionMatrix = Matrix::Identity;  //!< ビュープロジェクション行列 (64 bytes)

    //------------------------------------------------------------------------
    // 位置・回転
    //------------------------------------------------------------------------
    Vector2 position = Vector2::Zero;               //!< 位置 (8 bytes)
    float rotation = 0.0f;                          //!< 回転（ラジアン）(4 bytes)
    float zoom = 1.0f;                              //!< ズーム倍率 (4 bytes)

    //------------------------------------------------------------------------
    // ビューポート
    //------------------------------------------------------------------------
    float viewportWidth = 1280.0f;                  //!< ビューポート幅
    float viewportHeight = 720.0f;                  //!< ビューポート高さ

    //------------------------------------------------------------------------
    // フラグ
    //------------------------------------------------------------------------
    bool dirty = true;                              //!< ダーティフラグ
    bool _pad0[3] = {false, false, false};          //!< パディング

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Camera2DData() = default;

    Camera2DData(float width, float height)
        : viewportWidth(width), viewportHeight(height) {}

    Camera2DData(float width, float height, float z)
        : viewportWidth(width), viewportHeight(height), zoom(z) {}

    //------------------------------------------------------------------------
    // 位置設定
    //------------------------------------------------------------------------

    //! @brief 位置を設定
    void SetPosition(float x, float y) noexcept {
        position.x = x;
        position.y = y;
        dirty = true;
    }

    //! @brief 位置を設定
    void SetPosition(const Vector2& pos) noexcept {
        position = pos;
        dirty = true;
    }

    //! @brief 移動
    void Translate(const Vector2& delta) noexcept {
        position += delta;
        dirty = true;
    }

    //! @brief 移動
    void Translate(float dx, float dy) noexcept {
        position.x += dx;
        position.y += dy;
        dirty = true;
    }

    //------------------------------------------------------------------------
    // 回転設定
    //------------------------------------------------------------------------

    //! @brief 回転を設定（ラジアン）
    void SetRotation(float radians) noexcept {
        rotation = radians;
        dirty = true;
    }

    //! @brief 回転を設定（度）
    void SetRotationDegrees(float degrees) noexcept {
        rotation = degrees * (3.14159265358979323846f / 180.0f);
        dirty = true;
    }

    //! @brief 回転を取得（度）
    [[nodiscard]] float GetRotationDegrees() const noexcept {
        return rotation * (180.0f / 3.14159265358979323846f);
    }

    //------------------------------------------------------------------------
    // ズーム設定
    //------------------------------------------------------------------------

    //! @brief ズームを設定
    void SetZoom(float z) noexcept {
        zoom = (z > 0.001f) ? z : 0.001f;  // 最小値制限
        dirty = true;
    }

    //------------------------------------------------------------------------
    // ビューポート設定
    //------------------------------------------------------------------------

    //! @brief ビューポートサイズを設定
    void SetViewportSize(float width, float height) noexcept {
        viewportWidth = width;
        viewportHeight = height;
        dirty = true;
    }

    //------------------------------------------------------------------------
    // 行列計算
    //------------------------------------------------------------------------

    //! @brief 行列を更新（dirtyの場合）
    void UpdateMatrix() noexcept {
        if (!dirty) return;

        float hw = viewportWidth * 0.5f;
        float hh = viewportHeight * 0.5f;

        // 正射影行列（左手座標系）
        Matrix projection = LH::CreateOrthographicOffCenter(
            -hw, hw, -hh, hh, 0.0f, 1.0f);

        // ビュー行列：スケール → 回転 → 移動
        Matrix view = Matrix::CreateScale(zoom, zoom, 1.0f)
                    * Matrix::CreateRotationZ(-rotation)
                    * Matrix::CreateTranslation(-position.x, -position.y, 0.0f);

        viewProjectionMatrix = view * projection;
        dirty = false;
    }

    //! @brief ビュープロジェクション行列を取得
    [[nodiscard]] Matrix GetViewProjectionMatrix() const noexcept {
        if (dirty) {
            const_cast<Camera2DData*>(this)->UpdateMatrix();
        }
        return viewProjectionMatrix;
    }

    //------------------------------------------------------------------------
    // 座標変換
    //------------------------------------------------------------------------

    //! @brief スクリーン座標をワールド座標に変換
    [[nodiscard]] Vector2 ScreenToWorld(const Vector2& screenPos) const noexcept {
        // スクリーン座標を-1〜1に正規化
        float nx = (screenPos.x / viewportWidth) * 2.0f - 1.0f;
        float ny = 1.0f - (screenPos.y / viewportHeight) * 2.0f;

        // 逆変換
        float hw = viewportWidth * 0.5f;
        float hh = viewportHeight * 0.5f;
        float invZoom = 1.0f / zoom;

        float cosR = std::cos(rotation);
        float sinR = std::sin(rotation);

        float wx = nx * hw * invZoom;
        float wy = ny * hh * invZoom;

        // 回転の逆変換
        float rx = wx * cosR - wy * sinR;
        float ry = wx * sinR + wy * cosR;

        return Vector2(rx + position.x, ry + position.y);
    }

    //! @brief ワールド座標をスクリーン座標に変換
    [[nodiscard]] Vector2 WorldToScreen(const Vector2& worldPos) const noexcept {
        float dx = worldPos.x - position.x;
        float dy = worldPos.y - position.y;

        float cosR = std::cos(rotation);
        float sinR = std::sin(rotation);

        float rx = dx * cosR + dy * sinR;
        float ry = -dx * sinR + dy * cosR;

        float hw = viewportWidth * 0.5f;
        float hh = viewportHeight * 0.5f;

        float nx = (rx * zoom) / hw;
        float ny = (ry * zoom) / hh;

        return Vector2(
            (nx + 1.0f) * 0.5f * viewportWidth,
            (1.0f - ny) * 0.5f * viewportHeight
        );
    }

    //------------------------------------------------------------------------
    // ユーティリティ
    //------------------------------------------------------------------------

    //! @brief 指定位置を画面中央に映すようにカメラを移動
    void LookAt(const Vector2& target) noexcept {
        position = target;
        dirty = true;
    }

    //! @brief カメラを対象に追従（スムーズ）
    void Follow(const Vector2& target, float smoothing = 0.1f) noexcept {
        position = Vector2::Lerp(position, target, smoothing);
        dirty = true;
    }

    //! @brief カメラが映す領域の境界を取得
    void GetWorldBounds(Vector2& outMin, Vector2& outMax) const noexcept {
        float hw = (viewportWidth * 0.5f) / zoom;
        float hh = (viewportHeight * 0.5f) / zoom;

        outMin = Vector2(position.x - hw, position.y - hh);
        outMax = Vector2(position.x + hw, position.y + hh);
    }
};
#pragma warning(pop)

ECS_COMPONENT(Camera2DData);

} // namespace ECS
