//----------------------------------------------------------------------------
//! @file   camera2d.h
//! @brief  2Dカメラコンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "component.h"
#include "engine/scene/math_types.h"

//============================================================================
//! @brief 2Dカメラコンポーネント
//!
//! 2D空間でのビュー変換を管理する。
//! 位置・回転・ズームを設定し、ビュー行列とビュープロジェクション行列を生成。
//============================================================================
class Camera2D : public Component {
public:
    Camera2D() = default;

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param viewportWidth ビューポート幅
    //! @param viewportHeight ビューポート高さ
    //------------------------------------------------------------------------
    Camera2D(float viewportWidth, float viewportHeight)
        : viewportWidth_(viewportWidth), viewportHeight_(viewportHeight) {}

    //------------------------------------------------------------------------
    // 位置
    //------------------------------------------------------------------------

    [[nodiscard]] const Vector2& GetPosition() const noexcept { return position_; }

    void SetPosition(const Vector2& position) noexcept {
        position_ = position;
        dirty_ = true;
    }

    void SetPosition(float x, float y) noexcept {
        position_.x = x;
        position_.y = y;
        dirty_ = true;
    }

    //! @brief カメラを移動
    void Translate(const Vector2& delta) noexcept {
        position_ += delta;
        dirty_ = true;
    }

    //------------------------------------------------------------------------
    // 回転
    //------------------------------------------------------------------------

    //! @brief 回転角度を取得（ラジアン）
    [[nodiscard]] float GetRotation() const noexcept { return rotation_; }

    //! @brief 回転角度を取得（度）
    [[nodiscard]] float GetRotationDegrees() const noexcept { return ToDegrees(rotation_); }

    //! @brief 回転角度を設定（ラジアン）
    void SetRotation(float radians) noexcept {
        rotation_ = radians;
        dirty_ = true;
    }

    //! @brief 回転角度を設定（度）
    void SetRotationDegrees(float degrees) noexcept {
        rotation_ = ToRadians(degrees);
        dirty_ = true;
    }

    //------------------------------------------------------------------------
    // ズーム
    //------------------------------------------------------------------------

    //! @brief ズーム倍率を取得（1.0 = 等倍）
    [[nodiscard]] float GetZoom() const noexcept { return zoom_; }

    //! @brief ズーム倍率を設定
    void SetZoom(float zoom) noexcept {
        zoom_ = (zoom > 0.001f) ? zoom : 0.001f;  // 0以下防止
        dirty_ = true;
    }

    //------------------------------------------------------------------------
    // ビューポート
    //------------------------------------------------------------------------

    [[nodiscard]] float GetViewportWidth() const noexcept { return viewportWidth_; }
    [[nodiscard]] float GetViewportHeight() const noexcept { return viewportHeight_; }

    void SetViewportSize(float width, float height) noexcept {
        viewportWidth_ = width;
        viewportHeight_ = height;
        dirty_ = true;
    }

    //------------------------------------------------------------------------
    // 行列
    //------------------------------------------------------------------------

    //! @brief ビュー行列を取得
    [[nodiscard]] const Matrix& GetViewMatrix() {
        if (dirty_) UpdateMatrices();
        return viewMatrix_;
    }

    //! @brief ビュープロジェクション行列を取得
    [[nodiscard]] const Matrix& GetViewProjectionMatrix() {
        if (dirty_) UpdateMatrices();
        return viewProjectionMatrix_;
    }

    //------------------------------------------------------------------------
    // 座標変換
    //------------------------------------------------------------------------

    //! @brief スクリーン座標をワールド座標に変換
    //! @param screenPos スクリーン座標（左上が原点）
    //! @return ワールド座標
    [[nodiscard]] Vector2 ScreenToWorld(const Vector2& screenPos) {
        if (dirty_) UpdateMatrices();

        // スクリーン座標をNDCに変換してから逆行列で変換
        Matrix invViewProj = viewProjectionMatrix_.Invert();

        // スクリーン座標を-1〜1に正規化
        float ndcX = (screenPos.x / viewportWidth_) * 2.0f - 1.0f;
        float ndcY = 1.0f - (screenPos.y / viewportHeight_) * 2.0f;

        Vector3 worldPos = Vector3::Transform(Vector3(ndcX, ndcY, 0.0f), invViewProj);
        return Vector2(worldPos.x, worldPos.y);
    }

    //! @brief ワールド座標をスクリーン座標に変換
    //! @param worldPos ワールド座標
    //! @return スクリーン座標（左上が原点）
    [[nodiscard]] Vector2 WorldToScreen(const Vector2& worldPos) {
        if (dirty_) UpdateMatrices();

        Vector3 ndcPos = Vector3::Transform(Vector3(worldPos.x, worldPos.y, 0.0f), viewProjectionMatrix_);

        float screenX = (ndcPos.x + 1.0f) * 0.5f * viewportWidth_;
        float screenY = (1.0f - ndcPos.y) * 0.5f * viewportHeight_;
        return Vector2(screenX, screenY);
    }

    //! @brief カメラが映す領域の境界を取得
    //! @param outMin 左上のワールド座標
    //! @param outMax 右下のワールド座標
    void GetWorldBounds(Vector2& outMin, Vector2& outMax) {
        outMin = ScreenToWorld(Vector2::Zero);
        outMax = ScreenToWorld(Vector2(viewportWidth_, viewportHeight_));
    }

    //------------------------------------------------------------------------
    // ユーティリティ
    //------------------------------------------------------------------------

    //! @brief 指定位置を画面中央に映すようにカメラを移動
    void LookAt(const Vector2& target) {
        SetPosition(target);
    }

    //! @brief カメラを対象に追従（スムーズ）
    //! @param target 追従対象の位置
    //! @param smoothing スムージング係数（0-1、1で即座に追従）
    void Follow(const Vector2& target, float smoothing = 0.1f) {
        Vector2 diff = target - position_;
        Translate(diff * Clamp(smoothing, 0.0f, 1.0f));
    }

private:
    void UpdateMatrices() {
        // ビュー行列: カメラの逆変換
        // カメラ中心を画面中央に配置
        float halfWidth = viewportWidth_ * 0.5f;
        float halfHeight = viewportHeight_ * 0.5f;

        Matrix translation = Matrix::CreateTranslation(-position_.x, -position_.y, 0.0f);
        Matrix rotation = Matrix::CreateRotationZ(-rotation_);
        Matrix scale = Matrix::CreateScale(zoom_, zoom_, 1.0f);
        Matrix centerOffset = Matrix::CreateTranslation(halfWidth, halfHeight, 0.0f);

        viewMatrix_ = translation * rotation * scale * centerOffset;

        // プロジェクション行列: 2D正射影（左上原点）
        Matrix projection = Matrix::CreateOrthographicOffCenter(
            0.0f, viewportWidth_,
            viewportHeight_, 0.0f,
            0.0f, 1.0f
        );

        viewProjectionMatrix_ = viewMatrix_ * projection;
        viewProjectionMatrix_ = viewProjectionMatrix_.Transpose();  // シェーダー用に転置

        dirty_ = false;
    }

    Vector2 position_ = Vector2::Zero;
    float rotation_ = 0.0f;
    float zoom_ = 1.0f;

    float viewportWidth_ = 1280.0f;
    float viewportHeight_ = 720.0f;

    Matrix viewMatrix_ = Matrix::Identity;
    Matrix viewProjectionMatrix_ = Matrix::Identity;
    bool dirty_ = true;
};
