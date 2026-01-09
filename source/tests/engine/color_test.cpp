//----------------------------------------------------------------------------
//! @file   color_test.cpp
//! @brief  カラーユーティリティのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/math/color.h"
#include <cmath>

namespace
{

//============================================================================
// Colors 定数テスト
//============================================================================
TEST(ColorsTest, WhiteIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::White.x, 1.0f);
    EXPECT_FLOAT_EQ(Colors::White.y, 1.0f);
    EXPECT_FLOAT_EQ(Colors::White.z, 1.0f);
    EXPECT_FLOAT_EQ(Colors::White.w, 1.0f);
}

TEST(ColorsTest, BlackIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::Black.x, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Black.y, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Black.z, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Black.w, 1.0f);
}

TEST(ColorsTest, RedIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::Red.x, 1.0f);
    EXPECT_FLOAT_EQ(Colors::Red.y, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Red.z, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Red.w, 1.0f);
}

TEST(ColorsTest, GreenIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::Green.x, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Green.y, 1.0f);
    EXPECT_FLOAT_EQ(Colors::Green.z, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Green.w, 1.0f);
}

TEST(ColorsTest, BlueIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::Blue.x, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Blue.y, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Blue.z, 1.0f);
    EXPECT_FLOAT_EQ(Colors::Blue.w, 1.0f);
}

TEST(ColorsTest, TransparentIsCorrect)
{
    EXPECT_FLOAT_EQ(Colors::Transparent.x, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Transparent.y, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Transparent.z, 0.0f);
    EXPECT_FLOAT_EQ(Colors::Transparent.w, 0.0f);
}

//============================================================================
// ColorFromRGBA テスト
//============================================================================
TEST(ColorFromRGBATest, FullWhite)
{
    Color c = ColorFromRGBA(255, 255, 255, 255);
    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 1.0f);
    EXPECT_FLOAT_EQ(c.z, 1.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromRGBATest, FullBlack)
{
    Color c = ColorFromRGBA(0, 0, 0, 255);
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 0.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromRGBATest, HalfValues)
{
    Color c = ColorFromRGBA(128, 128, 128, 128);
    EXPECT_NEAR(c.x, 0.502f, 0.01f);
    EXPECT_NEAR(c.y, 0.502f, 0.01f);
    EXPECT_NEAR(c.z, 0.502f, 0.01f);
    EXPECT_NEAR(c.w, 0.502f, 0.01f);
}

TEST(ColorFromRGBATest, DefaultAlphaIsOpaque)
{
    Color c = ColorFromRGBA(255, 0, 0);
    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 0.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

//============================================================================
// ColorFromHex テスト
//============================================================================
TEST(ColorFromHexTest, WhiteOpaque)
{
    Color c = ColorFromHex(0xFFFFFFFF);
    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 1.0f);
    EXPECT_FLOAT_EQ(c.z, 1.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromHexTest, RedOpaque)
{
    Color c = ColorFromHex(0xFF0000FF);
    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 0.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromHexTest, GreenOpaque)
{
    Color c = ColorFromHex(0x00FF00FF);
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 1.0f);
    EXPECT_FLOAT_EQ(c.z, 0.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromHexTest, BlueOpaque)
{
    Color c = ColorFromHex(0x0000FFFF);
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 1.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromHexTest, Transparent)
{
    Color c = ColorFromHex(0x00000000);
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 0.0f);
    EXPECT_FLOAT_EQ(c.w, 0.0f);
}

//============================================================================
// ColorFromHSV テスト
//============================================================================
TEST(ColorFromHSVTest, RedAt0Degrees)
{
    Color c = ColorFromHSV(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(c.x, 1.0f, 0.01f);
    EXPECT_NEAR(c.y, 0.0f, 0.01f);
    EXPECT_NEAR(c.z, 0.0f, 0.01f);
}

TEST(ColorFromHSVTest, GreenAt120Degrees)
{
    Color c = ColorFromHSV(120.0f, 1.0f, 1.0f);
    EXPECT_NEAR(c.x, 0.0f, 0.01f);
    EXPECT_NEAR(c.y, 1.0f, 0.01f);
    EXPECT_NEAR(c.z, 0.0f, 0.01f);
}

TEST(ColorFromHSVTest, BlueAt240Degrees)
{
    Color c = ColorFromHSV(240.0f, 1.0f, 1.0f);
    EXPECT_NEAR(c.x, 0.0f, 0.01f);
    EXPECT_NEAR(c.y, 0.0f, 0.01f);
    EXPECT_NEAR(c.z, 1.0f, 0.01f);
}

TEST(ColorFromHSVTest, WhiteWithZeroSaturation)
{
    Color c = ColorFromHSV(0.0f, 0.0f, 1.0f);
    EXPECT_NEAR(c.x, 1.0f, 0.01f);
    EXPECT_NEAR(c.y, 1.0f, 0.01f);
    EXPECT_NEAR(c.z, 1.0f, 0.01f);
}

TEST(ColorFromHSVTest, BlackWithZeroValue)
{
    Color c = ColorFromHSV(0.0f, 1.0f, 0.0f);
    EXPECT_NEAR(c.x, 0.0f, 0.01f);
    EXPECT_NEAR(c.y, 0.0f, 0.01f);
    EXPECT_NEAR(c.z, 0.0f, 0.01f);
}

TEST(ColorFromHSVTest, DefaultAlphaIsOpaque)
{
    Color c = ColorFromHSV(0.0f, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(c.w, 1.0f);
}

TEST(ColorFromHSVTest, CustomAlpha)
{
    Color c = ColorFromHSV(0.0f, 1.0f, 1.0f, 0.5f);
    EXPECT_FLOAT_EQ(c.w, 0.5f);
}

} // namespace
