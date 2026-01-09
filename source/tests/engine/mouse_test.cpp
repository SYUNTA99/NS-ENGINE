//----------------------------------------------------------------------------
//! @file   mouse_test.cpp
//! @brief  Mouseクラスのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/input/mouse.h"

namespace
{

//============================================================================
// MouseButton enum テスト
//============================================================================
TEST(MouseButtonEnumTest, LeftIsZero)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Left), 0);
}

TEST(MouseButtonEnumTest, RightIsOne)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Right), 1);
}

TEST(MouseButtonEnumTest, MiddleIsTwo)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Middle), 2);
}

TEST(MouseButtonEnumTest, X1IsThree)
{
    EXPECT_EQ(static_cast<int>(MouseButton::X1), 3);
}

TEST(MouseButtonEnumTest, X2IsFour)
{
    EXPECT_EQ(static_cast<int>(MouseButton::X2), 4);
}

TEST(MouseButtonEnumTest, ButtonCountIsFive)
{
    EXPECT_EQ(static_cast<int>(MouseButton::ButtonCount), 5);
}

//============================================================================
// Mouse デフォルト状態テスト
//============================================================================
TEST(MouseTest, DefaultPositionIsZero)
{
    Mouse mouse;
    EXPECT_EQ(mouse.GetX(), 0);
    EXPECT_EQ(mouse.GetY(), 0);
}

TEST(MouseTest, DefaultDeltaIsZero)
{
    Mouse mouse;
    EXPECT_EQ(mouse.GetDeltaX(), 0);
    EXPECT_EQ(mouse.GetDeltaY(), 0);
}

TEST(MouseTest, DefaultWheelDeltaIsZero)
{
    Mouse mouse;
    EXPECT_FLOAT_EQ(mouse.GetWheelDelta(), 0.0f);
}

TEST(MouseTest, DefaultButtonNotPressed)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Right));
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Middle));
}

TEST(MouseTest, DefaultButtonDownIsFalse)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonDown(MouseButton::Left));
}

TEST(MouseTest, DefaultButtonUpIsFalse)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonUp(MouseButton::Left));
}

//============================================================================
// GetPosition テスト
//============================================================================
TEST(MouseTest, GetPositionReturnsVector2)
{
    Mouse mouse;
    mouse.SetPosition(100, 200);
    Vector2 pos = mouse.GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 200.0f);
}

//============================================================================
// SetPosition テスト
//============================================================================
TEST(MouseTest, SetPositionUpdatesX)
{
    Mouse mouse;
    mouse.SetPosition(150, 0);
    EXPECT_EQ(mouse.GetX(), 150);
}

TEST(MouseTest, SetPositionUpdatesY)
{
    Mouse mouse;
    mouse.SetPosition(0, 250);
    EXPECT_EQ(mouse.GetY(), 250);
}

TEST(MouseTest, SetPositionNegativeValues)
{
    Mouse mouse;
    mouse.SetPosition(-10, -20);
    EXPECT_EQ(mouse.GetX(), -10);
    EXPECT_EQ(mouse.GetY(), -20);
}

//============================================================================
// OnButtonDown/OnButtonUp テスト
//============================================================================
TEST(MouseTest, OnButtonDownSetsPressed)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Left);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, OnButtonDownSetsDown)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Left);
    EXPECT_TRUE(mouse.IsButtonDown(MouseButton::Left));
}

TEST(MouseTest, OnButtonUpClearsPressed)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Left);
    mouse.OnButtonUp(MouseButton::Left);
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, OnButtonUpSetsUp)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Left);
    mouse.OnButtonUp(MouseButton::Left);
    EXPECT_TRUE(mouse.IsButtonUp(MouseButton::Left));
}

TEST(MouseTest, RightButtonDown)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Right);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::Right));
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, MiddleButtonDown)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Middle);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::Middle));
}

TEST(MouseTest, X1ButtonDown)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::X1);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::X1));
}

TEST(MouseTest, X2ButtonDown)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::X2);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::X2));
}

TEST(MouseTest, MultipleButtonsCanBePressed)
{
    Mouse mouse;
    mouse.OnButtonDown(MouseButton::Left);
    mouse.OnButtonDown(MouseButton::Right);
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::Left));
    EXPECT_TRUE(mouse.IsButtonPressed(MouseButton::Right));
}

