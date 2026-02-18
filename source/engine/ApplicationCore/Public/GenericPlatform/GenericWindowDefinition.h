/// @file GenericWindowDefinition.h
/// @brief ウィンドウ作成パラメータ構造体
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include <string>

namespace NS
{
    /// ウィンドウ作成パラメータ
    struct GenericWindowDefinition
    {
        // ウィンドウ属性
        WindowType type = WindowType::Normal;
        WindowTransparency transparencySupport = WindowTransparency::None;
        bool hasOsWindowBorder = true;
        bool appearsInTaskbar = true;
        bool isTopmostWindow = false;
        bool acceptsInput = true;
        WindowActivationPolicy activationPolicy = WindowActivationPolicy::Always;
        bool focusWhenFirstShown = true;
        bool hasCloseButton = true;
        bool supportsMinimize = true;
        bool supportsMaximize = true;
        bool isModalWindow = false;
        bool isRegularWindow = true;
        bool hasSizingFrame = true;
        bool sizeWillChangeOften = false;
        bool shouldPreserveAspectRatio = false;

        // 位置・サイズ
        float xDesiredPositionOnScreen = -1.0F;
        float yDesiredPositionOnScreen = -1.0F;
        float widthDesiredOnScreen = 1280.0F;
        float heightDesiredOnScreen = 720.0F;
        int32_t expectedMaxWidth = -1;
        int32_t expectedMaxHeight = -1;

        // その他
        float opacity = 1.0F;
        float cornerRadius = 0.0F;
        std::basic_string<NS::TCHAR> title;
        bool bManualDPI = false;
        WindowSizeLimits sizeLimits;
    };

} // namespace NS
