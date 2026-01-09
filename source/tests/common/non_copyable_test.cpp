//----------------------------------------------------------------------------
//! @file   non_copyable_test.cpp
//! @brief  NonCopyable基底クラスのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <type_traits>
#include "common/utility/non_copyable.h"

namespace
{

//============================================================================
// NonCopyableNonMovable テスト
//============================================================================
class TestNonCopyableNonMovable : private NonCopyableNonMovable
{
public:
    TestNonCopyableNonMovable() = default;
    int value = 42;
};

TEST(NonCopyableNonMovableTest, IsNotCopyConstructible)
{
    EXPECT_FALSE(std::is_copy_constructible_v<TestNonCopyableNonMovable>);
}

TEST(NonCopyableNonMovableTest, IsNotCopyAssignable)
{
    EXPECT_FALSE(std::is_copy_assignable_v<TestNonCopyableNonMovable>);
}

TEST(NonCopyableNonMovableTest, IsNotMoveConstructible)
{
    EXPECT_FALSE(std::is_move_constructible_v<TestNonCopyableNonMovable>);
}

TEST(NonCopyableNonMovableTest, IsNotMoveAssignable)
{
    EXPECT_FALSE(std::is_move_assignable_v<TestNonCopyableNonMovable>);
}

TEST(NonCopyableNonMovableTest, IsDefaultConstructible)
{
    EXPECT_TRUE(std::is_default_constructible_v<TestNonCopyableNonMovable>);
    TestNonCopyableNonMovable obj;
    EXPECT_EQ(obj.value, 42);
}

//============================================================================
// NonCopyable テスト
//============================================================================
class TestNonCopyable : private NonCopyable
{
public:
    TestNonCopyable() = default;
    TestNonCopyable(TestNonCopyable&&) noexcept = default;
    TestNonCopyable& operator=(TestNonCopyable&&) noexcept = default;
    int value = 100;
};

TEST(NonCopyableTest, IsNotCopyConstructible)
{
    EXPECT_FALSE(std::is_copy_constructible_v<TestNonCopyable>);
}

TEST(NonCopyableTest, IsNotCopyAssignable)
{
    EXPECT_FALSE(std::is_copy_assignable_v<TestNonCopyable>);
}

TEST(NonCopyableTest, IsMoveConstructible)
{
    EXPECT_TRUE(std::is_move_constructible_v<TestNonCopyable>);
}

TEST(NonCopyableTest, IsMoveAssignable)
{
    EXPECT_TRUE(std::is_move_assignable_v<TestNonCopyable>);
}

TEST(NonCopyableTest, IsDefaultConstructible)
{
    EXPECT_TRUE(std::is_default_constructible_v<TestNonCopyable>);
    TestNonCopyable obj;
    EXPECT_EQ(obj.value, 100);
}

TEST(NonCopyableTest, MoveConstructorWorks)
{
    TestNonCopyable obj1;
    obj1.value = 200;
    TestNonCopyable obj2(std::move(obj1));
    EXPECT_EQ(obj2.value, 200);
}

TEST(NonCopyableTest, MoveAssignmentWorks)
{
    TestNonCopyable obj1;
    obj1.value = 300;
    TestNonCopyable obj2;
    obj2 = std::move(obj1);
    EXPECT_EQ(obj2.value, 300);
}

} // namespace
