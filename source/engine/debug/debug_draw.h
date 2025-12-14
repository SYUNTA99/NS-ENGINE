//----------------------------------------------------------------------------
//! @file   debug_draw.h
//! @brief  デバッグ描画ユーティリティ（Debugビルドのみ有効）
//----------------------------------------------------------------------------
#pragma once

#include "engine/math/math_types.h"
#include "engine/math/color.h"

#ifdef _DEBUG

#include "dx11/gpu/texture.h"

//----------------------------------------------------------------------------
//! @brief デバッグ描画クラス（Debugビルドのみ）
//----------------------------------------------------------------------------
class DebugDraw
{
public:
    //! @brief シングルトン取得
    static DebugDraw& Get();

    //! @brief 矩形の枠線を描画（中心基準）
    void DrawRectOutline(
        const Vector2& center,
        const Vector2& size,
        const Color& color,
        float lineWidth = 2.0f
    );

    //! @brief 矩形の枠線を描画（左上基準）
    void DrawRectOutlineTopLeft(
        const Vector2& topLeft,
        const Vector2& size,
        const Color& color,
        float lineWidth = 2.0f
    );

    //! @brief 塗りつぶし矩形を描画（中心基準）
    void DrawRectFilled(
        const Vector2& center,
        const Vector2& size,
        const Color& color
    );

private:
    DebugDraw() = default;
    ~DebugDraw() = default;
    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;

    void EnsureInitialized();

    TexturePtr whiteTexture_;
    bool initialized_ = false;
};

//----------------------------------------------------------------------------
// デバッグ描画マクロ（Debugビルド: 実行、Releaseビルド: 消える）
//----------------------------------------------------------------------------
#define DEBUG_DRAW_RECT_OUTLINE(center, size, color, ...) \
    DebugDraw::Get().DrawRectOutline(center, size, color, ##__VA_ARGS__)
#define DEBUG_DRAW_RECT_OUTLINE_TL(topLeft, size, color, ...) \
    DebugDraw::Get().DrawRectOutlineTopLeft(topLeft, size, color, ##__VA_ARGS__)
#define DEBUG_DRAW_RECT_FILLED(center, size, color) \
    DebugDraw::Get().DrawRectFilled(center, size, color)

#else

//----------------------------------------------------------------------------
// Releaseビルド: 全マクロが空になる
//----------------------------------------------------------------------------
#define DEBUG_DRAW_RECT_OUTLINE(...)    ((void)0)
#define DEBUG_DRAW_RECT_OUTLINE_TL(...) ((void)0)
#define DEBUG_DRAW_RECT_FILLED(...)     ((void)0)

#endif
