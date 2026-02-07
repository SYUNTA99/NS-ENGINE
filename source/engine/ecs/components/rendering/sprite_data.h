//----------------------------------------------------------------------------
//! @file   sprite_data.h
//! @brief  ECS SpriteData - スプライトデータ構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/texture/texture_handle.h"
#include "engine/math/color.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief スプライトデータ（POD構造体）
//!
//! 2Dスプライト描画に必要なデータを保持する。
//! SpriteRenderSystemによってSpriteBatchに送られる。
//!
//! @note メモリレイアウト最適化:
//!       - alignas(16)でSIMD命令に最適化
//!       - 16バイト境界でColorを配置
//!       - boolフラグをパッキングして無駄を削減
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) SpriteData : public IComponentData {
    //------------------------------------------------------------------------
    // 16バイト境界メンバー
    //------------------------------------------------------------------------
    Color color = Colors::White;                   //!< 乗算カラー (16 bytes)

    //------------------------------------------------------------------------
    // テクスチャとサイズ
    //------------------------------------------------------------------------
    TextureHandle texture;                         //!< テクスチャハンドル (4 bytes)
    int sortingLayer = 0;                          //!< 描画レイヤー（大きいほど手前）(4 bytes)
    int orderInLayer = 0;                          //!< レイヤー内の描画順 (4 bytes)
    uint32_t _pad0 = 0;                            //!< パディング (4 bytes) → 合計16bytes

    //------------------------------------------------------------------------
    // Vector2メンバー（8バイト）
    //------------------------------------------------------------------------
    Vector2 size = Vector2::Zero;                  //!< サイズ（0,0でテクスチャサイズ）(8 bytes)
    Vector2 pivot = Vector2::Zero;                 //!< 原点（0,0で左上）(8 bytes)
    Vector2 uvOffset = Vector2::Zero;              //!< UV開始位置 (8 bytes)
    Vector2 uvSize = Vector2::One;                 //!< UVサイズ（1,1で全体）(8 bytes)

    //------------------------------------------------------------------------
    // boolフラグ（パッキング）
    //------------------------------------------------------------------------
    bool flipX = false;                            //!< X軸反転
    bool flipY = false;                            //!< Y軸反転
    bool visible = true;                           //!< 表示フラグ
    bool _pad1 = false;                            //!< パディング (合計4bytes)

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    SpriteData() = default;

    explicit SpriteData(TextureHandle tex)
        : texture(tex) {}

    SpriteData(TextureHandle tex, const Vector2& sz)
        : texture(tex), size(sz) {}

    SpriteData(TextureHandle tex, const Vector2& sz, const Vector2& piv)
        : texture(tex), size(sz), pivot(piv) {}

    //------------------------------------------------------------------------
    // ヘルパー
    //------------------------------------------------------------------------

    //! @brief アルファ値を設定
    void SetAlpha(float alpha) noexcept {
        color.w = alpha;
    }

    //! @brief アルファ値を取得
    [[nodiscard]] float GetAlpha() const noexcept {
        return color.w;
    }

    //! @brief ピボットを中央に設定
    void SetPivotCenter() noexcept {
        pivot = size * 0.5f;
    }

    //! @brief UVをフレーム位置に設定（スプライトシート用）
    //! @param frameX フレームのX位置（0始まり）
    //! @param frameY フレームのY位置（0始まり）
    //! @param frameWidth 1フレームの幅（UV座標）
    //! @param frameHeight 1フレームの高さ（UV座標）
    void SetUVFrame(int frameX, int frameY, float frameWidth, float frameHeight) noexcept {
        uvOffset.x = static_cast<float>(frameX) * frameWidth;
        uvOffset.y = static_cast<float>(frameY) * frameHeight;
        uvSize.x = frameWidth;
        uvSize.y = frameHeight;
    }

    //! @brief UVをリセット（全体表示）
    void ResetUV() noexcept {
        uvOffset = Vector2::Zero;
        uvSize = Vector2::One;
    }
};
#pragma warning(pop)

ECS_COMPONENT(SpriteData);

} // namespace ECS
