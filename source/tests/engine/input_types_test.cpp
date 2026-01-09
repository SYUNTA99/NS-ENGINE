//----------------------------------------------------------------------------
//! @file   input_types_test.cpp
//! @brief  入力関連型のテスト（Key, MouseButton, GamepadButton）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/input/key.h"
#include "engine/input/gamepad.h"

namespace
{

//============================================================================
// Key enum テスト
//============================================================================
TEST(KeyTest, AlphaKeysAreSequential)
{
    EXPECT_EQ(static_cast<int>(Key::A), 0x41);
    EXPECT_EQ(static_cast<int>(Key::B), 0x42);
    EXPECT_EQ(static_cast<int>(Key::Z), 0x5A);
}

TEST(KeyTest, NumericKeysAreSequential)
{
    EXPECT_EQ(static_cast<int>(Key::Num0), 0x30);
    EXPECT_EQ(static_cast<int>(Key::Num1), 0x31);
    EXPECT_EQ(static_cast<int>(Key::Num9), 0x39);
}

TEST(KeyTest, FunctionKeysAreSequential)
{
    EXPECT_EQ(static_cast<int>(Key::F1), 0x70);
    EXPECT_EQ(static_cast<int>(Key::F2), 0x71);
    EXPECT_EQ(static_cast<int>(Key::F12), 0x7B);
}

TEST(KeyTest, ArrowKeys)
{
    EXPECT_EQ(static_cast<int>(Key::Left), 0x25);
    EXPECT_EQ(static_cast<int>(Key::Up), 0x26);
    EXPECT_EQ(static_cast<int>(Key::Right), 0x27);
    EXPECT_EQ(static_cast<int>(Key::Down), 0x28);
}

TEST(KeyTest, ControlKeys)
{
    EXPECT_EQ(static_cast<int>(Key::Escape), 0x1B);
    EXPECT_EQ(static_cast<int>(Key::Enter), 0x0D);
    EXPECT_EQ(static_cast<int>(Key::Tab), 0x09);
    EXPECT_EQ(static_cast<int>(Key::Space), 0x20);
    EXPECT_EQ(static_cast<int>(Key::Backspace), 0x08);
}

TEST(KeyTest, ModifierKeys)
{
    EXPECT_EQ(static_cast<int>(Key::LeftShift), 0xA0);
    EXPECT_EQ(static_cast<int>(Key::RightShift), 0xA1);
    EXPECT_EQ(static_cast<int>(Key::LeftControl), 0xA2);
    EXPECT_EQ(static_cast<int>(Key::RightControl), 0xA3);
    EXPECT_EQ(static_cast<int>(Key::LeftAlt), 0xA4);
    EXPECT_EQ(static_cast<int>(Key::RightAlt), 0xA5);
}

TEST(KeyTest, NumpadKeys)
{
    EXPECT_EQ(static_cast<int>(Key::Numpad0), 0x60);
    EXPECT_EQ(static_cast<int>(Key::Numpad9), 0x69);
    EXPECT_EQ(static_cast<int>(Key::NumpadAdd), 0x6B);
    EXPECT_EQ(static_cast<int>(Key::NumpadSubtract), 0x6D);
    EXPECT_EQ(static_cast<int>(Key::NumpadMultiply), 0x6A);
    EXPECT_EQ(static_cast<int>(Key::NumpadDivide), 0x6F);
}

TEST(KeyTest, KeyCountIs256)
{
    EXPECT_EQ(static_cast<int>(Key::KeyCount), 256);
}

//============================================================================
// MouseButton enum テスト
//============================================================================
TEST(MouseButtonTest, LeftIsZero)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Left), 0);
}

TEST(MouseButtonTest, RightIsOne)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Right), 1);
}

TEST(MouseButtonTest, MiddleIsTwo)
{
    EXPECT_EQ(static_cast<int>(MouseButton::Middle), 2);
}

TEST(MouseButtonTest, SideButtons)
{
    EXPECT_EQ(static_cast<int>(MouseButton::X1), 3);
    EXPECT_EQ(static_cast<int>(MouseButton::X2), 4);
}

TEST(MouseButtonTest, ButtonCountIsFive)
{
    EXPECT_EQ(static_cast<int>(MouseButton::ButtonCount), 5);
}

//============================================================================
// GamepadButton enum テスト
//============================================================================
TEST(GamepadButtonTest, DPadValues)
{
    // XInput defines these as specific bit flags
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::DPadUp), 0x0001);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::DPadDown), 0x0002);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::DPadLeft), 0x0004);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::DPadRight), 0x0008);
}

TEST(GamepadButtonTest, StartBackButtons)
{
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::Start), 0x0010);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::Back), 0x0020);
}

TEST(GamepadButtonTest, ThumbButtons)
{
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::LeftThumb), 0x0040);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::RightThumb), 0x0080);
}

TEST(GamepadButtonTest, ShoulderButtons)
{
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::LeftShoulder), 0x0100);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::RightShoulder), 0x0200);
}

TEST(GamepadButtonTest, FaceButtons)
{
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::A), 0x1000);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::B), 0x2000);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::X), 0x4000);
    EXPECT_EQ(static_cast<uint16_t>(GamepadButton::Y), 0x8000);
}

TEST(GamepadButtonTest, AllButtonsAreSingleBits)
{
    auto isSingleBit = [](uint16_t v) {
        return v != 0 && (v & (v - 1)) == 0;
    };

    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::DPadUp)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::DPadDown)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::DPadLeft)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::DPadRight)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::Start)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::Back)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::LeftThumb)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::RightThumb)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::LeftShoulder)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::RightShoulder)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::A)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::B)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::X)));
    EXPECT_TRUE(isSingleBit(static_cast<uint16_t>(GamepadButton::Y)));
}

TEST(GamepadButtonTest, AllButtonsAreUnique)
{
    uint16_t combined = 0;
    combined |= static_cast<uint16_t>(GamepadButton::DPadUp);
    combined |= static_cast<uint16_t>(GamepadButton::DPadDown);
    combined |= static_cast<uint16_t>(GamepadButton::DPadLeft);
    combined |= static_cast<uint16_t>(GamepadButton::DPadRight);
    combined |= static_cast<uint16_t>(GamepadButton::Start);
    combined |= static_cast<uint16_t>(GamepadButton::Back);
    combined |= static_cast<uint16_t>(GamepadButton::LeftThumb);
    combined |= static_cast<uint16_t>(GamepadButton::RightThumb);
    combined |= static_cast<uint16_t>(GamepadButton::LeftShoulder);
    combined |= static_cast<uint16_t>(GamepadButton::RightShoulder);
    combined |= static_cast<uint16_t>(GamepadButton::A);
    combined |= static_cast<uint16_t>(GamepadButton::B);
    combined |= static_cast<uint16_t>(GamepadButton::X);
    combined |= static_cast<uint16_t>(GamepadButton::Y);

    // All 14 buttons should be unique bits
    int bitCount = 0;
    for (int i = 0; i < 16; ++i) {
        if (combined & (1 << i)) ++bitCount;
    }
    EXPECT_EQ(bitCount, 14);
}

} // namespace
