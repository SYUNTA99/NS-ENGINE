/// @file PlatformProcess.h
/// @brief プロセス管理のエントリポイント
/// @details プラットフォーム固有のプロセス管理実装を自動選択してインクルードする。
#pragma once

#include "GenericPlatform/GenericPlatformProcess.h"
#include "HAL/Platform.h"

// プラットフォーム固有のプロセス管理をインクルード
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformProcess.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformProcess.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformProcess.h"
#else
#error "No platform-specific process implementation available"
#endif
