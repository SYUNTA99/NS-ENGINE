/// @file GenericPlatformMemory.cpp
/// @brief メモリ管理基底クラス静的メンバ定義

#include "GenericPlatform/GenericPlatformMemory.h"

namespace NS
{
    bool GenericPlatformMemory::s_initialized = false;
    PlatformMemoryConstants GenericPlatformMemory::s_constants = {};
} // namespace NS
