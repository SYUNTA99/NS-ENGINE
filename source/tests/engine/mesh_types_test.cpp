//----------------------------------------------------------------------------
//! @file   mesh_types_test.cpp
//! @brief  メッシュ関連型のテスト（BoundingBox, SubMesh, MeshDesc, 頂点フォーマット）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/mesh/mesh.h"
#include "engine/mesh/vertex_format.h"

namespace
{

//============================================================================
// MeshVertex テスト
//============================================================================
TEST(MeshVertexTest, SizeIs64Bytes)
{
    EXPECT_EQ(sizeof(MeshVertex), 64u);
}

TEST(MeshVertexTest, PositionOffset)
{
    EXPECT_EQ(offsetof(MeshVertex, position), 0u);
}

TEST(MeshVertexTest, NormalOffset)
{
    EXPECT_EQ(offsetof(MeshVertex, normal), 12u);
}

TEST(MeshVertexTest, TangentOffset)
{
    EXPECT_EQ(offsetof(MeshVertex, tangent), 24u);
}

TEST(MeshVertexTest, TexCoordOffset)
{
    EXPECT_EQ(offsetof(MeshVertex, texCoord), 40u);
}

TEST(MeshVertexTest, ColorOffset)
{
    EXPECT_EQ(offsetof(MeshVertex, color), 48u);
}

TEST(MeshVertexTest, StrideFunction)
{
    EXPECT_EQ(GetMeshVertexStride(), 64u);
}

//============================================================================
// SkinnedMeshVertex テスト
//============================================================================
TEST(SkinnedMeshVertexTest, SizeIs84Bytes)
{
    EXPECT_EQ(sizeof(SkinnedMeshVertex), 84u);
}

TEST(SkinnedMeshVertexTest, BoneIndicesOffset)
{
    EXPECT_EQ(offsetof(SkinnedMeshVertex, boneIndices), 64u);
}

TEST(SkinnedMeshVertexTest, BoneWeightsOffset)
{
    EXPECT_EQ(offsetof(SkinnedMeshVertex, boneWeights), 68u);
}

TEST(SkinnedMeshVertexTest, StrideFunction)
{
    EXPECT_EQ(GetSkinnedMeshVertexStride(), 84u);
}

//============================================================================
// BoundingBox テスト
//============================================================================
TEST(BoundingBoxTest, DefaultMinIsMaxFloat)
{
    BoundingBox box;
    EXPECT_FLOAT_EQ(box.min.x, FLT_MAX);
    EXPECT_FLOAT_EQ(box.min.y, FLT_MAX);
    EXPECT_FLOAT_EQ(box.min.z, FLT_MAX);
}

TEST(BoundingBoxTest, DefaultMaxIsNegMaxFloat)
{
    BoundingBox box;
    EXPECT_FLOAT_EQ(box.max.x, -FLT_MAX);
    EXPECT_FLOAT_EQ(box.max.y, -FLT_MAX);
    EXPECT_FLOAT_EQ(box.max.z, -FLT_MAX);
}

TEST(BoundingBoxTest, DefaultIsNotValid)
{
    BoundingBox box;
    EXPECT_FALSE(box.IsValid());
}

TEST(BoundingBoxTest, CenterOfUnitBox)
{
    BoundingBox box;
    box.min = Vector3(-1.0f, -1.0f, -1.0f);
    box.max = Vector3(1.0f, 1.0f, 1.0f);

    Vector3 center = box.Center();
    EXPECT_FLOAT_EQ(center.x, 0.0f);
    EXPECT_FLOAT_EQ(center.y, 0.0f);
    EXPECT_FLOAT_EQ(center.z, 0.0f);
}

TEST(BoundingBoxTest, CenterOfOffsetBox)
{
    BoundingBox box;
    box.min = Vector3(0.0f, 0.0f, 0.0f);
    box.max = Vector3(10.0f, 20.0f, 30.0f);

    Vector3 center = box.Center();
    EXPECT_FLOAT_EQ(center.x, 5.0f);
    EXPECT_FLOAT_EQ(center.y, 10.0f);
    EXPECT_FLOAT_EQ(center.z, 15.0f);
}

TEST(BoundingBoxTest, ExtentsOfUnitBox)
{
    BoundingBox box;
    box.min = Vector3(-1.0f, -1.0f, -1.0f);
    box.max = Vector3(1.0f, 1.0f, 1.0f);

    Vector3 extents = box.Extents();
    EXPECT_FLOAT_EQ(extents.x, 1.0f);
    EXPECT_FLOAT_EQ(extents.y, 1.0f);
    EXPECT_FLOAT_EQ(extents.z, 1.0f);
}

TEST(BoundingBoxTest, ExtentsOfAsymmetricBox)
{
    BoundingBox box;
    box.min = Vector3(0.0f, 0.0f, 0.0f);
    box.max = Vector3(4.0f, 6.0f, 8.0f);

    Vector3 extents = box.Extents();
    EXPECT_FLOAT_EQ(extents.x, 2.0f);
    EXPECT_FLOAT_EQ(extents.y, 3.0f);
    EXPECT_FLOAT_EQ(extents.z, 4.0f);
}

TEST(BoundingBoxTest, ExpandWithSinglePoint)
{
    BoundingBox box;
    box.Expand(Vector3(5.0f, 10.0f, 15.0f));

    EXPECT_FLOAT_EQ(box.min.x, 5.0f);
    EXPECT_FLOAT_EQ(box.min.y, 10.0f);
    EXPECT_FLOAT_EQ(box.min.z, 15.0f);
    EXPECT_FLOAT_EQ(box.max.x, 5.0f);
    EXPECT_FLOAT_EQ(box.max.y, 10.0f);
    EXPECT_FLOAT_EQ(box.max.z, 15.0f);
}

TEST(BoundingBoxTest, ExpandWithMultiplePoints)
{
    BoundingBox box;
    box.Expand(Vector3(0.0f, 0.0f, 0.0f));
    box.Expand(Vector3(10.0f, 20.0f, 30.0f));
    box.Expand(Vector3(-5.0f, 5.0f, 15.0f));

    EXPECT_FLOAT_EQ(box.min.x, -5.0f);
    EXPECT_FLOAT_EQ(box.min.y, 0.0f);
    EXPECT_FLOAT_EQ(box.min.z, 0.0f);
    EXPECT_FLOAT_EQ(box.max.x, 10.0f);
    EXPECT_FLOAT_EQ(box.max.y, 20.0f);
    EXPECT_FLOAT_EQ(box.max.z, 30.0f);
}

TEST(BoundingBoxTest, IsValidAfterExpand)
{
    BoundingBox box;
    box.Expand(Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_TRUE(box.IsValid());
}

TEST(BoundingBoxTest, IsValidForValidRange)
{
    BoundingBox box;
    box.min = Vector3(-1.0f, -1.0f, -1.0f);
    box.max = Vector3(1.0f, 1.0f, 1.0f);
    EXPECT_TRUE(box.IsValid());
}

TEST(BoundingBoxTest, IsValidForInvertedX)
{
    BoundingBox box;
    box.min = Vector3(1.0f, -1.0f, -1.0f);
    box.max = Vector3(-1.0f, 1.0f, 1.0f);
    EXPECT_FALSE(box.IsValid());
}

TEST(BoundingBoxTest, IsValidForInvertedY)
{
    BoundingBox box;
    box.min = Vector3(-1.0f, 1.0f, -1.0f);
    box.max = Vector3(1.0f, -1.0f, 1.0f);
    EXPECT_FALSE(box.IsValid());
}

TEST(BoundingBoxTest, IsValidForInvertedZ)
{
    BoundingBox box;
    box.min = Vector3(-1.0f, -1.0f, 1.0f);
    box.max = Vector3(1.0f, 1.0f, -1.0f);
    EXPECT_FALSE(box.IsValid());
}

TEST(BoundingBoxTest, IsValidForZeroSizeBox)
{
    BoundingBox box;
    box.min = Vector3(0.0f, 0.0f, 0.0f);
    box.max = Vector3(0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(box.IsValid());
}

//============================================================================
// SubMesh テスト
//============================================================================
TEST(SubMeshTest, DefaultIndexOffsetIsZero)
{
    SubMesh subMesh;
    EXPECT_EQ(subMesh.indexOffset, 0u);
}

TEST(SubMeshTest, DefaultIndexCountIsZero)
{
    SubMesh subMesh;
    EXPECT_EQ(subMesh.indexCount, 0u);
}

TEST(SubMeshTest, DefaultMaterialIndexIsZero)
{
    SubMesh subMesh;
    EXPECT_EQ(subMesh.materialIndex, 0u);
}

TEST(SubMeshTest, DefaultNameIsEmpty)
{
    SubMesh subMesh;
    EXPECT_TRUE(subMesh.name.empty());
}

TEST(SubMeshTest, CanSetValues)
{
    SubMesh subMesh;
    subMesh.indexOffset = 100;
    subMesh.indexCount = 500;
    subMesh.materialIndex = 2;
    subMesh.name = "TestSubMesh";

    EXPECT_EQ(subMesh.indexOffset, 100u);
    EXPECT_EQ(subMesh.indexCount, 500u);
    EXPECT_EQ(subMesh.materialIndex, 2u);
    EXPECT_EQ(subMesh.name, "TestSubMesh");
}

//============================================================================
// MeshDesc テスト
//============================================================================
TEST(MeshDescTest, DefaultVerticesIsEmpty)
{
    MeshDesc desc;
    EXPECT_TRUE(desc.vertices.empty());
}

TEST(MeshDescTest, DefaultIndicesIsEmpty)
{
    MeshDesc desc;
    EXPECT_TRUE(desc.indices.empty());
}

TEST(MeshDescTest, DefaultSubMeshesIsEmpty)
{
    MeshDesc desc;
    EXPECT_TRUE(desc.subMeshes.empty());
}

TEST(MeshDescTest, DefaultNameIsEmpty)
{
    MeshDesc desc;
    EXPECT_TRUE(desc.name.empty());
}

TEST(MeshDescTest, DefaultBoundsIsInvalid)
{
    MeshDesc desc;
    EXPECT_FALSE(desc.bounds.IsValid());
}

TEST(MeshDescTest, CanSetName)
{
    MeshDesc desc;
    desc.name = "TestMesh";
    EXPECT_EQ(desc.name, "TestMesh");
}

TEST(MeshDescTest, CanAddVertices)
{
    MeshDesc desc;
    MeshVertex vertex{};
    vertex.position = Vector3(1.0f, 2.0f, 3.0f);
    desc.vertices.push_back(vertex);

    EXPECT_EQ(desc.vertices.size(), 1u);
    EXPECT_FLOAT_EQ(desc.vertices[0].position.x, 1.0f);
}

TEST(MeshDescTest, CanAddIndices)
{
    MeshDesc desc;
    desc.indices.push_back(0);
    desc.indices.push_back(1);
    desc.indices.push_back(2);

    EXPECT_EQ(desc.indices.size(), 3u);
    EXPECT_EQ(desc.indices[0], 0u);
    EXPECT_EQ(desc.indices[1], 1u);
    EXPECT_EQ(desc.indices[2], 2u);
}

TEST(MeshDescTest, CanAddSubMeshes)
{
    MeshDesc desc;
    SubMesh subMesh;
    subMesh.indexOffset = 0;
    subMesh.indexCount = 36;
    subMesh.materialIndex = 0;
    desc.subMeshes.push_back(subMesh);

    EXPECT_EQ(desc.subMeshes.size(), 1u);
    EXPECT_EQ(desc.subMeshes[0].indexCount, 36u);
}

TEST(MeshDescTest, CanSetBounds)
{
    MeshDesc desc;
    desc.bounds.min = Vector3(-1.0f, -1.0f, -1.0f);
    desc.bounds.max = Vector3(1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(desc.bounds.IsValid());
    Vector3 center = desc.bounds.Center();
    EXPECT_FLOAT_EQ(center.x, 0.0f);
}

//============================================================================
// InputLayout テスト
//============================================================================
TEST(MeshInputLayoutsTest, MeshVertexLayoutCount)
{
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayoutCount, 5u);
}

TEST(MeshInputLayoutsTest, SkinnedMeshVertexLayoutCount)
{
    EXPECT_EQ(MeshInputLayouts::SkinnedMeshVertexLayoutCount, 7u);
}

TEST(MeshInputLayoutsTest, MeshVertexLayoutSemantics)
{
    EXPECT_STREQ(MeshInputLayouts::MeshVertexLayout[0].SemanticName, "POSITION");
    EXPECT_STREQ(MeshInputLayouts::MeshVertexLayout[1].SemanticName, "NORMAL");
    EXPECT_STREQ(MeshInputLayouts::MeshVertexLayout[2].SemanticName, "TANGENT");
    EXPECT_STREQ(MeshInputLayouts::MeshVertexLayout[3].SemanticName, "TEXCOORD");
    EXPECT_STREQ(MeshInputLayouts::MeshVertexLayout[4].SemanticName, "COLOR");
}

TEST(MeshInputLayoutsTest, SkinnedMeshVertexLayoutSemantics)
{
    EXPECT_STREQ(MeshInputLayouts::SkinnedMeshVertexLayout[5].SemanticName, "BLENDINDICES");
    EXPECT_STREQ(MeshInputLayouts::SkinnedMeshVertexLayout[6].SemanticName, "BLENDWEIGHT");
}

TEST(MeshInputLayoutsTest, MeshVertexLayoutOffsets)
{
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayout[0].AlignedByteOffset, 0u);
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayout[1].AlignedByteOffset, 12u);
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayout[2].AlignedByteOffset, 24u);
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayout[3].AlignedByteOffset, 40u);
    EXPECT_EQ(MeshInputLayouts::MeshVertexLayout[4].AlignedByteOffset, 48u);
}

} // namespace
