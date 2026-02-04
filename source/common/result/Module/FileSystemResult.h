/// @file FileSystemResult.h
/// @brief ファイルシステムエラーコード
///
/// FileError::Code との対応表:
/// | FileError::Code   | FileSystemResult                |
/// |-------------------|---------------------------------|
/// | None              | ResultSuccess()                 |
/// | NotFound          | ResultPathNotFound()            |
/// | AccessDenied      | ResultAccessDenied()            |
/// | InvalidPath       | ResultInvalidPathFormat()       |
/// | InvalidMount      | ResultMountNotFound()           |
/// | DiskFull          | ResultDiskFull()                |
/// | AlreadyExists     | ResultPathAlreadyExists()       |
/// | NotEmpty          | ResultDirectoryNotEmpty()       |
/// | IsDirectory       | ResultIsADirectory()            |
/// | IsNotDirectory    | ResultNotADirectory()           |
/// | PathTooLong       | ResultPathTooLong()             |
/// | ReadOnly          | ResultReadOnly()                |
/// | Cancelled         | CommonResult::ResultCancelled() |
/// | Unknown           | ResultUnknownError()            |
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// ファイルシステムエラー
    namespace FileSystemResult
    {

        //=========================================================================
        // パス関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultPathError, result::ModuleId::FileSystem, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultPathNotFound, result::ModuleId::FileSystem, 1);
        NS_DEFINE_ERROR_RESULT(ResultPathAlreadyExists, result::ModuleId::FileSystem, 2);
        NS_DEFINE_ERROR_RESULT(ResultPathTooLong, result::ModuleId::FileSystem, 3);
        NS_DEFINE_ERROR_RESULT(ResultInvalidPathFormat, result::ModuleId::FileSystem, 4);
        NS_DEFINE_ERROR_RESULT(ResultInvalidCharacter, result::ModuleId::FileSystem, 5);
        NS_DEFINE_ERROR_RESULT(ResultNotAFile, result::ModuleId::FileSystem, 6);
        NS_DEFINE_ERROR_RESULT(ResultNotADirectory, result::ModuleId::FileSystem, 7);
        NS_DEFINE_ERROR_RESULT(ResultIsADirectory, result::ModuleId::FileSystem, 8);
        NS_DEFINE_ERROR_RESULT(ResultDirectoryNotEmpty, result::ModuleId::FileSystem, 9);
        NS_DEFINE_ERROR_RESULT(ResultSymlinkLoop, result::ModuleId::FileSystem, 10);
        NS_DEFINE_ERROR_RESULT(ResultTooManySymlinks, result::ModuleId::FileSystem, 11);
        NS_DEFINE_ERROR_RESULT(ResultCrossDevice, result::ModuleId::FileSystem, 12);
        NS_DEFINE_ERROR_RESULT(ResultRootDirectory, result::ModuleId::FileSystem, 13);

        //=========================================================================
        // アクセス関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultAccessError, result::ModuleId::FileSystem, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultAccessDenied, result::ModuleId::FileSystem, 100);
        NS_DEFINE_ERROR_RESULT(ResultPermissionDenied, result::ModuleId::FileSystem, 101);
        NS_DEFINE_ERROR_RESULT(ResultFileLocked, result::ModuleId::FileSystem, 102);
        NS_DEFINE_ERROR_RESULT(ResultSharingViolation, result::ModuleId::FileSystem, 103);
        NS_DEFINE_ERROR_RESULT(ResultReadOnly, result::ModuleId::FileSystem, 104);
        NS_DEFINE_ERROR_RESULT(ResultWriteProtected, result::ModuleId::FileSystem, 105);
        NS_DEFINE_ERROR_RESULT(ResultInUse, result::ModuleId::FileSystem, 106);

        //=========================================================================
        // 操作関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultOperationError, result::ModuleId::FileSystem, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultOpenFailed, result::ModuleId::FileSystem, 200);
        NS_DEFINE_ERROR_RESULT(ResultCloseFailed, result::ModuleId::FileSystem, 201);
        NS_DEFINE_ERROR_RESULT(ResultReadFailed, result::ModuleId::FileSystem, 202);
        NS_DEFINE_ERROR_RESULT(ResultWriteFailed, result::ModuleId::FileSystem, 203);
        NS_DEFINE_ERROR_RESULT(ResultSeekFailed, result::ModuleId::FileSystem, 204);
        NS_DEFINE_ERROR_RESULT(ResultFlushFailed, result::ModuleId::FileSystem, 205);
        NS_DEFINE_ERROR_RESULT(ResultTruncateFailed, result::ModuleId::FileSystem, 206);
        NS_DEFINE_ERROR_RESULT(ResultRenameFailed, result::ModuleId::FileSystem, 207);
        NS_DEFINE_ERROR_RESULT(ResultCopyFailed, result::ModuleId::FileSystem, 208);
        NS_DEFINE_ERROR_RESULT(ResultDeleteFailed, result::ModuleId::FileSystem, 209);
        NS_DEFINE_ERROR_RESULT(ResultCreateDirectoryFailed, result::ModuleId::FileSystem, 210);
        NS_DEFINE_ERROR_RESULT(ResultRemoveDirectoryFailed, result::ModuleId::FileSystem, 211);

        //=========================================================================
        // ストレージ関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultStorageError, result::ModuleId::FileSystem, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultDiskFull, result::ModuleId::FileSystem, 300);
        NS_DEFINE_ERROR_RESULT(ResultQuotaExceeded, result::ModuleId::FileSystem, 301);
        NS_DEFINE_ERROR_RESULT(ResultFileTooLarge, result::ModuleId::FileSystem, 302);
        NS_DEFINE_ERROR_RESULT(ResultNoSpace, result::ModuleId::FileSystem, 303);
        NS_DEFINE_ERROR_RESULT(ResultDriveNotReady, result::ModuleId::FileSystem, 304);
        NS_DEFINE_ERROR_RESULT(ResultMediaRemoved, result::ModuleId::FileSystem, 305);
        NS_DEFINE_ERROR_RESULT(ResultMediaChanged, result::ModuleId::FileSystem, 306);
        NS_DEFINE_ERROR_RESULT(ResultMediaWriteProtected, result::ModuleId::FileSystem, 307);

        //=========================================================================
        // データ関連 (400-499)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDataError, result::ModuleId::FileSystem, 400, 500);
        NS_DEFINE_ERROR_RESULT(ResultEndOfFile, result::ModuleId::FileSystem, 400);
        NS_DEFINE_ERROR_RESULT(ResultUnexpectedEndOfFile, result::ModuleId::FileSystem, 401);
        NS_DEFINE_ERROR_RESULT(ResultDataCorrupted, result::ModuleId::FileSystem, 402);
        NS_DEFINE_ERROR_RESULT(ResultInvalidFileFormat, result::ModuleId::FileSystem, 403);
        NS_DEFINE_ERROR_RESULT(ResultUnsupportedFormat, result::ModuleId::FileSystem, 404);

        //=========================================================================
        // マウント関連 (500-599)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultMountError, result::ModuleId::FileSystem, 500, 600);
        /// マウントが見つからない、または無効
        NS_DEFINE_ERROR_RESULT(ResultMountNotFound, result::ModuleId::FileSystem, 500);
        /// マウント名が無効（形式エラー）
        NS_DEFINE_ERROR_RESULT(ResultInvalidMountName, result::ModuleId::FileSystem, 501);
        /// マウントが既に存在する
        NS_DEFINE_ERROR_RESULT(ResultMountAlreadyExists, result::ModuleId::FileSystem, 502);

        //=========================================================================
        // 汎用エラー (900-999)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultGenericError, result::ModuleId::FileSystem, 900, 1000);
        /// 不明なエラー
        NS_DEFINE_ERROR_RESULT(ResultUnknownError, result::ModuleId::FileSystem, 900);
        /// 内部エラー
        NS_DEFINE_ERROR_RESULT(ResultInternalError, result::ModuleId::FileSystem, 901);

    } // namespace FileSystemResult

} // namespace NS
