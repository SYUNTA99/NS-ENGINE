/// @file ApplicationCore.cpp
/// @brief ApplicationCoreモジュール共通実装
/// @details モジュール初期化とHAL依存の検証を含む。
#include "ApplicationCore/ApplicationCoreTypes.h"
#include "HAL/Platform.h"
#include "HAL/PlatformTypes.h"

// コンパイル検証: HAL基盤が利用可能であることを確認
static_assert(sizeof(::int32) == 4, "int32 must be 4 bytes");
static_assert(sizeof(::uint32) == 4, "uint32 must be 4 bytes");

// コンパイル検証: ApplicationCore列挙型
static_assert(NS::WindowMode::Fullscreen == 0, "Fullscreen must be 0");
static_assert(NS::WindowMode::NumWindowModes == 3, "Must have 3 window modes");
