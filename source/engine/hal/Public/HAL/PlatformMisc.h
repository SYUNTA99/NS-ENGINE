/// @file PlatformMisc.h
/// @brief プラットフォーム固有機能の統合ヘッダー
#pragma once

#include "GenericPlatform/GenericPlatformMisc.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMisc.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformMisc.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformMisc.h"
#endif
