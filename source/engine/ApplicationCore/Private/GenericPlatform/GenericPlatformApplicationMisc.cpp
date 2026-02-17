/// @file GenericPlatformApplicationMisc.cpp
/// @brief プラットフォーム共通アプリケーションユーティリティ実装

#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "GenericPlatform/NullApplication.h"

namespace NS
{
    // 静的メンバ定義
    GenericPlatformApplicationMisc::PlatformFocusCallback GenericPlatformApplicationMisc::OnFocusCallback;

    std::shared_ptr<GenericApplication> GenericPlatformApplicationMisc::CreateApplication()
    {
        // Generic デフォルト: プラットフォーム固有の派生クラスでオーバーライドすること
        return NullApplication::CreateNullApplication();
    }
} // namespace NS
