//----------------------------------------------------------------------------
//! @file   resource_handle_test.cpp
//! @brief  リソースハンドル（TextureHandle, MaterialHandle, MeshHandle）のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/texture/texture_handle.h"
#include "engine/material/material_handle.h"
#include "engine/mesh/mesh_handle.h"

namespace
{

//============================================================================
// TextureHandle テスト
//============================================================================
TEST(TextureHandleTest, DefaultIsInvalid)
{
    TextureHandle handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.id, 0u);
}

TEST(TextureHandleTest, InvalidStaticMethod)
{
    TextureHandle handle = TextureHandle::Invalid();
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.id, 0u);
}

TEST(TextureHandleTest, CreateValidHandle)
{
    TextureHandle handle = TextureHandle::Create(10, 5);
    EXPECT_TRUE(handle.IsValid());
    EXPECT_NE(handle.id, 0u);
}

TEST(TextureHandleTest, GetIndexReturnsCorrectValue)
{
    TextureHandle handle = TextureHandle::Create(42, 0);
    EXPECT_EQ(handle.GetIndex(), 42);
}

TEST(TextureHandleTest, GetGenerationReturnsCorrectValue)
{
    TextureHandle handle = TextureHandle::Create(0, 7);
    EXPECT_EQ(handle.GetGeneration(), 7);
}

TEST(TextureHandleTest, IndexAndGenerationCombination)
{
    TextureHandle handle = TextureHandle::Create(123, 45);
    EXPECT_EQ(handle.GetIndex(), 123);
    EXPECT_EQ(handle.GetGeneration(), 45);
}

TEST(TextureHandleTest, MaxIndex)
{
    TextureHandle handle = TextureHandle::Create(0xFFFF, 0);
    EXPECT_EQ(handle.GetIndex(), 0xFFFF);
}

TEST(TextureHandleTest, MaxGeneration)
{
    TextureHandle handle = TextureHandle::Create(0, 0xFFFE);  // Max is FFFE due to +1
    EXPECT_EQ(handle.GetGeneration(), 0xFFFE);
}

TEST(TextureHandleTest, BoolConversion)
{
    TextureHandle invalid;
    TextureHandle valid = TextureHandle::Create(1, 0);

    EXPECT_FALSE(static_cast<bool>(invalid));
    EXPECT_TRUE(static_cast<bool>(valid));
}

TEST(TextureHandleTest, EqualityOperator)
{
    TextureHandle h1 = TextureHandle::Create(10, 5);
    TextureHandle h2 = TextureHandle::Create(10, 5);
    TextureHandle h3 = TextureHandle::Create(10, 6);

    EXPECT_TRUE(h1 == h2);
    EXPECT_FALSE(h1 == h3);
}

TEST(TextureHandleTest, InequalityOperator)
{
    TextureHandle h1 = TextureHandle::Create(10, 5);
    TextureHandle h2 = TextureHandle::Create(10, 5);
    TextureHandle h3 = TextureHandle::Create(11, 5);

    EXPECT_FALSE(h1 != h2);
    EXPECT_TRUE(h1 != h3);
}

TEST(TextureHandleTest, ZeroIndexWithZeroGenerationIsValid)
{
    // This tests the +1 generation trick to ensure (0,0) is valid
    TextureHandle handle = TextureHandle::Create(0, 0);
    EXPECT_TRUE(handle.IsValid());
    EXPECT_EQ(handle.GetIndex(), 0);
    EXPECT_EQ(handle.GetGeneration(), 0);
}

//============================================================================
// MaterialHandle テスト
//============================================================================
TEST(MaterialHandleTest, DefaultIsInvalid)
{
    MaterialHandle handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.id, 0u);
}

TEST(MaterialHandleTest, InvalidStaticMethod)
{
    MaterialHandle handle = MaterialHandle::Invalid();
    EXPECT_FALSE(handle.IsValid());
}

TEST(MaterialHandleTest, CreateValidHandle)
{
    MaterialHandle handle = MaterialHandle::Create(10, 5);
    EXPECT_TRUE(handle.IsValid());
}

TEST(MaterialHandleTest, GetIndexReturnsCorrectValue)
{
    MaterialHandle handle = MaterialHandle::Create(99, 0);
    EXPECT_EQ(handle.GetIndex(), 99);
}

TEST(MaterialHandleTest, GetGenerationReturnsCorrectValue)
{
    MaterialHandle handle = MaterialHandle::Create(0, 33);
    EXPECT_EQ(handle.GetGeneration(), 33);
}

