//----------------------------------------------------------------------------
//! @file   collision_layer_test.cpp
//! @brief  CollisionLayerのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/c_systems/collision_layers.h"

namespace
{

//============================================================================
// CollisionLayer 定数テスト
//============================================================================
TEST(CollisionLayerTest, PlayerLayerValue)
{
    EXPECT_EQ(CollisionLayer::Player, 0x01u);
}

TEST(CollisionLayerTest, IndividualLayerValue)
{
    EXPECT_EQ(CollisionLayer::Individual, 0x04u);
}

TEST(CollisionLayerTest, ArrowLayerValue)
{
    EXPECT_EQ(CollisionLayer::Arrow, 0x08u);
}

//============================================================================
// レイヤー値の一意性テスト
//============================================================================
TEST(CollisionLayerTest, LayersAreUnique)
{
    EXPECT_NE(CollisionLayer::Player, CollisionLayer::Individual);
    EXPECT_NE(CollisionLayer::Player, CollisionLayer::Arrow);
    EXPECT_NE(CollisionLayer::Individual, CollisionLayer::Arrow);
}

//============================================================================
// レイヤー値がビットフラグとして有効
//============================================================================
TEST(CollisionLayerTest, LayersAreSingleBits)
{
    // Each layer should be a power of 2 (single bit)
    auto isSingleBit = [](uint32_t v) {
        return v != 0 && (v & (v - 1)) == 0;
    };

    EXPECT_TRUE(isSingleBit(CollisionLayer::Player));
    EXPECT_TRUE(isSingleBit(CollisionLayer::Individual));
    EXPECT_TRUE(isSingleBit(CollisionLayer::Arrow));
}

//============================================================================
// マスク定義テスト
//============================================================================
TEST(CollisionLayerTest, PlayerMaskIncludesIndividualAndArrow)
{
    EXPECT_EQ(CollisionLayer::PlayerMask, CollisionLayer::Individual | CollisionLayer::Arrow);
}

TEST(CollisionLayerTest, PlayerMaskDoesNotIncludePlayer)
{
    EXPECT_EQ(CollisionLayer::PlayerMask & CollisionLayer::Player, 0u);
}

TEST(CollisionLayerTest, IndividualMaskIncludesAll)
{
    EXPECT_EQ(CollisionLayer::IndividualMask,
              CollisionLayer::Player | CollisionLayer::Individual | CollisionLayer::Arrow);
}

TEST(CollisionLayerTest, ArrowMaskIncludesPlayerAndIndividual)
{
    EXPECT_EQ(CollisionLayer::ArrowMask, CollisionLayer::Player | CollisionLayer::Individual);
}

TEST(CollisionLayerTest, ArrowMaskDoesNotIncludeArrow)
{
    EXPECT_EQ(CollisionLayer::ArrowMask & CollisionLayer::Arrow, 0u);
}

//============================================================================
// マスクの衝突判定テスト
//============================================================================
TEST(CollisionLayerTest, PlayerCollidesWithIndividual)
{
    // Player's mask allows collision with Individual
    EXPECT_NE(CollisionLayer::PlayerMask & CollisionLayer::Individual, 0u);
}

TEST(CollisionLayerTest, PlayerCollidesWithArrow)
{
    // Player's mask allows collision with Arrow
    EXPECT_NE(CollisionLayer::PlayerMask & CollisionLayer::Arrow, 0u);
}

TEST(CollisionLayerTest, IndividualCollidesWithPlayer)
{
    // Individual's mask allows collision with Player
    EXPECT_NE(CollisionLayer::IndividualMask & CollisionLayer::Player, 0u);
}

TEST(CollisionLayerTest, IndividualCollidesWithIndividual)
{
    // Individual's mask allows collision with other Individuals
    EXPECT_NE(CollisionLayer::IndividualMask & CollisionLayer::Individual, 0u);
}

TEST(CollisionLayerTest, IndividualCollidesWithArrow)
{
    // Individual's mask allows collision with Arrow
    EXPECT_NE(CollisionLayer::IndividualMask & CollisionLayer::Arrow, 0u);
}

TEST(CollisionLayerTest, ArrowCollidesWithPlayer)
{
    // Arrow's mask allows collision with Player
    EXPECT_NE(CollisionLayer::ArrowMask & CollisionLayer::Player, 0u);
}

TEST(CollisionLayerTest, ArrowCollidesWithIndividual)
{
    // Arrow's mask allows collision with Individual
    EXPECT_NE(CollisionLayer::ArrowMask & CollisionLayer::Individual, 0u);
}

//============================================================================
// 双方向衝突整合性テスト
//============================================================================
TEST(CollisionLayerTest, PlayerIndividualCollisionIsBidirectional)
{
    // If Player can collide with Individual, Individual should also collide with Player
    bool playerToIndividual = (CollisionLayer::PlayerMask & CollisionLayer::Individual) != 0;
    bool individualToPlayer = (CollisionLayer::IndividualMask & CollisionLayer::Player) != 0;
    EXPECT_EQ(playerToIndividual, individualToPlayer);
}

TEST(CollisionLayerTest, PlayerArrowCollisionIsBidirectional)
{
    bool playerToArrow = (CollisionLayer::PlayerMask & CollisionLayer::Arrow) != 0;
    bool arrowToPlayer = (CollisionLayer::ArrowMask & CollisionLayer::Player) != 0;
    EXPECT_EQ(playerToArrow, arrowToPlayer);
}

TEST(CollisionLayerTest, IndividualArrowCollisionIsBidirectional)
{
    bool individualToArrow = (CollisionLayer::IndividualMask & CollisionLayer::Arrow) != 0;
    bool arrowToIndividual = (CollisionLayer::ArrowMask & CollisionLayer::Individual) != 0;
    EXPECT_EQ(individualToArrow, arrowToIndividual);
}

} // namespace
