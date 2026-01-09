//----------------------------------------------------------------------------
//! @file   animator_test.cpp
//! @brief  Animatorコンポーネントのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/animator.h"

namespace
{

//============================================================================
// Animator 基本テスト
//============================================================================
class AnimatorTest : public ::testing::Test
{
protected:
    Animator animator_{4, 8, 6};  // 4行8列、フレーム間隔6
};

TEST_F(AnimatorTest, ConstructorSetsRowCount)
{
    EXPECT_EQ(animator_.GetRowCount(), 4);
}

TEST_F(AnimatorTest, ConstructorSetsColumnCount)
{
    EXPECT_EQ(animator_.GetColumnCount(), 8);
}

TEST_F(AnimatorTest, ConstructorSetsFrameInterval)
{
    EXPECT_EQ(animator_.GetFrameInterval(), 6);
}

TEST_F(AnimatorTest, InitialRowIsZero)
{
    EXPECT_EQ(animator_.GetRow(), 0);
}

TEST_F(AnimatorTest, InitialColumnIsZero)
{
    EXPECT_EQ(animator_.GetColumn(), 0);
}

TEST_F(AnimatorTest, InitiallyPlaying)
{
    EXPECT_TRUE(animator_.IsPlaying());
}

TEST_F(AnimatorTest, InitiallyLooping)
{
    EXPECT_TRUE(animator_.IsLooping());
}

TEST_F(AnimatorTest, InitiallyNotMirrored)
{
    EXPECT_FALSE(animator_.GetMirror());
}

//============================================================================
// 再生制御テスト
//============================================================================
TEST_F(AnimatorTest, SetPlayingFalse)
{
    animator_.SetPlaying(false);
    EXPECT_FALSE(animator_.IsPlaying());
}

TEST_F(AnimatorTest, SetLoopingFalse)
{
    animator_.SetLooping(false);
    EXPECT_FALSE(animator_.IsLooping());
}

TEST_F(AnimatorTest, SetMirrorTrue)
{
    animator_.SetMirror(true);
    EXPECT_TRUE(animator_.GetMirror());
}

//============================================================================
// 行/列操作テスト
//============================================================================
TEST_F(AnimatorTest, SetRow)
{
    animator_.SetRow(2);
    EXPECT_EQ(animator_.GetRow(), 2);
    EXPECT_EQ(animator_.GetColumn(), 0);  // 列はリセット
}

TEST_F(AnimatorTest, SetColumn)
{
    animator_.SetColumn(5);
    EXPECT_EQ(animator_.GetColumn(), 5);
}

TEST_F(AnimatorTest, SetRowClamps)
{
    animator_.SetRow(100);  // 行数以上
    EXPECT_LT(animator_.GetRow(), animator_.GetRowCount());
}

TEST_F(AnimatorTest, SetColumnClamps)
{
    animator_.SetColumn(100);  // 列数以上
    EXPECT_LT(animator_.GetColumn(), animator_.GetColumnCount());
}

//============================================================================
// 行ごとのフレーム数テスト
//============================================================================
TEST_F(AnimatorTest, SetRowFrameCount)
{
    animator_.SetRowFrameCount(0, 4);
    EXPECT_EQ(animator_.GetRowFrameCount(0), 4);
}

TEST_F(AnimatorTest, DefaultRowFrameCountUsesAllColumns)
{
    // 未設定の場合はcolCount_を返す（全列使用）
    EXPECT_EQ(animator_.GetRowFrameCount(0), animator_.GetColumnCount());
}

TEST_F(AnimatorTest, SetRowFrameInterval)
{
    animator_.SetRowFrameInterval(1, 10);
    EXPECT_EQ(animator_.GetRowFrameInterval(1), 10);
}

TEST_F(AnimatorTest, SetRowFrameCountWithInterval)
{
    animator_.SetRowFrameCount(2, 5, 12);
    EXPECT_EQ(animator_.GetRowFrameCount(2), 5);
    EXPECT_EQ(animator_.GetRowFrameInterval(2), 12);
}

//============================================================================
// フレーム間隔テスト
//============================================================================
TEST_F(AnimatorTest, SetFrameInterval)
{
    animator_.SetFrameInterval(10);
    EXPECT_EQ(animator_.GetFrameInterval(), 10);
}

TEST_F(AnimatorTest, SetFrameIntervalMinimumIsOne)
{
    animator_.SetFrameInterval(0);
    EXPECT_EQ(animator_.GetFrameInterval(), 1);  // 0は1に正規化
}

TEST_F(AnimatorTest, SetFrameDuration)
{
    animator_.SetFrameDuration(1.0f);  // 1秒 = 60フレーム
    EXPECT_EQ(animator_.GetFrameInterval(), 60);
}

//============================================================================
// リセットテスト
//============================================================================
TEST_F(AnimatorTest, ResetSetsColumnToZero)
{
    animator_.SetColumn(5);
    animator_.Reset();
    EXPECT_EQ(animator_.GetColumn(), 0);
}

//============================================================================
// UV座標テスト
//============================================================================
TEST_F(AnimatorTest, GetUVSize)
{
    Animator anim(2, 4);  // 2行4列
    Vector2 uvSize = anim.GetUVSize();
    EXPECT_FLOAT_EQ(uvSize.x, 0.25f);  // 1/4
    EXPECT_FLOAT_EQ(uvSize.y, 0.5f);   // 1/2
}

TEST_F(AnimatorTest, GetUVSizeMirrored)
{
    Animator anim(2, 4);
    anim.SetMirror(true);
    Vector2 uvSize = anim.GetUVSize();
    EXPECT_FLOAT_EQ(uvSize.x, -0.25f);  // 反転で負
    EXPECT_FLOAT_EQ(uvSize.y, 0.5f);
}

TEST_F(AnimatorTest, GetUVCoordAtOrigin)
{
    Animator anim(2, 4);  // 2行4列
    anim.SetRow(0);
    anim.SetColumn(0);
    Vector2 uvCoord = anim.GetUVCoord();
    EXPECT_FLOAT_EQ(uvCoord.x, 0.0f);
    EXPECT_FLOAT_EQ(uvCoord.y, 0.0f);
}

TEST_F(AnimatorTest, GetUVCoordAtSecondFrame)
{
    Animator anim(2, 4);  // 2行4列
    anim.SetRow(0);
    anim.SetColumn(1);
    Vector2 uvCoord = anim.GetUVCoord();
    EXPECT_FLOAT_EQ(uvCoord.x, 0.25f);
    EXPECT_FLOAT_EQ(uvCoord.y, 0.0f);
}

TEST_F(AnimatorTest, GetUVCoordAtSecondRow)
{
    Animator anim(2, 4);  // 2行4列
    anim.SetRow(1);
    anim.SetColumn(0);
    Vector2 uvCoord = anim.GetUVCoord();
    EXPECT_FLOAT_EQ(uvCoord.x, 0.0f);
    EXPECT_FLOAT_EQ(uvCoord.y, 0.5f);
}

//============================================================================
// ソース矩形テスト
//============================================================================
TEST_F(AnimatorTest, GetSourceRectAtOrigin)
{
    Animator anim(2, 4);  // 2行4列
    anim.SetRow(0);
    anim.SetColumn(0);
    Vector4 rect = anim.GetSourceRect(256.0f, 128.0f);
    EXPECT_FLOAT_EQ(rect.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.y, 0.0f);
    EXPECT_FLOAT_EQ(rect.z, 64.0f);   // 256/4
    EXPECT_FLOAT_EQ(rect.w, 64.0f);   // 128/2
}

//============================================================================
// デフォルトコンストラクタテスト
//============================================================================
TEST(AnimatorDefaultTest, DefaultValues)
{
    Animator anim;
    EXPECT_EQ(anim.GetRowCount(), 1);
    EXPECT_EQ(anim.GetColumnCount(), 1);
    EXPECT_EQ(anim.GetFrameInterval(), 1);
}

//============================================================================
// 定数テスト
//============================================================================
TEST(AnimatorConstantsTest, MaxRows)
{
    EXPECT_EQ(Animator::kMaxRows, 16);
}

TEST(AnimatorConstantsTest, AssumedFrameRate)
{
    EXPECT_FLOAT_EQ(Animator::kAssumedFrameRate, 60.0f);
}

} // namespace
