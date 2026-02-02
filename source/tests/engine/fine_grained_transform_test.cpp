//----------------------------------------------------------------------------
//! @file   fine_grained_transform_test.cpp
//! @brief  LocalTransform / LocalToWorld / Parent Components テスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/components/transform/transform_components.h"

namespace
{

//============================================================================
// LocalTransform テスト
//============================================================================
class LocalTransformTest : public ::testing::Test {};

TEST_F(LocalTransformTest, DefaultConstruction)
{
    ECS::LocalTransform t;
    EXPECT_EQ(t.position, Vector3::Zero);
    EXPECT_EQ(t.rotation, Quaternion::Identity);
    EXPECT_EQ(t.scale, Vector3::One);
}

TEST_F(LocalTransformTest, SetPosition)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    EXPECT_EQ(t.position.x, 10.0f);
    EXPECT_EQ(t.position.y, 20.0f);
    EXPECT_EQ(t.position.z, 30.0f);
}

TEST_F(LocalTransformTest, SetRotation)
{
    ECS::LocalTransform t;
    Quaternion q = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, 1.5708f);
    t.rotation = q;
    EXPECT_FLOAT_EQ(t.rotation.x, q.x);
    EXPECT_FLOAT_EQ(t.rotation.y, q.y);
    EXPECT_FLOAT_EQ(t.rotation.z, q.z);
    EXPECT_FLOAT_EQ(t.rotation.w, q.w);
}

TEST_F(LocalTransformTest, SetScale)
{
    ECS::LocalTransform t;
    t.scale = Vector3(2.0f, 3.0f, 4.0f);
    EXPECT_EQ(t.scale, Vector3(2.0f, 3.0f, 4.0f));
}

TEST_F(LocalTransformTest, GetPosition2D)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    Vector2 pos2d = t.GetPosition2D();
    EXPECT_EQ(pos2d.x, 10.0f);
    EXPECT_EQ(pos2d.y, 20.0f);
}

TEST_F(LocalTransformTest, SetPosition2D)
{
    ECS::LocalTransform t;
    t.position.z = 100.0f;
    t.SetPosition2D(50.0f, 60.0f);
    EXPECT_EQ(t.position.x, 50.0f);
    EXPECT_EQ(t.position.y, 60.0f);
    EXPECT_EQ(t.position.z, 100.0f);  // Z preserved
}

TEST_F(LocalTransformTest, GetRotationZ)
{
    ECS::LocalTransform t;
    t.SetRotationZ(1.0f);
    EXPECT_NEAR(t.GetRotationZ(), 1.0f, 0.001f);
}

TEST_F(LocalTransformTest, SetRotationZ)
{
    ECS::LocalTransform t;
    t.SetRotationZ(2.0f);
    EXPECT_NEAR(t.GetRotationZ(), 2.0f, 0.001f);
}

TEST_F(LocalTransformTest, GetScale2D)
{
    ECS::LocalTransform t;
    t.scale = Vector3(2.0f, 3.0f, 4.0f);
    Vector2 scl2d = t.GetScale2D();
    EXPECT_EQ(scl2d.x, 2.0f);
    EXPECT_EQ(scl2d.y, 3.0f);
}

TEST_F(LocalTransformTest, SetScale2D)
{
    ECS::LocalTransform t;
    t.scale.z = 5.0f;
    t.SetScale2D(Vector2(2.0f, 3.0f));
    EXPECT_EQ(t.scale.x, 2.0f);
    EXPECT_EQ(t.scale.y, 3.0f);
    EXPECT_EQ(t.scale.z, 5.0f);  // Z preserved
}

TEST_F(LocalTransformTest, SetUniformScale)
{
    ECS::LocalTransform t;
    t.scale = Vector3(10.0f, 10.0f, 10.0f);
    EXPECT_EQ(t.scale, Vector3(10.0f, 10.0f, 10.0f));
}

