/// @file PlatformFile.h
/// @brief ファイルI/Oのエントリポイント
/// @details プラットフォーム固有のファイルI/O実装を自動選択してインクルードする。
#pragma once

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/Platform.h"

// プラットフォーム固有のファイルI/Oをインクルード
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformFile.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformFile.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformFile.h"
#else
#error "No platform-specific file implementation available"
#endif
