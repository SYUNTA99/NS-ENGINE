/// @file GenericPlatformTime.cpp
/// @brief 時間管理基底クラス静的メンバ定義

#include "GenericPlatform/GenericPlatformTime.h"

namespace NS
{
    double GenericPlatformTime::s_secondsPerCycle = 0.0;
    bool GenericPlatformTime::s_initialized = false;
} // namespace NS
