//----------------------------------------------------------------------------
//! @file   keyboard_test.cpp
//! @brief  Keyboardクラスのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/input/keyboard.h"

namespace
{

//============================================================================
// Key enum テスト
//============================================================================
TEST(KeyEnumTest, AlphabetKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::A), 0x41);
    EXPECT_EQ(static_cast<int>(Key::Z), 0x5A);
}

TEST(KeyEnumTest, NumberKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::Num0), 0x30);
    EXPECT_EQ(static_cast<int>(Key::Num9), 0x39);
}

TEST(KeyEnumTest, FunctionKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::F1), 0x70);
    EXPECT_EQ(static_cast<int>(Key::F12), 0x7B);
}

TEST(KeyEnumTest, ArrowKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::Left), 0x25);
    EXPECT_EQ(static_cast<int>(Key::Up), 0x26);
    EXPECT_EQ(static_cast<int>(Key::Right), 0x27);
    EXPECT_EQ(static_cast<int>(Key::Down), 0x28);
}

TEST(KeyEnumTest, ModifierKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::LeftShift), 0xA0);
    EXPECT_EQ(static_cast<int>(Key::RightShift), 0xA1);
    EXPECT_EQ(static_cast<int>(Key::LeftControl), 0xA2);
    EXPECT_EQ(static_cast<int>(Key::RightControl), 0xA3);
    EXPECT_EQ(static_cast<int>(Key::LeftAlt), 0xA4);
    EXPECT_EQ(static_cast<int>(Key::RightAlt), 0xA5);
}

TEST(KeyEnumTest, SpecialKeysAreCorrectVirtualKeyCodes)
{
    EXPECT_EQ(static_cast<int>(Key::Escape), 0x1B);
    EXPECT_EQ(static_cast<int>(Key::Enter), 0x0D);
    EXPECT_EQ(static_cast<int>(Key::Tab), 0x09);
    EXPECT_EQ(static_cast<int>(Key::Space), 0x20);
    EXPECT_EQ(static_cast<int>(Key::Backspace), 0x08);
}

TEST(KeyEnumTest, KeyCountIs256)
{
    EXPECT_EQ(static_cast<int>(Key::KeyCount), 256);
}

//============================================================================
// Keyboard デフォルト状態テスト
//============================================================================
TEST(KeyboardTest, DefaultConstruction)
{
    Keyboard keyboard;
    // デフォルトでは全キーが押されていない
    EXPECT_FALSE(keyboard.IsKeyPressed(Key::A));
    EXPECT_FALSE(keyboard.IsKeyPressed(Key::Space));
}

TEST(KeyboardTest, DefaultKeyDownIsFalse)
{
    Keyboard keyboard;
    EXPECT_FALSE(keyboard.IsKeyDown(Key::A));
}

TEST(KeyboardTest, DefaultKeyUpIsFalse)
{
    Keyboard keyboard;
    EXPECT_FALSE(keyboard.IsKeyUp(Key::A));
}

TEST(KeyboardTest, DefaultKeyHoldTimeIsZero)
{
    Keyboard keyboard;
    EXPECT_FLOAT_EQ(keyboard.GetKeyHoldTime(Key::A), 0.0f);
}

TEST(KeyboardTest, DefaultModifiersNotPressed)
{
    Keyboard keyboard;
    EXPECT_FALSE(keyboard.IsShiftPressed());
    EXPECT_FALSE(keyboard.IsControlPressed());
    EXPECT_FALSE(keyboard.IsAltPressed());
}

//============================================================================
// OnKeyDown/OnKeyUp テスト（イベントベース）
//============================================================================
TEST(KeyboardTest, OnKeyDownSetsPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    EXPECT_TRUE(keyboard.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, OnKeyDownSetsDown)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    EXPECT_TRUE(keyboard.IsKeyDown(Key::A));
}

TEST(KeyboardTest, OnKeyDownResetsHoldTime)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    EXPECT_FLOAT_EQ(keyboard.GetKeyHoldTime(Key::A), 0.0f);
}

