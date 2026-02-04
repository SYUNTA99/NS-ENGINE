/// @file OsResult.h
/// @brief OSエラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// OSエラー
    namespace OsResult
    {

        //=========================================================================
        // プロセス関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultProcessError, result::ModuleId::Os, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultProcessNotFound, result::ModuleId::Os, 1);
        NS_DEFINE_ERROR_RESULT(ResultProcessExitedAbnormally, result::ModuleId::Os, 2);
        NS_DEFINE_ERROR_RESULT(ResultProcessKilled, result::ModuleId::Os, 3);
        NS_DEFINE_ERROR_RESULT(ResultProcessCreateFailed, result::ModuleId::Os, 4);
        NS_DEFINE_ERROR_RESULT(ResultProcessTerminateFailed, result::ModuleId::Os, 5);
        NS_DEFINE_ERROR_RESULT(ResultWaitFailed, result::ModuleId::Os, 6);

        //=========================================================================
        // スレッド関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultThreadError, result::ModuleId::Os, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultThreadCreateFailed, result::ModuleId::Os, 100);
        NS_DEFINE_ERROR_RESULT(ResultThreadJoinFailed, result::ModuleId::Os, 101);
        NS_DEFINE_ERROR_RESULT(ResultThreadDetachFailed, result::ModuleId::Os, 102);
        NS_DEFINE_ERROR_RESULT(ResultThreadAffinityFailed, result::ModuleId::Os, 103);
        NS_DEFINE_ERROR_RESULT(ResultThreadPriorityFailed, result::ModuleId::Os, 104);

        //=========================================================================
        // 同期関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultSyncError, result::ModuleId::Os, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultMutexCreateFailed, result::ModuleId::Os, 200);
        NS_DEFINE_ERROR_RESULT(ResultMutexLockFailed, result::ModuleId::Os, 201);
        NS_DEFINE_ERROR_RESULT(ResultMutexUnlockFailed, result::ModuleId::Os, 202);
        NS_DEFINE_ERROR_RESULT(ResultSemaphoreCreateFailed, result::ModuleId::Os, 203);
        NS_DEFINE_ERROR_RESULT(ResultSemaphoreWaitFailed, result::ModuleId::Os, 204);
        NS_DEFINE_ERROR_RESULT(ResultSemaphoreSignalFailed, result::ModuleId::Os, 205);
        NS_DEFINE_ERROR_RESULT(ResultEventCreateFailed, result::ModuleId::Os, 206);
        NS_DEFINE_ERROR_RESULT(ResultEventWaitFailed, result::ModuleId::Os, 207);
        NS_DEFINE_ERROR_RESULT(ResultEventSignalFailed, result::ModuleId::Os, 208);
        NS_DEFINE_ERROR_RESULT(ResultDeadlockDetected, result::ModuleId::Os, 209);

        //=========================================================================
        // システム関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultSystemError, result::ModuleId::Os, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultSystemCallFailed, result::ModuleId::Os, 300);
        NS_DEFINE_ERROR_RESULT(ResultNotEnoughPrivilege, result::ModuleId::Os, 301);
        NS_DEFINE_ERROR_RESULT(ResultTooManyOpenFiles, result::ModuleId::Os, 302);
        NS_DEFINE_ERROR_RESULT(ResultTooManyProcesses, result::ModuleId::Os, 303);
        NS_DEFINE_ERROR_RESULT(ResultEnvironmentVariableNotFound, result::ModuleId::Os, 304);
        NS_DEFINE_ERROR_RESULT(ResultLibraryLoadFailed, result::ModuleId::Os, 305);
        NS_DEFINE_ERROR_RESULT(ResultSymbolNotFound, result::ModuleId::Os, 306);

    } // namespace OsResult

} // namespace NS
