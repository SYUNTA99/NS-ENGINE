/// @file PlatformAffinity.h
/// @brief スレッドアフィニティ・優先度管理のエントリポイント
#pragma once

#include "GenericPlatform/GenericPlatformAffinity.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformAffinity.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformAffinity.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformAffinity.h"
#else
#error "No platform-specific affinity implementation available"
#endif