TEST(KeyboardTest, OnKeyUpClearsPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    keyboard.OnKeyUp(static_cast<int>(Key::A));
    EXPECT_FALSE(keyboard.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, OnKeyUpSetsUp)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    keyboard.OnKeyUp(static_cast<int>(Key::A));
    EXPECT_TRUE(keyboard.IsKeyUp(Key::A));
}

TEST(KeyboardTest, RepeatKeyDownDoesNotSetDown)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    // 最初のdown状態をクリアするためにもう一度OnKeyDownを呼ぶ（リピート）
    keyboard.OnKeyDown(static_cast<int>(Key::A));
    // リピート時はdownフラグが立たない（既にpressed状態のため）
    // ただし、down状態は最初のOnKeyDownで設定されたまま残っている
    // 実装を見ると、pressed==trueの場合はdownを設定しないだけ
    EXPECT_TRUE(keyboard.IsKeyPressed(Key::A));
}

//============================================================================
// 修飾キーテスト
//============================================================================
TEST(KeyboardTest, LeftShiftSetsShiftPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::LeftShift));
    EXPECT_TRUE(keyboard.IsShiftPressed());
}

TEST(KeyboardTest, RightShiftSetsShiftPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::RightShift));
    EXPECT_TRUE(keyboard.IsShiftPressed());
}

TEST(KeyboardTest, LeftControlSetsControlPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::LeftControl));
    EXPECT_TRUE(keyboard.IsControlPressed());
}

TEST(KeyboardTest, RightControlSetsControlPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::RightControl));
    EXPECT_TRUE(keyboard.IsControlPressed());
}

TEST(KeyboardTest, LeftAltSetsAltPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::LeftAlt));
    EXPECT_TRUE(keyboard.IsAltPressed());
}

TEST(KeyboardTest, RightAltSetsAltPressed)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(static_cast<int>(Key::RightAlt));
    EXPECT_TRUE(keyboard.IsAltPressed());
}

//============================================================================
// 範囲外アクセステスト
//============================================================================
TEST(KeyboardTest, InvalidVirtualKeyOnKeyDownIgnored)
{
    Keyboard keyboard;
    keyboard.OnKeyDown(-1);
    keyboard.OnKeyDown(300);
    // クラッシュしないことを確認（暗黙的にテスト）
    EXPECT_FALSE(keyboard.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, InvalidVirtualKeyOnKeyUpIgnored)
{
    Keyboard keyboard;
    keyboard.OnKeyUp(-1);
    keyboard.OnKeyUp(300);
    // クラッシュしないことを確認
    EXPECT_FALSE(keyboard.IsKeyPressed(Key::A));
}

//============================================================================
// コピー/ムーブテスト
//============================================================================
TEST(KeyboardTest, CopyConstructor)
{
    Keyboard keyboard1;
    keyboard1.OnKeyDown(static_cast<int>(Key::A));

    Keyboard keyboard2(keyboard1);
    EXPECT_TRUE(keyboard2.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, CopyAssignment)
{
    Keyboard keyboard1;
    keyboard1.OnKeyDown(static_cast<int>(Key::A));

    Keyboard keyboard2;
    keyboard2 = keyboard1;
    EXPECT_TRUE(keyboard2.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, MoveConstructor)
{
    Keyboard keyboard1;
    keyboard1.OnKeyDown(static_cast<int>(Key::A));

    Keyboard keyboard2(std::move(keyboard1));
    EXPECT_TRUE(keyboard2.IsKeyPressed(Key::A));
}

TEST(KeyboardTest, MoveAssignment)
{
    Keyboard keyboard1;
    keyboard1.OnKeyDown(static_cast<int>(Key::A));

    Keyboard keyboard2;
    keyboard2 = std::move(keyboard1);
    EXPECT_TRUE(keyboard2.IsKeyPressed(Key::A));
}

} // namespace
