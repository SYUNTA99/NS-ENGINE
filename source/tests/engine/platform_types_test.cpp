//----------------------------------------------------------------------------
//! @file   platform_types_test.cpp
//! @brief  プラットフォーム関連型のテスト（WindowDesc, ApplicationDesc, ShadowMapSettings）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/platform/window.h"
#include "engine/platform/application.h"
#include "engine/lighting/shadow_map.h"

namespace
{

//============================================================================
// WindowDesc テスト
//============================================================================
TEST(WindowDescTest, DefaultHInstanceIsNull)
{
    WindowDesc desc;
    EXPECT_EQ(desc.hInstance, nullptr);
}

TEST(WindowDescTest, DefaultTitle)
{
    WindowDesc desc;
    EXPECT_EQ(desc.title, L"mutra Application");
}

TEST(WindowDescTest, DefaultWidth)
{
    WindowDesc desc;
    EXPECT_EQ(desc.width, 1280u);
}

TEST(WindowDescTest, DefaultHeight)
{
    WindowDesc desc;
    EXPECT_EQ(desc.height, 720u);
}

TEST(WindowDescTest, DefaultResizable)
{
    WindowDesc desc;
    EXPECT_TRUE(desc.resizable);
}

TEST(WindowDescTest, DefaultMinWidth)
{
    WindowDesc desc;
    EXPECT_EQ(desc.minWidth, 320u);
}

TEST(WindowDescTest, DefaultMinHeight)
{
    WindowDesc desc;
    EXPECT_EQ(desc.minHeight, 240u);
}

TEST(WindowDescTest, CanSetTitle)
{
    WindowDesc desc;
    desc.title = L"Test Window";
    EXPECT_EQ(desc.title, L"Test Window");
}

TEST(WindowDescTest, CanSetDimensions)
{
    WindowDesc desc;
    desc.width = 1920;
    desc.height = 1080;
    EXPECT_EQ(desc.width, 1920u);
    EXPECT_EQ(desc.height, 1080u);
}

TEST(WindowDescTest, CanSetResizable)
{
    WindowDesc desc;
    desc.resizable = false;
    EXPECT_FALSE(desc.resizable);
}

//============================================================================
// ApplicationDesc テスト
//============================================================================
TEST(ApplicationDescTest, DefaultHInstanceIsNull)
{
    ApplicationDesc desc;
    EXPECT_EQ(desc.hInstance, nullptr);
}

TEST(ApplicationDescTest, DefaultRenderWidth)
{
    ApplicationDesc desc;
    EXPECT_EQ(desc.renderWidth, 1920u);
}

TEST(ApplicationDescTest, DefaultRenderHeight)
{
    ApplicationDesc desc;
    EXPECT_EQ(desc.renderHeight, 1080u);
}

TEST(ApplicationDescTest, DefaultEnableDebugLayer)
{
    ApplicationDesc desc;
    EXPECT_TRUE(desc.enableDebugLayer);
}

TEST(ApplicationDescTest, DefaultVsyncIsOn)
{
    ApplicationDesc desc;
    EXPECT_EQ(desc.vsync, VSyncMode::On);
}

TEST(ApplicationDescTest, DefaultMaxDeltaTime)
{
    ApplicationDesc desc;
    EXPECT_FLOAT_EQ(desc.maxDeltaTime, 0.25f);
}

TEST(ApplicationDescTest, CanSetRenderResolution)
{
    ApplicationDesc desc;
    desc.renderWidth = 3840;
    desc.renderHeight = 2160;
    EXPECT_EQ(desc.renderWidth, 3840u);
    EXPECT_EQ(desc.renderHeight, 2160u);
}

TEST(ApplicationDescTest, CanSetDebugLayer)
{
    ApplicationDesc desc;
    desc.enableDebugLayer = false;
    EXPECT_FALSE(desc.enableDebugLayer);
}

TEST(ApplicationDescTest, CanSetVsync)
{
    ApplicationDesc desc;
    desc.vsync = VSyncMode::Off;
    EXPECT_EQ(desc.vsync, VSyncMode::Off);
}

TEST(ApplicationDescTest, WindowDescIsEmbedded)
{
    ApplicationDesc desc;
    desc.window.width = 800;
    desc.window.height = 600;
    EXPECT_EQ(desc.window.width, 800u);
    EXPECT_EQ(desc.window.height, 600u);
}

//============================================================================
// ShadowMapSettings テスト
//============================================================================
TEST(ShadowMapSettingsTest, DefaultResolution)
{
    ShadowMapSettings settings;
    EXPECT_EQ(settings.resolution, 2048u);
}

TEST(ShadowMapSettingsTest, DefaultNearPlane)
{
    ShadowMapSettings settings;
    EXPECT_FLOAT_EQ(settings.nearPlane, 0.1f);
}

TEST(ShadowMapSettingsTest, DefaultFarPlane)
{
    ShadowMapSettings settings;
    EXPECT_FLOAT_EQ(settings.farPlane, 100.0f);
}

TEST(ShadowMapSettingsTest, DefaultOrthoSize)
{
    ShadowMapSettings settings;
    EXPECT_FLOAT_EQ(settings.orthoSize, 50.0f);
}

TEST(ShadowMapSettingsTest, DefaultDepthBias)
{
    ShadowMapSettings settings;
    EXPECT_FLOAT_EQ(settings.depthBias, 0.005f);
}

TEST(ShadowMapSettingsTest, DefaultNormalBias)
{
    ShadowMapSettings settings;
    EXPECT_FLOAT_EQ(settings.normalBias, 0.01f);
}

TEST(ShadowMapSettingsTest, CanSetResolution)
{
    ShadowMapSettings settings;
    settings.resolution = 4096;
    EXPECT_EQ(settings.resolution, 4096u);
}

TEST(ShadowMapSettingsTest, CanSetClipPlanes)
{
    ShadowMapSettings settings;
    settings.nearPlane = 1.0f;
    settings.farPlane = 500.0f;
    EXPECT_FLOAT_EQ(settings.nearPlane, 1.0f);
    EXPECT_FLOAT_EQ(settings.farPlane, 500.0f);
}

TEST(ShadowMapSettingsTest, CanSetOrthoSize)
{
    ShadowMapSettings settings;
    settings.orthoSize = 100.0f;
    EXPECT_FLOAT_EQ(settings.orthoSize, 100.0f);
}

TEST(ShadowMapSettingsTest, CanSetBiases)
{
    ShadowMapSettings settings;
    settings.depthBias = 0.01f;
    settings.normalBias = 0.02f;
    EXPECT_FLOAT_EQ(settings.depthBias, 0.01f);
    EXPECT_FLOAT_EQ(settings.normalBias, 0.02f);
}

//============================================================================
// VSyncMode enum テスト
//============================================================================
TEST(VSyncModeTest, OffIsDefined)
{
    VSyncMode mode = VSyncMode::Off;
    EXPECT_EQ(mode, VSyncMode::Off);
}

TEST(VSyncModeTest, OnIsDefined)
{
    VSyncMode mode = VSyncMode::On;
    EXPECT_EQ(mode, VSyncMode::On);
}

TEST(VSyncModeTest, HalfIsDefined)
{
    VSyncMode mode = VSyncMode::Half;
    EXPECT_EQ(mode, VSyncMode::Half);
}

TEST(VSyncModeTest, AllModesAreDistinct)
{
    EXPECT_NE(VSyncMode::Off, VSyncMode::On);
    EXPECT_NE(VSyncMode::On, VSyncMode::Half);
    EXPECT_NE(VSyncMode::Half, VSyncMode::Off);
}

TEST(VSyncModeTest, OffIsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(VSyncMode::Off), 0u);
}

TEST(VSyncModeTest, OnIsOne)
{
    EXPECT_EQ(static_cast<uint32_t>(VSyncMode::On), 1u);
}

TEST(VSyncModeTest, HalfIsTwo)
{
    EXPECT_EQ(static_cast<uint32_t>(VSyncMode::Half), 2u);
}

} // namespace
