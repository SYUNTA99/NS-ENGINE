/// @file MallocAnsi.cpp
/// @brief 標準Cライブラリベースのアロケータ実装

#include "HAL/MallocAnsi.h"
#include "HAL/Platform.h"
#include <cstring>

#if PLATFORM_WINDOWS
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace NS
{
    void* MallocAnsi::Alloc(SIZE_T count, uint32 alignment)
    {
        if (count == 0)
        {
            return nullptr;
        }

        uint32 actualAlignment = GetActualAlignment(count, alignment);

#if PLATFORM_WINDOWS
        void* ptr = _aligned_malloc(count, actualAlignment);
#else
        // C11 aligned_alloc はサイズがアライメントの倍数である必要がある
        SIZE_T alignedCount = (count + actualAlignment - 1) & ~static_cast<SIZE_T>(actualAlignment - 1);
        void* ptr = aligned_alloc(actualAlignment, alignedCount);
#endif

        if (!ptr)
        {
            m_lastError = MallocError::OutOfMemory;
        }

        return ptr;
    }

    void* MallocAnsi::TryAlloc(SIZE_T count, uint32 alignment)
    {
        // 標準Cライブラリは失敗時にnullptrを返すので同じ実装
        return Alloc(count, alignment);
    }

    void* MallocAnsi::Realloc(void* ptr, SIZE_T newCount, uint32 alignment)
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

        uint32 actualAlignment = GetActualAlignment(newCount, alignment);

#if PLATFORM_WINDOWS
        void* newPtr = _aligned_realloc(ptr, newCount, actualAlignment);
        if (!newPtr)
        {
            m_lastError = MallocError::OutOfMemory;
        }
        return newPtr;
#else
        // 非Windowsでは新規確保 + コピー + 解放
        SIZE_T oldSize = 0;
        bool hasOldSize = GetAllocationSize(ptr, oldSize);

        void* newPtr = Alloc(newCount, alignment);
        if (newPtr)
        {
            SIZE_T copySize = newCount;
            if (hasOldSize && oldSize < copySize)
            {
                copySize = oldSize;
            }
            std::memcpy(newPtr, ptr, copySize);
            Free(ptr);
        }
        return newPtr;
#endif
    }

    void MallocAnsi::Free(void* ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

#if PLATFORM_WINDOWS
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    bool MallocAnsi::GetAllocationSize(void* ptr, SIZE_T& outSize)
    {
        if (ptr == nullptr)
        {
            outSize = 0;
            return true;
        }

#if PLATFORM_WINDOWS
        outSize = _aligned_msize(ptr, kMinAlignment, 0);
        return true;
#else
        // 非Windowsではサイズ取得不可
        outSize = 0;
        return false;
#endif
    }

    uint32 MallocAnsi::GetActualAlignment(SIZE_T count, uint32 alignment) const
    {
        if (alignment == kDefaultAlignment)
        {
            // サイズに応じてアライメントを決定
            // 16バイト以上は16バイトアライメント（SIMD対応）
            if (count >= 16)
            {
                return 16;
            }
            return kMinAlignment;
        }

        // 最低でも kMinAlignment
        return (alignment < kMinAlignment) ? kMinAlignment : alignment;
    }
} // namespace NS
