//----------------------------------------------------------------------------
//! @file   test_file_system.h
//! @brief  FileSystem test declarations
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace tests {

//! Run all FileSystem tests
//! @param [in] hostTestDir Directory for HostFileSystem tests (empty to skip)
//! @return true if all tests passed
bool RunFileSystemTests(const std::wstring& hostTestDir = L"");

} // namespace tests
