//----------------------------------------------------------------------------
//! @file   light_test.cpp
//! @brief  ライト関連のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/lighting/light.h"
#include <cmath>

namespace
{

//============================================================================
// LightType テスト
//============================================================================
TEST(LightTypeTest, DirectionalIsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(LightType::Directional), 0u);
}

TEST(LightTypeTest, PointIsOne)
{
    EXPECT_EQ(static_cast<uint32_t>(LightType::Point), 1u);
}

TEST(LightTypeTest, SpotIsTwo)
{
    EXPECT_EQ(static_cast<uint32_t>(LightType::Spot), 2u);
}

//============================================================================
// LightData 構造体テスト
//============================================================================
TEST(LightDataTest, SizeIs64Bytes)
{
    EXPECT_EQ(sizeof(LightData), 64u);
}

TEST(LightDataTest, Is16ByteAligned)
{
    EXPECT_EQ(alignof(LightData), 16u);
}

TEST(LightDataTest, DefaultInitialization)
{
    LightData data = {};
    EXPECT_FLOAT_EQ(data.position.x, 0.0f);
    EXPECT_FLOAT_EQ(data.position.y, 0.0f);
    EXPECT_FLOAT_EQ(data.position.z, 0.0f);
    EXPECT_FLOAT_EQ(data.position.w, 0.0f);
}

//============================================================================
// LightingConstants 構造体テスト
//============================================================================
TEST(LightingConstantsTest, SizeIs560Bytes)
{
    EXPECT_EQ(sizeof(LightingConstants), 560u);
}

TEST(LightingConstantsTest, Is16ByteAligned)
{
    EXPECT_EQ(alignof(LightingConstants), 16u);
}

TEST(LightingConstantsTest, MaxLightsIsEight)
{
    EXPECT_EQ(kMaxLights, 8u);
}

//============================================================================
// LightBuilder::Directional テスト
//============================================================================
TEST(LightBuilderTest, DirectionalSetsType)
{
    Vector3 direction(0.0f, -1.0f, 0.0f);
    Color color = Colors::White;
    float intensity = 1.0f;

    LightData light = LightBuilder::Directional(direction, color, intensity);

    EXPECT_FLOAT_EQ(light.position.w, static_cast<float>(LightType::Directional));
}

TEST(LightBuilderTest, DirectionalSetsDirection)
{
    Vector3 direction(0.5f, -0.7f, 0.3f);
    Color color = Colors::White;

    LightData light = LightBuilder::Directional(direction, color, 1.0f);

    EXPECT_FLOAT_EQ(light.direction.x, direction.x);
    EXPECT_FLOAT_EQ(light.direction.y, direction.y);
    EXPECT_FLOAT_EQ(light.direction.z, direction.z);
}

TEST(LightBuilderTest, DirectionalSetsColor)
{
    Vector3 direction(0.0f, -1.0f, 0.0f);
    Color color(1.0f, 0.5f, 0.25f, 1.0f);
    float intensity = 2.0f;

    LightData light = LightBuilder::Directional(direction, color, intensity);

    EXPECT_FLOAT_EQ(light.color.R(), 1.0f);
    EXPECT_FLOAT_EQ(light.color.G(), 0.5f);
    EXPECT_FLOAT_EQ(light.color.B(), 0.25f);
    EXPECT_FLOAT_EQ(light.color.A(), intensity);
}

TEST(LightBuilderTest, DirectionalPositionIsZero)
{
    Vector3 direction(0.0f, -1.0f, 0.0f);
    LightData light = LightBuilder::Directional(direction, Colors::White, 1.0f);

    EXPECT_FLOAT_EQ(light.position.x, 0.0f);
    EXPECT_FLOAT_EQ(light.position.y, 0.0f);
    EXPECT_FLOAT_EQ(light.position.z, 0.0f);
}

//============================================================================
// LightBuilder::Point テスト
//============================================================================
TEST(LightBuilderTest, PointSetsType)
{
    Vector3 position(10.0f, 5.0f, 3.0f);
    Color color = Colors::Red;

    LightData light = LightBuilder::Point(position, color, 1.0f, 10.0f);

    EXPECT_FLOAT_EQ(light.position.w, static_cast<float>(LightType::Point));
}

TEST(LightBuilderTest, PointSetsPosition)
{
    Vector3 position(10.0f, 5.0f, 3.0f);
    Color color = Colors::Red;

    LightData light = LightBuilder::Point(position, color, 1.0f, 10.0f);

    EXPECT_FLOAT_EQ(light.position.x, position.x);
    EXPECT_FLOAT_EQ(light.position.y, position.y);
    EXPECT_FLOAT_EQ(light.position.z, position.z);
}

