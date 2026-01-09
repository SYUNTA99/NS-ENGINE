//----------------------------------------------------------------------------
//! @file   memory_file_system_test.cpp
//! @brief  MemoryFileSystemのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/fs/memory_file_system.h"
#include <string>

namespace
{

//============================================================================
// MemoryFileSystem テスト
//============================================================================
class MemoryFileSystemTest : public ::testing::Test
{
protected:
    MemoryFileSystem fs_;

    void SetUp() override {
        fs_.clear();
    }
};

TEST_F(MemoryFileSystemTest, InitiallyEmpty)
{
    EXPECT_FALSE(fs_.exists("test.txt"));
}

TEST_F(MemoryFileSystemTest, AddFileAndExists)
{
    std::vector<std::byte> data = { std::byte{0x41}, std::byte{0x42}, std::byte{0x43} };
    fs_.addFile("test.bin", data);

    EXPECT_TRUE(fs_.exists("test.bin"));
}

TEST_F(MemoryFileSystemTest, AddTextFileAndExists)
{
    fs_.addTextFile("hello.txt", "Hello, World!");

    EXPECT_TRUE(fs_.exists("hello.txt"));
}

TEST_F(MemoryFileSystemTest, GetFileSizeReturnsCorrectSize)
{
    fs_.addTextFile("test.txt", "12345");

    EXPECT_EQ(fs_.getFileSize("test.txt"), 5);
}

TEST_F(MemoryFileSystemTest, GetFileSizeReturnsMinusOneForMissing)
{
    EXPECT_EQ(fs_.getFileSize("missing.txt"), -1);
}

TEST_F(MemoryFileSystemTest, IsFileReturnsTrue)
{
    fs_.addTextFile("file.txt", "content");
    EXPECT_TRUE(fs_.isFile("file.txt"));
}

TEST_F(MemoryFileSystemTest, IsFileReturnsFalseForMissing)
{
    EXPECT_FALSE(fs_.isFile("missing.txt"));
}

TEST_F(MemoryFileSystemTest, IsDirectoryAlwaysFalse)
{
    // MemoryFileSystemはディレクトリをサポートしない
    fs_.addTextFile("file.txt", "content");
    EXPECT_FALSE(fs_.isDirectory("file.txt"));
    EXPECT_FALSE(fs_.isDirectory("somedir"));
}

TEST_F(MemoryFileSystemTest, ReadReturnsCorrectData)
{
    fs_.addTextFile("test.txt", "Hello!");

    auto result = fs_.read("test.txt");
    ASSERT_TRUE(result.success);

    std::string content(reinterpret_cast<const char*>(result.bytes.data()), result.bytes.size());
    EXPECT_EQ(content, "Hello!");
}

TEST_F(MemoryFileSystemTest, ReadFailsForMissingFile)
{
    auto result = fs_.read("missing.txt");
    EXPECT_FALSE(result.success);
}

TEST_F(MemoryFileSystemTest, OpenReturnsValidHandle)
{
    fs_.addTextFile("test.txt", "content");

    auto handle = fs_.open("test.txt");
    EXPECT_NE(handle, nullptr);
}

TEST_F(MemoryFileSystemTest, OpenReturnsNullForMissing)
{
    auto handle = fs_.open("missing.txt");
    EXPECT_EQ(handle, nullptr);
}

TEST_F(MemoryFileSystemTest, ClearRemovesAllFiles)
{
    fs_.addTextFile("file1.txt", "a");
    fs_.addTextFile("file2.txt", "b");

    fs_.clear();

    EXPECT_FALSE(fs_.exists("file1.txt"));
    EXPECT_FALSE(fs_.exists("file2.txt"));
}

TEST_F(MemoryFileSystemTest, BinaryDataPreserved)
{
    std::vector<std::byte> original = {
        std::byte{0x00}, std::byte{0xFF},
        std::byte{0x12}, std::byte{0x34}
    };
    fs_.addFile("binary.dat", original);

    auto result = fs_.read("binary.dat");
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.bytes.size(), 4);

    EXPECT_EQ(result.bytes[0], std::byte{0x00});
    EXPECT_EQ(result.bytes[1], std::byte{0xFF});
    EXPECT_EQ(result.bytes[2], std::byte{0x12});
    EXPECT_EQ(result.bytes[3], std::byte{0x34});
}

TEST_F(MemoryFileSystemTest, OverwriteExistingFile)
{
    fs_.addTextFile("test.txt", "original");
    fs_.addTextFile("test.txt", "updated");

    auto result = fs_.read("test.txt");
    ASSERT_TRUE(result.success);

    std::string content(reinterpret_cast<const char*>(result.bytes.data()), result.bytes.size());
    EXPECT_EQ(content, "updated");
}

TEST_F(MemoryFileSystemTest, GetFreeSpaceSizeReturnsMax)
{
    // メモリファイルシステムは実質無制限
    EXPECT_GT(fs_.getFreeSpaceSize(), 0);
}

} // namespace
