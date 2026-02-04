/// @file PhysicsResult.h
/// @brief 物理エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// 物理エラー
    namespace PhysicsResult
    {

        //=========================================================================
        // ワールド関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultWorldError, result::ModuleId::Physics, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultWorldCreationFailed, result::ModuleId::Physics, 1);
        NS_DEFINE_ERROR_RESULT(ResultSimulationFailed, result::ModuleId::Physics, 2);
        NS_DEFINE_ERROR_RESULT(ResultStepFailed, result::ModuleId::Physics, 3);

        //=========================================================================
        // ボディ関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultBodyError, result::ModuleId::Physics, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultBodyCreationFailed, result::ModuleId::Physics, 100);
        NS_DEFINE_ERROR_RESULT(ResultBodyNotFound, result::ModuleId::Physics, 101);
        NS_DEFINE_ERROR_RESULT(ResultInvalidBodyType, result::ModuleId::Physics, 102);

        //=========================================================================
        // コライダー関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultColliderError, result::ModuleId::Physics, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultColliderCreationFailed, result::ModuleId::Physics, 200);
        NS_DEFINE_ERROR_RESULT(ResultInvalidColliderShape, result::ModuleId::Physics, 201);
        NS_DEFINE_ERROR_RESULT(ResultCollisionDetectionFailed, result::ModuleId::Physics, 202);

    } // namespace PhysicsResult

} // namespace NS