TEST(LightBuilderTest, PointSetsRange)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    float range = 25.0f;

    LightData light = LightBuilder::Point(position, Colors::White, 1.0f, range);

    EXPECT_FLOAT_EQ(light.direction.w, range);
}

TEST(LightBuilderTest, PointSetsColorAndIntensity)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Color color(0.8f, 0.6f, 0.4f, 1.0f);
    float intensity = 3.0f;

    LightData light = LightBuilder::Point(position, color, intensity, 10.0f);

    EXPECT_FLOAT_EQ(light.color.R(), 0.8f);
    EXPECT_FLOAT_EQ(light.color.G(), 0.6f);
    EXPECT_FLOAT_EQ(light.color.B(), 0.4f);
    EXPECT_FLOAT_EQ(light.color.A(), intensity);
}

TEST(LightBuilderTest, PointSetsAttenuation)
{
    Vector3 position(0.0f, 0.0f, 0.0f);

    LightData light = LightBuilder::Point(position, Colors::White, 1.0f, 10.0f);

    // Attenuation coefficient is in spotParams.z
    EXPECT_FLOAT_EQ(light.spotParams.z, 1.0f);
}

//============================================================================
// LightBuilder::Spot テスト
//============================================================================
TEST(LightBuilderTest, SpotSetsType)
{
    Vector3 position(0.0f, 10.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, 20.0f, 30.0f, 45.0f);

    EXPECT_FLOAT_EQ(light.position.w, static_cast<float>(LightType::Spot));
}

TEST(LightBuilderTest, SpotSetsPosition)
{
    Vector3 position(5.0f, 10.0f, 15.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, 20.0f, 30.0f, 45.0f);

    EXPECT_FLOAT_EQ(light.position.x, position.x);
    EXPECT_FLOAT_EQ(light.position.y, position.y);
    EXPECT_FLOAT_EQ(light.position.z, position.z);
}

TEST(LightBuilderTest, SpotSetsDirection)
{
    Vector3 position(0.0f, 10.0f, 0.0f);
    Vector3 direction(0.5f, -0.8f, 0.3f);

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, 20.0f, 30.0f, 45.0f);

    EXPECT_FLOAT_EQ(light.direction.x, direction.x);
    EXPECT_FLOAT_EQ(light.direction.y, direction.y);
    EXPECT_FLOAT_EQ(light.direction.z, direction.z);
}

TEST(LightBuilderTest, SpotSetsRange)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    float range = 50.0f;

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, range, 30.0f, 45.0f);

    EXPECT_FLOAT_EQ(light.direction.w, range);
}

TEST(LightBuilderTest, SpotSetsInnerAndOuterAngles)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    float innerAngle = 30.0f;
    float outerAngle = 45.0f;

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, 20.0f, innerAngle, outerAngle);

    // cos values should be calculated correctly
    float expectedInnerCos = std::cos(ToRadians(innerAngle * 0.5f));
    float expectedOuterCos = std::cos(ToRadians(outerAngle * 0.5f));

    EXPECT_NEAR(light.spotParams.x, expectedInnerCos, 0.0001f);
    EXPECT_NEAR(light.spotParams.y, expectedOuterCos, 0.0001f);
}

TEST(LightBuilderTest, SpotInnerCosGreaterThanOuterCos)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);

    LightData light = LightBuilder::Spot(position, direction, Colors::White, 1.0f, 20.0f, 30.0f, 60.0f);

    // Inner angle is smaller, so cos(inner) > cos(outer)
    EXPECT_GT(light.spotParams.x, light.spotParams.y);
}

TEST(LightBuilderTest, SpotSetsColorAndIntensity)
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 direction(0.0f, -1.0f, 0.0f);
    Color color(0.9f, 0.8f, 0.7f, 1.0f);
    float intensity = 5.0f;

    LightData light = LightBuilder::Spot(position, direction, color, intensity, 20.0f, 30.0f, 45.0f);

    EXPECT_FLOAT_EQ(light.color.R(), 0.9f);
    EXPECT_FLOAT_EQ(light.color.G(), 0.8f);
    EXPECT_FLOAT_EQ(light.color.B(), 0.7f);
    EXPECT_FLOAT_EQ(light.color.A(), intensity);
}

} // namespace
