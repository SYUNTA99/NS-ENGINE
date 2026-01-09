//----------------------------------------------------------------------------
//! @file   file_watcher_types_test.cpp
//! @brief  ファイル監視関連型のテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/fs/file_watcher.h"

namespace
{

//============================================================================
// FileChangeType enum テスト
//============================================================================
TEST(FileChangeTypeTest, ModifiedIsDefined)
{
    FileChangeType type = FileChangeType::Modified;
    EXPECT_EQ(type, FileChangeType::Modified);
}

TEST(FileChangeTypeTest, CreatedIsDefined)
{
    FileChangeType type = FileChangeType::Created;
    EXPECT_EQ(type, FileChangeType::Created);
}

TEST(FileChangeTypeTest, DeletedIsDefined)
{
    FileChangeType type = FileChangeType::Deleted;
    EXPECT_EQ(type, FileChangeType::Deleted);
}

TEST(FileChangeTypeTest, RenamedIsDefined)
{
    FileChangeType type = FileChangeType::Renamed;
    EXPECT_EQ(type, FileChangeType::Renamed);
}

TEST(FileChangeTypeTest, AllTypesAreDistinct)
{
    EXPECT_NE(FileChangeType::Modified, FileChangeType::Created);
    EXPECT_NE(FileChangeType::Created, FileChangeType::Deleted);
    EXPECT_NE(FileChangeType::Deleted, FileChangeType::Renamed);
    EXPECT_NE(FileChangeType::Renamed, FileChangeType::Modified);
}

//============================================================================
// FileChangeEvent テスト
//============================================================================
TEST(FileChangeEventTest, CanSetType)
{
    FileChangeEvent event;
    event.type = FileChangeType::Modified;
    EXPECT_EQ(event.type, FileChangeType::Modified);
}

TEST(FileChangeEventTest, CanSetPath)
{
    FileChangeEvent event;
    event.path = L"C:\\test\\file.txt";
    EXPECT_EQ(event.path, L"C:\\test\\file.txt");
}

TEST(FileChangeEventTest, CanSetOldPath)
{
    FileChangeEvent event;
    event.type = FileChangeType::Renamed;
    event.path = L"C:\\test\\newname.txt";
    event.oldPath = L"C:\\test\\oldname.txt";
    EXPECT_EQ(event.oldPath, L"C:\\test\\oldname.txt");
}

TEST(FileChangeEventTest, OldPathEmptyForNonRename)
{
    FileChangeEvent event;
    event.type = FileChangeType::Modified;
    event.path = L"C:\\test\\file.txt";
    event.oldPath = L"";
    EXPECT_TRUE(event.oldPath.empty());
}

TEST(FileChangeEventTest, PathWithJapaneseCharacters)
{
    FileChangeEvent event;
    event.path = L"C:\\テスト\\ファイル.txt";
    EXPECT_EQ(event.path, L"C:\\テスト\\ファイル.txt");
}

//============================================================================
// FileWatcher 基本テスト（状態のみ、I/Oなし）
//============================================================================
TEST(FileWatcherTest, DefaultNotWatching)
{
    FileWatcher watcher;
    EXPECT_FALSE(watcher.isWatching());
}

TEST(FileWatcherTest, DefaultWatchPathIsEmpty)
{
    FileWatcher watcher;
    EXPECT_TRUE(watcher.getWatchPath().empty());
}

TEST(FileWatcherTest, IsNotCopyConstructible)
{
    EXPECT_FALSE(std::is_copy_constructible_v<FileWatcher>);
}

TEST(FileWatcherTest, IsNotCopyAssignable)
{
    EXPECT_FALSE(std::is_copy_assignable_v<FileWatcher>);
}

TEST(FileWatcherTest, IsMoveConstructible)
{
    EXPECT_TRUE(std::is_move_constructible_v<FileWatcher>);
}

TEST(FileWatcherTest, IsMoveAssignable)
{
    EXPECT_TRUE(std::is_move_assignable_v<FileWatcher>);
}

} // namespace
