//----------------------------------------------------------------------------
//! @file   time_manager_test.cpp
//! @brief  TimeManagerのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/time/time_manager.h"

namespace
{

//============================================================================
// TimeState enum テスト
//============================================================================
TEST(TimeStateTest, NormalIsDefined)
{
    TimeState state = TimeState::Normal;
    EXPECT_EQ(state, TimeState::Normal);
}

TEST(TimeStateTest, FrozenIsDefined)
{
    TimeState state = TimeState::Frozen;
    EXPECT_EQ(state, TimeState::Frozen);
}

TEST(TimeStateTest, SlowMoIsDefined)
{
    TimeState state = TimeState::SlowMo;
    EXPECT_EQ(state, TimeState::SlowMo);
}

TEST(TimeStateTest, StatesAreDistinct)
{
    EXPECT_NE(TimeState::Normal, TimeState::Frozen);
    EXPECT_NE(TimeState::Normal, TimeState::SlowMo);
    EXPECT_NE(TimeState::Frozen, TimeState::SlowMo);
}

//============================================================================
// TimeManager シングルトンテスト
//============================================================================
class TimeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override {
        TimeManager::Create();
    }

    void TearDown() override {
        TimeManager::Destroy();
    }
};

TEST_F(TimeManagerTest, InitialStateIsNormal)
{
    EXPECT_EQ(TimeManager::Get().GetState(), TimeState::Normal);
}

TEST_F(TimeManagerTest, InitialTimeScaleIsOne)
{
    EXPECT_FLOAT_EQ(TimeManager::Get().GetTimeScale(), 1.0f);
}

TEST_F(TimeManagerTest, InitiallyNotFrozen)
{
    EXPECT_FALSE(TimeManager::Get().IsFrozen());
}

TEST_F(TimeManagerTest, InitiallyIsNormal)
{
    EXPECT_TRUE(TimeManager::Get().IsNormal());
}

//============================================================================
// Freeze/Resume テスト
//============================================================================
TEST_F(TimeManagerTest, FreezeChangesState)
{
    TimeManager::Get().Freeze();
    EXPECT_EQ(TimeManager::Get().GetState(), TimeState::Frozen);
}

TEST_F(TimeManagerTest, FreezeSetsFrozenFlag)
{
    TimeManager::Get().Freeze();
    EXPECT_TRUE(TimeManager::Get().IsFrozen());
}

TEST_F(TimeManagerTest, FreezeUnsetsNormalFlag)
{
    TimeManager::Get().Freeze();
    EXPECT_FALSE(TimeManager::Get().IsNormal());
}

TEST_F(TimeManagerTest, FreezeTimeScaleIsZero)
{
    TimeManager::Get().Freeze();
    EXPECT_FLOAT_EQ(TimeManager::Get().GetTimeScale(), 0.0f);
}

TEST_F(TimeManagerTest, ResumeChangesState)
{
    TimeManager::Get().Freeze();
    TimeManager::Get().Resume();
    EXPECT_EQ(TimeManager::Get().GetState(), TimeState::Normal);
}

TEST_F(TimeManagerTest, ResumeRestoresTimeScale)
{
    TimeManager::Get().Freeze();
    TimeManager::Get().Resume();
    EXPECT_FLOAT_EQ(TimeManager::Get().GetTimeScale(), 1.0f);
}

TEST_F(TimeManagerTest, ResumeUnsetsFrozenFlag)
{
    TimeManager::Get().Freeze();
    TimeManager::Get().Resume();
    EXPECT_FALSE(TimeManager::Get().IsFrozen());
}

TEST_F(TimeManagerTest, ResumeSetsNormalFlag)
{
    TimeManager::Get().Freeze();
    TimeManager::Get().Resume();
    EXPECT_TRUE(TimeManager::Get().IsNormal());
}

//============================================================================
// SlowMotion テスト
//============================================================================
TEST_F(TimeManagerTest, SetSlowMotionChangesState)
{
    TimeManager::Get().SetSlowMotion(0.5f);
    EXPECT_EQ(TimeManager::Get().GetState(), TimeState::SlowMo);
}

TEST_F(TimeManagerTest, SetSlowMotionSetsTimeScale)
{
    TimeManager::Get().SetSlowMotion(0.5f);
    EXPECT_FLOAT_EQ(TimeManager::Get().GetTimeScale(), 0.5f);
}

TEST_F(TimeManagerTest, SetSlowMotionIsNotFrozen)
{
    TimeManager::Get().SetSlowMotion(0.5f);
    EXPECT_FALSE(TimeManager::Get().IsFrozen());
}

TEST_F(TimeManagerTest, SetSlowMotionIsNotNormal)
{
    TimeManager::Get().SetSlowMotion(0.5f);
    EXPECT_FALSE(TimeManager::Get().IsNormal());
}

//============================================================================
// GetScaledDeltaTime テスト
//============================================================================
TEST_F(TimeManagerTest, GetScaledDeltaTimeNormal)
{
    float rawDelta = 0.016f;  // ~60fps
    float scaled = TimeManager::Get().GetScaledDeltaTime(rawDelta);
    EXPECT_FLOAT_EQ(scaled, rawDelta);  // Scale is 1.0
}

TEST_F(TimeManagerTest, GetScaledDeltaTimeFrozen)
{
    TimeManager::Get().Freeze();
    float rawDelta = 0.016f;
    float scaled = TimeManager::Get().GetScaledDeltaTime(rawDelta);
    EXPECT_FLOAT_EQ(scaled, 0.0f);  // Scale is 0.0
}

TEST_F(TimeManagerTest, GetScaledDeltaTimeSlowMo)
{
    TimeManager::Get().SetSlowMotion(0.5f);
    float rawDelta = 0.016f;
    float scaled = TimeManager::Get().GetScaledDeltaTime(rawDelta);
    EXPECT_FLOAT_EQ(scaled, 0.008f);  // Scale is 0.5
}

//============================================================================
// コールバックテスト
//============================================================================
TEST_F(TimeManagerTest, StateChangedCallbackOnFreeze)
{
    TimeState receivedState = TimeState::Normal;
    TimeManager::Get().SetOnStateChanged([&receivedState](TimeState state) {
        receivedState = state;
    });

    TimeManager::Get().Freeze();
    EXPECT_EQ(receivedState, TimeState::Frozen);
}

TEST_F(TimeManagerTest, StateChangedCallbackOnResume)
{
    TimeState receivedState = TimeState::Frozen;
    TimeManager::Get().SetOnStateChanged([&receivedState](TimeState state) {
        receivedState = state;
    });

    TimeManager::Get().Freeze();
    TimeManager::Get().Resume();
    EXPECT_EQ(receivedState, TimeState::Normal);
}

TEST_F(TimeManagerTest, StateChangedCallbackOnSlowMo)
{
    TimeState receivedState = TimeState::Normal;
    TimeManager::Get().SetOnStateChanged([&receivedState](TimeState state) {
        receivedState = state;
    });

    TimeManager::Get().SetSlowMotion(0.5f);
    EXPECT_EQ(receivedState, TimeState::SlowMo);
}

} // namespace
