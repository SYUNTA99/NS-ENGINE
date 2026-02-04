/// @file EcsResult.h
/// @brief ECSエラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// ECSエラー
    namespace EcsResult
    {

        //=========================================================================
        // エンティティ関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultEntityError, result::ModuleId::Ecs, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultEntityNotFound, result::ModuleId::Ecs, 1);
        NS_DEFINE_ERROR_RESULT(ResultEntityAlreadyExists, result::ModuleId::Ecs, 2);
        NS_DEFINE_ERROR_RESULT(ResultEntityDestroyed, result::ModuleId::Ecs, 3);
        NS_DEFINE_ERROR_RESULT(ResultEntityVersionMismatch, result::ModuleId::Ecs, 4);
        NS_DEFINE_ERROR_RESULT(ResultMaxEntitiesReached, result::ModuleId::Ecs, 5);
        NS_DEFINE_ERROR_RESULT(ResultInvalidEntityId, result::ModuleId::Ecs, 6);

        //=========================================================================
        // コンポーネント関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultComponentError, result::ModuleId::Ecs, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultComponentNotFound, result::ModuleId::Ecs, 100);
        NS_DEFINE_ERROR_RESULT(ResultComponentAlreadyExists, result::ModuleId::Ecs, 101);
        NS_DEFINE_ERROR_RESULT(ResultComponentTypeMismatch, result::ModuleId::Ecs, 102);
        NS_DEFINE_ERROR_RESULT(ResultMaxComponentsReached, result::ModuleId::Ecs, 103);
        NS_DEFINE_ERROR_RESULT(ResultComponentNotRegistered, result::ModuleId::Ecs, 104);
        NS_DEFINE_ERROR_RESULT(ResultInvalidComponentId, result::ModuleId::Ecs, 105);
        NS_DEFINE_ERROR_RESULT(ResultComponentSizeExceeded, result::ModuleId::Ecs, 106);

        //=========================================================================
        // アーキタイプ関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultArchetypeError, result::ModuleId::Ecs, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultArchetypeNotFound, result::ModuleId::Ecs, 200);
        NS_DEFINE_ERROR_RESULT(ResultArchetypeMismatch, result::ModuleId::Ecs, 201);
        NS_DEFINE_ERROR_RESULT(ResultChunkAllocationFailed, result::ModuleId::Ecs, 202);
        NS_DEFINE_ERROR_RESULT(ResultChunkCapacityExceeded, result::ModuleId::Ecs, 203);
        NS_DEFINE_ERROR_RESULT(ResultArchetypeLayoutInvalid, result::ModuleId::Ecs, 204);

        //=========================================================================
        // システム関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultSystemError, result::ModuleId::Ecs, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultSystemNotFound, result::ModuleId::Ecs, 300);
        NS_DEFINE_ERROR_RESULT(ResultSystemAlreadyExists, result::ModuleId::Ecs, 301);
        NS_DEFINE_ERROR_RESULT(ResultSystemDependencyCycle, result::ModuleId::Ecs, 302);
        NS_DEFINE_ERROR_RESULT(ResultSystemExecutionFailed, result::ModuleId::Ecs, 303);
        NS_DEFINE_ERROR_RESULT(ResultSystemDisabled, result::ModuleId::Ecs, 304);

        //=========================================================================
        // クエリ関連 (400-499)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultQueryError, result::ModuleId::Ecs, 400, 500);
        NS_DEFINE_ERROR_RESULT(ResultQueryCreationFailed, result::ModuleId::Ecs, 400);
        NS_DEFINE_ERROR_RESULT(ResultQueryInvalidFilter, result::ModuleId::Ecs, 401);
        NS_DEFINE_ERROR_RESULT(ResultQueryNoMatch, result::ModuleId::Ecs, 402);
        NS_DEFINE_ERROR_RESULT(ResultQueryIteratorInvalid, result::ModuleId::Ecs, 403);

    } // namespace EcsResult

} // namespace NS
