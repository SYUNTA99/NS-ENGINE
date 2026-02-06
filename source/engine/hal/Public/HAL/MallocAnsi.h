/// @file MallocAnsi.h
/// @brief 標準Cライブラリベースのアロケータ
#pragma once

#include "HAL/MemoryBase.h"

namespace NS
{
    /// 標準Cライブラリベースのアロケータ
    ///
    /// aligned_alloc / _aligned_malloc を使用した基本的なアロケータ。
    /// 他のアロケータが使えない場合のフォールバックとして使用。
    ///
    /// ## スレッドセーフティ
    ///
    /// 標準Cライブラリのメモリ関数はスレッドセーフなため、
    /// このクラスもスレッドセーフ。
    class MallocAnsi : public Malloc
    {
    public:
        MallocAnsi() = default;
        ~MallocAnsi() override = default;

        // =====================================================================
        // Malloc インターフェース実装
        // =====================================================================

        void* Alloc(SIZE_T count, uint32 alignment = kDefaultAlignment) override;
        void* TryAlloc(SIZE_T count, uint32 alignment = kDefaultAlignment) override;
        void* Realloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment) override;
        void Free(void* ptr) override;

        bool GetAllocationSize(void* ptr, SIZE_T& outSize) override;

        const TCHAR* GetDescriptiveName() override { return TEXT("MallocAnsi"); }
        bool IsInternallyThreadSafe() const override { return true; }

    private:
        /// 実際のアライメントを計算
        /// @param count 要求サイズ
        /// @param alignment 要求アライメント（0の場合は自動決定）
        /// @return 使用するアライメント
        uint32 GetActualAlignment(SIZE_T count, uint32 alignment) const;
    };
} // namespace NS