//============================================================================
// OnWheel テスト
//============================================================================
TEST(MouseTest, OnWheelPositive)
{
    Mouse mouse;
    mouse.OnWheel(1.0f);
    EXPECT_FLOAT_EQ(mouse.GetWheelDelta(), 1.0f);
}

TEST(MouseTest, OnWheelNegative)
{
    Mouse mouse;
    mouse.OnWheel(-1.0f);
    EXPECT_FLOAT_EQ(mouse.GetWheelDelta(), -1.0f);
}

TEST(MouseTest, OnWheelAccumulates)
{
    Mouse mouse;
    mouse.OnWheel(1.0f);
    mouse.OnWheel(0.5f);
    EXPECT_FLOAT_EQ(mouse.GetWheelDelta(), 1.5f);
}

TEST(MouseTest, OnWheelAccumulatesPositiveAndNegative)
{
    Mouse mouse;
    mouse.OnWheel(2.0f);
    mouse.OnWheel(-0.5f);
    EXPECT_FLOAT_EQ(mouse.GetWheelDelta(), 1.5f);
}

//============================================================================
// 範囲外アクセステスト
//============================================================================
TEST(MouseTest, InvalidButtonDownIgnored)
{
    Mouse mouse;
    mouse.OnButtonDown(static_cast<MouseButton>(-1));
    mouse.OnButtonDown(static_cast<MouseButton>(10));
    // クラッシュしないことを確認
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, InvalidButtonUpIgnored)
{
    Mouse mouse;
    mouse.OnButtonUp(static_cast<MouseButton>(-1));
    mouse.OnButtonUp(static_cast<MouseButton>(10));
    // クラッシュしないことを確認
    EXPECT_FALSE(mouse.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, InvalidButtonPressedReturnsFalse)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonPressed(static_cast<MouseButton>(-1)));
    EXPECT_FALSE(mouse.IsButtonPressed(static_cast<MouseButton>(10)));
}

TEST(MouseTest, InvalidButtonDownReturnsFalse)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonDown(static_cast<MouseButton>(-1)));
    EXPECT_FALSE(mouse.IsButtonDown(static_cast<MouseButton>(10)));
}

TEST(MouseTest, InvalidButtonUpReturnsFalse)
{
    Mouse mouse;
    EXPECT_FALSE(mouse.IsButtonUp(static_cast<MouseButton>(-1)));
    EXPECT_FALSE(mouse.IsButtonUp(static_cast<MouseButton>(10)));
}

//============================================================================
// コピー/ムーブテスト
//============================================================================
TEST(MouseTest, CopyConstructor)
{
    Mouse mouse1;
    mouse1.SetPosition(100, 200);
    mouse1.OnButtonDown(MouseButton::Left);

    Mouse mouse2(mouse1);
    EXPECT_EQ(mouse2.GetX(), 100);
    EXPECT_EQ(mouse2.GetY(), 200);
    EXPECT_TRUE(mouse2.IsButtonPressed(MouseButton::Left));
}

TEST(MouseTest, CopyAssignment)
{
    Mouse mouse1;
    mouse1.SetPosition(100, 200);
    mouse1.OnButtonDown(MouseButton::Right);

    Mouse mouse2;
    mouse2 = mouse1;
    EXPECT_EQ(mouse2.GetX(), 100);
    EXPECT_TRUE(mouse2.IsButtonPressed(MouseButton::Right));
}

TEST(MouseTest, MoveConstructor)
{
    Mouse mouse1;
    mouse1.SetPosition(50, 75);
    mouse1.OnButtonDown(MouseButton::Middle);

    Mouse mouse2(std::move(mouse1));
    EXPECT_EQ(mouse2.GetX(), 50);
    EXPECT_EQ(mouse2.GetY(), 75);
    EXPECT_TRUE(mouse2.IsButtonPressed(MouseButton::Middle));
}

TEST(MouseTest, MoveAssignment)
{
    Mouse mouse1;
    mouse1.SetPosition(30, 40);
    mouse1.OnWheel(2.0f);

    Mouse mouse2;
    mouse2 = std::move(mouse1);
    EXPECT_EQ(mouse2.GetX(), 30);
    EXPECT_FLOAT_EQ(mouse2.GetWheelDelta(), 2.0f);
}

} // namespace
