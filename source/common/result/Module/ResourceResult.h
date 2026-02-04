/// @file ResourceResult.h
/// @brief リソース管理エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// リソース管理エラー
    namespace ResourceResult
    {

        //=========================================================================
        // ロード関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultLoadError, result::ModuleId::Resource, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultLoadFailed, result::ModuleId::Resource, 1);
        NS_DEFINE_ERROR_RESULT(ResultResourceNotFound, result::ModuleId::Resource, 2);
        NS_DEFINE_ERROR_RESULT(ResultInvalidResourceType, result::ModuleId::Resource, 3);
        NS_DEFINE_ERROR_RESULT(ResultLoaderNotFound, result::ModuleId::Resource, 4);
        NS_DEFINE_ERROR_RESULT(ResultDependencyMissing, result::ModuleId::Resource, 5);
        NS_DEFINE_ERROR_RESULT(ResultCircularDependency, result::ModuleId::Resource, 6);
        NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, result::ModuleId::Resource, 7);

        //=========================================================================
        // キャッシュ関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultCacheError, result::ModuleId::Resource, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultCacheMiss, result::ModuleId::Resource, 100);
        NS_DEFINE_ERROR_RESULT(ResultCacheCorrupted, result::ModuleId::Resource, 101);
        NS_DEFINE_ERROR_RESULT(ResultCacheFull, result::ModuleId::Resource, 102);
        NS_DEFINE_ERROR_RESULT(ResultCacheEvictionFailed, result::ModuleId::Resource, 103);

        //=========================================================================
        // ハンドル関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultHandleError, result::ModuleId::Resource, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultInvalidHandle, result::ModuleId::Resource, 200);
        NS_DEFINE_ERROR_RESULT(ResultHandleExpired, result::ModuleId::Resource, 201);
        NS_DEFINE_ERROR_RESULT(ResultHandleTypeMismatch, result::ModuleId::Resource, 202);
        NS_DEFINE_ERROR_RESULT(ResultResourceNotReady, result::ModuleId::Resource, 203);
        NS_DEFINE_ERROR_RESULT(ResultResourceStillReferenced, result::ModuleId::Resource, 204);

        //=========================================================================
        // パッケージ関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultPackageError, result::ModuleId::Resource, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultPackageNotFound, result::ModuleId::Resource, 300);
        NS_DEFINE_ERROR_RESULT(ResultPackageCorrupted, result::ModuleId::Resource, 301);
        NS_DEFINE_ERROR_RESULT(ResultPackageVersionMismatch, result::ModuleId::Resource, 302);
        NS_DEFINE_ERROR_RESULT(ResultPackageMountFailed, result::ModuleId::Resource, 303);
        NS_DEFINE_ERROR_RESULT(ResultPackageUnmountFailed, result::ModuleId::Resource, 304);

    } // namespace ResourceResult

} // namespace NS