TEST_F(LocalTransformTest, ToMatrix)
{
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    t.scale = Vector3(2.0f, 2.0f, 2.0f);

    Matrix mat = t.ToMatrix();
    Vector3 translation = mat.Translation();
    EXPECT_NEAR(translation.x, 10.0f, 0.001f);
    EXPECT_NEAR(translation.y, 20.0f, 0.001f);
    EXPECT_NEAR(translation.z, 30.0f, 0.001f);
}

TEST_F(LocalTransformTest, GetForward)
{
    ECS::LocalTransform t;
    Vector3 forward = t.GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 0.001f);
    EXPECT_NEAR(forward.y, 0.0f, 0.001f);
    EXPECT_NEAR(forward.z, 1.0f, 0.001f);
}

TEST_F(LocalTransformTest, GetRight)
{
    ECS::LocalTransform t;
    Vector3 right = t.GetRight();
    EXPECT_NEAR(right.x, 1.0f, 0.001f);
    EXPECT_NEAR(right.y, 0.0f, 0.001f);
    EXPECT_NEAR(right.z, 0.0f, 0.001f);
}

TEST_F(LocalTransformTest, GetUp)
{
    ECS::LocalTransform t;
    Vector3 up = t.GetUp();
    EXPECT_NEAR(up.x, 0.0f, 0.001f);
    EXPECT_NEAR(up.y, 1.0f, 0.001f);
    EXPECT_NEAR(up.z, 0.0f, 0.001f);
}

TEST_F(LocalTransformTest, SizeIs48Bytes)
{
    static_assert(sizeof(ECS::LocalTransform) == 48, "LocalTransform must be 48 bytes");
}

TEST_F(LocalTransformTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::LocalTransform>,
        "LocalTransform must be trivially copyable");
}

//============================================================================
// LocalToWorld テスト
//============================================================================
class LocalToWorldTest : public ::testing::Test {};

TEST_F(LocalToWorldTest, DefaultConstruction)
{
    ECS::LocalToWorld ltw;
    EXPECT_EQ(ltw.value, Matrix::Identity);
}

TEST_F(LocalToWorldTest, ConstructWithMatrix)
{
    Matrix mat = Matrix::CreateTranslation(10.0f, 20.0f, 30.0f);
    ECS::LocalToWorld ltw(mat);
    EXPECT_EQ(ltw.value, mat);
}

TEST_F(LocalToWorldTest, GetPosition)
{
    ECS::LocalToWorld ltw(Matrix::CreateTranslation(10.0f, 20.0f, 30.0f));
    Vector3 pos = ltw.GetPosition();
    EXPECT_NEAR(pos.x, 10.0f, 0.001f);
    EXPECT_NEAR(pos.y, 20.0f, 0.001f);
    EXPECT_NEAR(pos.z, 30.0f, 0.001f);
}

TEST_F(LocalToWorldTest, GetPosition2D)
{
    ECS::LocalToWorld ltw(Matrix::CreateTranslation(10.0f, 20.0f, 30.0f));
    Vector2 pos2d = ltw.GetPosition2D();
    EXPECT_NEAR(pos2d.x, 10.0f, 0.001f);
    EXPECT_NEAR(pos2d.y, 20.0f, 0.001f);
}

TEST_F(LocalToWorldTest, GetScale)
{
    Matrix mat = Matrix::CreateScale(2.0f, 3.0f, 4.0f);
    ECS::LocalToWorld ltw(mat);
    Vector3 scl = ltw.GetScale();
    EXPECT_NEAR(scl.x, 2.0f, 0.001f);
    EXPECT_NEAR(scl.y, 3.0f, 0.001f);
    EXPECT_NEAR(scl.z, 4.0f, 0.001f);
}

TEST_F(LocalToWorldTest, GetForward)
{
    ECS::LocalToWorld ltw;
    Vector3 forward = ltw.GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 0.001f);
    EXPECT_NEAR(forward.y, 0.0f, 0.001f);
    EXPECT_NEAR(forward.z, 1.0f, 0.001f);
}

