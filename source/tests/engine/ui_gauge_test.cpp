//----------------------------------------------------------------------------
//! @file   ui_gauge_test.cpp
//! @brief  UIゲージコンポーネントのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/ui_gauge_component.h"

namespace
{

//============================================================================
// UIGaugeComponent デフォルト値テスト
//============================================================================
TEST(UIGaugeComponentTest, DefaultValue)
{
    UIGaugeComponent gauge;
    EXPECT_FLOAT_EQ(gauge.GetValue(), 1.0f);
}

TEST(UIGaugeComponentTest, DefaultSizeWidth)
{
    UIGaugeComponent gauge;
    EXPECT_FLOAT_EQ(gauge.GetSize().x, 100.0f);
}

TEST(UIGaugeComponentTest, DefaultSizeHeight)
{
    UIGaugeComponent gauge;
    EXPECT_FLOAT_EQ(gauge.GetSize().y, 10.0f);
}

TEST(UIGaugeComponentTest, DefaultBackgroundColor)
{
    UIGaugeComponent gauge;
    const Color& bg = gauge.GetBackgroundColor();
    EXPECT_FLOAT_EQ(bg.R(), 0.2f);
    EXPECT_FLOAT_EQ(bg.G(), 0.2f);
    EXPECT_FLOAT_EQ(bg.B(), 0.2f);
    EXPECT_FLOAT_EQ(bg.A(), 0.8f);
}

TEST(UIGaugeComponentTest, DefaultFillColor)
{
    UIGaugeComponent gauge;
    const Color& fill = gauge.GetFillColor();
    EXPECT_FLOAT_EQ(fill.R(), 0.0f);
    EXPECT_FLOAT_EQ(fill.G(), 1.0f);
    EXPECT_FLOAT_EQ(fill.B(), 0.0f);
    EXPECT_FLOAT_EQ(fill.A(), 1.0f);
}

//============================================================================
// SetValue テスト
//============================================================================
TEST(UIGaugeComponentTest, SetValueNormal)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.5f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.5f);
}

TEST(UIGaugeComponentTest, SetValueClampsToZero)
{
    UIGaugeComponent gauge;
    gauge.SetValue(-0.5f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.0f);
}

TEST(UIGaugeComponentTest, SetValueClampsToOne)
{
    UIGaugeComponent gauge;
    gauge.SetValue(1.5f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 1.0f);
}

TEST(UIGaugeComponentTest, SetValueZero)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.0f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.0f);
}

TEST(UIGaugeComponentTest, SetValueOne)
{
    UIGaugeComponent gauge;
    gauge.SetValue(1.0f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 1.0f);
}

//============================================================================
// AddValue テスト
//============================================================================
TEST(UIGaugeComponentTest, AddValuePositive)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.5f);
    gauge.AddValue(0.2f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.7f);
}

TEST(UIGaugeComponentTest, AddValueNegative)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.5f);
    gauge.AddValue(-0.2f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.3f);
}

TEST(UIGaugeComponentTest, AddValueClampsToOne)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.8f);
    gauge.AddValue(0.5f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 1.0f);
}

TEST(UIGaugeComponentTest, AddValueClampsToZero)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.2f);
    gauge.AddValue(-0.5f);
    EXPECT_FLOAT_EQ(gauge.GetValue(), 0.0f);
}

//============================================================================
// IsEmpty/IsFull テスト
//============================================================================
TEST(UIGaugeComponentTest, IsEmptyWhenZero)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.0f);
    EXPECT_TRUE(gauge.IsEmpty());
}

TEST(UIGaugeComponentTest, IsNotEmptyWhenPositive)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.1f);
    EXPECT_FALSE(gauge.IsEmpty());
}

TEST(UIGaugeComponentTest, IsFullWhenOne)
{
    UIGaugeComponent gauge;
    gauge.SetValue(1.0f);
    EXPECT_TRUE(gauge.IsFull());
}

TEST(UIGaugeComponentTest, IsNotFullWhenLessThanOne)
{
    UIGaugeComponent gauge;
    gauge.SetValue(0.99f);
    EXPECT_FALSE(gauge.IsFull());
}

TEST(UIGaugeComponentTest, DefaultIsFull)
{
    UIGaugeComponent gauge;
    EXPECT_TRUE(gauge.IsFull());
    EXPECT_FALSE(gauge.IsEmpty());
}

//============================================================================
// Size テスト
//============================================================================
TEST(UIGaugeComponentTest, SetSize)
{
    UIGaugeComponent gauge;
    gauge.SetSize(Vector2(200.0f, 20.0f));
    EXPECT_FLOAT_EQ(gauge.GetSize().x, 200.0f);
    EXPECT_FLOAT_EQ(gauge.GetSize().y, 20.0f);
}

//============================================================================
// Color テスト
//============================================================================
TEST(UIGaugeComponentTest, SetBackgroundColor)
{
    UIGaugeComponent gauge;
    gauge.SetBackgroundColor(Color(0.5f, 0.5f, 0.5f, 1.0f));
    const Color& bg = gauge.GetBackgroundColor();
    EXPECT_FLOAT_EQ(bg.R(), 0.5f);
    EXPECT_FLOAT_EQ(bg.G(), 0.5f);
    EXPECT_FLOAT_EQ(bg.B(), 0.5f);
    EXPECT_FLOAT_EQ(bg.A(), 1.0f);
}

TEST(UIGaugeComponentTest, SetFillColor)
{
    UIGaugeComponent gauge;
    gauge.SetFillColor(Color(1.0f, 0.0f, 0.0f, 1.0f));
    const Color& fill = gauge.GetFillColor();
    EXPECT_FLOAT_EQ(fill.R(), 1.0f);
    EXPECT_FLOAT_EQ(fill.G(), 0.0f);
    EXPECT_FLOAT_EQ(fill.B(), 0.0f);
}

TEST(UIGaugeComponentTest, SetColors)
{
    UIGaugeComponent gauge;
    gauge.SetColors(Color(0.1f, 0.1f, 0.1f, 0.5f), Color(0.0f, 0.0f, 1.0f, 1.0f));

    const Color& bg = gauge.GetBackgroundColor();
    EXPECT_FLOAT_EQ(bg.R(), 0.1f);
    EXPECT_FLOAT_EQ(bg.A(), 0.5f);

    const Color& fill = gauge.GetFillColor();
    EXPECT_FLOAT_EQ(fill.B(), 1.0f);
}

} // namespace
