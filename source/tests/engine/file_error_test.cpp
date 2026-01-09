//----------------------------------------------------------------------------
//! @file   file_error_test.cpp
//! @brief  FileErrorのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/fs/file_error.h"

namespace
{

//============================================================================
// FileError::Code テスト
//============================================================================
TEST(FileErrorCodeTest, NoneIsDefault)
{
    FileError error;
    EXPECT_EQ(error.code, FileError::Code::None);
}

TEST(FileErrorCodeTest, AllCodesAreDefined)
{
    // Verify all error codes are distinct
    EXPECT_NE(static_cast<int>(FileError::Code::None), static_cast<int>(FileError::Code::NotFound));
    EXPECT_NE(static_cast<int>(FileError::Code::NotFound), static_cast<int>(FileError::Code::AccessDenied));
    EXPECT_NE(static_cast<int>(FileError::Code::AccessDenied), static_cast<int>(FileError::Code::InvalidPath));
    EXPECT_NE(static_cast<int>(FileError::Code::InvalidPath), static_cast<int>(FileError::Code::InvalidMount));
    EXPECT_NE(static_cast<int>(FileError::Code::InvalidMount), static_cast<int>(FileError::Code::DiskFull));
    EXPECT_NE(static_cast<int>(FileError::Code::DiskFull), static_cast<int>(FileError::Code::AlreadyExists));
    EXPECT_NE(static_cast<int>(FileError::Code::AlreadyExists), static_cast<int>(FileError::Code::NotEmpty));
    EXPECT_NE(static_cast<int>(FileError::Code::NotEmpty), static_cast<int>(FileError::Code::IsDirectory));
    EXPECT_NE(static_cast<int>(FileError::Code::IsDirectory), static_cast<int>(FileError::Code::IsNotDirectory));
    EXPECT_NE(static_cast<int>(FileError::Code::IsNotDirectory), static_cast<int>(FileError::Code::PathTooLong));
    EXPECT_NE(static_cast<int>(FileError::Code::PathTooLong), static_cast<int>(FileError::Code::ReadOnly));
    EXPECT_NE(static_cast<int>(FileError::Code::ReadOnly), static_cast<int>(FileError::Code::Cancelled));
    EXPECT_NE(static_cast<int>(FileError::Code::Cancelled), static_cast<int>(FileError::Code::Unknown));
}

//============================================================================
// FileError isOk テスト
//============================================================================
TEST(FileErrorTest, IsOkReturnsTrueForNone)
{
    FileError error;
    error.code = FileError::Code::None;
    EXPECT_TRUE(error.isOk());
}

TEST(FileErrorTest, IsOkReturnsFalseForNotFound)
{
    FileError error;
    error.code = FileError::Code::NotFound;
    EXPECT_FALSE(error.isOk());
}

TEST(FileErrorTest, IsOkReturnsFalseForAccessDenied)
{
    FileError error;
    error.code = FileError::Code::AccessDenied;
    EXPECT_FALSE(error.isOk());
}

TEST(FileErrorTest, IsOkReturnsFalseForUnknown)
{
    FileError error;
    error.code = FileError::Code::Unknown;
    EXPECT_FALSE(error.isOk());
}

//============================================================================
// FileError make テスト
//============================================================================
TEST(FileErrorTest, MakeCreatesError)
{
    FileError error = FileError::make(FileError::Code::NotFound);
    EXPECT_EQ(error.code, FileError::Code::NotFound);
    EXPECT_EQ(error.nativeError, 0);
    EXPECT_TRUE(error.context.empty());
}

TEST(FileErrorTest, MakeWithNativeError)
{
    FileError error = FileError::make(FileError::Code::AccessDenied, 5);
    EXPECT_EQ(error.code, FileError::Code::AccessDenied);
    EXPECT_EQ(error.nativeError, 5);
    EXPECT_TRUE(error.context.empty());
}

TEST(FileErrorTest, MakeWithContext)
{
    FileError error = FileError::make(FileError::Code::InvalidPath, 0, "/invalid/path");
    EXPECT_EQ(error.code, FileError::Code::InvalidPath);
    EXPECT_EQ(error.nativeError, 0);
    EXPECT_EQ(error.context, "/invalid/path");
}

TEST(FileErrorTest, MakeWithAllParameters)
{
    FileError error = FileError::make(FileError::Code::DiskFull, 112, "C:/temp/file.txt");
    EXPECT_EQ(error.code, FileError::Code::DiskFull);
    EXPECT_EQ(error.nativeError, 112);
    EXPECT_EQ(error.context, "C:/temp/file.txt");
}

//============================================================================
// FileError message テスト
//============================================================================
TEST(FileErrorTest, MessageForNone)
{
    FileError error = FileError::make(FileError::Code::None);
    std::string msg = error.message();
    // Should contain something meaningful
    EXPECT_FALSE(msg.empty());
}

TEST(FileErrorTest, MessageForNotFound)
{
    FileError error = FileError::make(FileError::Code::NotFound, 0, "test.txt");
    std::string msg = error.message();
    EXPECT_FALSE(msg.empty());
    // Context should be included if present
}

TEST(FileErrorTest, MessageForUnknown)
{
    FileError error = FileError::make(FileError::Code::Unknown);
    std::string msg = error.message();
    EXPECT_FALSE(msg.empty());
}

//============================================================================
// FileErrorToString テスト
//============================================================================
TEST(FileErrorToStringTest, NoneReturnsValidString)
{
    const char* str = FileErrorToString(FileError::Code::None);
    EXPECT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

TEST(FileErrorToStringTest, NotFoundReturnsValidString)
{
    const char* str = FileErrorToString(FileError::Code::NotFound);
    EXPECT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

TEST(FileErrorToStringTest, AccessDeniedReturnsValidString)
{
    const char* str = FileErrorToString(FileError::Code::AccessDenied);
    EXPECT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

TEST(FileErrorToStringTest, InvalidPathReturnsValidString)
{
    const char* str = FileErrorToString(FileError::Code::InvalidPath);
    EXPECT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

TEST(FileErrorToStringTest, UnknownReturnsValidString)
{
    const char* str = FileErrorToString(FileError::Code::Unknown);
    EXPECT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

TEST(FileErrorToStringTest, AllCodesReturnUniqueStrings)
{
    // Each error code should have a distinct message
    std::set<std::string> messages;
    messages.insert(FileErrorToString(FileError::Code::None));
    messages.insert(FileErrorToString(FileError::Code::NotFound));
    messages.insert(FileErrorToString(FileError::Code::AccessDenied));
    messages.insert(FileErrorToString(FileError::Code::InvalidPath));
    messages.insert(FileErrorToString(FileError::Code::InvalidMount));
    messages.insert(FileErrorToString(FileError::Code::DiskFull));
    messages.insert(FileErrorToString(FileError::Code::AlreadyExists));
    messages.insert(FileErrorToString(FileError::Code::NotEmpty));
    messages.insert(FileErrorToString(FileError::Code::IsDirectory));
    messages.insert(FileErrorToString(FileError::Code::IsNotDirectory));
    messages.insert(FileErrorToString(FileError::Code::PathTooLong));
    messages.insert(FileErrorToString(FileError::Code::ReadOnly));
    messages.insert(FileErrorToString(FileError::Code::Cancelled));
    messages.insert(FileErrorToString(FileError::Code::Unknown));

    // All 14 error codes should have unique messages
    EXPECT_EQ(messages.size(), 14u);
}

//============================================================================
// FileError 構造体初期化テスト
//============================================================================
TEST(FileErrorTest, DefaultInitialization)
{
    FileError error;
    EXPECT_EQ(error.code, FileError::Code::None);
    EXPECT_EQ(error.nativeError, 0);
    EXPECT_TRUE(error.context.empty());
}

TEST(FileErrorTest, AggregateInitialization)
{
    FileError error{ FileError::Code::ReadOnly, 123, "readonly.txt" };
    EXPECT_EQ(error.code, FileError::Code::ReadOnly);
    EXPECT_EQ(error.nativeError, 123);
    EXPECT_EQ(error.context, "readonly.txt");
}

} // namespace
