//----------------------------------------------------------------------------
//! @file   file_system_types_test.cpp
//! @brief  ファイルシステム関連型のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/fs/file_system_types.h"
#include <thread>
#include <chrono>

namespace
{

//============================================================================
// 定数テスト
//============================================================================
TEST(FileSystemConstantsTest, MountNameLengthMax)
{
    EXPECT_EQ(MountNameLengthMax, 15);
}

TEST(FileSystemConstantsTest, PathLengthMax)
{
    EXPECT_EQ(PathLengthMax, 260);
}

//============================================================================
// FileReadResult テスト
//============================================================================
TEST(FileReadResultTest, DefaultSuccessIsFalse)
{
    FileReadResult result{};
    EXPECT_FALSE(result.success);
}

TEST(FileReadResultTest, DefaultBytesIsEmpty)
{
    FileReadResult result{};
    EXPECT_TRUE(result.bytes.empty());
}

TEST(FileReadResultTest, DefaultErrorIsOk)
{
    FileReadResult result{};
    EXPECT_TRUE(result.error.isOk());
}

TEST(FileReadResultTest, ErrorMessageReturnsErrorMessage)
{
    FileReadResult result{};
    result.error = FileError::make(FileError::Code::NotFound, 0, "test.txt");
    EXPECT_EQ(result.errorMessage(), "NotFound: test.txt");
}

TEST(FileReadResultTest, CanSetSuccess)
{
    FileReadResult result{};
    result.success = true;
    EXPECT_TRUE(result.success);
}

TEST(FileReadResultTest, CanSetBytes)
{
    FileReadResult result{};
    result.bytes.push_back(std::byte{0x42});
    result.bytes.push_back(std::byte{0x43});
    EXPECT_EQ(result.bytes.size(), 2u);
    EXPECT_EQ(result.bytes[0], std::byte{0x42});
}

//============================================================================
// FileOperationResult テスト
//============================================================================
TEST(FileOperationResultTest, DefaultSuccessIsFalse)
{
    FileOperationResult result{};
    EXPECT_FALSE(result.success);
}

TEST(FileOperationResultTest, DefaultErrorIsOk)
{
    FileOperationResult result{};
    EXPECT_TRUE(result.error.isOk());
}

TEST(FileOperationResultTest, ErrorMessageReturnsErrorMessage)
{
    FileOperationResult result{};
    result.error = FileError::make(FileError::Code::AccessDenied, 0, "secret.txt");
    EXPECT_EQ(result.errorMessage(), "AccessDenied: secret.txt");
}

TEST(FileOperationResultTest, CanSetSuccess)
{
    FileOperationResult result{};
    result.success = true;
    EXPECT_TRUE(result.success);
}

//============================================================================
// FileEntryType enum テスト
//============================================================================
TEST(FileEntryTypeTest, FileIsDefined)
{
    FileEntryType type = FileEntryType::File;
    EXPECT_EQ(type, FileEntryType::File);
}

TEST(FileEntryTypeTest, DirectoryIsDefined)
{
    FileEntryType type = FileEntryType::Directory;
    EXPECT_EQ(type, FileEntryType::Directory);
}

TEST(FileEntryTypeTest, FileAndDirectoryAreDistinct)
{
    EXPECT_NE(FileEntryType::File, FileEntryType::Directory);
}

//============================================================================
// DirectoryEntry テスト
//============================================================================
TEST(DirectoryEntryTest, DefaultNameIsEmpty)
{
    DirectoryEntry entry{};
    EXPECT_TRUE(entry.name.empty());
}

TEST(DirectoryEntryTest, DefaultSizeIsZero)
{
    DirectoryEntry entry{};
    EXPECT_EQ(entry.size, 0);
}

TEST(DirectoryEntryTest, CanSetName)
{
    DirectoryEntry entry{};
    entry.name = "test.txt";
    EXPECT_EQ(entry.name, "test.txt");
}

TEST(DirectoryEntryTest, CanSetTypeAsFile)
{
    DirectoryEntry entry{};
    entry.type = FileEntryType::File;
    EXPECT_EQ(entry.type, FileEntryType::File);
}

TEST(DirectoryEntryTest, CanSetTypeAsDirectory)
{
    DirectoryEntry entry{};
    entry.type = FileEntryType::Directory;
    EXPECT_EQ(entry.type, FileEntryType::Directory);
}

TEST(DirectoryEntryTest, CanSetSize)
{
    DirectoryEntry entry{};
    entry.size = 1024;
    EXPECT_EQ(entry.size, 1024);
}

TEST(DirectoryEntryTest, CanSetLargeSize)
{
    DirectoryEntry entry{};
    entry.size = INT64_MAX;
    EXPECT_EQ(entry.size, INT64_MAX);
}

//============================================================================
// AsyncReadState enum テスト
//============================================================================
TEST(AsyncReadStateTest, PendingIsDefined)
{
    AsyncReadState state = AsyncReadState::Pending;
    EXPECT_EQ(state, AsyncReadState::Pending);
}

TEST(AsyncReadStateTest, RunningIsDefined)
{
    AsyncReadState state = AsyncReadState::Running;
    EXPECT_EQ(state, AsyncReadState::Running);
}

TEST(AsyncReadStateTest, CompletedIsDefined)
{
    AsyncReadState state = AsyncReadState::Completed;
    EXPECT_EQ(state, AsyncReadState::Completed);
}

TEST(AsyncReadStateTest, CancelledIsDefined)
{
    AsyncReadState state = AsyncReadState::Cancelled;
    EXPECT_EQ(state, AsyncReadState::Cancelled);
}

TEST(AsyncReadStateTest, FailedIsDefined)
{
    AsyncReadState state = AsyncReadState::Failed;
    EXPECT_EQ(state, AsyncReadState::Failed);
}

TEST(AsyncReadStateTest, AllStatesAreDistinct)
{
    EXPECT_NE(AsyncReadState::Pending, AsyncReadState::Running);
    EXPECT_NE(AsyncReadState::Running, AsyncReadState::Completed);
    EXPECT_NE(AsyncReadState::Completed, AsyncReadState::Cancelled);
    EXPECT_NE(AsyncReadState::Cancelled, AsyncReadState::Failed);
    EXPECT_NE(AsyncReadState::Failed, AsyncReadState::Pending);
}

//============================================================================
// AsyncReadHandle テスト
//============================================================================
TEST(AsyncReadHandleTest, DefaultConstructedIsNotValid)
{
    AsyncReadHandle handle;
    EXPECT_FALSE(handle.isValid());
}

TEST(AsyncReadHandleTest, DefaultConstructedGetStateFailed)
{
    AsyncReadHandle handle;
    EXPECT_EQ(handle.getState(), AsyncReadState::Failed);
}

TEST(AsyncReadHandleTest, DefaultConstructedIsReady)
{
    AsyncReadHandle handle;
    EXPECT_TRUE(handle.isReady());
}

TEST(AsyncReadHandleTest, DefaultConstructedGetReturnsError)
{
    AsyncReadHandle handle;
    FileReadResult result = handle.get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.isOk());
}