TEST_F(LocalToWorldTest, GetRight)
{
    ECS::LocalToWorld ltw;
    Vector3 right = ltw.GetRight();
    EXPECT_NEAR(right.x, 1.0f, 0.001f);
    EXPECT_NEAR(right.y, 0.0f, 0.001f);
    EXPECT_NEAR(right.z, 0.0f, 0.001f);
}

TEST_F(LocalToWorldTest, GetUp)
{
    ECS::LocalToWorld ltw;
    Vector3 up = ltw.GetUp();
    EXPECT_NEAR(up.x, 0.0f, 0.001f);
    EXPECT_NEAR(up.y, 1.0f, 0.001f);
    EXPECT_NEAR(up.z, 0.0f, 0.001f);
}

TEST_F(LocalToWorldTest, ComputeLocalMatrix)
{
    // Compute local matrix using LocalTransform
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    t.rotation = Quaternion::Identity;
    t.scale = Vector3(2.0f, 2.0f, 2.0f);

    Matrix mat = t.ToMatrix();
    Vector3 translation = mat.Translation();
    EXPECT_NEAR(translation.x, 10.0f, 0.001f);
    EXPECT_NEAR(translation.y, 20.0f, 0.001f);
    EXPECT_NEAR(translation.z, 30.0f, 0.001f);
}

TEST_F(LocalToWorldTest, ComputeLocalMatrixNoScale)
{
    // Compute local matrix without scale using LocalTransform
    ECS::LocalTransform t;
    t.position = Vector3(10.0f, 20.0f, 30.0f);
    t.rotation = Quaternion::Identity;
    // scale defaults to 1,1,1

    Matrix mat = t.ToMatrix();
    Vector3 translation = mat.Translation();
    EXPECT_NEAR(translation.x, 10.0f, 0.001f);
    EXPECT_NEAR(translation.y, 20.0f, 0.001f);
    EXPECT_NEAR(translation.z, 30.0f, 0.001f);
}

TEST_F(LocalToWorldTest, SizeIs64Bytes)
{
    static_assert(sizeof(ECS::LocalToWorld) == 64, "LocalToWorld must be 64 bytes");
}

TEST_F(LocalToWorldTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::LocalToWorld>,
        "LocalToWorld must be trivially copyable");
}

//============================================================================
// Parent テスト
//============================================================================
class ParentTest : public ::testing::Test {};

TEST_F(ParentTest, DefaultConstruction)
{
    ECS::Parent p;
    EXPECT_FALSE(p.value.IsValid());
}

TEST_F(ParentTest, ConstructWithActor)
{
    ECS::Actor parent(123);
    ECS::Parent p(parent);
    EXPECT_TRUE(p.value.IsValid());
    EXPECT_EQ(p.value.id, 123u);
}

TEST_F(ParentTest, HasParent)
{
    ECS::Parent p1;
    EXPECT_FALSE(p1.HasParent());

    ECS::Parent p2(ECS::Actor(1));
    EXPECT_TRUE(p2.HasParent());
}

TEST_F(ParentTest, SizeIs4Bytes)
{
    static_assert(sizeof(ECS::Parent) == 4, "Parent must be 4 bytes");
}

TEST_F(ParentTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::Parent>,
        "Parent must be trivially copyable");
}

//============================================================================
// PreviousParent テスト
//============================================================================
class PreviousParentTest : public ::testing::Test {};

TEST_F(PreviousParentTest, DefaultConstruction)
{
    ECS::PreviousParent pp;
    EXPECT_FALSE(pp.value.IsValid());
}

TEST_F(PreviousParentTest, ConstructWithActor)
{
    ECS::Actor parent(456);
    ECS::PreviousParent pp(parent);
    EXPECT_TRUE(pp.value.IsValid());
    EXPECT_EQ(pp.value.id, 456u);
}

