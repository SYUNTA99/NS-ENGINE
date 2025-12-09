//----------------------------------------------------------------------------
//! @file   test_texture.h
//! @brief  Texture system test declarations
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace tests {

//! Run all texture system tests
//! @param [in] textureDir Directory containing test textures (empty to skip file tests)
//! @return true if all tests passed
//! @note Requires D3D11 device
bool RunTextureTests(const std::wstring& textureDir = L"");

} // namespace tests
