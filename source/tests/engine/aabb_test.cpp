//----------------------------------------------------------------------------
//! @file   aabb_test.cpp
//! @brief  AABB (軸平行境界ボックス) のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/c_systems/collision_manager.h"

namespace
{

//============================================================================
// AABB テスト
//============================================================================
TEST(AABBTest, DefaultConstruction)
{
    AABB aabb;
    EXPECT_FLOAT_EQ(aabb.minX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.minY, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, 0.0f);
}

TEST(AABBTest, ConstructFromPositionAndSize)
{
    AABB aabb(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_FLOAT_EQ(aabb.minX, 10.0f);
    EXPECT_FLOAT_EQ(aabb.minY, 20.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 110.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, 70.0f);
}

TEST(AABBTest, GetCenter)
{
    AABB aabb(0.0f, 0.0f, 100.0f, 100.0f);
    Vector2 center = aabb.GetCenter();
    EXPECT_FLOAT_EQ(center.x, 50.0f);
    EXPECT_FLOAT_EQ(center.y, 50.0f);
}

TEST(AABBTest, GetSize)
{
    AABB aabb(10.0f, 20.0f, 30.0f, 40.0f);
    Vector2 size = aabb.GetSize();
    EXPECT_FLOAT_EQ(size.x, 30.0f);
    EXPECT_FLOAT_EQ(size.y, 40.0f);
}

//============================================================================
// AABB Intersects テスト
//============================================================================
TEST(AABBTest, IntersectsOverlapping)
{
    AABB a(0.0f, 0.0f, 100.0f, 100.0f);
    AABB b(50.0f, 50.0f, 100.0f, 100.0f);
    EXPECT_TRUE(a.Intersects(b));
    EXPECT_TRUE(b.Intersects(a));
}

TEST(AABBTest, IntersectsTouching)
{
    // 触れている（境界が一致）場合は交差しない
    AABB a(0.0f, 0.0f, 100.0f, 100.0f);
    AABB b(100.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABBTest, IntersectsSeparated)
{
    AABB a(0.0f, 0.0f, 50.0f, 50.0f);
    AABB b(100.0f, 100.0f, 50.0f, 50.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABBTest, IntersectsContained)
{
    AABB outer(0.0f, 0.0f, 100.0f, 100.0f);
    AABB inner(25.0f, 25.0f, 50.0f, 50.0f);
    EXPECT_TRUE(outer.Intersects(inner));
    EXPECT_TRUE(inner.Intersects(outer));
}

TEST(AABBTest, IntersectsSelf)
{
    AABB a(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_TRUE(a.Intersects(a));
}

//============================================================================
// AABB Contains テスト
//============================================================================
TEST(AABBTest, ContainsPointInside)
{
    AABB aabb(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_TRUE(aabb.Contains(50.0f, 50.0f));
}

TEST(AABBTest, ContainsPointAtMinBoundary)
{
    AABB aabb(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_TRUE(aabb.Contains(0.0f, 0.0f));
}

TEST(AABBTest, ContainsPointAtMaxBoundary)
{
    AABB aabb(0.0f, 0.0f, 100.0f, 100.0f);
    // maxは含まない（<ではなく<）
    EXPECT_FALSE(aabb.Contains(100.0f, 100.0f));
}

TEST(AABBTest, ContainsPointOutside)
{
    AABB aabb(0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_FALSE(aabb.Contains(150.0f, 50.0f));
    EXPECT_FALSE(aabb.Contains(50.0f, 150.0f));
    EXPECT_FALSE(aabb.Contains(-10.0f, 50.0f));
}

//============================================================================
// ColliderHandle テスト
//============================================================================
TEST(ColliderHandleTest, DefaultIsInvalid)
{
    ColliderHandle handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.index, CollisionConstants::kInvalidIndex);
}

TEST(ColliderHandleTest, ValidHandle)
{
    ColliderHandle handle;
    handle.index = 0;
    handle.generation = 1;
    EXPECT_TRUE(handle.IsValid());
}

TEST(ColliderHandleTest, Equality)
{
    ColliderHandle a{ 5, 10 };
    ColliderHandle b{ 5, 10 };
    ColliderHandle c{ 5, 11 };
    ColliderHandle d{ 6, 10 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
}

//============================================================================
// CollisionConstants テスト
//============================================================================
TEST(CollisionConstantsTest, DefaultValues)
{
    EXPECT_EQ(CollisionConstants::kInvalidIndex, UINT16_MAX);
    EXPECT_EQ(CollisionConstants::kDefaultLayer, 0x01);
    EXPECT_EQ(CollisionConstants::kDefaultMask, 0xFF);
    EXPECT_EQ(CollisionConstants::kDefaultCellSize, 256);
}

} // namespace
