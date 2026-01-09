//----------------------------------------------------------------------------
//! @file   ui_button_types_test.cpp
//! @brief  UIボタン関連型のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/ui_button_component.h"

namespace
{

//============================================================================
// UIButtonComponent::State enum テスト
//============================================================================
TEST(UIButtonStateTest, NormalIsDefined)
{
    UIButtonComponent::State state = UIButtonComponent::State::Normal;
    EXPECT_EQ(state, UIButtonComponent::State::Normal);
}

TEST(UIButtonStateTest, HoverIsDefined)
{
    UIButtonComponent::State state = UIButtonComponent::State::Hover;
    EXPECT_EQ(state, UIButtonComponent::State::Hover);
}

TEST(UIButtonStateTest, PressedIsDefined)
{
    UIButtonComponent::State state = UIButtonComponent::State::Pressed;
    EXPECT_EQ(state, UIButtonComponent::State::Pressed);
}

TEST(UIButtonStateTest, AllStatesAreDistinct)
{
    EXPECT_NE(UIButtonComponent::State::Normal, UIButtonComponent::State::Hover);
    EXPECT_NE(UIButtonComponent::State::Hover, UIButtonComponent::State::Pressed);
    EXPECT_NE(UIButtonComponent::State::Pressed, UIButtonComponent::State::Normal);
}

TEST(UIButtonStateTest, NormalIsZero)
{
    EXPECT_EQ(static_cast<int>(UIButtonComponent::State::Normal), 0);
}

TEST(UIButtonStateTest, HoverIsOne)
{
    EXPECT_EQ(static_cast<int>(UIButtonComponent::State::Hover), 1);
}

TEST(UIButtonStateTest, PressedIsTwo)
{
    EXPECT_EQ(static_cast<int>(UIButtonComponent::State::Pressed), 2);
}

} // namespace
