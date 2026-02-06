/// @file PlatformCrashContext.h
/// @brief クラッシュコンテキストのエントリポイント
#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformCrashContext.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformCrashContext.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformCrashContext.h"
#else
#error "No platform-specific crash context implementation available"
#endif
