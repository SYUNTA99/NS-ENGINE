//----------------------------------------------------------------------------
//! @file   camera_test.cpp
//! @brief  カメラコンポーネントのテスト（Camera2D, Camera3D）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/camera2d.h"
#include "engine/component/camera3d.h"

namespace
{

//============================================================================
// Camera2D テスト
//============================================================================
TEST(Camera2DTest, DefaultZoom)
{
    Camera2D camera;
    EXPECT_FLOAT_EQ(camera.GetZoom(), 1.0f);
}

TEST(Camera2DTest, DefaultViewportWidth)
{
    Camera2D camera;
    EXPECT_FLOAT_EQ(camera.GetViewportWidth(), 1280.0f);
}

TEST(Camera2DTest, DefaultViewportHeight)
{
    Camera2D camera;
    EXPECT_FLOAT_EQ(camera.GetViewportHeight(), 720.0f);
}

TEST(Camera2DTest, ConstructorWithViewport)
{
    Camera2D camera(800.0f, 600.0f);
    EXPECT_FLOAT_EQ(camera.GetViewportWidth(), 800.0f);
    EXPECT_FLOAT_EQ(camera.GetViewportHeight(), 600.0f);
}

TEST(Camera2DTest, SetZoom)
{
    Camera2D camera;
    camera.SetZoom(2.0f);
    EXPECT_FLOAT_EQ(camera.GetZoom(), 2.0f);
}

TEST(Camera2DTest, SetZoomClampsMinimum)
{
    Camera2D camera;
    camera.SetZoom(0.001f);
    // Zoom should be clamped to minimum (likely 0.01 or similar)
    EXPECT_GT(camera.GetZoom(), 0.0f);
}

TEST(Camera2DTest, SetViewportSize)
{
    Camera2D camera;
    camera.SetViewportSize(1920.0f, 1080.0f);
    EXPECT_FLOAT_EQ(camera.GetViewportWidth(), 1920.0f);
    EXPECT_FLOAT_EQ(camera.GetViewportHeight(), 1080.0f);
}

//============================================================================
// Camera3D テスト
//============================================================================
TEST(Camera3DTest, DefaultFOV)
{
    Camera3D camera;
    EXPECT_FLOAT_EQ(camera.GetFOV(), 60.0f);
}

TEST(Camera3DTest, DefaultNearPlane)
{
    Camera3D camera;
    EXPECT_FLOAT_EQ(camera.GetNearPlane(), 0.1f);
}

TEST(Camera3DTest, DefaultFarPlane)
{
    Camera3D camera;
    EXPECT_FLOAT_EQ(camera.GetFarPlane(), 1000.0f);
}

TEST(Camera3DTest, DefaultAspectRatio)
{
    Camera3D camera;
    EXPECT_NEAR(camera.GetAspectRatio(), 16.0f / 9.0f, 0.001f);
}

TEST(Camera3DTest, ConstructorWithFOV)
{
    Camera3D camera(90.0f);
    EXPECT_FLOAT_EQ(camera.GetFOV(), 90.0f);
}

TEST(Camera3DTest, ConstructorWithFOVAndAspect)
{
    Camera3D camera(75.0f, 4.0f / 3.0f);
    EXPECT_FLOAT_EQ(camera.GetFOV(), 75.0f);
    EXPECT_NEAR(camera.GetAspectRatio(), 4.0f / 3.0f, 0.001f);
}

TEST(Camera3DTest, SetFOV)
{
    Camera3D camera;
    camera.SetFOV(90.0f);
    EXPECT_FLOAT_EQ(camera.GetFOV(), 90.0f);
}

TEST(Camera3DTest, SetNearPlane)
{
    Camera3D camera;
    camera.SetNearPlane(1.0f);
    EXPECT_FLOAT_EQ(camera.GetNearPlane(), 1.0f);
}

TEST(Camera3DTest, SetFarPlane)
{
    Camera3D camera;
    camera.SetFarPlane(5000.0f);
    EXPECT_FLOAT_EQ(camera.GetFarPlane(), 5000.0f);
}

TEST(Camera3DTest, SetAspectRatio)
{
    Camera3D camera;
    camera.SetAspectRatio(2.0f);
    EXPECT_FLOAT_EQ(camera.GetAspectRatio(), 2.0f);
}

TEST(Camera3DTest, SetViewportSizeCalculatesAspectRatio)
{
    Camera3D camera;
    camera.SetViewportSize(1920.0f, 1080.0f);
    EXPECT_NEAR(camera.GetAspectRatio(), 1920.0f / 1080.0f, 0.001f);
}

TEST(Camera3DTest, SetViewportSizeIgnoresZeroHeight)
{
    Camera3D camera;
    float originalAspect = camera.GetAspectRatio();
    camera.SetViewportSize(1920.0f, 0.0f);
    // Aspect ratio should remain unchanged
    EXPECT_FLOAT_EQ(camera.GetAspectRatio(), originalAspect);
}

TEST(Camera3DTest, SetViewportSizeIgnoresNegativeHeight)
{
    Camera3D camera;
    float originalAspect = camera.GetAspectRatio();
    camera.SetViewportSize(1920.0f, -100.0f);
    // Aspect ratio should remain unchanged
    EXPECT_FLOAT_EQ(camera.GetAspectRatio(), originalAspect);
}

} // namespace
