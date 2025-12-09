//----------------------------------------------------------------------------
//! @file   sprite_renderer.h
//! @brief  スプライトレンダラーコンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "component.h"
#include "engine/color.h"
#include "engine/scene/math_types.h"

// 前方宣言
class Texture;

//============================================================================
//! @brief スプライトの矩形領域
//============================================================================
struct SpriteRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    SpriteRect() = default;
    SpriteRect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}
};

//============================================================================
//! @brief スプライトレンダラーコンポーネント
//!
//! テクスチャを2Dスプライトとして描画するためのコンポーネント。
//! Transform2Dと組み合わせて使用する。
//============================================================================
class SpriteRenderer : public Component {
public:
    SpriteRenderer() = default;

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param texture テクスチャ
    //------------------------------------------------------------------------
    explicit SpriteRenderer(Texture* texture)
        : texture_(texture) {}

    //------------------------------------------------------------------------
    // テクスチャ
    //------------------------------------------------------------------------

    [[nodiscard]] Texture* GetTexture() const noexcept { return texture_; }
    void SetTexture(Texture* texture) noexcept { texture_ = texture; }

    //------------------------------------------------------------------------
    // ソース矩形（テクスチャ内の描画領域）
    //------------------------------------------------------------------------

    [[nodiscard]] const SpriteRect& GetSourceRect() const noexcept { return sourceRect_; }
    void SetSourceRect(const SpriteRect& rect) noexcept { sourceRect_ = rect; }
    void SetSourceRect(float x, float y, float w, float h) noexcept {
        sourceRect_ = SpriteRect(x, y, w, h);
    }

    //! @brief テクスチャ全体を使用
    void UseFullTexture() noexcept {
        sourceRect_ = SpriteRect(0.0f, 0.0f, 0.0f, 0.0f);  // 0,0,0,0で全体
    }

    //------------------------------------------------------------------------
    // カラー
    //------------------------------------------------------------------------

    [[nodiscard]] const Color& GetColor() const noexcept { return color_; }
    void SetColor(const Color& color) noexcept { color_ = color; }
    void SetColor(float r, float g, float b, float a = 1.0f) noexcept {
        color_ = Color(r, g, b, a);
    }

    //! @brief アルファ値のみ設定
    void SetAlpha(float alpha) noexcept {
        color_.w = alpha;
    }
    [[nodiscard]] float GetAlpha() const noexcept { return color_.w; }

    //------------------------------------------------------------------------
    // 描画順（レイヤー）
    //------------------------------------------------------------------------

    [[nodiscard]] int GetSortingLayer() const noexcept { return sortingLayer_; }
    void SetSortingLayer(int layer) noexcept { sortingLayer_ = layer; }

    [[nodiscard]] int GetOrderInLayer() const noexcept { return orderInLayer_; }
    void SetOrderInLayer(int order) noexcept { orderInLayer_ = order; }

    //------------------------------------------------------------------------
    // 反転
    //------------------------------------------------------------------------

    [[nodiscard]] bool IsFlipX() const noexcept { return flipX_; }
    void SetFlipX(bool flip) noexcept { flipX_ = flip; }

    [[nodiscard]] bool IsFlipY() const noexcept { return flipY_; }
    void SetFlipY(bool flip) noexcept { flipY_ = flip; }

    //------------------------------------------------------------------------
    // サイズ
    //------------------------------------------------------------------------

    //! @brief カスタムサイズを取得（0,0の場合はテクスチャサイズを使用）
    [[nodiscard]] const Vector2& GetSize() const noexcept { return size_; }

    //! @brief カスタムサイズを設定
    void SetSize(const Vector2& size) noexcept { size_ = size; }
    void SetSize(float width, float height) noexcept {
        size_.x = width;
        size_.y = height;
    }

    //! @brief テクスチャサイズを使用（デフォルト）
    void UseTextureSize() noexcept {
        size_ = Vector2::Zero;
    }

private:
    Texture* texture_ = nullptr;
    SpriteRect sourceRect_;          //!< テクスチャ内の描画領域（0,0,0,0で全体）
    Color color_ = Colors::White;    //!< 乗算カラー
    Vector2 size_ = Vector2::Zero;   //!< カスタムサイズ（0,0でテクスチャサイズ）

    int sortingLayer_ = 0;           //!< 描画レイヤー（大きいほど手前）
    int orderInLayer_ = 0;           //!< レイヤー内の描画順

    bool flipX_ = false;             //!< X軸反転
    bool flipY_ = false;             //!< Y軸反転
};
