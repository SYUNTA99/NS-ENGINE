//----------------------------------------------------------------------------
//! @file   sprite_renderer_test.cpp
//! @brief  SpriteRendererコンポーネントのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/sprite_renderer.h"

namespace
{

//============================================================================
// SpriteRenderer デフォルト値テスト
//============================================================================
TEST(SpriteRendererTest, DefaultTextureIsNull)
{
    SpriteRenderer renderer;
    EXPECT_EQ(renderer.GetTexture(), nullptr);
}

TEST(SpriteRendererTest, DefaultColorIsWhite)
{
    SpriteRenderer renderer;
    const Color& color = renderer.GetColor();
    EXPECT_FLOAT_EQ(color.R(), 1.0f);
    EXPECT_FLOAT_EQ(color.G(), 1.0f);
    EXPECT_FLOAT_EQ(color.B(), 1.0f);
    EXPECT_FLOAT_EQ(color.A(), 1.0f);
}

TEST(SpriteRendererTest, DefaultAlphaIsOne)
{
    SpriteRenderer renderer;
    EXPECT_FLOAT_EQ(renderer.GetAlpha(), 1.0f);
}

TEST(SpriteRendererTest, DefaultSizeIsZero)
{
    SpriteRenderer renderer;
    EXPECT_FLOAT_EQ(renderer.GetSize().x, 0.0f);
    EXPECT_FLOAT_EQ(renderer.GetSize().y, 0.0f);
}

TEST(SpriteRendererTest, DefaultPivotIsZero)
{
    SpriteRenderer renderer;
    EXPECT_FLOAT_EQ(renderer.GetPivot().x, 0.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().y, 0.0f);
}

TEST(SpriteRendererTest, DefaultSortingLayerIsZero)
{
    SpriteRenderer renderer;
    EXPECT_EQ(renderer.GetSortingLayer(), 0);
}

TEST(SpriteRendererTest, DefaultOrderInLayerIsZero)
{
    SpriteRenderer renderer;
    EXPECT_EQ(renderer.GetOrderInLayer(), 0);
}

TEST(SpriteRendererTest, DefaultFlipXIsFalse)
{
    SpriteRenderer renderer;
    EXPECT_FALSE(renderer.IsFlipX());
}

TEST(SpriteRendererTest, DefaultFlipYIsFalse)
{
    SpriteRenderer renderer;
    EXPECT_FALSE(renderer.IsFlipY());
}

TEST(SpriteRendererTest, DefaultHasNoPivot)
{
    SpriteRenderer renderer;
    EXPECT_FALSE(renderer.HasPivot());
}

//============================================================================
// Color テスト
//============================================================================
TEST(SpriteRendererTest, SetColor)
{
    SpriteRenderer renderer;
    renderer.SetColor(Color(0.5f, 0.6f, 0.7f, 0.8f));
    const Color& color = renderer.GetColor();
    EXPECT_FLOAT_EQ(color.R(), 0.5f);
    EXPECT_FLOAT_EQ(color.G(), 0.6f);
    EXPECT_FLOAT_EQ(color.B(), 0.7f);
    EXPECT_FLOAT_EQ(color.A(), 0.8f);
}

TEST(SpriteRendererTest, SetColorWithComponents)
{
    SpriteRenderer renderer;
    renderer.SetColor(0.1f, 0.2f, 0.3f, 0.4f);
    const Color& color = renderer.GetColor();
    EXPECT_FLOAT_EQ(color.R(), 0.1f);
    EXPECT_FLOAT_EQ(color.G(), 0.2f);
    EXPECT_FLOAT_EQ(color.B(), 0.3f);
    EXPECT_FLOAT_EQ(color.A(), 0.4f);
}

TEST(SpriteRendererTest, SetColorDefaultAlpha)
{
    SpriteRenderer renderer;
    renderer.SetColor(0.1f, 0.2f, 0.3f);
    EXPECT_FLOAT_EQ(renderer.GetAlpha(), 1.0f);
}

TEST(SpriteRendererTest, SetAlpha)
{
    SpriteRenderer renderer;
    renderer.SetAlpha(0.5f);
    EXPECT_FLOAT_EQ(renderer.GetAlpha(), 0.5f);
}

//============================================================================
// Flip テスト
//============================================================================
TEST(SpriteRendererTest, SetFlipX)
{
    SpriteRenderer renderer;
    renderer.SetFlipX(true);
    EXPECT_TRUE(renderer.IsFlipX());
}

TEST(SpriteRendererTest, SetFlipY)
{
    SpriteRenderer renderer;
    renderer.SetFlipY(true);
    EXPECT_TRUE(renderer.IsFlipY());
}

TEST(SpriteRendererTest, ToggleFlipX)
{
    SpriteRenderer renderer;
    renderer.SetFlipX(true);
    EXPECT_TRUE(renderer.IsFlipX());
    renderer.SetFlipX(false);
    EXPECT_FALSE(renderer.IsFlipX());
}

//============================================================================
// Size テスト
//============================================================================
TEST(SpriteRendererTest, SetSizeVector)
{
    SpriteRenderer renderer;
    renderer.SetSize(Vector2(100.0f, 50.0f));
    EXPECT_FLOAT_EQ(renderer.GetSize().x, 100.0f);
    EXPECT_FLOAT_EQ(renderer.GetSize().y, 50.0f);
}

TEST(SpriteRendererTest, SetSizeComponents)
{
    SpriteRenderer renderer;
    renderer.SetSize(200.0f, 150.0f);
    EXPECT_FLOAT_EQ(renderer.GetSize().x, 200.0f);
    EXPECT_FLOAT_EQ(renderer.GetSize().y, 150.0f);
}

TEST(SpriteRendererTest, UseTextureSize)
{
    SpriteRenderer renderer;
    renderer.SetSize(100.0f, 100.0f);
    renderer.UseTextureSize();
    EXPECT_FLOAT_EQ(renderer.GetSize().x, 0.0f);
    EXPECT_FLOAT_EQ(renderer.GetSize().y, 0.0f);
}

//============================================================================
// Pivot テスト
//============================================================================
TEST(SpriteRendererTest, SetPivotVector)
{
    SpriteRenderer renderer;
    renderer.SetPivot(Vector2(32.0f, 32.0f));
    EXPECT_FLOAT_EQ(renderer.GetPivot().x, 32.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().y, 32.0f);
}

TEST(SpriteRendererTest, SetPivotComponents)
{
    SpriteRenderer renderer;
    renderer.SetPivot(16.0f, 24.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().x, 16.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().y, 24.0f);
}

TEST(SpriteRendererTest, SetPivotFromCenter)
{
    SpriteRenderer renderer;
    renderer.SetPivotFromCenter(64.0f, 64.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().x, 32.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().y, 32.0f);
}

TEST(SpriteRendererTest, SetPivotFromCenterWithOffset)
{
    SpriteRenderer renderer;
    renderer.SetPivotFromCenter(64.0f, 64.0f, 10.0f, -5.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().x, 42.0f);
    EXPECT_FLOAT_EQ(renderer.GetPivot().y, 27.0f);
}

TEST(SpriteRendererTest, HasPivotWhenSet)
{
    SpriteRenderer renderer;
    renderer.SetPivot(1.0f, 0.0f);
    EXPECT_TRUE(renderer.HasPivot());
}

TEST(SpriteRendererTest, HasPivotWhenYOnly)
{
    SpriteRenderer renderer;
    renderer.SetPivot(0.0f, 1.0f);
    EXPECT_TRUE(renderer.HasPivot());
}

//============================================================================
// Sorting Layer テスト
//============================================================================
TEST(SpriteRendererTest, SetSortingLayer)
{
    SpriteRenderer renderer;
    renderer.SetSortingLayer(5);
    EXPECT_EQ(renderer.GetSortingLayer(), 5);
}

TEST(SpriteRendererTest, SetSortingLayerNegative)
{
    SpriteRenderer renderer;
    renderer.SetSortingLayer(-3);
    EXPECT_EQ(renderer.GetSortingLayer(), -3);
}

TEST(SpriteRendererTest, SetOrderInLayer)
{
    SpriteRenderer renderer;
    renderer.SetOrderInLayer(10);
    EXPECT_EQ(renderer.GetOrderInLayer(), 10);
}

TEST(SpriteRendererTest, SetOrderInLayerNegative)
{
    SpriteRenderer renderer;
    renderer.SetOrderInLayer(-5);
    EXPECT_EQ(renderer.GetOrderInLayer(), -5);
}

} // namespace
