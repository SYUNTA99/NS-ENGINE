#pragma once
#include "engine/math/math_types.h"
#include "engine/math/color.h"
#include <functional>


class UIButton
{
public:
  
    //! @brief コンストラクタ
    //! @param pos ボタンの中心位置（スクリーン座標）
    //! @param size ボタンのサイズ（幅, 高さ）
    UIButton(const Vector2& pos, const Vector2& size);

   
    //更新
    void Update();  // マウス入力をチェックしてクリック判定
    void Render();  // ボタンを描画

   //クリック時の処理
    void SetOnClick(std::function<void()> callback) { onClick_ = callback; }

    //色設定
    void SetNormalColor(const Color& c) { normalColor_ = c; }  // 通常時
    void SetHoverColor(const Color& c) { hoverColor_ = c; }    // マウス乗せ時
    void SetPressColor(const Color& c) { pressColor_ = c; }    // クリック中

private:
  
    // マウスがボタンの範囲内か判定
    bool IsMouseOver() const;

    // 位置とサイズ
    Vector2 position_;   // ボタン中心のスクリーン座標
    Vector2 size_;       // ボタンの幅と高さ

    // 各状態の色
    Color normalColor_ = Color(0.3f, 0.3f, 0.3f, 1.0f);  // 通常: 暗いグレー
    Color hoverColor_ = Color(0.5f, 0.5f, 0.5f, 1.0f);  // ホバー: 明るいグレー
    Color pressColor_ = Color(0.2f, 0.2f, 0.2f, 1.0f);  // 押下: より暗いグレー

    // クリック時に呼ばれる関数（コールバック）
    std::function<void()> onClick_;
};