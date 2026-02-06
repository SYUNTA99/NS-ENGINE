/// @file PlatformTLS.h
/// @brief スレッドローカルストレージのエントリポイント
#pragma once

#include "GenericPlatform/GenericPlatformTLS.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformTLS.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformTLS.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformTLS.h"
#else
#error "No platform-specific TLS implementation available"
#endif
