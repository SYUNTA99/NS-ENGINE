/// @file WindowsPlatformTypes.h
/// @brief Windows固有の型定義
/// @details GenericPlatformTypesを継承し、Windows固有の型を提供する。
#pragma once

#include "GenericPlatform/GenericPlatformTypes.h"

namespace NS
{
    /// Windows固有の型定義
    ///
    /// 現時点ではGenericPlatformTypesと同一。
    /// 将来的にWindows固有の型が必要になった場合にオーバーライド可能。
    struct WindowsPlatformTypes : public GenericPlatformTypes
    {
        // Windows固有の型オーバーライド（必要に応じて追加）

        // Windows: wchar_t は常に2バイト（UTF-16）
        // 注: GenericPlatformTypesの定義で十分だが、明示的に記載
    };

    /// 現在のプラットフォームの型定義
    using PlatformTypes = WindowsPlatformTypes;
} // namespace NS
