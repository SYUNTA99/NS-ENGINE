//----------------------------------------------------------------------------
//! @file   collision3d_test.cpp
//! @brief  3D衝突判定関連型のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/c_systems/collision_manager3d.h"

namespace
{

//============================================================================
// CollisionConstants3D テスト
//============================================================================
TEST(CollisionConstants3DTest, InvalidIndex)
{
    EXPECT_EQ(CollisionConstants3D::kInvalidIndex, UINT16_MAX);
}

TEST(CollisionConstants3DTest, DefaultLayer)
{
    EXPECT_EQ(CollisionConstants3D::kDefaultLayer, 0x01);
}

TEST(CollisionConstants3DTest, DefaultMask)
{
    EXPECT_EQ(CollisionConstants3D::kDefaultMask, 0xFF);
}

TEST(CollisionConstants3DTest, DefaultCellSize)
{
    EXPECT_EQ(CollisionConstants3D::kDefaultCellSize, 100);
}

//============================================================================
// ColliderShape3D enum テスト
//============================================================================
TEST(ColliderShape3DTest, AABBIsDefined)
{
    ColliderShape3D shape = ColliderShape3D::AABB;
    EXPECT_EQ(shape, ColliderShape3D::AABB);
}

TEST(ColliderShape3DTest, SphereIsDefined)
{
    ColliderShape3D shape = ColliderShape3D::Sphere;
    EXPECT_EQ(shape, ColliderShape3D::Sphere);
}

TEST(ColliderShape3DTest, CapsuleIsDefined)
{
    ColliderShape3D shape = ColliderShape3D::Capsule;
    EXPECT_EQ(shape, ColliderShape3D::Capsule);
}

TEST(ColliderShape3DTest, AllShapesAreDistinct)
{
    EXPECT_NE(ColliderShape3D::AABB, ColliderShape3D::Sphere);
    EXPECT_NE(ColliderShape3D::Sphere, ColliderShape3D::Capsule);
    EXPECT_NE(ColliderShape3D::Capsule, ColliderShape3D::AABB);
}

//============================================================================
// Collider3DHandle テスト
//============================================================================
TEST(Collider3DHandleTest, DefaultIsInvalid)
{
    Collider3DHandle handle;
    EXPECT_FALSE(handle.IsValid());
}

TEST(Collider3DHandleTest, DefaultIndexIsInvalidIndex)
{
    Collider3DHandle handle;
    EXPECT_EQ(handle.index, CollisionConstants3D::kInvalidIndex);
}

TEST(Collider3DHandleTest, DefaultGenerationIsZero)
{
    Collider3DHandle handle;
    EXPECT_EQ(handle.generation, 0u);
}

TEST(Collider3DHandleTest, ValidHandleWithZeroIndex)
{
    Collider3DHandle handle;
    handle.index = 0;
    handle.generation = 1;
    EXPECT_TRUE(handle.IsValid());
}

TEST(Collider3DHandleTest, EqualityOperator)
{
    Collider3DHandle a, b;
    a.index = 5;
    a.generation = 3;
    b.index = 5;
    b.generation = 3;
    EXPECT_TRUE(a == b);
}

TEST(Collider3DHandleTest, InequalityByIndex)
{
    Collider3DHandle a, b;
    a.index = 5;
    a.generation = 3;
    b.index = 6;
    b.generation = 3;
    EXPECT_FALSE(a == b);
}

TEST(Collider3DHandleTest, InequalityByGeneration)
{
    Collider3DHandle a, b;
    a.index = 5;
    a.generation = 3;
    b.index = 5;
    b.generation = 4;
    EXPECT_FALSE(a == b);
}

//============================================================================
// AABB3D テスト
//============================================================================
TEST(AABB3DTest, DefaultAllZeros)
{
    AABB3D aabb;
    EXPECT_FLOAT_EQ(aabb.minX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.minY, 0.0f);
    EXPECT_FLOAT_EQ(aabb.minZ, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxZ, 0.0f);
}

TEST(AABB3DTest, ConstructorFromPositionAndSize)
{
    AABB3D aabb(10.0f, 20.0f, 30.0f, 5.0f, 10.0f, 15.0f);
    EXPECT_FLOAT_EQ(aabb.minX, 10.0f);
    EXPECT_FLOAT_EQ(aabb.minY, 20.0f);
    EXPECT_FLOAT_EQ(aabb.minZ, 30.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 15.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, 30.0f);
    EXPECT_FLOAT_EQ(aabb.maxZ, 45.0f);
}

TEST(AABB3DTest, GetCenter)
{
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 20.0f, 30.0f);
    Vector3 center = aabb.GetCenter();
    EXPECT_FLOAT_EQ(center.x, 5.0f);
    EXPECT_FLOAT_EQ(center.y, 10.0f);
    EXPECT_FLOAT_EQ(center.z, 15.0f);
}