TEST(MaterialHandleTest, IndexAndGenerationCombination)
{
    MaterialHandle handle = MaterialHandle::Create(500, 200);
    EXPECT_EQ(handle.GetIndex(), 500);
    EXPECT_EQ(handle.GetGeneration(), 200);
}

TEST(MaterialHandleTest, BoolConversion)
{
    MaterialHandle invalid;
    MaterialHandle valid = MaterialHandle::Create(1, 0);

    EXPECT_FALSE(static_cast<bool>(invalid));
    EXPECT_TRUE(static_cast<bool>(valid));
}

TEST(MaterialHandleTest, EqualityOperator)
{
    MaterialHandle h1 = MaterialHandle::Create(10, 5);
    MaterialHandle h2 = MaterialHandle::Create(10, 5);
    MaterialHandle h3 = MaterialHandle::Create(10, 6);

    EXPECT_TRUE(h1 == h2);
    EXPECT_FALSE(h1 == h3);
}

TEST(MaterialHandleTest, InequalityOperator)
{
    MaterialHandle h1 = MaterialHandle::Create(10, 5);
    MaterialHandle h2 = MaterialHandle::Create(10, 5);
    MaterialHandle h3 = MaterialHandle::Create(11, 5);

    EXPECT_FALSE(h1 != h2);
    EXPECT_TRUE(h1 != h3);
}

TEST(MaterialHandleTest, ZeroIndexWithZeroGenerationIsValid)
{
    MaterialHandle handle = MaterialHandle::Create(0, 0);
    EXPECT_TRUE(handle.IsValid());
}

//============================================================================
// MeshHandle テスト
//============================================================================
TEST(MeshHandleTest, DefaultIsInvalid)
{
    MeshHandle handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.id, 0u);
}

TEST(MeshHandleTest, InvalidStaticMethod)
{
    MeshHandle handle = MeshHandle::Invalid();
    EXPECT_FALSE(handle.IsValid());
}

TEST(MeshHandleTest, CreateValidHandle)
{
    MeshHandle handle = MeshHandle::Create(10, 5);
    EXPECT_TRUE(handle.IsValid());
}

TEST(MeshHandleTest, GetIndexReturnsCorrectValue)
{
    MeshHandle handle = MeshHandle::Create(77, 0);
    EXPECT_EQ(handle.GetIndex(), 77);
}

TEST(MeshHandleTest, GetGenerationReturnsCorrectValue)
{
    MeshHandle handle = MeshHandle::Create(0, 88);
    EXPECT_EQ(handle.GetGeneration(), 88);
}

TEST(MeshHandleTest, IndexAndGenerationCombination)
{
    MeshHandle handle = MeshHandle::Create(1000, 500);
    EXPECT_EQ(handle.GetIndex(), 1000);
    EXPECT_EQ(handle.GetGeneration(), 500);
}

TEST(MeshHandleTest, BoolConversion)
{
    MeshHandle invalid;
    MeshHandle valid = MeshHandle::Create(1, 0);

    EXPECT_FALSE(static_cast<bool>(invalid));
    EXPECT_TRUE(static_cast<bool>(valid));
}

TEST(MeshHandleTest, EqualityOperator)
{
    MeshHandle h1 = MeshHandle::Create(10, 5);
    MeshHandle h2 = MeshHandle::Create(10, 5);
    MeshHandle h3 = MeshHandle::Create(10, 6);

    EXPECT_TRUE(h1 == h2);
    EXPECT_FALSE(h1 == h3);
}

TEST(MeshHandleTest, InequalityOperator)
{
    MeshHandle h1 = MeshHandle::Create(10, 5);
    MeshHandle h2 = MeshHandle::Create(10, 5);
    MeshHandle h3 = MeshHandle::Create(11, 5);

    EXPECT_FALSE(h1 != h2);
    EXPECT_TRUE(h1 != h3);
}

TEST(MeshHandleTest, ZeroIndexWithZeroGenerationIsValid)
{
    MeshHandle handle = MeshHandle::Create(0, 0);
    EXPECT_TRUE(handle.IsValid());
}

//============================================================================
// ハンドル間の独立性テスト
//============================================================================
TEST(ResourceHandleTest, DifferentHandleTypesAreIndependent)
{
    // Same values but different types
    TextureHandle tex = TextureHandle::Create(10, 5);
    MaterialHandle mat = MaterialHandle::Create(10, 5);
    MeshHandle mesh = MeshHandle::Create(10, 5);

    // All should have the same id value
    EXPECT_EQ(tex.id, mat.id);
    EXPECT_EQ(mat.id, mesh.id);

    // But they're different types (this is a compile-time check)
    // tex == mat; // This should not compile
}

} // namespace
