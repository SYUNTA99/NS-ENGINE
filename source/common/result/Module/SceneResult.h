/// @file SceneResult.h
/// @brief シーン管理エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// シーン管理エラー
    namespace SceneResult
    {

        //=========================================================================
        // シーン関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultSceneError, result::ModuleId::Scene, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultSceneNotFound, result::ModuleId::Scene, 1);
        NS_DEFINE_ERROR_RESULT(ResultSceneAlreadyLoaded, result::ModuleId::Scene, 2);
        NS_DEFINE_ERROR_RESULT(ResultSceneLoadFailed, result::ModuleId::Scene, 3);
        NS_DEFINE_ERROR_RESULT(ResultSceneUnloadFailed, result::ModuleId::Scene, 4);
        NS_DEFINE_ERROR_RESULT(ResultSceneTransitionFailed, result::ModuleId::Scene, 5);

        //=========================================================================
        // オブジェクト関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultObjectError, result::ModuleId::Scene, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultObjectNotFound, result::ModuleId::Scene, 100);
        NS_DEFINE_ERROR_RESULT(ResultObjectNameConflict, result::ModuleId::Scene, 101);
        NS_DEFINE_ERROR_RESULT(ResultInvalidHierarchy, result::ModuleId::Scene, 102);
        NS_DEFINE_ERROR_RESULT(ResultCircularParent, result::ModuleId::Scene, 103);

    } // namespace SceneResult

} // namespace NS
