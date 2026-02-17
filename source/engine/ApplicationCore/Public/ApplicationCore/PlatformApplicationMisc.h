/// @file PlatformApplicationMisc.h
/// @brief プラットフォーム固有 ApplicationMisc の自動選択エントリポイント
#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformApplicationMisc.h"
namespace NS
{
    using PlatformApplicationMisc = WindowsPlatformApplicationMisc;
}
#elif PLATFORM_LINUX
namespace NS
{
    using PlatformApplicationMisc = GenericPlatformApplicationMisc;
}
#elif PLATFORM_MAC
namespace NS
{
    using PlatformApplicationMisc = GenericPlatformApplicationMisc;
}
#else
namespace NS
{
    using PlatformApplicationMisc = GenericPlatformApplicationMisc;
}
#endif