TEST(AsyncReadHandleTest, DefaultConstructedCancellationNotRequested)
{
    AsyncReadHandle handle;
    EXPECT_FALSE(handle.isCancellationRequested());
}

TEST(AsyncReadHandleTest, ConstructedWithFutureIsValid)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    EXPECT_TRUE(handle.isValid());
}

TEST(AsyncReadHandleTest, ConstructedWithFutureInitialStateIsRunning)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    EXPECT_EQ(handle.getState(), AsyncReadState::Running);
}

TEST(AsyncReadHandleTest, NotReadyBeforeCompletion)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    EXPECT_FALSE(handle.isReady());
}

TEST(AsyncReadHandleTest, IsReadyAfterCompletion)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult result;
    result.success = true;
    promise.set_value(result);

    EXPECT_TRUE(handle.isReady());
}

TEST(AsyncReadHandleTest, GetReturnsResultAfterCompletion)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult expected;
    expected.success = true;
    expected.bytes.push_back(std::byte{0x42});
    promise.set_value(expected);

    FileReadResult actual = handle.get();
    EXPECT_TRUE(actual.success);
    EXPECT_EQ(actual.bytes.size(), 1u);
    EXPECT_EQ(actual.bytes[0], std::byte{0x42});
}

TEST(AsyncReadHandleTest, GetCanBeCalledMultipleTimes)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult expected;
    expected.success = true;
    promise.set_value(expected);

    FileReadResult result1 = handle.get();
    FileReadResult result2 = handle.get();
    FileReadResult result3 = handle.get();

    EXPECT_TRUE(result1.success);
    EXPECT_TRUE(result2.success);
    EXPECT_TRUE(result3.success);
}

TEST(AsyncReadHandleTest, StateChangesToCompletedOnSuccess)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult result;
    result.success = true;
    promise.set_value(result);

    handle.get();  // Trigger state transition
    EXPECT_EQ(handle.getState(), AsyncReadState::Completed);
}

TEST(AsyncReadHandleTest, StateChangesToFailedOnFailure)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult result;
    result.success = false;
    result.error = FileError::make(FileError::Code::NotFound, 0, "Not found");
    promise.set_value(result);

    handle.get();  // Trigger state transition
    EXPECT_EQ(handle.getState(), AsyncReadState::Failed);
}

TEST(AsyncReadHandleTest, RequestCancellationSetsCancellationFlag)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    EXPECT_FALSE(handle.isCancellationRequested());
    handle.requestCancellation();
    EXPECT_TRUE(handle.isCancellationRequested());
}

TEST(AsyncReadHandleTest, RequestCancellationChangesStateToCancelled)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    handle.requestCancellation();
    EXPECT_EQ(handle.getState(), AsyncReadState::Cancelled);
}

TEST(AsyncReadHandleTest, GetAfterCancellationReturnsError)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    handle.requestCancellation();

    FileReadResult expected;
    expected.success = true;  // Original result was success
    promise.set_value(expected);

    FileReadResult result = handle.get();
    EXPECT_FALSE(result.success);  // But cancellation overrides it
    EXPECT_EQ(result.error.code, FileError::Code::Cancelled);
}

TEST(AsyncReadHandleTest, GetForReturnsNulloptBeforeCompletion)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    auto result = handle.getFor(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}

TEST(AsyncReadHandleTest, GetForReturnsValueAfterCompletion)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    FileReadResult expected;
    expected.success = true;
    promise.set_value(expected);

    auto result = handle.getFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
}

TEST(AsyncReadHandleTest, GetCancellationTokenReturnsValidToken)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    auto token = handle.getCancellationToken();
    EXPECT_NE(token, nullptr);
    EXPECT_FALSE(token->load());
}

TEST(AsyncReadHandleTest, CancellationTokenIsSharedWithHandle)
{
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future));

    auto token = handle.getCancellationToken();
    handle.requestCancellation();
    EXPECT_TRUE(token->load());
}

TEST(AsyncReadHandleTest, ExternalCancellationTokenWorks)
{
    auto externalToken = std::make_shared<std::atomic<bool>>(false);
    std::promise<FileReadResult> promise;
    std::future<FileReadResult> future = promise.get_future();
    AsyncReadHandle handle(std::move(future), externalToken);

    externalToken->store(true);
    EXPECT_TRUE(handle.isCancellationRequested());
}

} // namespace
