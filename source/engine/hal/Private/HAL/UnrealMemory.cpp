/// @file UnrealMemory.cpp
/// @brief 統一メモリAPI実装

#include "HAL/UnrealMemory.h"
#include "HAL/Platform.h"
#include <cstring>

#if PLATFORM_WINDOWS
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace NS
{
    void* Memory::Malloc(SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            // GMalloc未初期化時はSystemMallocにフォールバック
            return SystemMalloc(count);
        }
        return GMalloc->Alloc(count, alignment);
    }

    void* Memory::Realloc(void* ptr, SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            // GMalloc未初期化時は未サポート
            return nullptr;
        }
        return GMalloc->Realloc(ptr, count, alignment);
    }

    void Memory::Free(void* ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        if (!GMalloc)
        {
            // GMalloc未初期化時はSystemFree
            SystemFree(ptr);
            return;
        }
        GMalloc->Free(ptr);
    }

    SIZE_T Memory::GetAllocSize(void* ptr)
    {
        if (ptr == nullptr || !GMalloc)
        {
            return 0;
        }

        SIZE_T size = 0;
        GMalloc->GetAllocationSize(ptr, size);
        return size;
    }

    void* Memory::MallocZeroed(SIZE_T count, uint32 alignment)
    {
        void* ptr = Malloc(count, alignment);
        if (ptr)
        {
            std::memset(ptr, 0, count);
        }
        return ptr;
    }

    SIZE_T Memory::QuantizeSize(SIZE_T count, uint32 alignment)
    {
        if (!GMalloc)
        {
            return count;
        }
        return GMalloc->QuantizeSize(count, alignment);
    }

    void* Memory::SystemMalloc(SIZE_T size)
    {
        if (size == 0)
        {
            return nullptr;
        }

#if PLATFORM_WINDOWS
        return _aligned_malloc(size, kMinAlignment);
#else
        SIZE_T alignedSize = (size + kMinAlignment - 1) & ~static_cast<SIZE_T>(kMinAlignment - 1);
        return aligned_alloc(kMinAlignment, alignedSize);
#endif
    }

    void Memory::SystemFree(void* ptr)
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

    void Memory::Trim(bool trimThreadCaches)
    {
        if (GMalloc)
        {
            GMalloc->Trim(trimThreadCaches);
        }
    }

    bool Memory::TestMemory()
    {
        if (!GMalloc)
        {
            return false;
        }
        return GMalloc->ValidateHeap();
    }

    bool Memory::IsGMallocReady()
    {
        return GMalloc != nullptr;
    }
} // namespace NS
