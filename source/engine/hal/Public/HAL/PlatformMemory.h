/// @file PlatformMemory.h
/// @brief メモリ管理のエントリポイント
/// @details プラットフォーム固有のメモリ管理実装を自動選択してインクルードする。
#pragma once

#include "GenericPlatform/GenericPlatformMemory.h"
#include "HAL/Platform.h"

// プラットフォーム固有のメモリ管理をインクルード
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMemory.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformMemory.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformMemory.h"
#else
#error "No platform-specific memory implementation available"
#endif
