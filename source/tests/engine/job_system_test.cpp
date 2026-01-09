//----------------------------------------------------------------------------
//! @file   job_system_test.cpp
//! @brief  JobSystem関連クラスのテスト（CancelToken, JobCounter, JobHandle, JobDesc）
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/core/job_system.h"
#include <thread>

namespace
{

//============================================================================
// CancelToken テスト
//============================================================================
TEST(CancelTokenTest, InitiallyNotCancelled)
{
    CancelToken token;
    EXPECT_FALSE(token.IsCancelled());
}

TEST(CancelTokenTest, CancelSetsCancelledFlag)
{
    CancelToken token;
    token.Cancel();
    EXPECT_TRUE(token.IsCancelled());
}

TEST(CancelTokenTest, ResetClearsCancelledFlag)
{
    CancelToken token;
    token.Cancel();
    EXPECT_TRUE(token.IsCancelled());

    token.Reset();
    EXPECT_FALSE(token.IsCancelled());
}

TEST(CancelTokenTest, MultipleCancelsAreIdempotent)
{
    CancelToken token;
    token.Cancel();
    token.Cancel();
    token.Cancel();
    EXPECT_TRUE(token.IsCancelled());
}

TEST(CancelTokenTest, SharedPtrUsage)
{
    auto token = std::make_shared<CancelToken>();
    EXPECT_FALSE(token->IsCancelled());

    token->Cancel();
    EXPECT_TRUE(token->IsCancelled());
}

TEST(CancelTokenTest, MakeCancelTokenHelper)
{
    auto token = MakeCancelToken();
    ASSERT_NE(token, nullptr);
    EXPECT_FALSE(token->IsCancelled());
}

//============================================================================
// JobPriority テスト
//============================================================================
TEST(JobPriorityTest, HighIsZero)
{
    EXPECT_EQ(static_cast<uint8_t>(JobPriority::High), 0);
}

TEST(JobPriorityTest, NormalIsOne)
{
    EXPECT_EQ(static_cast<uint8_t>(JobPriority::Normal), 1);
}

TEST(JobPriorityTest, LowIsTwo)
{
    EXPECT_EQ(static_cast<uint8_t>(JobPriority::Low), 2);
}

TEST(JobPriorityTest, CountIsThree)
{
    EXPECT_EQ(static_cast<uint8_t>(JobPriority::Count), 3);
}

TEST(JobPriorityTest, HighHasHighestPriority)
{
    EXPECT_LT(static_cast<uint8_t>(JobPriority::High),
              static_cast<uint8_t>(JobPriority::Normal));
    EXPECT_LT(static_cast<uint8_t>(JobPriority::Normal),
              static_cast<uint8_t>(JobPriority::Low));
}

//============================================================================
// JobResult テスト
//============================================================================
TEST(JobResultTest, PendingIsZero)
{
    EXPECT_EQ(static_cast<uint8_t>(JobResult::Pending), 0);
}

TEST(JobResultTest, SuccessIsOne)
{
    EXPECT_EQ(static_cast<uint8_t>(JobResult::Success), 1);
}

TEST(JobResultTest, CancelledIsTwo)
{
    EXPECT_EQ(static_cast<uint8_t>(JobResult::Cancelled), 2);
}

TEST(JobResultTest, ExceptionIsThree)
{
    EXPECT_EQ(static_cast<uint8_t>(JobResult::Exception), 3);
}

//============================================================================
// JobCounter テスト
//============================================================================
TEST(JobCounterTest, DefaultConstructorStartsAtZero)
{
    JobCounter counter;
    EXPECT_EQ(counter.GetCount(), 0u);
    EXPECT_TRUE(counter.IsComplete());
}

TEST(JobCounterTest, ConstructorWithInitialCount)
{
    JobCounter counter(5);
    EXPECT_EQ(counter.GetCount(), 5u);
    EXPECT_FALSE(counter.IsComplete());
}

TEST(JobCounterTest, IncrementIncreasesCount)
{
    JobCounter counter;
    counter.Increment();
    EXPECT_EQ(counter.GetCount(), 1u);
    counter.Increment();
    EXPECT_EQ(counter.GetCount(), 2u);
}

TEST(JobCounterTest, DecrementDecreasesCount)
{
    JobCounter counter(3);
    counter.Decrement();
    EXPECT_EQ(counter.GetCount(), 2u);
    counter.Decrement();
    EXPECT_EQ(counter.GetCount(), 1u);
}

TEST(JobCounterTest, IsCompleteWhenCountReachesZero)
{
    JobCounter counter(2);
    EXPECT_FALSE(counter.IsComplete());

    counter.Decrement();
    EXPECT_FALSE(counter.IsComplete());

    counter.Decrement();
    EXPECT_TRUE(counter.IsComplete());
}

TEST(JobCounterTest, ResetSetsNewCount)
{
    JobCounter counter(5);
    counter.Decrement();
    counter.Decrement();

    counter.Reset(10);
    EXPECT_EQ(counter.GetCount(), 10u);
}

TEST(JobCounterTest, DefaultResultIsPending)
{
    JobCounter counter;
    EXPECT_EQ(counter.GetResult(), JobResult::Pending);
}

TEST(JobCounterTest, SetResultChangesResult)
{
    JobCounter counter;
    counter.SetResult(JobResult::Success);
    EXPECT_EQ(counter.GetResult(), JobResult::Success);
}

TEST(JobCounterTest, SetResultToCancelled)
{
    JobCounter counter;
    counter.SetResult(JobResult::Cancelled);
    EXPECT_EQ(counter.GetResult(), JobResult::Cancelled);
}

TEST(JobCounterTest, SetResultToException)
{
    JobCounter counter;
    counter.SetResult(JobResult::Exception);
    EXPECT_EQ(counter.GetResult(), JobResult::Exception);
}

//============================================================================
// JobHandle テスト
//============================================================================
TEST(JobHandleTest, DefaultConstructorIsInvalid)
{
    JobHandle handle;
    EXPECT_FALSE(handle.IsValid());
}

TEST(JobHandleTest, InvalidHandleIsNotComplete)
{
    JobHandle handle;
    EXPECT_FALSE(handle.IsComplete());
}

TEST(JobHandleTest, InvalidHandleResultIsPending)
{
    JobHandle handle;
    EXPECT_EQ(handle.GetResult(), JobResult::Pending);
}

TEST(JobHandleTest, InvalidHandleHasNoError)
{
    JobHandle handle;
    EXPECT_FALSE(handle.HasError());
}

TEST(JobHandleTest, InvalidHandleIsNotSuccess)
{
    JobHandle handle;
    EXPECT_FALSE(handle.IsSuccess());
}

TEST(JobHandleTest, WaitOnInvalidHandleDoesNothing)
{
    JobHandle handle;
    handle.Wait();  // Should not block or crash
    SUCCEED();
}

//============================================================================
// JobDesc テスト
//============================================================================
TEST(JobDescTest, DefaultConstructor)
{
    JobDesc desc;
    // Should not crash
    SUCCEED();
}

TEST(JobDescTest, ConstructorWithFunction)
{
    bool executed = false;
    JobDesc desc([&executed]() { executed = true; });
    // Function is stored but not executed
    EXPECT_FALSE(executed);
}

TEST(JobDescTest, SetPriorityReturnsReference)
{
    JobDesc desc;
    JobDesc& result = desc.SetPriority(JobPriority::High);
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescTest, SetMainThreadOnlyReturnsReference)
{
    JobDesc desc;
    JobDesc& result = desc.SetMainThreadOnly(true);
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescTest, SetCancelTokenReturnsReference)
{
    JobDesc desc;
    auto token = MakeCancelToken();
    JobDesc& result = desc.SetCancelToken(token);
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescTest, SetNameReturnsReference)
{
    JobDesc desc;
    JobDesc& result = desc.SetName("TestJob");
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescTest, ChainedBuilderPattern)
{
    auto token = MakeCancelToken();

    JobDesc desc;
    desc.SetPriority(JobPriority::High)
        .SetMainThreadOnly(true)
        .SetCancelToken(token)
        .SetName("ChainedJob");

    SUCCEED();
}

//============================================================================
// JobDesc ファクトリ関数テスト
//============================================================================
TEST(JobDescFactoryTest, MainThread)
{
    bool executed = false;
    auto desc = JobDesc::MainThread([&executed]() { executed = true; });
    // Factory creates valid desc
    EXPECT_FALSE(executed);
}

TEST(JobDescFactoryTest, HighPriority)
{
    auto desc = JobDesc::HighPriority([]() {});
    // Factory creates valid desc
    SUCCEED();
}

TEST(JobDescFactoryTest, LowPriority)
{
    auto desc = JobDesc::LowPriority([]() {});
    // Factory creates valid desc
    SUCCEED();
}

TEST(JobDescFactoryTest, After)
{
    JobHandle dependency;  // Invalid handle
    auto desc = JobDesc::After(dependency, []() {});
    // Factory creates valid desc even with invalid dependency
    SUCCEED();
}

TEST(JobDescFactoryTest, AfterAll)
{
    std::vector<JobHandle> dependencies;
    auto desc = JobDesc::AfterAll(dependencies, []() {});
    // Factory creates valid desc
    SUCCEED();
}

TEST(JobDescFactoryTest, CancellableWithoutTokenOutput)
{
    auto desc = JobDesc::Cancellable([](const CancelToken& ct) {
        (void)ct;
    });
    SUCCEED();
}

TEST(JobDescFactoryTest, CancellableWithTokenOutput)
{
    CancelTokenPtr token;
    auto desc = JobDesc::Cancellable([](const CancelToken& ct) {
        (void)ct;
    }, &token);

    ASSERT_NE(token, nullptr);
    EXPECT_FALSE(token->IsCancelled());

    // Can cancel through output token
    token->Cancel();
    EXPECT_TRUE(token->IsCancelled());
}

//============================================================================
// JobDesc 依存関係テスト
//============================================================================
TEST(JobDescDependencyTest, AddDependencyWithInvalidHandle)
{
    JobHandle invalid;
    JobDesc desc;
    JobDesc& result = desc.AddDependency(invalid);
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescDependencyTest, AddDependenciesWithEmptyVector)
{
    std::vector<JobHandle> deps;
    JobDesc desc;
    JobDesc& result = desc.AddDependencies(deps);
    EXPECT_EQ(&result, &desc);
}

TEST(JobDescDependencyTest, AddMultipleInvalidDependencies)
{
    std::vector<JobHandle> deps(5);  // 5 invalid handles
    JobDesc desc;
    desc.AddDependencies(deps);
    SUCCEED();
}

} // namespace
