//----------------------------------------------------------------------------
//! @file   ecs_components_test.cpp
//! @brief  ECSコンポーネントデータ構造体のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/components/sprite_data.h"
#include "engine/ecs/components/mesh_data.h"

namespace
{

//============================================================================
// TransformData テスト
//============================================================================
class TransformDataTest : public ::testing::Test {};

TEST_F(TransformDataTest, DefaultConstruction)
{
    ECS::TransformData t;
    EXPECT_EQ(t.position, Vector3::Zero);
    EXPECT_EQ(t.rotation, Quaternion::Identity);
    EXPECT_EQ(t.scale, Vector3::One);
    EXPECT_EQ(t.pivot, Vector2::Zero);
    EXPECT_FALSE(t.parent.IsValid());
    EXPECT_TRUE(t.dirty);
}

TEST_F(TransformDataTest, ConstructWithPosition)
{
    ECS::TransformData t(Vector3(10.0f, 20.0f, 30.0f));
    EXPECT_EQ(t.position.x, 10.0f);
    EXPECT_EQ(t.position.y, 20.0f);
    EXPECT_EQ(t.position.z, 30.0f);
}

TEST_F(TransformDataTest, ConstructWithPositionAndRotation)
{
    Quaternion rot = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, 3.14159f / 2.0f);
    ECS::TransformData t(Vector3::One, rot);
    EXPECT_EQ(t.position, Vector3::One);
    EXPECT_FLOAT_EQ(t.rotation.x, rot.x);
    EXPECT_FLOAT_EQ(t.rotation.y, rot.y);
    EXPECT_FLOAT_EQ(t.rotation.z, rot.z);
    EXPECT_FLOAT_EQ(t.rotation.w, rot.w);
}

