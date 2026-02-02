//----------------------------------------------------------------------------
//! @file   transform_test.cpp
//! @brief  Transformコンポーネントのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/transform.h"
#include <cmath>

namespace
{

//============================================================================
// Transform 基本テスト
//============================================================================
class TransformTest : public ::testing::Test
{
protected:
    Transform transform_;
};

TEST_F(TransformTest, DefaultPosition)
{
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 0.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().z, 0.0f);
}

TEST_F(TransformTest, DefaultRotation)
{
    Quaternion q = transform_.GetRotation();
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST_F(TransformTest, DefaultScale)
{
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 1.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 1.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().z, 1.0f);
}

TEST_F(TransformTest, SetPositionVector3)
{
    transform_.SetPosition(Vector3(10.0f, 20.0f, 30.0f));
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 20.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().z, 30.0f);
}

TEST_F(TransformTest, SetPositionXYZ)
{
    transform_.SetPosition(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().z, 3.0f);
}

TEST_F(TransformTest, TranslateVector3)
{
    transform_.SetPosition(10.0f, 20.0f, 30.0f);
    transform_.Translate(Vector3(5.0f, -5.0f, 10.0f));
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 15.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 15.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().z, 40.0f);
}

TEST_F(TransformTest, TranslateXYZ)
{
    transform_.SetPosition(10.0f, 20.0f, 30.0f);
    transform_.Translate(3.0f, 7.0f, -10.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 13.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 27.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().z, 20.0f);
}

//============================================================================
// Rotation テスト
//============================================================================
TEST_F(TransformTest, SetRotationQuaternion)
{
    Quaternion q = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, DirectX::XM_PI / 4.0f);
    transform_.SetRotation(q);
    Quaternion result = transform_.GetRotation();
    EXPECT_NEAR(result.x, q.x, 0.001f);
    EXPECT_NEAR(result.y, q.y, 0.001f);
    EXPECT_NEAR(result.z, q.z, 0.001f);
    EXPECT_NEAR(result.w, q.w, 0.001f);
}

TEST_F(TransformTest, RotateAroundAxis)
{
    transform_.SetRotation(Quaternion::Identity);
    transform_.Rotate(Vector3::UnitY, DirectX::XM_PI / 2.0f);

    Quaternion expected = Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XM_PI / 2.0f);
    Quaternion result = transform_.GetRotation();
    EXPECT_NEAR(result.x, expected.x, 0.001f);
    EXPECT_NEAR(result.y, expected.y, 0.001f);
    EXPECT_NEAR(result.z, expected.z, 0.001f);
    EXPECT_NEAR(result.w, expected.w, 0.001f);
}

//============================================================================
// Scale テスト
//============================================================================
TEST_F(TransformTest, SetScaleVector3)
{
    transform_.SetScale(Vector3(2.0f, 3.0f, 4.0f));
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 2.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 3.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().z, 4.0f);
}

TEST_F(TransformTest, SetScaleUniform)
{
    transform_.SetScale(5.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 5.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 5.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().z, 5.0f);
}

//============================================================================
// 方向ベクトルテスト
//============================================================================
TEST_F(TransformTest, GetForwardDefault)
{
    Vector3 forward = transform_.GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 0.001f);
    EXPECT_NEAR(forward.y, 0.0f, 0.001f);
    EXPECT_NEAR(forward.z, 1.0f, 0.001f);  // LH forward is +Z
}

TEST_F(TransformTest, GetRightDefault)
{
    Vector3 right = transform_.GetRight();
    EXPECT_NEAR(right.x, 1.0f, 0.001f);
    EXPECT_NEAR(right.y, 0.0f, 0.001f);
    EXPECT_NEAR(right.z, 0.0f, 0.001f);
}

TEST_F(TransformTest, GetUpDefault)
{
    Vector3 up = transform_.GetUp();
    EXPECT_NEAR(up.x, 0.0f, 0.001f);
    EXPECT_NEAR(up.y, 1.0f, 0.001f);
    EXPECT_NEAR(up.z, 0.0f, 0.001f);
}

//============================================================================
// 親子階層テスト
//============================================================================
TEST_F(TransformTest, InitiallyNoParent)
{
    EXPECT_EQ(transform_.GetParent(), nullptr);
    EXPECT_EQ(transform_.GetChildCount(), 0);
}

