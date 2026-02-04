/// @file MemoryResult.h
/// @brief メモリ管理エラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// メモリ管理エラー
    namespace MemoryResult
    {

        //=========================================================================
        // アロケーション関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultAllocationError, result::ModuleId::Memory, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultAllocationFailed, result::ModuleId::Memory, 1);
        NS_DEFINE_ERROR_RESULT(ResultAlignmentError, result::ModuleId::Memory, 2);
        NS_DEFINE_ERROR_RESULT(ResultSizeOverflow, result::ModuleId::Memory, 3);
        NS_DEFINE_ERROR_RESULT(ResultZeroSizeAllocation, result::ModuleId::Memory, 4);
        NS_DEFINE_ERROR_RESULT(ResultFragmentation, result::ModuleId::Memory, 5);

        //=========================================================================
        // 解放関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDeallocationError, result::ModuleId::Memory, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultDoubleFree, result::ModuleId::Memory, 100);
        NS_DEFINE_ERROR_RESULT(ResultInvalidPointer, result::ModuleId::Memory, 101);
        NS_DEFINE_ERROR_RESULT(ResultHeapCorruption, result::ModuleId::Memory, 102);
        NS_DEFINE_ERROR_RESULT(ResultUseAfterFree, result::ModuleId::Memory, 103);

        //=========================================================================
        // プール・アロケータ関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultPoolError, result::ModuleId::Memory, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultPoolExhausted, result::ModuleId::Memory, 200);
        NS_DEFINE_ERROR_RESULT(ResultPoolNotInitialized, result::ModuleId::Memory, 201);
        NS_DEFINE_ERROR_RESULT(ResultPoolAlreadyInitialized, result::ModuleId::Memory, 202);
        NS_DEFINE_ERROR_RESULT(ResultWrongPool, result::ModuleId::Memory, 203);
        NS_DEFINE_ERROR_RESULT(ResultBlockSizeMismatch, result::ModuleId::Memory, 204);

        //=========================================================================
        // 仮想メモリ関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultVirtualMemoryError, result::ModuleId::Memory, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultVirtualAllocFailed, result::ModuleId::Memory, 300);
        NS_DEFINE_ERROR_RESULT(ResultVirtualFreeFailed, result::ModuleId::Memory, 301);
        NS_DEFINE_ERROR_RESULT(ResultProtectionChangeFailed, result::ModuleId::Memory, 302);
        NS_DEFINE_ERROR_RESULT(ResultPageFault, result::ModuleId::Memory, 303);
        NS_DEFINE_ERROR_RESULT(ResultAddressNotMapped, result::ModuleId::Memory, 304);

    } // namespace MemoryResult

} // namespace NS