TEST_F(TransformDataTest, ConstructFull)
{
    ECS::TransformData t(
        Vector3(1.0f, 2.0f, 3.0f),
        Quaternion::Identity,
        Vector3(2.0f, 2.0f, 2.0f)
    );
    EXPECT_EQ(t.position, Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(t.scale, Vector3(2.0f, 2.0f, 2.0f));
}

TEST_F(TransformDataTest, GetSetPosition2D)
{
    ECS::TransformData t;
    t.position.z = 5.0f;  // Zを設定しておく

    t.SetPosition2D(10.0f, 20.0f);
    EXPECT_EQ(t.GetPosition2D(), Vector2(10.0f, 20.0f));
    EXPECT_EQ(t.position.z, 5.0f);  // Z は維持
    EXPECT_TRUE(t.dirty);
}

TEST_F(TransformDataTest, GetSetRotationZ)
{
    ECS::TransformData t;
    t.SetRotationZ(1.5708f);  // 約90度

    float rotZ = t.GetRotationZ();
    EXPECT_NEAR(rotZ, 1.5708f, 0.001f);
    EXPECT_TRUE(t.dirty);
}

TEST_F(TransformDataTest, GetSetScale2D)
{
    ECS::TransformData t;
    t.SetScale2D(Vector2(2.0f, 3.0f));

    EXPECT_EQ(t.GetScale2D(), Vector2(2.0f, 3.0f));
    EXPECT_TRUE(t.dirty);
}

TEST_F(TransformDataTest, SetUniformScale)
{
    ECS::TransformData t;
    t.SetUniformScale(5.0f);

    EXPECT_EQ(t.scale, Vector3(5.0f, 5.0f, 5.0f));
}

TEST_F(TransformDataTest, Translate)
{
    ECS::TransformData t;
    t.dirty = false;  // リセット

    t.Translate(Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(t.position, Vector3(1.0f, 2.0f, 3.0f));
    EXPECT_TRUE(t.dirty);

    t.dirty = false;
    t.Translate(1.0f, 1.0f);
    EXPECT_EQ(t.position, Vector3(2.0f, 3.0f, 3.0f));
    EXPECT_TRUE(t.dirty);
}

TEST_F(TransformDataTest, RotateZ)
{
    ECS::TransformData t;
    t.RotateZ(1.0f);
    t.RotateZ(1.0f);

    float rotZ = t.GetRotationZ();
    EXPECT_NEAR(rotZ, 2.0f, 0.001f);
}

TEST_F(TransformDataTest, GetForward)
{
    ECS::TransformData t;
    // デフォルト（回転なし）の前方向は(0, 0, 1)
    Vector3 forward = t.GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 0.001f);
    EXPECT_NEAR(forward.y, 0.0f, 0.001f);
    EXPECT_NEAR(forward.z, 1.0f, 0.001f);
}

TEST_F(TransformDataTest, GetRight)
{
    ECS::TransformData t;
    Vector3 right = t.GetRight();
    EXPECT_NEAR(right.x, 1.0f, 0.001f);
    EXPECT_NEAR(right.y, 0.0f, 0.001f);
    EXPECT_NEAR(right.z, 0.0f, 0.001f);
}

TEST_F(TransformDataTest, GetUp)
{
    ECS::TransformData t;
    Vector3 up = t.GetUp();
    EXPECT_NEAR(up.x, 0.0f, 0.001f);
    EXPECT_NEAR(up.y, 1.0f, 0.001f);
    EXPECT_NEAR(up.z, 0.0f, 0.001f);
}

//============================================================================
// SpriteData テスト
//============================================================================
class SpriteDataTest : public ::testing::Test {};

TEST_F(SpriteDataTest, DefaultConstruction)
{
    ECS::SpriteData s;
    EXPECT_FALSE(s.texture.IsValid());
    EXPECT_EQ(s.color, Colors::White);
    EXPECT_EQ(s.size, Vector2::Zero);
    EXPECT_EQ(s.pivot, Vector2::Zero);
    EXPECT_EQ(s.sortingLayer, 0);
    EXPECT_EQ(s.orderInLayer, 0);
    EXPECT_FALSE(s.flipX);
    EXPECT_FALSE(s.flipY);
    EXPECT_TRUE(s.visible);
    EXPECT_EQ(s.uvOffset, Vector2::Zero);
    EXPECT_EQ(s.uvSize, Vector2::One);
}

TEST_F(SpriteDataTest, ConstructWithTexture)
{
    TextureHandle tex = TextureHandle::Create(1, 0);
    ECS::SpriteData s(tex);
    EXPECT_TRUE(s.texture.IsValid());
    EXPECT_EQ(s.texture.GetIndex(), 1u);
}

TEST_F(SpriteDataTest, ConstructWithTextureAndSize)
{
    TextureHandle tex = TextureHandle::Create(1, 0);
    ECS::SpriteData s(tex, Vector2(64.0f, 64.0f));
    EXPECT_TRUE(s.texture.IsValid());
    EXPECT_EQ(s.size, Vector2(64.0f, 64.0f));
}

TEST_F(SpriteDataTest, ConstructWithTextureAndSizeAndPivot)
{
    TextureHandle tex = TextureHandle::Create(1, 0);
    ECS::SpriteData s(tex, Vector2(64.0f, 64.0f), Vector2(32.0f, 32.0f));
    EXPECT_EQ(s.size, Vector2(64.0f, 64.0f));
    EXPECT_EQ(s.pivot, Vector2(32.0f, 32.0f));
}

TEST_F(SpriteDataTest, SetAlpha)
{
    ECS::SpriteData s;
    s.SetAlpha(0.5f);
    EXPECT_FLOAT_EQ(s.GetAlpha(), 0.5f);
    EXPECT_FLOAT_EQ(s.color.w, 0.5f);
}

TEST_F(SpriteDataTest, SetPivotCenter)
{
    ECS::SpriteData s;
    s.size = Vector2(100.0f, 80.0f);
    s.SetPivotCenter();
    EXPECT_EQ(s.pivot, Vector2(50.0f, 40.0f));
}

TEST_F(SpriteDataTest, SetUVFrame)
{
    ECS::SpriteData s;
    s.SetUVFrame(2, 1, 0.25f, 0.5f);  // 4列2行のスプライトシートの(2,1)

    EXPECT_FLOAT_EQ(s.uvOffset.x, 0.5f);   // 2 * 0.25
    EXPECT_FLOAT_EQ(s.uvOffset.y, 0.5f);   // 1 * 0.5
    EXPECT_FLOAT_EQ(s.uvSize.x, 0.25f);
    EXPECT_FLOAT_EQ(s.uvSize.y, 0.5f);
}

TEST_F(SpriteDataTest, ResetUV)
{
    ECS::SpriteData s;
    s.uvOffset = Vector2(0.5f, 0.5f);
    s.uvSize = Vector2(0.25f, 0.25f);

    s.ResetUV();
    EXPECT_EQ(s.uvOffset, Vector2::Zero);
    EXPECT_EQ(s.uvSize, Vector2::One);
}

//============================================================================
// MeshData テスト
//============================================================================
class MeshDataTest : public ::testing::Test {};

TEST_F(MeshDataTest, DefaultConstruction)
{
    ECS::MeshData m;
    EXPECT_FALSE(m.mesh.IsValid());
    EXPECT_TRUE(m.materials.empty());
    EXPECT_TRUE(m.visible);
    EXPECT_TRUE(m.castShadow);
    EXPECT_TRUE(m.receiveShadow);
    EXPECT_EQ(m.renderLayer, 0u);
}

TEST_F(MeshDataTest, ConstructWithMesh)
{
    MeshHandle msh = MeshHandle::Create(5, 0);
    ECS::MeshData m(msh);
    EXPECT_TRUE(m.mesh.IsValid());
    EXPECT_EQ(m.mesh.GetIndex(), 5u);
}

TEST_F(MeshDataTest, ConstructWithMeshAndMaterial)
{
    MeshHandle msh = MeshHandle::Create(1, 0);
    MaterialHandle mat = MaterialHandle::Create(2, 0);
    ECS::MeshData m(msh, mat);

    EXPECT_TRUE(m.mesh.IsValid());
    EXPECT_EQ(m.GetMaterialCount(), 1u);
    EXPECT_TRUE(m.GetMaterial(0).IsValid());
}

TEST_F(MeshDataTest, ConstructWithMeshAndMaterials)
{
    MeshHandle msh = MeshHandle::Create(1, 0);
    std::vector<MaterialHandle> mats = {
        MaterialHandle::Create(1, 0),
        MaterialHandle::Create(2, 0),
        MaterialHandle::Create(3, 0)
    };
    ECS::MeshData m(msh, mats);

    EXPECT_EQ(m.GetMaterialCount(), 3u);
}

TEST_F(MeshDataTest, GetMaterialOutOfRange)
{
    ECS::MeshData m;
    EXPECT_FALSE(m.GetMaterial(0).IsValid());
    EXPECT_FALSE(m.GetMaterial(100).IsValid());
}

TEST_F(MeshDataTest, SetMaterialSingle)
{
    ECS::MeshData m;
    MaterialHandle mat = MaterialHandle::Create(5, 0);
    m.SetMaterial(mat);

    EXPECT_EQ(m.GetMaterialCount(), 1u);
    EXPECT_EQ(m.GetMaterial(0).GetIndex(), 5u);
}

TEST_F(MeshDataTest, SetMaterialAtIndex)
{
    ECS::MeshData m;
    MaterialHandle mat = MaterialHandle::Create(5, 0);
    m.SetMaterial(2, mat);  // インデックス2に設定

    EXPECT_EQ(m.GetMaterialCount(), 3u);  // 0,1,2で3個
    EXPECT_FALSE(m.GetMaterial(0).IsValid());
    EXPECT_FALSE(m.GetMaterial(1).IsValid());
    EXPECT_TRUE(m.GetMaterial(2).IsValid());
}

TEST_F(MeshDataTest, HasValidMesh)
{
    ECS::MeshData m1;
    EXPECT_FALSE(m1.HasValidMesh());

    ECS::MeshData m2(MeshHandle::Create(1, 0));
    EXPECT_TRUE(m2.HasValidMesh());
}

} // namespace