TEST_F(PreviousParentTest, SizeIs4Bytes)
{
    static_assert(sizeof(ECS::PreviousParent) == 4, "PreviousParent must be 4 bytes");
}

TEST_F(PreviousParentTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::PreviousParent>,
        "PreviousParent must be trivially copyable");
}

//============================================================================
// PostTransformMatrix テスト
//============================================================================
class PostTransformMatrixTest : public ::testing::Test {};

TEST_F(PostTransformMatrixTest, DefaultConstruction)
{
    ECS::PostTransformMatrix ptm;
    EXPECT_EQ(ptm.value, Matrix::Identity);
}

TEST_F(PostTransformMatrixTest, ConstructWithMatrix)
{
    Matrix mat = Matrix::CreateScale(2.0f, 1.0f, 1.0f);
    ECS::PostTransformMatrix ptm(mat);
    EXPECT_EQ(ptm.value, mat);
}

TEST_F(PostTransformMatrixTest, SizeIs64Bytes)
{
    static_assert(sizeof(ECS::PostTransformMatrix) == 64, "PostTransformMatrix must be 64 bytes");
}

TEST_F(PostTransformMatrixTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::PostTransformMatrix>,
        "PostTransformMatrix must be trivially copyable");
}

//============================================================================
// HierarchyDepthData テスト
//============================================================================
class HierarchyDepthDataTest : public ::testing::Test {};

TEST_F(HierarchyDepthDataTest, DefaultConstruction)
{
    ECS::HierarchyDepthData hd;
    EXPECT_EQ(hd.depth, 0);
}

TEST_F(HierarchyDepthDataTest, ConstructWithDepth)
{
    ECS::HierarchyDepthData hd(5);
    EXPECT_EQ(hd.depth, 5);
}

TEST_F(HierarchyDepthDataTest, IsRoot)
{
    ECS::HierarchyDepthData hd1(0);
    EXPECT_TRUE(hd1.IsRoot());

    ECS::HierarchyDepthData hd2(1);
    EXPECT_FALSE(hd2.IsRoot());
}

TEST_F(HierarchyDepthDataTest, SetDepth)
{
    ECS::HierarchyDepthData hd;
    hd.SetDepth(10);
    EXPECT_EQ(hd.depth, 10);
}

TEST_F(HierarchyDepthDataTest, IncrementDepth)
{
    ECS::HierarchyDepthData hd(5);
    hd.IncrementDepth();
    EXPECT_EQ(hd.depth, 6);
}

TEST_F(HierarchyDepthDataTest, SizeIs2Bytes)
{
    static_assert(sizeof(ECS::HierarchyDepthData) == 2, "HierarchyDepthData must be 2 bytes");
}

TEST_F(HierarchyDepthDataTest, IsTriviallyCopyable)
{
    static_assert(std::is_trivially_copyable_v<ECS::HierarchyDepthData>,
        "HierarchyDepthData must be trivially copyable");
}

//============================================================================
// Tag Components テスト
//============================================================================
class TransformTagsTest : public ::testing::Test {};

TEST_F(TransformTagsTest, TransformDirtyIsTagComponent)
{
    static_assert(ECS::is_tag_component_v<ECS::TransformDirty>,
        "TransformDirty must be a tag component");
    static_assert(sizeof(ECS::TransformDirty) == 1,
        "TransformDirty should be 1 byte (empty struct)");
}

TEST_F(TransformTagsTest, StaticTransformIsTagComponent)
{
    static_assert(ECS::is_tag_component_v<ECS::StaticTransform>,
        "StaticTransform must be a tag component");
}

TEST_F(TransformTagsTest, HierarchyRootIsTagComponent)
{
    static_assert(ECS::is_tag_component_v<ECS::HierarchyRoot>,
        "HierarchyRoot must be a tag component");
}

TEST_F(TransformTagsTest, TransformInitializedIsTagComponent)
{
    static_assert(ECS::is_tag_component_v<ECS::TransformInitialized>,
        "TransformInitialized must be a tag component");
}

} // namespace
