/// @file PlatformStackWalk.h
/// @brief スタックウォークのエントリポイント
#pragma once

#include "GenericPlatform/GenericPlatformStackWalk.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformStackWalk.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformStackWalk.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformStackWalk.h"
#else
#error "No platform-specific stack walk implementation available"
#endif
