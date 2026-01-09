//----------------------------------------------------------------------------
//! @file   mesh_renderer_test.cpp
//! @brief  MeshRendererコンポーネントのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/component/mesh_renderer.h"

namespace
{

//============================================================================
// デフォルト状態テスト
//============================================================================
TEST(MeshRendererTest, DefaultMeshIsInvalid)
{
    MeshRenderer renderer;
    EXPECT_FALSE(renderer.GetMesh().IsValid());
}

TEST(MeshRendererTest, DefaultMaterialCountIsZero)
{
    MeshRenderer renderer;
    EXPECT_EQ(renderer.GetMaterialCount(), 0u);
}

TEST(MeshRendererTest, DefaultGetMaterialReturnsInvalid)
{
    MeshRenderer renderer;
    EXPECT_FALSE(renderer.GetMaterial().IsValid());
}

TEST(MeshRendererTest, DefaultIsVisibleIsTrue)
{
    MeshRenderer renderer;
    EXPECT_TRUE(renderer.IsVisible());
}

TEST(MeshRendererTest, DefaultCastShadowIsTrue)
{
    MeshRenderer renderer;
    EXPECT_TRUE(renderer.IsCastShadow());
}

TEST(MeshRendererTest, DefaultReceiveShadowIsTrue)
{
    MeshRenderer renderer;
    EXPECT_TRUE(renderer.IsReceiveShadow());
}

TEST(MeshRendererTest, DefaultRenderLayerIsZero)
{
    MeshRenderer renderer;
    EXPECT_EQ(renderer.GetRenderLayer(), 0u);
}

//============================================================================
// コンストラクタテスト
//============================================================================
TEST(MeshRendererTest, ConstructWithMesh)
{
    MeshHandle mesh = MeshHandle::Create(5, 0);
    MeshRenderer renderer(mesh);

    EXPECT_TRUE(renderer.GetMesh().IsValid());
    EXPECT_EQ(renderer.GetMesh(), mesh);
}

TEST(MeshRendererTest, ConstructWithMeshAndMaterial)
{
    MeshHandle mesh = MeshHandle::Create(5, 0);
    MaterialHandle material = MaterialHandle::Create(3, 0);
    MeshRenderer renderer(mesh, material);

    EXPECT_EQ(renderer.GetMesh(), mesh);
    EXPECT_EQ(renderer.GetMaterialCount(), 1u);
    EXPECT_EQ(renderer.GetMaterial(), material);
}

TEST(MeshRendererTest, ConstructWithMeshAndInvalidMaterial)
{
    MeshHandle mesh = MeshHandle::Create(5, 0);
    MeshRenderer renderer(mesh, MaterialHandle::Invalid());

    EXPECT_EQ(renderer.GetMesh(), mesh);
    EXPECT_EQ(renderer.GetMaterialCount(), 0u);
}

//============================================================================
// メッシュ設定テスト
//============================================================================
TEST(MeshRendererTest, SetMesh)
{
    MeshRenderer renderer;
    MeshHandle mesh = MeshHandle::Create(10, 2);
    renderer.SetMesh(mesh);

    EXPECT_EQ(renderer.GetMesh(), mesh);
}

TEST(MeshRendererTest, SetMeshToInvalid)
{
    MeshHandle mesh = MeshHandle::Create(10, 2);
    MeshRenderer renderer(mesh);

    renderer.SetMesh(MeshHandle::Invalid());
    EXPECT_FALSE(renderer.GetMesh().IsValid());
}

//============================================================================
// 単一マテリアル設定テスト
//============================================================================
TEST(MeshRendererTest, SetMaterialSingle)
{
    MeshRenderer renderer;
    MaterialHandle material = MaterialHandle::Create(7, 1);
    renderer.SetMaterial(material);

    EXPECT_EQ(renderer.GetMaterialCount(), 1u);
    EXPECT_EQ(renderer.GetMaterial(), material);
}

TEST(MeshRendererTest, SetMaterialClearsPrevious)
{
    MeshRenderer renderer;
    renderer.SetMaterial(MaterialHandle::Create(1, 0));
    renderer.SetMaterial(MaterialHandle::Create(2, 0));

    EXPECT_EQ(renderer.GetMaterialCount(), 1u);
    EXPECT_EQ(renderer.GetMaterial().GetIndex(), 2u);
}

TEST(MeshRendererTest, SetMaterialInvalidClearsAll)
{
    MeshRenderer renderer;
    renderer.SetMaterial(MaterialHandle::Create(1, 0));
    renderer.SetMaterial(MaterialHandle::Invalid());

    EXPECT_EQ(renderer.GetMaterialCount(), 0u);
}

//============================================================================
// インデックス指定マテリアル設定テスト
//============================================================================
TEST(MeshRendererTest, SetMaterialByIndex)
{
    MeshRenderer renderer;
    MaterialHandle material = MaterialHandle::Create(5, 0);
    renderer.SetMaterial(0, material);

    EXPECT_EQ(renderer.GetMaterialCount(), 1u);
    EXPECT_EQ(renderer.GetMaterial(0), material);
}

TEST(MeshRendererTest, SetMaterialByIndexResizes)
{
    MeshRenderer renderer;
    MaterialHandle material = MaterialHandle::Create(5, 0);
    renderer.SetMaterial(3, material);

    // 配列がリサイズされる（0,1,2はInvalid、3がmaterial）
    EXPECT_EQ(renderer.GetMaterialCount(), 4u);
    EXPECT_FALSE(renderer.GetMaterial(0).IsValid());
    EXPECT_FALSE(renderer.GetMaterial(1).IsValid());
    EXPECT_FALSE(renderer.GetMaterial(2).IsValid());
    EXPECT_EQ(renderer.GetMaterial(3), material);
}

TEST(MeshRendererTest, SetMaterialByIndexMultiple)
{
    MeshRenderer renderer;
    MaterialHandle mat0 = MaterialHandle::Create(10, 0);
    MaterialHandle mat1 = MaterialHandle::Create(20, 0);
    MaterialHandle mat2 = MaterialHandle::Create(30, 0);

    renderer.SetMaterial(0, mat0);
    renderer.SetMaterial(1, mat1);
    renderer.SetMaterial(2, mat2);

    EXPECT_EQ(renderer.GetMaterialCount(), 3u);
    EXPECT_EQ(renderer.GetMaterial(0), mat0);
    EXPECT_EQ(renderer.GetMaterial(1), mat1);
    EXPECT_EQ(renderer.GetMaterial(2), mat2);
}

TEST(MeshRendererTest, GetMaterialOutOfRangeReturnsInvalid)
{
    MeshRenderer renderer;
    renderer.SetMaterial(MaterialHandle::Create(1, 0));

    EXPECT_FALSE(renderer.GetMaterial(10).IsValid());
}

//============================================================================
// 複数マテリアル設定テスト
//============================================================================
TEST(MeshRendererTest, SetMaterialsVector)
{
    MeshRenderer renderer;
    std::vector<MaterialHandle> materials = {
        MaterialHandle::Create(1, 0),
        MaterialHandle::Create(2, 0),
        MaterialHandle::Create(3, 0)
    };
    renderer.SetMaterials(materials);

    EXPECT_EQ(renderer.GetMaterialCount(), 3u);
    EXPECT_EQ(renderer.GetMaterial(0).GetIndex(), 1u);
    EXPECT_EQ(renderer.GetMaterial(1).GetIndex(), 2u);
    EXPECT_EQ(renderer.GetMaterial(2).GetIndex(), 3u);
}

TEST(MeshRendererTest, GetMaterialsReturnsVector)
{
    MeshRenderer renderer;
    std::vector<MaterialHandle> materials = {
        MaterialHandle::Create(5, 0),
        MaterialHandle::Create(6, 0)
    };
    renderer.SetMaterials(materials);

    const auto& result = renderer.GetMaterials();
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].GetIndex(), 5u);
    EXPECT_EQ(result[1].GetIndex(), 6u);
}

