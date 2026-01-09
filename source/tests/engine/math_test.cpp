//----------------------------------------------------------------------------
//! @file   math_test.cpp
//! @brief  数学ユーティリティのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/math/math_types.h"
#include <cmath>

namespace
{

//============================================================================
// ToRadians / ToDegrees テスト
//============================================================================
TEST(MathTest, ToRadiansConvertsCorrectly)
{
    EXPECT_FLOAT_EQ(ToRadians(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(ToRadians(180.0f), DirectX::XM_PI);
    EXPECT_FLOAT_EQ(ToRadians(90.0f), DirectX::XM_PI / 2.0f);
    EXPECT_FLOAT_EQ(ToRadians(360.0f), DirectX::XM_2PI);
}

TEST(MathTest, ToDegreesConvertsCorrectly)
{
    EXPECT_FLOAT_EQ(ToDegrees(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(ToDegrees(DirectX::XM_PI), 180.0f);
    EXPECT_FLOAT_EQ(ToDegrees(DirectX::XM_PI / 2.0f), 90.0f);
    EXPECT_FLOAT_EQ(ToDegrees(DirectX::XM_2PI), 360.0f);
}

TEST(MathTest, ToRadiansAndToDegreesAreInverse)
{
    float degrees = 45.0f;
    EXPECT_FLOAT_EQ(ToDegrees(ToRadians(degrees)), degrees);

    float radians = 1.0f;
    EXPECT_FLOAT_EQ(ToRadians(ToDegrees(radians)), radians);
}

//============================================================================
// Clamp テスト
//============================================================================
TEST(MathTest, ClampWithinRange)
{
    EXPECT_EQ(Clamp(5, 0, 10), 5);
    EXPECT_FLOAT_EQ(Clamp(0.5f, 0.0f, 1.0f), 0.5f);
}

TEST(MathTest, ClampBelowMin)
{
    EXPECT_EQ(Clamp(-5, 0, 10), 0);
    EXPECT_FLOAT_EQ(Clamp(-0.5f, 0.0f, 1.0f), 0.0f);
}

TEST(MathTest, ClampAboveMax)
{
    EXPECT_EQ(Clamp(15, 0, 10), 10);
    EXPECT_FLOAT_EQ(Clamp(1.5f, 0.0f, 1.0f), 1.0f);
}

TEST(MathTest, ClampAtBoundaries)
{
    EXPECT_EQ(Clamp(0, 0, 10), 0);
    EXPECT_EQ(Clamp(10, 0, 10), 10);
}

//============================================================================
// Lerp テスト
//============================================================================
TEST(MathTest, LerpAtZero)
{
    EXPECT_FLOAT_EQ(Lerp(0.0f, 100.0f, 0.0f), 0.0f);
}

TEST(MathTest, LerpAtOne)
{
    EXPECT_FLOAT_EQ(Lerp(0.0f, 100.0f, 1.0f), 100.0f);
}

TEST(MathTest, LerpAtHalf)
{
    EXPECT_FLOAT_EQ(Lerp(0.0f, 100.0f, 0.5f), 50.0f);
}

TEST(MathTest, LerpWithNegativeValues)
{
    EXPECT_FLOAT_EQ(Lerp(-100.0f, 100.0f, 0.5f), 0.0f);
}

TEST(MathTest, LerpExtrapolatesAboveOne)
{
    EXPECT_FLOAT_EQ(Lerp(0.0f, 100.0f, 2.0f), 200.0f);
}

TEST(MathTest, LerpExtrapolatesBelowZero)
{
    EXPECT_FLOAT_EQ(Lerp(0.0f, 100.0f, -1.0f), -100.0f);
}

//============================================================================
// LerpClamped テスト
//============================================================================
TEST(MathTest, LerpClampedClampsToZero)
{
    EXPECT_FLOAT_EQ(LerpClamped(0.0f, 100.0f, -1.0f), 0.0f);
}

TEST(MathTest, LerpClampedClampsToOne)
{
    EXPECT_FLOAT_EQ(LerpClamped(0.0f, 100.0f, 2.0f), 100.0f);
}

TEST(MathTest, LerpClampedNormalRange)
{
    EXPECT_FLOAT_EQ(LerpClamped(0.0f, 100.0f, 0.5f), 50.0f);
}

//============================================================================
// LineSegment テスト
//============================================================================
TEST(LineSegmentTest, DefaultConstruction)
{
    LineSegment seg;
    EXPECT_FLOAT_EQ(seg.start.x, 0.0f);
    EXPECT_FLOAT_EQ(seg.start.y, 0.0f);
    EXPECT_FLOAT_EQ(seg.end.x, 0.0f);
    EXPECT_FLOAT_EQ(seg.end.y, 0.0f);
}

TEST(LineSegmentTest, ConstructFromVectors)
{
    LineSegment seg(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
    EXPECT_FLOAT_EQ(seg.start.x, 1.0f);
    EXPECT_FLOAT_EQ(seg.start.y, 2.0f);
    EXPECT_FLOAT_EQ(seg.end.x, 3.0f);
    EXPECT_FLOAT_EQ(seg.end.y, 4.0f);
}

TEST(LineSegmentTest, ConstructFromFloats)
{
    LineSegment seg(0.0f, 0.0f, 10.0f, 0.0f);
    EXPECT_FLOAT_EQ(seg.start.x, 0.0f);
    EXPECT_FLOAT_EQ(seg.end.x, 10.0f);
}

TEST(LineSegmentTest, Direction)
{
    LineSegment seg(0.0f, 0.0f, 10.0f, 0.0f);
    Vector2 dir = seg.Direction();
    EXPECT_FLOAT_EQ(dir.x, 10.0f);
    EXPECT_FLOAT_EQ(dir.y, 0.0f);
}

TEST(LineSegmentTest, Length)
{
    LineSegment seg(0.0f, 0.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(seg.Length(), 5.0f);
}

TEST(LineSegmentTest, LengthSquared)
{
    LineSegment seg(0.0f, 0.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(seg.LengthSquared(), 25.0f);
}

TEST(LineSegmentTest, IntersectsTrue)
{
    LineSegment seg1(0.0f, 0.0f, 10.0f, 10.0f);
    LineSegment seg2(0.0f, 10.0f, 10.0f, 0.0f);
    EXPECT_TRUE(seg1.Intersects(seg2));
}

TEST(LineSegmentTest, IntersectsFalseParallel)
{
    LineSegment seg1(0.0f, 0.0f, 10.0f, 0.0f);
    LineSegment seg2(0.0f, 5.0f, 10.0f, 5.0f);
    EXPECT_FALSE(seg1.Intersects(seg2));
}

TEST(LineSegmentTest, IntersectsFalseNoContact)
{
    LineSegment seg1(0.0f, 0.0f, 5.0f, 0.0f);
    LineSegment seg2(10.0f, 0.0f, 15.0f, 0.0f);
    EXPECT_FALSE(seg1.Intersects(seg2));
}

TEST(LineSegmentTest, IntersectsWithIntersectionPoint)
{
    LineSegment seg1(0.0f, 0.0f, 10.0f, 10.0f);
    LineSegment seg2(0.0f, 10.0f, 10.0f, 0.0f);
    Vector2 intersection;
    EXPECT_TRUE(seg1.Intersects(seg2, intersection));
    EXPECT_NEAR(intersection.x, 5.0f, 0.001f);
    EXPECT_NEAR(intersection.y, 5.0f, 0.001f);
}

TEST(LineSegmentTest, DistanceToPointOnSegment)
{
    LineSegment seg(0.0f, 0.0f, 10.0f, 0.0f);
    float dist = seg.DistanceToPoint(Vector2(5.0f, 0.0f));
    EXPECT_FLOAT_EQ(dist, 0.0f);
}

TEST(LineSegmentTest, DistanceToPointPerpendicularOffset)
{
    LineSegment seg(0.0f, 0.0f, 10.0f, 0.0f);
    float dist = seg.DistanceToPoint(Vector2(5.0f, 3.0f));
    EXPECT_FLOAT_EQ(dist, 3.0f);
}

TEST(LineSegmentTest, DistanceToPointNearStart)
{
    LineSegment seg(0.0f, 0.0f, 10.0f, 0.0f);
    float dist = seg.DistanceToPoint(Vector2(-3.0f, 4.0f));
    EXPECT_FLOAT_EQ(dist, 5.0f);  // 3-4-5の三角形
}

} // namespace
