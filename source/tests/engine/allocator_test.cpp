//----------------------------------------------------------------------------
//! @file   allocator_test.cpp
//! @brief  メモリアロケータのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/memory/linear_allocator.h"
#include "engine/memory/heap_allocator.h"

namespace
{

//============================================================================
// HeapAllocator テスト
//============================================================================
class HeapAllocatorTest : public ::testing::Test {
protected:
    Memory::HeapAllocator allocator_;
};

TEST_F(HeapAllocatorTest, AllocateAndDeallocate)
{
    void* ptr = allocator_.Allocate(64, 8);
    ASSERT_NE(ptr, nullptr);

    // 解放してもクラッシュしない
    allocator_.Deallocate(ptr, 64);
}

TEST_F(HeapAllocatorTest, AllocateZeroReturnsNull)
{
    void* ptr = allocator_.Allocate(0, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(HeapAllocatorTest, StatsTrackAllocations)
{
    auto statsBefore = allocator_.GetStats();

    void* ptr = allocator_.Allocate(128, 8);
    ASSERT_NE(ptr, nullptr);

    auto statsAfter = allocator_.GetStats();
    EXPECT_GT(statsAfter.allocationCount, statsBefore.allocationCount);

    allocator_.Deallocate(ptr, 128);
}

TEST_F(HeapAllocatorTest, GetName)
{
    EXPECT_STREQ(allocator_.GetName(), "HeapAllocator");
}

//============================================================================
// LinearAllocator テスト
//============================================================================
class LinearAllocatorTest : public ::testing::Test {
protected:
    static constexpr size_t kCapacity = 1024;
};

TEST_F(LinearAllocatorTest, Construction)
{
    Memory::LinearAllocator allocator(kCapacity);

    EXPECT_EQ(allocator.GetCapacity(), kCapacity);
    EXPECT_EQ(allocator.GetUsed(), 0u);
    EXPECT_EQ(allocator.GetRemaining(), kCapacity);
}

TEST_F(LinearAllocatorTest, Allocate)
{
    Memory::LinearAllocator allocator(kCapacity);

    void* ptr = allocator.Allocate(64, 8);
    ASSERT_NE(ptr, nullptr);

    EXPECT_GE(allocator.GetUsed(), 64u);
    EXPECT_TRUE(allocator.Owns(ptr));
}

TEST_F(LinearAllocatorTest, MultipleAllocations)
{
    Memory::LinearAllocator allocator(kCapacity);

    void* ptr1 = allocator.Allocate(32, 8);
    void* ptr2 = allocator.Allocate(64, 8);
    void* ptr3 = allocator.Allocate(128, 8);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // 全て異なるアドレス
    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    EXPECT_NE(ptr1, ptr3);

    // 全てこのアロケータが所有
    EXPECT_TRUE(allocator.Owns(ptr1));
    EXPECT_TRUE(allocator.Owns(ptr2));
    EXPECT_TRUE(allocator.Owns(ptr3));
}

TEST_F(LinearAllocatorTest, AllocateWithAlignment)
{
    Memory::LinearAllocator allocator(kCapacity);

    // アラインメントを崩す確保
    void* ptr1 = allocator.Allocate(1, 1);
    ASSERT_NE(ptr1, nullptr);

    // 8バイトアラインメントで確保
    void* ptr2 = allocator.Allocate(32, 8);
    ASSERT_NE(ptr2, nullptr);

    // ptr2はptr1より後ろにある
    EXPECT_GT(reinterpret_cast<uintptr_t>(ptr2), reinterpret_cast<uintptr_t>(ptr1));

    // オフセットがアラインメントを考慮している
    // 1バイト確保後、8バイト境界に揃えて確保されるので
    // 使用量は最低でも8+32=40バイト
    EXPECT_GE(allocator.GetUsed(), 40u);
}

TEST_F(LinearAllocatorTest, AllocateZeroReturnsNull)
{
    Memory::LinearAllocator allocator(kCapacity);

    void* ptr = allocator.Allocate(0, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LinearAllocatorTest, Reset)
{
    Memory::LinearAllocator allocator(kCapacity);

    allocator.Allocate(256, 8);
    allocator.Allocate(256, 8);
    EXPECT_GT(allocator.GetUsed(), 0u);

    allocator.Reset();
    EXPECT_EQ(allocator.GetUsed(), 0u);
    EXPECT_EQ(allocator.GetRemaining(), kCapacity);
}

TEST_F(LinearAllocatorTest, OwnsReturnsFalseForNull)
{
    Memory::LinearAllocator allocator(kCapacity);

    EXPECT_FALSE(allocator.Owns(nullptr));
}

TEST_F(LinearAllocatorTest, OwnsReturnsFalseForExternalPointer)
{
    Memory::LinearAllocator allocator(kCapacity);

    int stackVar = 0;
    EXPECT_FALSE(allocator.Owns(&stackVar));
}

TEST_F(LinearAllocatorTest, UsageRatio)
{
    Memory::LinearAllocator allocator(kCapacity);

    EXPECT_NEAR(allocator.GetUsageRatio(), 0.0f, 0.001f);

    // 半分使用
    allocator.Allocate(kCapacity / 2, 1);
    EXPECT_GT(allocator.GetUsageRatio(), 0.4f);
    EXPECT_LT(allocator.GetUsageRatio(), 0.6f);
}

TEST_F(LinearAllocatorTest, GetName)
{
    Memory::LinearAllocator allocator(kCapacity);
    EXPECT_STREQ(allocator.GetName(), "LinearAllocator");
}

TEST_F(LinearAllocatorTest, MoveConstruction)
{
    Memory::LinearAllocator allocator1(kCapacity);
    void* ptr = allocator1.Allocate(64, 8);
    ASSERT_NE(ptr, nullptr);

    Memory::LinearAllocator allocator2(std::move(allocator1));

    // 移動後はallocator2が所有
    EXPECT_TRUE(allocator2.Owns(ptr));
    EXPECT_EQ(allocator2.GetCapacity(), kCapacity);
}

//============================================================================
// ScopedLinearAllocator テスト
//============================================================================
TEST(ScopedLinearAllocatorTest, BasicUsage)
{
    Memory::ScopedLinearAllocator scoped(512);

    void* ptr = scoped.Allocate(64);
    ASSERT_NE(ptr, nullptr);

    EXPECT_TRUE(scoped.Get().Owns(ptr));
}

} // namespace
