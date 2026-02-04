/// @file AudioResult.h
/// @brief オーディオエラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// オーディオエラー
    namespace AudioResult
    {

        //=========================================================================
        // デバイス関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, result::ModuleId::Audio, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultDeviceCreationFailed, result::ModuleId::Audio, 1);
        NS_DEFINE_ERROR_RESULT(ResultDeviceNotAvailable, result::ModuleId::Audio, 2);
        NS_DEFINE_ERROR_RESULT(ResultDeviceLost, result::ModuleId::Audio, 3);
        NS_DEFINE_ERROR_RESULT(ResultFormatNotSupported, result::ModuleId::Audio, 4);
        NS_DEFINE_ERROR_RESULT(ResultDriverError, result::ModuleId::Audio, 5);

        //=========================================================================
        // 再生関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultPlaybackError, result::ModuleId::Audio, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultPlaybackFailed, result::ModuleId::Audio, 100);
        NS_DEFINE_ERROR_RESULT(ResultBufferUnderrun, result::ModuleId::Audio, 101);
        NS_DEFINE_ERROR_RESULT(ResultBufferOverrun, result::ModuleId::Audio, 102);
        NS_DEFINE_ERROR_RESULT(ResultSourceNotFound, result::ModuleId::Audio, 103);
        NS_DEFINE_ERROR_RESULT(ResultVoiceLimitReached, result::ModuleId::Audio, 104);
        NS_DEFINE_ERROR_RESULT(ResultMixerError, result::ModuleId::Audio, 105);

        //=========================================================================
        // フォーマット関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultFormatError, result::ModuleId::Audio, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, result::ModuleId::Audio, 200);
        NS_DEFINE_ERROR_RESULT(ResultDecodeFailed, result::ModuleId::Audio, 201);
        NS_DEFINE_ERROR_RESULT(ResultEncodeFailed, result::ModuleId::Audio, 202);
        NS_DEFINE_ERROR_RESULT(ResultStreamError, result::ModuleId::Audio, 203);
        NS_DEFINE_ERROR_RESULT(ResultCodecNotFound, result::ModuleId::Audio, 204);

    } // namespace AudioResult

} // namespace NS
