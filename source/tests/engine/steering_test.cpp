//----------------------------------------------------------------------------
//! @file   steering_test.cpp
//! @brief  ステアリング行動のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <SimpleMath.h>
#include "engine/ai/steering.h"
#include <cmath>

namespace
{

using Vector2 = DirectX::SimpleMath::Vector2;

//============================================================================
// Seek テスト
//============================================================================
TEST(SteeringTest, SeekReturnsZeroWhenAtTarget)
{
    Vector2 position(10.0f, 20.0f);
    Vector2 target(10.0f, 20.0f);
    Vector2 result = Steering::Seek(position, target, 5.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, SeekReturnsCorrectDirection)
{
    Vector2 position(0.0f, 0.0f);
    Vector2 target(10.0f, 0.0f);
    Vector2 result = Steering::Seek(position, target, 5.0f);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, SeekNormalizesSpeed)
{
    Vector2 position(0.0f, 0.0f);
    Vector2 target(3.0f, 4.0f);  // 距離5
    float maxSpeed = 10.0f;
    Vector2 result = Steering::Seek(position, target, maxSpeed);
    float length = result.Length();
    EXPECT_FLOAT_EQ(length, maxSpeed);
}

//============================================================================
// Flee テスト
//============================================================================
TEST(SteeringTest, FleeReturnsZeroWhenAtThreat)
{
    Vector2 position(10.0f, 20.0f);
    Vector2 threat(10.0f, 20.0f);
    Vector2 result = Steering::Flee(position, threat, 5.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, FleeReturnsOppositeDirection)
{
    Vector2 position(0.0f, 0.0f);
    Vector2 threat(10.0f, 0.0f);
    Vector2 result = Steering::Flee(position, threat, 5.0f);
    EXPECT_FLOAT_EQ(result.x, -5.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, FleeIsOppositeOfSeek)
{
    Vector2 position(5.0f, 5.0f);
    Vector2 target(10.0f, 15.0f);
    float speed = 3.0f;

    Vector2 seekResult = Steering::Seek(position, target, speed);
    Vector2 fleeResult = Steering::Flee(position, target, speed);

    EXPECT_NEAR(seekResult.x, -fleeResult.x, 0.001f);
    EXPECT_NEAR(seekResult.y, -fleeResult.y, 0.001f);
}

//============================================================================
// Arrive テスト
//============================================================================
TEST(SteeringTest, ArriveReturnsZeroWhenAtTarget)
{
    Vector2 position(10.0f, 20.0f);
    Vector2 target(10.0f, 20.0f);
    Vector2 result = Steering::Arrive(position, target, 5.0f, 10.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, ArriveFullSpeedOutsideSlowRadius)
{
    Vector2 position(0.0f, 0.0f);
    Vector2 target(100.0f, 0.0f);  // 100単位先
    float maxSpeed = 10.0f;
    float slowRadius = 20.0f;
    Vector2 result = Steering::Arrive(position, target, maxSpeed, slowRadius);
    EXPECT_FLOAT_EQ(result.Length(), maxSpeed);
}

TEST(SteeringTest, ArriveSlowsDownInsideSlowRadius)
{
    Vector2 position(0.0f, 0.0f);
    Vector2 target(10.0f, 0.0f);  // 10単位先
    float maxSpeed = 10.0f;
    float slowRadius = 20.0f;  // 減速領域内
    Vector2 result = Steering::Arrive(position, target, maxSpeed, slowRadius);
    float expectedSpeed = maxSpeed * (10.0f / 20.0f);  // 半分の速度
    EXPECT_FLOAT_EQ(result.Length(), expectedSpeed);
}

//============================================================================
// Wander テスト
//============================================================================
TEST(SteeringTest, WanderReturnsNormalizedVector)
{
    Vector2 position(0.0f, 0.0f);
    float angle = 0.0f;
    Vector2 result = Steering::Wander(position, 1.0f, angle);
    EXPECT_NEAR(result.Length(), 1.0f, 0.01f);
}

TEST(SteeringTest, WanderUpdatesAngle)
{
    Vector2 position(0.0f, 0.0f);
    float angle = 0.0f;
    float originalAngle = angle;

    // 複数回呼び出して角度が変化することを確認
    bool angleChanged = false;
    for (int i = 0; i < 100; ++i) {
        (void)Steering::Wander(position, 1.0f, angle);
        if (std::abs(angle - originalAngle) > 0.001f) {
            angleChanged = true;
            break;
        }
    }
    EXPECT_TRUE(angleChanged);
}

//============================================================================
// Separation テスト
//============================================================================
TEST(SteeringTest, SeparationReturnsZeroWithNoNeighbors)
{
    Vector2 position(0.0f, 0.0f);
    std::vector<Vector2> neighbors;
    Vector2 result = Steering::Separation(position, neighbors, 10.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, SeparationReturnsZeroWhenNeighborsOutOfRange)
{
    Vector2 position(0.0f, 0.0f);
    std::vector<Vector2> neighbors = {
        Vector2(100.0f, 0.0f),
        Vector2(0.0f, 100.0f)
    };
    Vector2 result = Steering::Separation(position, neighbors, 10.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, SeparationPushesAwayFromNeighbor)
{
    Vector2 position(0.0f, 0.0f);
    std::vector<Vector2> neighbors = { Vector2(5.0f, 0.0f) };
    Vector2 result = Steering::Separation(position, neighbors, 10.0f);
    EXPECT_LT(result.x, 0.0f);  // 左に押し出される
    EXPECT_NEAR(result.y, 0.0f, 0.001f);
}

//============================================================================
// Cohesion テスト
//============================================================================
TEST(SteeringTest, CohesionReturnsZeroWithNoNeighbors)
{
    Vector2 position(0.0f, 0.0f);
    std::vector<Vector2> neighbors;
    Vector2 result = Steering::Cohesion(position, neighbors, 5.0f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, CohesionMovesTowardCenter)
{
    Vector2 position(0.0f, 0.0f);
    std::vector<Vector2> neighbors = {
        Vector2(10.0f, 0.0f),
        Vector2(10.0f, 10.0f),
        Vector2(0.0f, 10.0f)
    };
    // 中心は約 (6.67, 6.67)
    Vector2 result = Steering::Cohesion(position, neighbors, 5.0f);
    EXPECT_GT(result.x, 0.0f);  // 右方向
    EXPECT_GT(result.y, 0.0f);  // 上方向
}

//============================================================================
// Alignment テスト
//============================================================================
TEST(SteeringTest, AlignmentReturnsZeroWithNoVelocities)
{
    std::vector<Vector2> velocities;
    Vector2 result = Steering::Alignment(velocities);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(SteeringTest, AlignmentReturnsAverageVelocity)
{
    std::vector<Vector2> velocities = {
        Vector2(10.0f, 0.0f),
        Vector2(0.0f, 10.0f)
    };
    Vector2 result = Steering::Alignment(velocities);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
}

TEST(SteeringTest, AlignmentWithSingleVelocity)
{
    std::vector<Vector2> velocities = { Vector2(3.0f, 4.0f) };
    Vector2 result = Steering::Alignment(velocities);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

} // namespace
