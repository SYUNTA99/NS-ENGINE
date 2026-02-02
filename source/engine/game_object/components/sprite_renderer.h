//----------------------------------------------------------------------------
//! @file   sprite_renderer.h
//! @brief  SpriteRenderer - 純粋OOP 2D描画コンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"

//============================================================================
//! @brief スプライトの原点位置
//============================================================================
enum class SpriteOrigin {
    Center,         //!< 中心
    TopLeft,        //!< 左上
    TopCenter,      //!< 上中央
    TopRight,       //!< 右上
    MiddleLeft,     //!< 左中央
    MiddleRight,    //!< 右中央
    BottomLeft,     //!< 左下
    BottomCenter,   //!< 下中央
    BottomRight     //!< 右下
};

//============================================================================
//! @brief スプライトの反転
//============================================================================
enum class SpriteFlip {
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Both = 3
};

//============================================================================
//! @brief SpriteRenderer - 2D描画コンポーネント
//!
//! Unity MonoBehaviour風の設計。スプライト描画の設定を管理。
//!
//! @code
//! auto* go = world.CreateGameObject("Player");
//! go->AddComponent<Transform>();
//! auto* sr = go->AddComponent<SpriteRenderer>();
//!
//! sr->SetTexture(playerTexture);
//! sr->SetColor(Color::White);
//! sr->SetSize(64, 64);
//! sr->SetFlip(SpriteFlip::Horizontal);
//! @endcode
//============================================================================
class SpriteRenderer final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    SpriteRenderer() = default;

    explicit SpriteRenderer(uint32_t textureHandle)
        : textureHandle_(textureHandle) {}

    SpriteRenderer(uint32_t textureHandle, float width, float height)
        : textureHandle_(textureHandle)
        , width_(width)
        , height_(height) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
    }

    //========================================================================
    // テクスチャ
    //========================================================================

    [[nodiscard]] uint32_t GetTexture() const noexcept { return textureHandle_; }
    void SetTexture(uint32_t handle) noexcept { textureHandle_ = handle; }

    //========================================================================
    // サイズ
    //========================================================================

    [[nodiscard]] float GetWidth() const noexcept { return width_; }
    [[nodiscard]] float GetHeight() const noexcept { return height_; }
    [[nodiscard]] Vector2 GetSize() const noexcept { return Vector2(width_, height_); }

    void SetWidth(float width) noexcept { width_ = width; }
    void SetHeight(float height) noexcept { height_ = height; }
    void SetSize(float width, float height) noexcept { width_ = width; height_ = height; }
    void SetSize(const Vector2& size) noexcept { width_ = size.x; height_ = size.y; }

    //========================================================================
    // カラー・透明度
    //========================================================================

    [[nodiscard]] const Color& GetColor() const noexcept { return color_; }
    [[nodiscard]] float GetAlpha() const noexcept { return color_.w; }

    void SetColor(const Color& color) noexcept { color_ = color; }
    void SetColor(float r, float g, float b, float a = 1.0f) noexcept {
        color_ = Color(r, g, b, a);
    }
    void SetAlpha(float alpha) noexcept { color_.w = alpha; }

    //========================================================================
    // UV座標（アニメーション用）
    //========================================================================

    [[nodiscard]] const Vector4& GetUVRect() const noexcept { return uvRect_; }

    //! @brief UV矩形を設定 (x=left, y=top, z=right, w=bottom)
    void SetUVRect(const Vector4& rect) noexcept { uvRect_ = rect; }
    void SetUVRect(float left, float top, float right, float bottom) noexcept {
        uvRect_ = Vector4(left, top, right, bottom);
    }

    //! @brief スプライトシートからUVを計算
    //! @param frameX X方向のフレーム番号
    //! @param frameY Y方向のフレーム番号
    //! @param framesX 横のフレーム数
    //! @param framesY 縦のフレーム数
    void SetSpriteSheetFrame(int frameX, int frameY, int framesX, int framesY) {
        float fw = 1.0f / framesX;
        float fh = 1.0f / framesY;
        uvRect_ = Vector4(
            frameX * fw,
            frameY * fh,
            (frameX + 1) * fw,
            (frameY + 1) * fh
        );
    }

    //========================================================================
    // 反転・回転
    //========================================================================

    [[nodiscard]] SpriteFlip GetFlip() const noexcept { return flip_; }
    void SetFlip(SpriteFlip flip) noexcept { flip_ = flip; }

    void SetFlipHorizontal(bool flip) noexcept {
        if (flip) {
            flip_ = static_cast<SpriteFlip>(static_cast<int>(flip_) | 1);
        } else {
            flip_ = static_cast<SpriteFlip>(static_cast<int>(flip_) & ~1);
        }
    }

    void SetFlipVertical(bool flip) noexcept {
        if (flip) {
            flip_ = static_cast<SpriteFlip>(static_cast<int>(flip_) | 2);
        } else {
            flip_ = static_cast<SpriteFlip>(static_cast<int>(flip_) & ~2);
        }
    }

    [[nodiscard]] bool IsFlippedHorizontal() const noexcept {
        return (static_cast<int>(flip_) & 1) != 0;
    }

    [[nodiscard]] bool IsFlippedVertical() const noexcept {
        return (static_cast<int>(flip_) & 2) != 0;
    }

    //========================================================================
    // 原点
    //========================================================================

    [[nodiscard]] SpriteOrigin GetOrigin() const noexcept { return origin_; }
    [[nodiscard]] const Vector2& GetCustomOrigin() const noexcept { return customOrigin_; }

    void SetOrigin(SpriteOrigin origin) noexcept { origin_ = origin; }
    void SetCustomOrigin(const Vector2& origin) noexcept { customOrigin_ = origin; }
    void SetCustomOrigin(float x, float y) noexcept { customOrigin_ = Vector2(x, y); }

    //! @brief 原点のオフセットを計算
    [[nodiscard]] Vector2 GetOriginOffset() const {
        switch (origin_) {
            case SpriteOrigin::Center:       return Vector2(0, 0);
            case SpriteOrigin::TopLeft:      return Vector2(-width_ * 0.5f, height_ * 0.5f);
            case SpriteOrigin::TopCenter:    return Vector2(0, height_ * 0.5f);
            case SpriteOrigin::TopRight:     return Vector2(width_ * 0.5f, height_ * 0.5f);
            case SpriteOrigin::MiddleLeft:   return Vector2(-width_ * 0.5f, 0);
            case SpriteOrigin::MiddleRight:  return Vector2(width_ * 0.5f, 0);
            case SpriteOrigin::BottomLeft:   return Vector2(-width_ * 0.5f, -height_ * 0.5f);
            case SpriteOrigin::BottomCenter: return Vector2(0, -height_ * 0.5f);
            case SpriteOrigin::BottomRight:  return Vector2(width_ * 0.5f, -height_ * 0.5f);
            default:                         return customOrigin_;
        }
    }

    //========================================================================
    // レイヤー・ソート
    //========================================================================

    [[nodiscard]] int GetSortingLayer() const noexcept { return sortingLayer_; }
    [[nodiscard]] int GetOrderInLayer() const noexcept { return orderInLayer_; }

    void SetSortingLayer(int layer) noexcept { sortingLayer_ = layer; }
    void SetOrderInLayer(int order) noexcept { orderInLayer_ = order; }

    //========================================================================
    // 表示
    //========================================================================

    [[nodiscard]] bool IsVisible() const noexcept { return isVisible_; }
    void SetVisible(bool visible) noexcept { isVisible_ = visible; }

    //========================================================================
    // 描画情報取得（レンダラー用）
    //========================================================================

    //! @brief 描画用のワールド行列を取得
    [[nodiscard]] Matrix GetRenderMatrix() const {
        if (!transform_) return Matrix::Identity;

        Vector2 offset = GetOriginOffset();
        Matrix offsetMatrix = Matrix::CreateTranslation(offset.x, offset.y, 0);

        Matrix flipMatrix = Matrix::Identity;
        if (IsFlippedHorizontal()) flipMatrix._11 = -1.0f;
        if (IsFlippedVertical()) flipMatrix._22 = -1.0f;

        return offsetMatrix * flipMatrix * transform_->GetLocalMatrix();
    }

private:
    Transform* transform_ = nullptr;

    // テクスチャ
    uint32_t textureHandle_ = 0;

    // サイズ
    float width_ = 32.0f;
    float height_ = 32.0f;

    // カラー
    Color color_ = Color(1, 1, 1, 1);

    // UV
    Vector4 uvRect_ = Vector4(0, 0, 1, 1);  // left, top, right, bottom

    // 反転・原点
    SpriteFlip flip_ = SpriteFlip::None;
    SpriteOrigin origin_ = SpriteOrigin::Center;
    Vector2 customOrigin_ = Vector2::Zero;

    // ソート
    int sortingLayer_ = 0;
    int orderInLayer_ = 0;

    // 表示
    bool isVisible_ = true;
};

OOP_COMPONENT(SpriteRenderer);
