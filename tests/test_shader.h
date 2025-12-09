//----------------------------------------------------------------------------
//! @file   test_shader.h
//! @brief  Shader system test declarations
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace tests {

//! Run all shader system tests
//! @param [in] assetsDir Directory containing test assets (empty to skip file-based tests)
//! @return true if all tests passed
//! @note Requires D3D11 device for some tests
bool RunShaderTests(const std::wstring& assetsDir = L"");

} // namespace tests
