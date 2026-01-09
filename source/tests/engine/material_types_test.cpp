//----------------------------------------------------------------------------
//! @file   material_types_test.cpp
//! @brief  マテリアル関連型のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/material/material.h"

namespace
{

//============================================================================
// MaterialTextureSlot enum テスト
//============================================================================
TEST(MaterialTextureSlotTest, AlbedoIsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::Albedo), 0u);
}

TEST(MaterialTextureSlotTest, NormalIsOne)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::Normal), 1u);
}

TEST(MaterialTextureSlotTest, MetallicIsTwo)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::Metallic), 2u);
}

TEST(MaterialTextureSlotTest, RoughnessIsThree)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::Roughness), 3u);
}

TEST(MaterialTextureSlotTest, AOIsFour)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::AO), 4u);
}

TEST(MaterialTextureSlotTest, CountIsFive)
{
    EXPECT_EQ(static_cast<uint32_t>(MaterialTextureSlot::Count), 5u);
}

//============================================================================
// MaterialParams 構造体テスト
//============================================================================
TEST(MaterialParamsTest, SizeIs64Bytes)
{
    EXPECT_EQ(sizeof(MaterialParams), 64u);
}

TEST(MaterialParamsTest, Is16ByteAligned)
{
    EXPECT_EQ(alignof(MaterialParams), 16u);
}

TEST(MaterialParamsTest, DefaultAlbedoColorIsWhite)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.albedoColor.R(), Colors::White.R());
    EXPECT_FLOAT_EQ(params.albedoColor.G(), Colors::White.G());
    EXPECT_FLOAT_EQ(params.albedoColor.B(), Colors::White.B());
    EXPECT_FLOAT_EQ(params.albedoColor.A(), Colors::White.A());
}

TEST(MaterialParamsTest, DefaultMetallicIsZero)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.metallic, 0.0f);
}

TEST(MaterialParamsTest, DefaultRoughnessIsHalf)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.roughness, 0.5f);
}

TEST(MaterialParamsTest, DefaultAOIsOne)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.ao, 1.0f);
}

TEST(MaterialParamsTest, DefaultEmissiveStrengthIsZero)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.emissiveStrength, 0.0f);
}

TEST(MaterialParamsTest, DefaultEmissiveColorIsBlack)
{
    MaterialParams params{};
    EXPECT_FLOAT_EQ(params.emissiveColor.R(), 0.0f);
    EXPECT_FLOAT_EQ(params.emissiveColor.G(), 0.0f);
    EXPECT_FLOAT_EQ(params.emissiveColor.B(), 0.0f);
}

TEST(MaterialParamsTest, DefaultTextureMapFlagsAreZero)
{
    MaterialParams params{};
    EXPECT_EQ(params.useAlbedoMap, 0u);
    EXPECT_EQ(params.useNormalMap, 0u);
    EXPECT_EQ(params.useMetallicMap, 0u);
    EXPECT_EQ(params.useRoughnessMap, 0u);
}

TEST(MaterialParamsTest, CanSetCustomValues)
{
    MaterialParams params{};
    params.metallic = 0.8f;
    params.roughness = 0.2f;
    params.ao = 0.9f;
    params.emissiveStrength = 2.0f;

    EXPECT_FLOAT_EQ(params.metallic, 0.8f);
    EXPECT_FLOAT_EQ(params.roughness, 0.2f);
    EXPECT_FLOAT_EQ(params.ao, 0.9f);
    EXPECT_FLOAT_EQ(params.emissiveStrength, 2.0f);
}

TEST(MaterialParamsTest, CanSetTextureMapFlags)
{
    MaterialParams params{};
    params.useAlbedoMap = 1;
    params.useNormalMap = 1;
    params.useMetallicMap = 1;
    params.useRoughnessMap = 1;

    EXPECT_EQ(params.useAlbedoMap, 1u);
    EXPECT_EQ(params.useNormalMap, 1u);
    EXPECT_EQ(params.useMetallicMap, 1u);
    EXPECT_EQ(params.useRoughnessMap, 1u);
}

//============================================================================
// MaterialDesc 構造体テスト
//============================================================================
TEST(MaterialDescTest, DefaultTextureHandlesAreInvalid)
{
    MaterialDesc desc{};
    for (size_t i = 0; i < static_cast<size_t>(MaterialTextureSlot::Count); ++i) {
        EXPECT_FALSE(desc.textures[i].IsValid());
    }
}

TEST(MaterialDescTest, DefaultNameIsEmpty)
{
    MaterialDesc desc{};
    EXPECT_TRUE(desc.name.empty());
}

TEST(MaterialDescTest, CanSetName)
{
    MaterialDesc desc{};
    desc.name = "TestMaterial";
    EXPECT_EQ(desc.name, "TestMaterial");
}

TEST(MaterialDescTest, CanSetTextureHandles)
{
    MaterialDesc desc{};
    TextureHandle handle = TextureHandle::Create(10, 5);
    desc.textures[static_cast<size_t>(MaterialTextureSlot::Albedo)] = handle;

    EXPECT_TRUE(desc.textures[static_cast<size_t>(MaterialTextureSlot::Albedo)].IsValid());
    EXPECT_FALSE(desc.textures[static_cast<size_t>(MaterialTextureSlot::Normal)].IsValid());
}

TEST(MaterialDescTest, ParamsAreDefaultInitialized)
{
    MaterialDesc desc{};
    EXPECT_FLOAT_EQ(desc.params.metallic, 0.0f);
    EXPECT_FLOAT_EQ(desc.params.roughness, 0.5f);
}

} // namespace
