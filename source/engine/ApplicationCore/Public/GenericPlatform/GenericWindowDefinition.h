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
        WindowType Type = WindowType::Normal;
        WindowTransparency TransparencySupport = WindowTransparency::None;
        bool HasOSWindowBorder = true;
        bool AppearsInTaskbar = true;
        bool IsTopmostWindow = false;
        bool AcceptsInput = true;
        WindowActivationPolicy ActivationPolicy = WindowActivationPolicy::Always;
        bool FocusWhenFirstShown = true;
        bool HasCloseButton = true;
        bool SupportsMinimize = true;
        bool SupportsMaximize = true;
        bool IsModalWindow = false;
        bool IsRegularWindow = true;
        bool HasSizingFrame = true;
        bool SizeWillChangeOften = false;
        bool ShouldPreserveAspectRatio = false;

        // 位置・サイズ
        float XDesiredPositionOnScreen = -1.0f;
        float YDesiredPositionOnScreen = -1.0f;
        float WidthDesiredOnScreen = 1280.0f;
        float HeightDesiredOnScreen = 720.0f;
        int32_t ExpectedMaxWidth = -1;
        int32_t ExpectedMaxHeight = -1;

        // その他
        float Opacity = 1.0f;
        float CornerRadius = 0.0f;
        std::basic_string<NS::TCHAR> Title;
        bool bManualDPI = false;
        WindowSizeLimits SizeLimits;
    };

} // namespace NS
