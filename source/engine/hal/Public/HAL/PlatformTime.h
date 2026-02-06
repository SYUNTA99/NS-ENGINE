/// @file PlatformTime.h
/// @brief 時間管理のエントリポイント
/// @details プラットフォーム固有の時間管理実装を自動選択してインクルードする。
#pragma once

#include "GenericPlatform/GenericPlatformTime.h"
#include "HAL/Platform.h"

// プラットフォーム固有の時間管理をインクルード
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformTime.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformTime.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformTime.h"
#else
#error "No platform-specific time implementation available"
#endif
