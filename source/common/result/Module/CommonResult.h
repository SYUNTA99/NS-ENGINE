/// @file CommonResult.h
/// @brief 共通エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// 共通エラーコード
    /// 全モジュールで発生しうる汎用的なエラー
    namespace CommonResult
    {

        //=========================================================================
        // 汎用 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RESULT(ResultOperationFailed, result::ModuleId::Common, 1);
        NS_DEFINE_ERROR_RESULT(ResultNotImplemented, result::ModuleId::Common, 2);
        NS_DEFINE_ERROR_RESULT(ResultNotSupported, result::ModuleId::Common, 3);

        //=========================================================================
        // 引数エラー (100-149)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultInvalidArgument, result::ModuleId::Common, 100, 150);
        NS_DEFINE_ERROR_RESULT(ResultNullPointer, result::ModuleId::Common, 100);
        NS_DEFINE_ERROR_RESULT(ResultOutOfRange, result::ModuleId::Common, 101);
        NS_DEFINE_ERROR_RESULT(ResultInvalidSize, result::ModuleId::Common, 102);
        NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, result::ModuleId::Common, 103);
        NS_DEFINE_ERROR_RESULT(ResultInvalidHandle, result::ModuleId::Common, 104);
        NS_DEFINE_ERROR_RESULT(ResultInvalidId, result::ModuleId::Common, 105);
        NS_DEFINE_ERROR_RESULT(ResultInvalidState, result::ModuleId::Common, 106);
        NS_DEFINE_ERROR_RESULT(ResultInvalidOperation, result::ModuleId::Common, 107);
        NS_DEFINE_ERROR_RESULT(ResultInvalidPath, result::ModuleId::Common, 108);
        NS_DEFINE_ERROR_RESULT(ResultInvalidName, result::ModuleId::Common, 109);
        NS_DEFINE_ERROR_RESULT(ResultInvalidType, result::ModuleId::Common, 110);
        NS_DEFINE_ERROR_RESULT(ResultInvalidData, result::ModuleId::Common, 111);
        NS_DEFINE_ERROR_RESULT(ResultInvalidVersion, result::ModuleId::Common, 112);
        NS_DEFINE_ERROR_RESULT(ResultInvalidConfiguration, result::ModuleId::Common, 113);

        //=========================================================================
        // 状態エラー (150-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultStateError, result::ModuleId::Common, 150, 200);
        NS_DEFINE_ERROR_RESULT(ResultNotInitialized, result::ModuleId::Common, 150);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyInitialized, result::ModuleId::Common, 151);
        NS_DEFINE_ERROR_RESULT(ResultNotStarted, result::ModuleId::Common, 152);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyStarted, result::ModuleId::Common, 153);
        NS_DEFINE_ERROR_RESULT(ResultNotFinished, result::ModuleId::Common, 154);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyFinished, result::ModuleId::Common, 155);
        NS_DEFINE_ERROR_RESULT(ResultNotOpen, result::ModuleId::Common, 156);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyOpen, result::ModuleId::Common, 157);
        NS_DEFINE_ERROR_RESULT(ResultNotClosed, result::ModuleId::Common, 158);
        NS_DEFINE_ERROR_RESULT(ResultAlreadyClosed, result::ModuleId::Common, 159);

        //=========================================================================
        // リソースエラー (200-299) - Transient + Recoverable
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT_CAT(
            ResultResourceError, result::ModuleId::Common, 200, 300, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultOutOfMemory, result::ModuleId::Common, 200, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultOutOfResources, result::ModuleId::Common, 201, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultResourceBusy, result::ModuleId::Common, 202);
        NS_DEFINE_ERROR_RESULT(ResultResourceNotFound, result::ModuleId::Common, 203);
        NS_DEFINE_ERROR_RESULT(ResultResourceAlreadyExists, result::ModuleId::Common, 204);
        NS_DEFINE_ERROR_RESULT(ResultResourceLocked, result::ModuleId::Common, 205);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultResourceExhausted, result::ModuleId::Common, 206, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultCapacityExceeded, result::ModuleId::Common, 207);
        NS_DEFINE_ERROR_RESULT(ResultLimitReached, result::ModuleId::Common, 208);
        NS_DEFINE_ERROR_RESULT(ResultQuotaExceeded, result::ModuleId::Common, 209);

        //=========================================================================
        // タイミング・同期エラー (300-399) - Transient + Recoverable
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT_CAT(
            ResultTimingError, result::ModuleId::Common, 300, 400, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultTimeout, result::ModuleId::Common, 300, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultDeadlock, result::ModuleId::Common, 301);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultWouldBlock, result::ModuleId::Common, 302, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultInterrupted, result::ModuleId::Common, 303, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultCancelled, result::ModuleId::Common, 304);
        NS_DEFINE_ERROR_RESULT(ResultAborted, result::ModuleId::Common, 305);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultRetryLater, result::ModuleId::Common, 306, NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultPending, result::ModuleId::Common, 307);
        NS_DEFINE_ERROR_RESULT(ResultInProgress, result::ModuleId::Common, 308);

        //=========================================================================
        // アクセス・権限エラー (400-499) - Permanent + Recoverable
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT_CAT(
            ResultAccessError, result::ModuleId::Common, 400, 500, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultAccessDenied, result::ModuleId::Common, 400, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultPermissionDenied, result::ModuleId::Common, 401, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultReadOnly, result::ModuleId::Common, 402);
        NS_DEFINE_ERROR_RESULT(ResultWriteProtected, result::ModuleId::Common, 403);
        NS_DEFINE_ERROR_RESULT(ResultNotAuthorized, result::ModuleId::Common, 404);
        NS_DEFINE_ERROR_RESULT(ResultForbidden, result::ModuleId::Common, 405);

        //=========================================================================
        // データ整合性エラー (500-599) - Permanent + Recoverable
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT_CAT(
            ResultDataError, result::ModuleId::Common, 500, 600, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultDataCorrupted, result::ModuleId::Common, 500, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_RECOVERABLE);
        NS_DEFINE_ERROR_RESULT(ResultChecksumMismatch, result::ModuleId::Common, 501);
        NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, result::ModuleId::Common, 502);
        NS_DEFINE_ERROR_RESULT(ResultFormatError, result::ModuleId::Common, 503);
        NS_DEFINE_ERROR_RESULT(ResultParseError, result::ModuleId::Common, 504);
        NS_DEFINE_ERROR_RESULT(ResultSerializationError, result::ModuleId::Common, 505);
        NS_DEFINE_ERROR_RESULT(ResultDeserializationError, result::ModuleId::Common, 506);
        NS_DEFINE_ERROR_RESULT(ResultEncodingError, result::ModuleId::Common, 507);
        NS_DEFINE_ERROR_RESULT(ResultDecodingError, result::ModuleId::Common, 508);
        NS_DEFINE_ERROR_RESULT(ResultValidationFailed, result::ModuleId::Common, 509);
        NS_DEFINE_ERROR_RESULT(ResultConstraintViolation, result::ModuleId::Common, 510);

        //=========================================================================
        // 初期化・終了エラー (600-699)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultLifecycleError, result::ModuleId::Common, 600, 700);
        NS_DEFINE_ERROR_RESULT(ResultInitializationFailed, result::ModuleId::Common, 600);
        NS_DEFINE_ERROR_RESULT(ResultShutdownFailed, result::ModuleId::Common, 601);
        NS_DEFINE_ERROR_RESULT(ResultLoadFailed, result::ModuleId::Common, 602);
        NS_DEFINE_ERROR_RESULT(ResultUnloadFailed, result::ModuleId::Common, 603);
        NS_DEFINE_ERROR_RESULT(ResultCreateFailed, result::ModuleId::Common, 604);
        NS_DEFINE_ERROR_RESULT(ResultDestroyFailed, result::ModuleId::Common, 605);
        NS_DEFINE_ERROR_RESULT(ResultConnectFailed, result::ModuleId::Common, 606);
        NS_DEFINE_ERROR_RESULT(ResultDisconnectFailed, result::ModuleId::Common, 607);
        NS_DEFINE_ERROR_RESULT(ResultBindFailed, result::ModuleId::Common, 608);
        NS_DEFINE_ERROR_RESULT(ResultUnbindFailed, result::ModuleId::Common, 609);

        //=========================================================================
        // 内部エラー (700-799) - Permanent + Fatal
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT_CAT(
            ResultInternalError, result::ModuleId::Common, 700, 800, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultUnknownError, result::ModuleId::Common, 700, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultAssertionFailed, result::ModuleId::Common, 701, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultInvariantViolation, result::ModuleId::Common, 702, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultUnreachable, result::ModuleId::Common, 703, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);
        NS_DEFINE_ERROR_RESULT_CAT(
            ResultBugDetected, result::ModuleId::Common, 704, NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);

    } // namespace CommonResult

} // namespace NS
