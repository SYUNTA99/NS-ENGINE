//----------------------------------------------------------------------------
//! @file   path_utility_test.cpp
//! @brief  PathUtilityのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/fs/path_utility.h"

namespace
{

//============================================================================
// getFileName テスト
//============================================================================
TEST(PathUtilityTest, GetFileNameSimple)
{
    EXPECT_EQ(PathUtility::getFileName("file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetFileNameWithPath)
{
    EXPECT_EQ(PathUtility::getFileName("dir/file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetFileNameWithDeepPath)
{
    EXPECT_EQ(PathUtility::getFileName("a/b/c/file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetFileNameWithBackslash)
{
    EXPECT_EQ(PathUtility::getFileName("dir\\file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetFileNameWithMountPath)
{
    EXPECT_EQ(PathUtility::getFileName("assets:/dir/file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetFileNameMountPathOnly)
{
    EXPECT_EQ(PathUtility::getFileName("assets:/file.txt"), "file.txt");
}

//============================================================================
// getExtension テスト
//============================================================================
TEST(PathUtilityTest, GetExtensionSimple)
{
    EXPECT_EQ(PathUtility::getExtension("file.txt"), ".txt");
}

TEST(PathUtilityTest, GetExtensionWithPath)
{
    EXPECT_EQ(PathUtility::getExtension("dir/file.png"), ".png");
}

TEST(PathUtilityTest, GetExtensionNoExtension)
{
    EXPECT_EQ(PathUtility::getExtension("file"), "");
}

TEST(PathUtilityTest, GetExtensionHiddenFile)
{
    EXPECT_EQ(PathUtility::getExtension(".gitignore"), "");
}

TEST(PathUtilityTest, GetExtensionDoubleExtension)
{
    EXPECT_EQ(PathUtility::getExtension("file.tar.gz"), ".gz");
}

TEST(PathUtilityTest, GetExtensionMountPath)
{
    EXPECT_EQ(PathUtility::getExtension("assets:/shaders/vs.hlsl"), ".hlsl");
}

//============================================================================
// getParentPath テスト
//============================================================================
TEST(PathUtilityTest, GetParentPathSimple)
{
    EXPECT_EQ(PathUtility::getParentPath("dir/file.txt"), "dir");
}

TEST(PathUtilityTest, GetParentPathDeep)
{
    EXPECT_EQ(PathUtility::getParentPath("a/b/c/file.txt"), "a/b/c");
}

TEST(PathUtilityTest, GetParentPathNoParent)
{
    EXPECT_EQ(PathUtility::getParentPath("file.txt"), "");
}

TEST(PathUtilityTest, GetParentPathEmpty)
{
    EXPECT_EQ(PathUtility::getParentPath(""), "");
}

TEST(PathUtilityTest, GetParentPathMountRoot)
{
    EXPECT_EQ(PathUtility::getParentPath("assets:/"), "");
}

TEST(PathUtilityTest, GetParentPathMountFile)
{
    EXPECT_EQ(PathUtility::getParentPath("assets:/file.txt"), "assets:/");
}

TEST(PathUtilityTest, GetParentPathMountDeep)
{
    EXPECT_EQ(PathUtility::getParentPath("assets:/dir/file.txt"), "assets:/dir");
}

TEST(PathUtilityTest, GetParentPathRootSlash)
{
    EXPECT_EQ(PathUtility::getParentPath("/"), "");
}

TEST(PathUtilityTest, GetParentPathRootFile)
{
    EXPECT_EQ(PathUtility::getParentPath("/file.txt"), "/");
}

//============================================================================
// combine テスト
//============================================================================
TEST(PathUtilityTest, CombineSimple)
{
    EXPECT_EQ(PathUtility::combine("dir", "file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, CombineWithTrailingSlash)
{
    EXPECT_EQ(PathUtility::combine("dir/", "file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, CombineWithTrailingBackslash)
{
    EXPECT_EQ(PathUtility::combine("dir\\", "file.txt"), "dir\\file.txt");
}

TEST(PathUtilityTest, CombineEmptyBase)
{
    EXPECT_EQ(PathUtility::combine("", "file.txt"), "file.txt");
}

TEST(PathUtilityTest, CombineEmptyRelative)
{
    EXPECT_EQ(PathUtility::combine("dir", ""), "dir");
}

TEST(PathUtilityTest, CombineBothEmpty)
{
    EXPECT_EQ(PathUtility::combine("", ""), "");
}

TEST(PathUtilityTest, CombineMountPath)
{
    EXPECT_EQ(PathUtility::combine("assets:/", "file.txt"), "assets:/file.txt");
}

TEST(PathUtilityTest, CombineDeep)
{
    EXPECT_EQ(PathUtility::combine("a/b/c", "d/e.txt"), "a/b/c/d/e.txt");
}

//============================================================================
// getMountName テスト
//============================================================================
TEST(PathUtilityTest, GetMountNameSimple)
{
    EXPECT_EQ(PathUtility::getMountName("assets:/file.txt"), "assets");
}

TEST(PathUtilityTest, GetMountNameWithPath)
{
    EXPECT_EQ(PathUtility::getMountName("shaders:/vs/basic.hlsl"), "shaders");
}

TEST(PathUtilityTest, GetMountNameNoMount)
{
    EXPECT_EQ(PathUtility::getMountName("dir/file.txt"), "");
}

TEST(PathUtilityTest, GetMountNameEmpty)
{
    EXPECT_EQ(PathUtility::getMountName(""), "");
}

//============================================================================
// getRelativePath テスト
//============================================================================
TEST(PathUtilityTest, GetRelativePathSimple)
{
    EXPECT_EQ(PathUtility::getRelativePath("assets:/file.txt"), "file.txt");
}

TEST(PathUtilityTest, GetRelativePathWithDir)
{
    EXPECT_EQ(PathUtility::getRelativePath("assets:/dir/file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, GetRelativePathNoMount)
{
    EXPECT_EQ(PathUtility::getRelativePath("dir/file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, GetRelativePathMountOnly)
{
    EXPECT_EQ(PathUtility::getRelativePath("assets:/"), "");
}

//============================================================================
// normalize テスト
//============================================================================
TEST(PathUtilityTest, NormalizeBackslash)
{
    EXPECT_EQ(PathUtility::normalize("dir\\file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, NormalizeDoubleSlash)
{
    EXPECT_EQ(PathUtility::normalize("dir//file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, NormalizeDot)
{
    EXPECT_EQ(PathUtility::normalize("dir/./file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, NormalizeDotDot)
{
    EXPECT_EQ(PathUtility::normalize("dir/sub/../file.txt"), "dir/file.txt");
}

TEST(PathUtilityTest, NormalizeDotDotAtRoot)
{
    // Security: ".." at root is ignored
    EXPECT_EQ(PathUtility::normalize("/../file.txt"), "/file.txt");
}

TEST(PathUtilityTest, NormalizeMountPathDotDot)
{
    // Security: ".." cannot escape mount root
    EXPECT_EQ(PathUtility::normalize("assets:/../etc/passwd"), "assets:/etc/passwd");
}

TEST(PathUtilityTest, NormalizeMountPathComplex)
{
    EXPECT_EQ(PathUtility::normalize("assets:/a/b/../c/./d//e"), "assets:/a/c/d/e");
}

TEST(PathUtilityTest, NormalizeEmpty)
{
    EXPECT_EQ(PathUtility::normalize(""), "");
}

TEST(PathUtilityTest, NormalizeMultipleDotDot)
{
    EXPECT_EQ(PathUtility::normalize("a/b/c/../../d"), "a/d");
}

//============================================================================
// equals テスト
//============================================================================
TEST(PathUtilityTest, EqualsSamePath)
{
    EXPECT_TRUE(PathUtility::equals("dir/file.txt", "dir/file.txt"));
}

TEST(PathUtilityTest, EqualsNormalizedPath)
{
    EXPECT_TRUE(PathUtility::equals("dir//file.txt", "dir/file.txt"));
}

TEST(PathUtilityTest, EqualsDifferentPath)
{
    EXPECT_FALSE(PathUtility::equals("dir/file1.txt", "dir/file2.txt"));
}

TEST(PathUtilityTest, EqualsWithDotDot)
{
    EXPECT_TRUE(PathUtility::equals("dir/sub/../file.txt", "dir/file.txt"));
}

//============================================================================
// isAbsolute テスト
//============================================================================
TEST(PathUtilityTest, IsAbsoluteDriveLetter)
{
    EXPECT_TRUE(PathUtility::isAbsolute("C:/Users/test"));
}

TEST(PathUtilityTest, IsAbsoluteDriveLetterBackslash)
{
    EXPECT_TRUE(PathUtility::isAbsolute("D:\\Projects\\test"));
}

TEST(PathUtilityTest, IsAbsoluteUNC)
{
    EXPECT_TRUE(PathUtility::isAbsolute("\\\\server\\share\\file"));
}

TEST(PathUtilityTest, IsAbsoluteRelative)
{
    EXPECT_FALSE(PathUtility::isAbsolute("dir/file.txt"));
}

TEST(PathUtilityTest, IsAbsoluteMountPath)
{
    // Mount paths are not absolute Windows paths
    EXPECT_FALSE(PathUtility::isAbsolute("assets:/file.txt"));
}

TEST(PathUtilityTest, IsAbsoluteEmpty)
{
    EXPECT_FALSE(PathUtility::isAbsolute(""));
}

//============================================================================
// equalsIgnoreCase テスト
//============================================================================
TEST(PathUtilityTest, EqualsIgnoreCaseSame)
{
    EXPECT_TRUE(PathUtility::equalsIgnoreCase("Dir/File.txt", "Dir/File.txt"));
}

TEST(PathUtilityTest, EqualsIgnoreCaseDifferentCase)
{
    EXPECT_TRUE(PathUtility::equalsIgnoreCase("DIR/FILE.TXT", "dir/file.txt"));
}

TEST(PathUtilityTest, EqualsIgnoreCaseDifferentPath)
{
    EXPECT_FALSE(PathUtility::equalsIgnoreCase("dir/file1.txt", "dir/file2.txt"));
}

TEST(PathUtilityTest, EqualsIgnoreCaseNormalized)
{
    EXPECT_TRUE(PathUtility::equalsIgnoreCase("Dir//File.txt", "dir/file.txt"));
}

//============================================================================
// normalizeW テスト
//============================================================================
TEST(PathUtilityTest, NormalizeWBackslash)
{
    EXPECT_EQ(PathUtility::normalizeW(L"dir\\file.txt"), L"dir/file.txt");
}

TEST(PathUtilityTest, NormalizeWDriveLetter)
{
    EXPECT_EQ(PathUtility::normalizeW(L"C:\\Users\\test"), L"C:/Users/test");
}

TEST(PathUtilityTest, NormalizeWUNC)
{
    std::wstring result = PathUtility::normalizeW(L"\\\\server\\share\\dir\\file");
    EXPECT_TRUE(result.find(L"\\\\server") == 0);  // Starts with UNC prefix
}

TEST(PathUtilityTest, NormalizeWDotDot)
{
    EXPECT_EQ(PathUtility::normalizeW(L"C:/a/b/../c"), L"C:/a/c");
}

TEST(PathUtilityTest, NormalizeWEmpty)
{
    EXPECT_EQ(PathUtility::normalizeW(L""), L"");
}

//============================================================================
// isAbsoluteW テスト
//============================================================================
TEST(PathUtilityTest, IsAbsoluteWDriveLetter)
{
    EXPECT_TRUE(PathUtility::isAbsoluteW(L"C:/Users"));
}

TEST(PathUtilityTest, IsAbsoluteWUNC)
{
    EXPECT_TRUE(PathUtility::isAbsoluteW(L"\\\\server\\share"));
}

TEST(PathUtilityTest, IsAbsoluteWRelative)
{
    EXPECT_FALSE(PathUtility::isAbsoluteW(L"dir/file.txt"));
}

//============================================================================
// toNarrowString / toWideString テスト
//============================================================================
TEST(PathUtilityTest, ToNarrowStringSimple)
{
    EXPECT_EQ(PathUtility::toNarrowString(L"hello"), "hello");
}

TEST(PathUtilityTest, ToNarrowStringEmpty)
{
    EXPECT_EQ(PathUtility::toNarrowString(L""), "");
}

TEST(PathUtilityTest, ToWideStringSimple)
{
    EXPECT_EQ(PathUtility::toWideString("hello"), L"hello");
}

TEST(PathUtilityTest, ToWideStringEmpty)
{
    EXPECT_EQ(PathUtility::toWideString(""), L"");
}

TEST(PathUtilityTest, RoundTripConversion)
{
    std::string original = "path/to/file.txt";
    std::wstring wide = PathUtility::toWideString(original);
    std::string narrow = PathUtility::toNarrowString(wide);
    EXPECT_EQ(narrow, original);
}

} // namespace
