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
    EXPECT_FLOAT_EQ(transform_.GetZ(), 0.0f);
}

TEST_F(TransformTest, DefaultRotation)
{
    EXPECT_FLOAT_EQ(transform_.GetRotation(), 0.0f);
}

TEST_F(TransformTest, DefaultScale)
{
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 1.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 1.0f);
}

TEST_F(TransformTest, SetPositionVector2)
{
    transform_.SetPosition(Vector2(10.0f, 20.0f));
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 20.0f);
}

TEST_F(TransformTest, SetPositionXY)
{
    transform_.SetPosition(15.0f, 25.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 15.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 25.0f);
}

TEST_F(TransformTest, SetPositionXYZ)
{
    transform_.SetPosition(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(transform_.GetZ(), 3.0f);
}

TEST_F(TransformTest, GetPosition3D)
{
    transform_.SetPosition(5.0f, 10.0f, 15.0f);
    Vector3 pos = transform_.GetPosition3D();
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
    EXPECT_FLOAT_EQ(pos.y, 10.0f);
    EXPECT_FLOAT_EQ(pos.z, 15.0f);
}

TEST_F(TransformTest, TranslateXY)
{
    transform_.SetPosition(10.0f, 20.0f);
    transform_.Translate(5.0f, -5.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 15.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 15.0f);
}

TEST_F(TransformTest, TranslateVector2)
{
    transform_.SetPosition(10.0f, 20.0f);
    transform_.Translate(Vector2(3.0f, 7.0f));
    EXPECT_FLOAT_EQ(transform_.GetPosition().x, 13.0f);
    EXPECT_FLOAT_EQ(transform_.GetPosition().y, 27.0f);
}

//============================================================================
// Rotation テスト
//============================================================================
TEST_F(TransformTest, SetRotationRadians)
{
    float radians = DirectX::XM_PI / 4.0f;  // 45度
    transform_.SetRotation(radians);
    EXPECT_FLOAT_EQ(transform_.GetRotation(), radians);
}

TEST_F(TransformTest, SetRotationDegrees)
{
    transform_.SetRotationDegrees(90.0f);
    EXPECT_FLOAT_EQ(transform_.GetRotationDegrees(), 90.0f);
    EXPECT_FLOAT_EQ(transform_.GetRotation(), DirectX::XM_PI / 2.0f);
}

TEST_F(TransformTest, Rotate)
{
    transform_.SetRotation(0.0f);
    transform_.Rotate(DirectX::XM_PI);
    EXPECT_FLOAT_EQ(transform_.GetRotation(), DirectX::XM_PI);
}

TEST_F(TransformTest, RotateDegrees)
{
    transform_.SetRotationDegrees(0.0f);
    transform_.RotateDegrees(45.0f);
    EXPECT_FLOAT_EQ(transform_.GetRotationDegrees(), 45.0f);
}

//============================================================================
// Scale テスト
//============================================================================
TEST_F(TransformTest, SetScaleVector2)
{
    transform_.SetScale(Vector2(2.0f, 3.0f));
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 2.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 3.0f);
}

TEST_F(TransformTest, SetScaleUniform)
{
    transform_.SetScale(5.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 5.0f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 5.0f);
}

TEST_F(TransformTest, SetScaleXY)
{
    transform_.SetScale(1.5f, 2.5f);
    EXPECT_FLOAT_EQ(transform_.GetScale().x, 1.5f);
    EXPECT_FLOAT_EQ(transform_.GetScale().y, 2.5f);
}

//============================================================================
// Pivot テスト
//============================================================================
TEST_F(TransformTest, DefaultPivot)
{
    EXPECT_FLOAT_EQ(transform_.GetPivot().x, 0.0f);
    EXPECT_FLOAT_EQ(transform_.GetPivot().y, 0.0f);
}

TEST_F(TransformTest, SetPivotVector2)
{
    transform_.SetPivot(Vector2(50.0f, 50.0f));
    EXPECT_FLOAT_EQ(transform_.GetPivot().x, 50.0f);
    EXPECT_FLOAT_EQ(transform_.GetPivot().y, 50.0f);
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
    transform_.SetPosition(100.0f, 200.0f);
    Vector2 worldPos = transform_.GetWorldPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 100.0f);
    EXPECT_FLOAT_EQ(worldPos.y, 200.0f);
}

TEST_F(TransformTest, WorldPositionWithParent)
{
    Transform parent;
    parent.SetPosition(100.0f, 100.0f);

    transform_.SetParent(&parent);
    transform_.SetPosition(50.0f, 50.0f);

    Vector2 worldPos = transform_.GetWorldPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 150.0f);
    EXPECT_FLOAT_EQ(worldPos.y, 150.0f);
}

TEST_F(TransformTest, WorldRotationWithoutParent)
{
    transform_.SetRotationDegrees(45.0f);
    float worldRot = transform_.GetWorldRotation();
    EXPECT_NEAR(ToDegrees(worldRot), 45.0f, 0.01f);
}

TEST_F(TransformTest, WorldRotationWithParent)
{
    Transform parent;
    parent.SetRotationDegrees(30.0f);

    transform_.SetParent(&parent);
    transform_.SetRotationDegrees(15.0f);

    float worldRot = transform_.GetWorldRotation();
    EXPECT_NEAR(ToDegrees(worldRot), 45.0f, 0.01f);
}

TEST_F(TransformTest, WorldScaleWithParent)
{
    Transform parent;
    parent.SetScale(2.0f, 3.0f);

    transform_.SetParent(&parent);
    transform_.SetScale(1.5f, 2.0f);

    Vector2 worldScale = transform_.GetWorldScale();
    EXPECT_FLOAT_EQ(worldScale.x, 3.0f);   // 2.0 * 1.5
    EXPECT_FLOAT_EQ(worldScale.y, 6.0f);   // 3.0 * 2.0
}

//============================================================================
// 3D回転テスト
//============================================================================
TEST_F(TransformTest, Default3DRotationDisabled)
{
    EXPECT_FALSE(transform_.Is3DRotationEnabled());
}

TEST_F(TransformTest, Enable3DRotation)
{
    transform_.Enable3DRotation();
    EXPECT_TRUE(transform_.Is3DRotationEnabled());
}

TEST_F(TransformTest, Disable3DRotation)
{
    transform_.Enable3DRotation();
    transform_.Disable3DRotation();
    EXPECT_FALSE(transform_.Is3DRotationEnabled());
}

TEST_F(TransformTest, SetRotation3DQuaternion)
{
    Quaternion q = Quaternion::CreateFromAxisAngle(Vector3::UnitY, DirectX::XM_PI / 2.0f);
    transform_.SetRotation3D(q);

    EXPECT_TRUE(transform_.Is3DRotationEnabled());
    Quaternion result = transform_.GetRotation3D();
    EXPECT_NEAR(result.x, q.x, 0.001f);
    EXPECT_NEAR(result.y, q.y, 0.001f);
    EXPECT_NEAR(result.z, q.z, 0.001f);
    EXPECT_NEAR(result.w, q.w, 0.001f);
}

} // namespace
