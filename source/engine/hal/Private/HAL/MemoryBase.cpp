/// @file MemoryBase.cpp
/// @brief メモリアロケータ基底クラス実装

#include "HAL/MemoryBase.h"
#include <cstring>

namespace NS
{
    // グローバルアロケータ（初期はnullptr、起動時に設定）
    Malloc* g_gMalloc = nullptr;

    void* Malloc::TryAlloc(SIZE_T count, uint32 alignment)
    {
        // デフォルト実装：Allocをそのまま呼ぶ（派生クラスでオーバーライド推奨）
        return Alloc(count, alignment);
    }

    void* Malloc::TryRealloc(void* ptr, SIZE_T newCount, uint32 alignment)
    {
        // デフォルト実装：Reallocをそのまま呼ぶ
        return Realloc(ptr, newCount, alignment);
    }

    void* Malloc::Realloc(void* ptr, SIZE_T newCount, uint32 alignment)
    {
        if (ptr == nullptr)
        {
            return Alloc(newCount, alignment);
        }

        if (newCount == 0)
        {
            Free(ptr);
            return nullptr;
        }

        // デフォルト実装：新規確保 + コピー + 解放
        void* newPtr = Alloc(newCount, alignment);
        if (newPtr != nullptr)
        {
            SIZE_T oldSize = 0;
            if (GetAllocationSize(ptr, oldSize))
            {
                SIZE_T const copySize = (oldSize < newCount) ? oldSize : newCount;
                std::memcpy(newPtr, ptr, copySize);
            }
            else
            {
                // サイズ不明の場合は新サイズ分コピー（危険だが仕方ない）
                std::memcpy(newPtr, ptr, newCount);
            }
            Free(ptr);
        }
        return newPtr;
    }

    void* Malloc::AllocZeroed(SIZE_T count, uint32 alignment)
    {
        void* ptr = Alloc(count, alignment);
        if (ptr != nullptr)
        {
            std::memset(ptr, 0, count);
        }
        return ptr;
    }

    void* Malloc::TryAllocZeroed(SIZE_T count, uint32 alignment)
    {
        void* ptr = TryAlloc(count, alignment);
        if (ptr != nullptr)
        {
            std::memset(ptr, 0, count);
        }
        return ptr;
    }

    SIZE_T Malloc::QuantizeSize(SIZE_T count, uint32 alignment)
    {
        // デフォルト：量子化なし
        NS_UNUSED(alignment);
        return count;
    }

    bool Malloc::GetAllocationSize(void* ptr, SIZE_T& outSize)
    {
        // デフォルト：サイズ取得不可
        NS_UNUSED(ptr);
        outSize = 0;
        return false;
    }

    bool Malloc::ValidateHeap()
    {
        // デフォルト：常に有効
        return true;
    }

    void Malloc::Trim(bool trimThreadCaches)
    {
        // デフォルト：何もしない
        NS_UNUSED(trimThreadCaches);
    }

    void Malloc::GetAllocatorStats(AllocatorStats& outStats)
    {
        // デフォルト：全てゼロ
        outStats.totalAllocated = 0;
        outStats.totalAllocations = 0;
        outStats.peakAllocated = 0;
        outStats.currentUsed = 0;
    }

    void Malloc::DumpAllocatorStats(OutputDevice& output)
    {
        // デフォルト：何もしない
        NS_UNUSED(output);
    }

#if NS_MALLOC_DEBUG
    bool AllocationHeader::ValidateGuards(void* userPtr) const
    {
        // 前ガード検証
        const uint8* preGuard = static_cast<const uint8*>(userPtr) - kGuardByteSize;
        for (SIZE_T i = 0; i < kGuardByteSize; ++i)
        {
            if (preGuard[i] != kGuardByteFill) {
                return false;
}
        }

        // 後ガード検証
        const uint8* postGuard = static_cast<const uint8*>(userPtr) + requestedSize;
        for (SIZE_T i = 0; i < kGuardByteSize; ++i)
        {
            if (postGuard[i] != kGuardByteFill) {
                return false;
}
        }

        return true;
    }

    void Malloc::InitializeGuards(AllocationHeader* header, void* userPtr, SIZE_T size)
    {
        NS_UNUSED(header);

        // 前ガード
        uint8* preGuard = static_cast<uint8*>(userPtr) - kGuardByteSize;
        std::memset(preGuard, kGuardByteFill, kGuardByteSize);

        // 後ガード
        uint8* postGuard = static_cast<uint8*>(userPtr) + size;
        std::memset(postGuard, kGuardByteFill, kGuardByteSize);
    }

    void Malloc::PoisonFreedMemory(void* userPtr, SIZE_T size)
    {
        std::memset(userPtr, kFreedByteFill, size);
    }
#endif

} // namespace NS
