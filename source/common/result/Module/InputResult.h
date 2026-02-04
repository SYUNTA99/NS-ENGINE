/// @file InputResult.h
/// @brief 入力エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// 入力エラー
    namespace InputResult
    {

        //=========================================================================
        // デバイス関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, result::ModuleId::Input, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultDeviceNotFound, result::ModuleId::Input, 1);
        NS_DEFINE_ERROR_RESULT(ResultDeviceDisconnected, result::ModuleId::Input, 2);
        NS_DEFINE_ERROR_RESULT(ResultDeviceNotSupported, result::ModuleId::Input, 3);
        NS_DEFINE_ERROR_RESULT(ResultDeviceInitFailed, result::ModuleId::Input, 4);
        NS_DEFINE_ERROR_RESULT(ResultDeviceBusy, result::ModuleId::Input, 5);

        //=========================================================================
        // バインディング関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultBindingError, result::ModuleId::Input, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultBindingNotFound, result::ModuleId::Input, 100);
        NS_DEFINE_ERROR_RESULT(ResultBindingConflict, result::ModuleId::Input, 101);
        NS_DEFINE_ERROR_RESULT(ResultInvalidBinding, result::ModuleId::Input, 102);
        NS_DEFINE_ERROR_RESULT(ResultActionNotFound, result::ModuleId::Input, 103);

    } // namespace InputResult

} // namespace NS