TEST(AABB3DTest, GetCenterSymmetric)
{
    AABB3D aabb;
    aabb.minX = -5.0f;
    aabb.minY = -10.0f;
    aabb.minZ = -15.0f;
    aabb.maxX = 5.0f;
    aabb.maxY = 10.0f;
    aabb.maxZ = 15.0f;
    Vector3 center = aabb.GetCenter();
    EXPECT_FLOAT_EQ(center.x, 0.0f);
    EXPECT_FLOAT_EQ(center.y, 0.0f);
    EXPECT_FLOAT_EQ(center.z, 0.0f);
}

TEST(AABB3DTest, GetSize)
{
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 20.0f, 30.0f);
    Vector3 size = aabb.GetSize();
    EXPECT_FLOAT_EQ(size.x, 10.0f);
    EXPECT_FLOAT_EQ(size.y, 20.0f);
    EXPECT_FLOAT_EQ(size.z, 30.0f);
}

TEST(AABB3DTest, IntersectsOverlapping)
{
    AABB3D a(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    AABB3D b(5.0f, 5.0f, 5.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(a.Intersects(b));
    EXPECT_TRUE(b.Intersects(a));
}

TEST(AABB3DTest, IntersectsSeparatedX)
{
    AABB3D a(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    AABB3D b(20.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABB3DTest, IntersectsSeparatedY)
{
    AABB3D a(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    AABB3D b(0.0f, 20.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABB3DTest, IntersectsSeparatedZ)
{
    AABB3D a(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    AABB3D b(0.0f, 0.0f, 20.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABB3DTest, IntersectsTouching)
{
    AABB3D a(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    AABB3D b(10.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    // Touching but not overlapping (min < max check means no overlap)
    EXPECT_FALSE(a.Intersects(b));
}

TEST(AABB3DTest, IntersectsContained)
{
    AABB3D outer(0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f);
    AABB3D inner(5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f);
    EXPECT_TRUE(outer.Intersects(inner));
    EXPECT_TRUE(inner.Intersects(outer));
}

TEST(AABB3DTest, IntersectsSelf)
{
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(aabb.Intersects(aabb));
}

//============================================================================
// BoundingSphere3D テスト
//============================================================================
TEST(BoundingSphere3DTest, DefaultCenter)
{
    BoundingSphere3D sphere;
    EXPECT_FLOAT_EQ(sphere.center.x, 0.0f);
    EXPECT_FLOAT_EQ(sphere.center.y, 0.0f);
    EXPECT_FLOAT_EQ(sphere.center.z, 0.0f);
}

TEST(BoundingSphere3DTest, DefaultRadius)
{
    BoundingSphere3D sphere;
    EXPECT_FLOAT_EQ(sphere.radius, 0.5f);
}

TEST(BoundingSphere3DTest, Constructor)
{
    BoundingSphere3D sphere(Vector3(1.0f, 2.0f, 3.0f), 5.0f);
    EXPECT_FLOAT_EQ(sphere.center.x, 1.0f);
    EXPECT_FLOAT_EQ(sphere.center.y, 2.0f);
    EXPECT_FLOAT_EQ(sphere.center.z, 3.0f);
    EXPECT_FLOAT_EQ(sphere.radius, 5.0f);
}

TEST(BoundingSphere3DTest, IntersectsSpheresOverlapping)
{
    BoundingSphere3D a(Vector3(0.0f, 0.0f, 0.0f), 5.0f);
    BoundingSphere3D b(Vector3(8.0f, 0.0f, 0.0f), 5.0f);
    EXPECT_TRUE(a.Intersects(b));
    EXPECT_TRUE(b.Intersects(a));
}

TEST(BoundingSphere3DTest, IntersectsSpheresSeparated)
{
    BoundingSphere3D a(Vector3(0.0f, 0.0f, 0.0f), 5.0f);
    BoundingSphere3D b(Vector3(20.0f, 0.0f, 0.0f), 5.0f);
    EXPECT_FALSE(a.Intersects(b));
}

TEST(BoundingSphere3DTest, IntersectsSpheresSelf)
{
    BoundingSphere3D sphere(Vector3(1.0f, 2.0f, 3.0f), 5.0f);
    EXPECT_TRUE(sphere.Intersects(sphere));
}

TEST(BoundingSphere3DTest, IntersectsSpheresContained)
{
    BoundingSphere3D outer(Vector3(0.0f, 0.0f, 0.0f), 10.0f);
    BoundingSphere3D inner(Vector3(0.0f, 0.0f, 0.0f), 2.0f);
    EXPECT_TRUE(outer.Intersects(inner));
    EXPECT_TRUE(inner.Intersects(outer));
}

TEST(BoundingSphere3DTest, IntersectsAABBInside)
{
    BoundingSphere3D sphere(Vector3(5.0f, 5.0f, 5.0f), 2.0f);
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(sphere.Intersects(aabb));
}

TEST(BoundingSphere3DTest, IntersectsAABBOutside)
{
    BoundingSphere3D sphere(Vector3(20.0f, 20.0f, 20.0f), 2.0f);
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_FALSE(sphere.Intersects(aabb));
}

TEST(BoundingSphere3DTest, IntersectsAABBOnEdge)
{
    BoundingSphere3D sphere(Vector3(12.0f, 5.0f, 5.0f), 3.0f);  // Center 2 units from AABB edge
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(sphere.Intersects(aabb));
}

TEST(BoundingSphere3DTest, IntersectsAABBCorner)
{
    // Sphere centered at corner of AABB
    BoundingSphere3D sphere(Vector3(0.0f, 0.0f, 0.0f), 5.0f);
    AABB3D aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    EXPECT_TRUE(sphere.Intersects(aabb));
}

//============================================================================
// RaycastHit3D テスト
//============================================================================
TEST(RaycastHit3DTest, DefaultColliderIsNull)
{
    RaycastHit3D hit;
    EXPECT_EQ(hit.collider, nullptr);
}

TEST(RaycastHit3DTest, DefaultDistanceIsZero)
{
    RaycastHit3D hit;
    EXPECT_FLOAT_EQ(hit.distance, 0.0f);
}

TEST(RaycastHit3DTest, CanSetValues)
{
    RaycastHit3D hit;
    hit.distance = 10.5f;
    hit.point = Vector3(1.0f, 2.0f, 3.0f);
    hit.normal = Vector3(0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(hit.distance, 10.5f);
    EXPECT_FLOAT_EQ(hit.point.x, 1.0f);
    EXPECT_FLOAT_EQ(hit.point.y, 2.0f);
    EXPECT_FLOAT_EQ(hit.point.z, 3.0f);
    EXPECT_FLOAT_EQ(hit.normal.x, 0.0f);
    EXPECT_FLOAT_EQ(hit.normal.y, 1.0f);
    EXPECT_FLOAT_EQ(hit.normal.z, 0.0f);
}

} // namespace
