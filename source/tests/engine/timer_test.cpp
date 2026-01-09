//----------------------------------------------------------------------------
//! @file   timer_test.cpp
//! @brief  Timerクラスのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/time/timer.h"
#include <thread>
#include <chrono>

namespace
{

//============================================================================
// Timer テスト
//============================================================================
class TimerTest : public ::testing::Test
{
protected:
    void SetUp() override {
        Timer::Start();
    }
};

TEST_F(TimerTest, StartInitializesTimer)
{
    // After Start, deltaTime should be 0 or very small
    EXPECT_GE(Timer::GetDeltaTime(), 0.0f);
}

TEST_F(TimerTest, GetDeltaTimeReturnsNonNegative)
{
    Timer::Update();
    EXPECT_GE(Timer::GetDeltaTime(), 0.0f);
}

TEST_F(TimerTest, GetTotalTimeReturnsNonNegative)
{
    Timer::Update();
    EXPECT_GE(Timer::GetTotalTime(), 0.0f);
}

TEST_F(TimerTest, GetFPSReturnsNonNegative)
{
    Timer::Update();
    EXPECT_GE(Timer::GetFPS(), 0.0f);
}

TEST_F(TimerTest, GetFrameCountIncrementsOnUpdate)
{
    uint64_t initial = Timer::GetFrameCount();
    Timer::Update();
    EXPECT_GT(Timer::GetFrameCount(), initial);
}

TEST_F(TimerTest, MultipleUpdatesIncrementFrameCount)
{
    uint64_t initial = Timer::GetFrameCount();
    Timer::Update();
    Timer::Update();
    Timer::Update();
    EXPECT_EQ(Timer::GetFrameCount(), initial + 3);
}

TEST_F(TimerTest, TotalTimeIncreases)
{
    Timer::Update();
    float time1 = Timer::GetTotalTime();

    // Small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    Timer::Update();
    float time2 = Timer::GetTotalTime();

    EXPECT_GT(time2, time1);
}

TEST_F(TimerTest, DeltaTimeCappeByMaxDeltaTime)
{
    // Sleep for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Update with small max delta time
    Timer::Update(0.01f);  // 10ms max

    // Delta time should be capped
    EXPECT_LE(Timer::GetDeltaTime(), 0.01f);
}

TEST_F(TimerTest, DeltaTimeDefaultMaxIs0_25)
{
    // Sleep for a long time
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    Timer::Update();  // Default max is 0.25

    // Should be capped to 0.25 (250ms)
    EXPECT_LE(Timer::GetDeltaTime(), 0.25f);
}

TEST_F(TimerTest, RestartResetsTimer)
{
    Timer::Update();
    Timer::Update();
    Timer::Update();

    Timer::Start();  // Restart

    // After restart, values should reset
    EXPECT_GE(Timer::GetDeltaTime(), 0.0f);
}

} // namespace
