/// @file PlatformAtomics.h
/// @brief アトミック操作のエントリポイント
/// @details プラットフォーム固有のアトミック操作実装を自動選択してインクルードする。
#pragma once

#include "GenericPlatform/GenericPlatformAtomics.h"
#include "HAL/Platform.h"

// プラットフォーム固有のアトミック操作をインクルード
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformAtomics.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformAtomics.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformAtomics.h"
#else
#error "No platform-specific atomics implementation available"
#endif