TEST_F(TransformTest, SetParent)
{
    Transform parent;
    transform_.SetParent(&parent);

    EXPECT_EQ(transform_.GetParent(), &parent);
    EXPECT_EQ(parent.GetChildCount(), 1);
}

TEST_F(TransformTest, SetParentNull)
{
    Transform parent;
    transform_.SetParent(&parent);
    transform_.SetParent(nullptr);

    EXPECT_EQ(transform_.GetParent(), nullptr);
    EXPECT_EQ(parent.GetChildCount(), 0);
}

TEST_F(TransformTest, AddChild)
{
    Transform child;
    transform_.AddChild(&child);

    EXPECT_EQ(child.GetParent(), &transform_);
    EXPECT_EQ(transform_.GetChildCount(), 1);
}

TEST_F(TransformTest, RemoveChild)
{
    Transform child;
    transform_.AddChild(&child);
    transform_.RemoveChild(&child);

    EXPECT_EQ(child.GetParent(), nullptr);
    EXPECT_EQ(transform_.GetChildCount(), 0);
}

TEST_F(TransformTest, DetachFromParent)
{
    Transform parent;
    transform_.SetParent(&parent);
    transform_.DetachFromParent();

    EXPECT_EQ(transform_.GetParent(), nullptr);
    EXPECT_EQ(parent.GetChildCount(), 0);
}

TEST_F(TransformTest, DetachAllChildren)
{
    Transform child1, child2, child3;
    transform_.AddChild(&child1);
    transform_.AddChild(&child2);
    transform_.AddChild(&child3);

    transform_.DetachAllChildren();

    EXPECT_EQ(transform_.GetChildCount(), 0);
    EXPECT_EQ(child1.GetParent(), nullptr);
    EXPECT_EQ(child2.GetParent(), nullptr);
    EXPECT_EQ(child3.GetParent(), nullptr);
}

TEST_F(TransformTest, PreventCyclicReference)
{
    Transform child;
    transform_.AddChild(&child);

    // 親を子に設定しようとしても無視される
    child.AddChild(&transform_);

    EXPECT_EQ(transform_.GetParent(), nullptr);
    EXPECT_EQ(child.GetParent(), &transform_);
}

//============================================================================
// ワールド座標テスト
//============================================================================
TEST_F(TransformTest, WorldPositionWithoutParent)
{
    transform_.SetPosition(100.0f, 200.0f, 50.0f);
    Vector3 worldPos = transform_.GetWorldPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 100.0f);
    EXPECT_FLOAT_EQ(worldPos.y, 200.0f);
    EXPECT_FLOAT_EQ(worldPos.z, 50.0f);
}

TEST_F(TransformTest, WorldPositionWithParent)
{
    Transform parent;
    parent.SetPosition(100.0f, 100.0f, 0.0f);

    transform_.SetParent(&parent);
    transform_.SetPosition(50.0f, 50.0f, 10.0f);

    Vector3 worldPos = transform_.GetWorldPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 150.0f);
    EXPECT_FLOAT_EQ(worldPos.y, 150.0f);
    EXPECT_FLOAT_EQ(worldPos.z, 10.0f);
}

TEST_F(TransformTest, WorldRotationWithoutParent)
{
    Quaternion q = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, DirectX::XM_PI / 4.0f);
    transform_.SetRotation(q);
    Quaternion worldRot = transform_.GetWorldRotation();
    EXPECT_NEAR(worldRot.x, q.x, 0.001f);
    EXPECT_NEAR(worldRot.y, q.y, 0.001f);
    EXPECT_NEAR(worldRot.z, q.z, 0.001f);
    EXPECT_NEAR(worldRot.w, q.w, 0.001f);
}

TEST_F(TransformTest, WorldScaleWithParent)
{
    Transform parent;
    parent.SetScale(Vector3(2.0f, 3.0f, 1.5f));

    transform_.SetParent(&parent);
    transform_.SetScale(Vector3(1.5f, 2.0f, 2.0f));

    Vector3 worldScale = transform_.GetWorldScale();
    EXPECT_FLOAT_EQ(worldScale.x, 3.0f);   // 2.0 * 1.5
    EXPECT_FLOAT_EQ(worldScale.y, 6.0f);   // 3.0 * 2.0
    EXPECT_FLOAT_EQ(worldScale.z, 3.0f);   // 1.5 * 2.0
}

} // namespace
