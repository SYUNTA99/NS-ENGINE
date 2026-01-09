//----------------------------------------------------------------------------
//! @file   shader_cache_test.cpp
//! @brief  シェーダーキャッシュのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "engine/shader/shader_cache.h"

namespace
{

//============================================================================
// ShaderCache テスト
//============================================================================
class ShaderCacheTest : public ::testing::Test
{
protected:
    ShaderCache cache_;
};

TEST_F(ShaderCacheTest, InitiallyEmpty)
{
    EXPECT_TRUE(cache_.isEmpty());
    EXPECT_EQ(cache_.size(), 0);
}

TEST_F(ShaderCacheTest, FindReturnsNullForMissingKey)
{
    auto* result = cache_.find(12345);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ShaderCacheTest, StatsTrackHitsAndMisses)
{
    // ミスを発生させる（戻り値は無視してOK）
    (void)cache_.find(1);
    (void)cache_.find(2);
    (void)cache_.find(3);

    auto stats = cache_.getStats();
    EXPECT_EQ(stats.hitCount, 0);
    EXPECT_EQ(stats.missCount, 3);
}

TEST_F(ShaderCacheTest, ResetStatsClearsCounters)
{
    (void)cache_.find(1);  // ミス
    cache_.resetStats();

    auto stats = cache_.getStats();
    EXPECT_EQ(stats.hitCount, 0);
    EXPECT_EQ(stats.missCount, 0);
}

//============================================================================
// NullShaderCache テスト
//============================================================================
TEST(NullShaderCacheTest, AlwaysReturnsNull)
{
    NullShaderCache cache;

    EXPECT_EQ(cache.find(12345), nullptr);
    EXPECT_EQ(cache.find(0), nullptr);
}

//============================================================================
// ShaderResourceCache テスト
//============================================================================
class ShaderResourceCacheTest : public ::testing::Test
{
protected:
    ShaderResourceCache cache_;
};

TEST_F(ShaderResourceCacheTest, InitiallyEmpty)
{
    EXPECT_EQ(cache_.Count(), 0);
}

TEST_F(ShaderResourceCacheTest, GetReturnsNullForMissingKey)
{
    auto result = cache_.Get(12345);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ShaderResourceCacheTest, ClearRemovesAllEntries)
{
    cache_.Clear();
    EXPECT_EQ(cache_.Count(), 0);
}

} // namespace