TEST(MeshRendererTest, SetMaterialsEmptyVector)
{
    MeshRenderer renderer;
    renderer.SetMaterial(MaterialHandle::Create(1, 0));

    std::vector<MaterialHandle> empty;
    renderer.SetMaterials(empty);

    EXPECT_EQ(renderer.GetMaterialCount(), 0u);
}

//============================================================================
// 描画設定テスト
//============================================================================
TEST(MeshRendererTest, SetVisibleFalse)
{
    MeshRenderer renderer;
    renderer.SetVisible(false);
    EXPECT_FALSE(renderer.IsVisible());
}

TEST(MeshRendererTest, SetVisibleTrue)
{
    MeshRenderer renderer;
    renderer.SetVisible(false);
    renderer.SetVisible(true);
    EXPECT_TRUE(renderer.IsVisible());
}

TEST(MeshRendererTest, SetCastShadowFalse)
{
    MeshRenderer renderer;
    renderer.SetCastShadow(false);
    EXPECT_FALSE(renderer.IsCastShadow());
}

TEST(MeshRendererTest, SetCastShadowTrue)
{
    MeshRenderer renderer;
    renderer.SetCastShadow(false);
    renderer.SetCastShadow(true);
    EXPECT_TRUE(renderer.IsCastShadow());
}

TEST(MeshRendererTest, SetReceiveShadowFalse)
{
    MeshRenderer renderer;
    renderer.SetReceiveShadow(false);
    EXPECT_FALSE(renderer.IsReceiveShadow());
}

TEST(MeshRendererTest, SetReceiveShadowTrue)
{
    MeshRenderer renderer;
    renderer.SetReceiveShadow(false);
    renderer.SetReceiveShadow(true);
    EXPECT_TRUE(renderer.IsReceiveShadow());
}

//============================================================================
// レンダリングレイヤーテスト
//============================================================================
TEST(MeshRendererTest, SetRenderLayer)
{
    MeshRenderer renderer;
    renderer.SetRenderLayer(5);
    EXPECT_EQ(renderer.GetRenderLayer(), 5u);
}

TEST(MeshRendererTest, SetRenderLayerBitMask)
{
    MeshRenderer renderer;
    renderer.SetRenderLayer(0xFF00FF00);
    EXPECT_EQ(renderer.GetRenderLayer(), 0xFF00FF00u);
}

TEST(MeshRendererTest, SetRenderLayerMax)
{
    MeshRenderer renderer;
    renderer.SetRenderLayer(0xFFFFFFFF);
    EXPECT_EQ(renderer.GetRenderLayer(), 0xFFFFFFFFu);
}

//============================================================================
// Component基底クラステスト
//============================================================================
TEST(MeshRendererTest, InheritsFromComponent)
{
    MeshRenderer renderer;
    Component* component = &renderer;
    EXPECT_NE(component, nullptr);
}

TEST(MeshRendererTest, DefaultOwnerIsNull)
{
    MeshRenderer renderer;
    EXPECT_EQ(renderer.GetOwner(), nullptr);
}

TEST(MeshRendererTest, DefaultIsEnabled)
{
    MeshRenderer renderer;
    EXPECT_TRUE(renderer.IsEnabled());
}

TEST(MeshRendererTest, SetEnabled)
{
    MeshRenderer renderer;
    renderer.SetEnabled(false);
    EXPECT_FALSE(renderer.IsEnabled());

    renderer.SetEnabled(true);
    EXPECT_TRUE(renderer.IsEnabled());
}

} // namespace
