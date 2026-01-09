//----------------------------------------------------------------------------
//! @file   hash_test.cpp
//! @brief  ハッシュユーティリティのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "common/utility/hash.h"

namespace
{

using namespace HashUtil;

//============================================================================
// Fnv1a ハッシュテスト
//============================================================================
TEST(HashTest, Fnv1aString_SameInputProducesSameHash)
{
    std::string str = "test_shader.hlsl";

    uint64_t hash1 = Fnv1aString(str);
    uint64_t hash2 = Fnv1aString(str);

    EXPECT_EQ(hash1, hash2);
}

TEST(HashTest, Fnv1aString_DifferentInputProducesDifferentHash)
{
    uint64_t hash1 = Fnv1aString("shader_a.hlsl");
    uint64_t hash2 = Fnv1aString("shader_b.hlsl");

    EXPECT_NE(hash1, hash2);
}

TEST(HashTest, Fnv1aString_EmptyStringHasValidHash)
{
    uint64_t hash = Fnv1aString("");

    // FNV-1a offset basis for 64-bit
    EXPECT_EQ(hash, 14695981039346656037ULL);
}

TEST(HashTest, Fnv1a_ChainedHashDiffersFromSingle)
{
    std::string str1 = "hello";
    std::string str2 = "world";

    // 連結してハッシュ
    uint64_t hash1 = Fnv1aString(str1);
    uint64_t hash2 = Fnv1aString(str2, hash1);  // チェーン

    // 別々にハッシュ
    uint64_t hash3 = Fnv1aString(str2);

    // チェーンしたハッシュは単独とは異なる
    EXPECT_NE(hash2, hash3);
}

} // namespace
