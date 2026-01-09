//----------------------------------------------------------------------------
//! @file   state_machine_test.cpp
//! @brief  StateMachineのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/state/state_machine.h"
#include <vector>

namespace
{

// テスト用の状態enum
enum class TestState {
    Idle,
    Walking,
    Running,
    Jumping,
    Falling
};

//============================================================================
// StateMachine テスト
//============================================================================
class StateMachineTest : public ::testing::Test
{
protected:
    StateMachine<TestState> sm_;
};

TEST_F(StateMachineTest, DefaultStateIsDefaultConstructed)
{
    EXPECT_EQ(sm_.GetState(), TestState::Idle);  // enumのデフォルト値
}

TEST_F(StateMachineTest, InitialStateConstructor)
{
    StateMachine<TestState> sm(TestState::Running);
    EXPECT_EQ(sm.GetState(), TestState::Running);
}

TEST_F(StateMachineTest, SetStateChangesState)
{
    EXPECT_TRUE(sm_.SetState(TestState::Walking));
    EXPECT_EQ(sm_.GetState(), TestState::Walking);
}

TEST_F(StateMachineTest, SetStateSameStateSucceeds)
{
    sm_.SetState(TestState::Walking);
    EXPECT_TRUE(sm_.SetState(TestState::Walking));
    EXPECT_EQ(sm_.GetState(), TestState::Walking);
}

TEST_F(StateMachineTest, IsStateReturnsTrue)
{
    sm_.SetState(TestState::Jumping);
    EXPECT_TRUE(sm_.IsState(TestState::Jumping));
}

TEST_F(StateMachineTest, IsStateReturnsFalse)
{
    sm_.SetState(TestState::Jumping);
    EXPECT_FALSE(sm_.IsState(TestState::Walking));
}

//============================================================================
// ロック機能テスト
//============================================================================
TEST_F(StateMachineTest, InitiallyNotLocked)
{
    EXPECT_FALSE(sm_.IsLocked());
}

TEST_F(StateMachineTest, LockPreventsStateChange)
{
    sm_.Lock();
    EXPECT_TRUE(sm_.IsLocked());
    EXPECT_FALSE(sm_.SetState(TestState::Running));
    EXPECT_EQ(sm_.GetState(), TestState::Idle);
}

TEST_F(StateMachineTest, UnlockAllowsStateChange)
{
    sm_.Lock();
    sm_.Unlock();
    EXPECT_FALSE(sm_.IsLocked());
    EXPECT_TRUE(sm_.SetState(TestState::Running));
    EXPECT_EQ(sm_.GetState(), TestState::Running);
}

//============================================================================
// コールバックテスト
//============================================================================
TEST_F(StateMachineTest, CallbackFiredOnStateChange)
{
    bool callbackFired = false;
    TestState oldState{};
    TestState newState{};

    sm_.SetOnStateChanged([&](TestState o, TestState n) {
        callbackFired = true;
        oldState = o;
        newState = n;
    });

    sm_.SetState(TestState::Walking);

    EXPECT_TRUE(callbackFired);
    EXPECT_EQ(oldState, TestState::Idle);
    EXPECT_EQ(newState, TestState::Walking);
}

TEST_F(StateMachineTest, CallbackNotFiredOnSameState)
{
    sm_.SetState(TestState::Walking);

    int callCount = 0;
    sm_.SetOnStateChanged([&](TestState, TestState) {
        callCount++;
    });

    sm_.SetState(TestState::Walking);  // 同じ状態

    EXPECT_EQ(callCount, 0);
}

TEST_F(StateMachineTest, CallbackNotFiredWhenLocked)
{
    int callCount = 0;
    sm_.SetOnStateChanged([&](TestState, TestState) {
        callCount++;
    });

    sm_.Lock();
    sm_.SetState(TestState::Running);

    EXPECT_EQ(callCount, 0);
}

TEST_F(StateMachineTest, MultipleStateTransitionsTracked)
{
    std::vector<std::pair<TestState, TestState>> transitions;

    sm_.SetOnStateChanged([&](TestState o, TestState n) {
        transitions.push_back({o, n});
    });

    sm_.SetState(TestState::Walking);
    sm_.SetState(TestState::Running);
    sm_.SetState(TestState::Jumping);

    ASSERT_EQ(transitions.size(), 3);

    EXPECT_EQ(transitions[0].first, TestState::Idle);
    EXPECT_EQ(transitions[0].second, TestState::Walking);

    EXPECT_EQ(transitions[1].first, TestState::Walking);
    EXPECT_EQ(transitions[1].second, TestState::Running);

    EXPECT_EQ(transitions[2].first, TestState::Running);
    EXPECT_EQ(transitions[2].second, TestState::Jumping);
}

//============================================================================
// 整数enumテスト
//============================================================================
enum class IntState : int {
    A = 0,
    B = 1,
    C = 2
};

TEST(StateMachineIntEnumTest, WorksWithIntEnum)
{
    StateMachine<IntState> sm(IntState::A);
    EXPECT_EQ(sm.GetState(), IntState::A);

    sm.SetState(IntState::C);
    EXPECT_EQ(sm.GetState(), IntState::C);
}

} // namespace
