/// @file ErrorInfo.h
/// @brief エラー情報取得ユーティリティ
#pragma once


#include "common/result/Core/Result.h"
#include "common/result/Module/ModuleId.h"

#include <string_view>

namespace NS::result
{

    /// エラー情報
    struct ErrorInfo
    {
        int module = 0;
        int description = 0;
        std::string_view moduleName;
        std::string_view errorName;
        std::string_view message;
    };

    /// Resultからエラー情報を取得
    [[nodiscard]] inline ErrorInfo GetErrorInfo(::NS::Result result) noexcept
    {
        ErrorInfo info;
        info.module = result.GetModule();
        info.description = result.GetDescription();
        info.moduleName = GetModuleName(info.module);
        info.errorName = "Unknown";
        info.message = {};

        // TODO: エラー名とメッセージはResultRegistryから取得可能にする
        // 現時点ではモジュール名のみ

        return info;
    }

} // namespace NS::result
