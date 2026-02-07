/// @file IRHIResource.cpp
/// @brief IRHIResource 基底クラス実装
#include "IRHIResource.h"

namespace NS::RHI
{
    //=========================================================================
    // ResourceId 生成
    //=========================================================================

    static std::atomic<ResourceId> s_nextResourceId{1};

    ResourceId GenerateResourceId()
    {
        return s_nextResourceId.fetch_add(1, std::memory_order_relaxed);
    }

    //=========================================================================
    // GPUMask ユーティリティ
    //=========================================================================

    uint32 GPUMask::GetFirstIndex() const
    {
        if (mask == 0)
            return kInvalidGPUIndex;

#if NS_COMPILER_MSVC
        unsigned long index;
        _BitScanForward(&index, mask);
        return static_cast<uint32>(index);
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
        return static_cast<uint32>(__builtin_ctz(mask));
#else
        for (uint32 i = 0; i < 32; ++i)
        {
            if (mask & (1u << i))
                return i;
        }
        return kInvalidGPUIndex;
#endif
    }

    uint32 GPUMask::CountBits() const
    {
#if NS_COMPILER_MSVC
        return __popcnt(mask);
#elif NS_COMPILER_GCC || NS_COMPILER_CLANG
        return static_cast<uint32>(__builtin_popcount(mask));
#else
        uint32 count = 0;
        uint32 v = mask;
        while (v)
        {
            count += v & 1;
            v >>= 1;
        }
        return count;
#endif
    }

    //=========================================================================
    // IRHIResource
    //=========================================================================

    IRHIResource::IRHIResource(ERHIResourceType type)
        : m_refCount(1), m_resourceId(GenerateResourceId()), m_resourceType(type)
    {}

    IRHIResource::~IRHIResource() = default;

    uint32 IRHIResource::AddRef() const noexcept
    {
        return m_refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32 IRHIResource::Release() const noexcept
    {
        uint32 count = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0)
        {
            OnZeroRefCount();
        }
        return count;
    }

    uint32 IRHIResource::GetRefCount() const noexcept
    {
        return m_refCount.load(std::memory_order_relaxed);
    }

    void IRHIResource::SetDebugName(const char* name)
    {
        std::lock_guard<std::mutex> lock(m_debugNameMutex);
        m_debugName = name ? name : "";
    }

    size_t IRHIResource::GetDebugName(char* outBuffer, size_t bufferSize) const
    {
        if (!outBuffer || bufferSize == 0)
            return 0;

        std::lock_guard<std::mutex> lock(m_debugNameMutex);
        const size_t len = (m_debugName.size() < bufferSize - 1) ? m_debugName.size() : (bufferSize - 1);
        std::memcpy(outBuffer, m_debugName.c_str(), len);
        outBuffer[len] = '\0';
        return len;
    }

    bool IRHIResource::HasDebugName() const
    {
        std::lock_guard<std::mutex> lock(m_debugNameMutex);
        return !m_debugName.empty();
    }

    void IRHIResource::MarkForDeferredDelete() const noexcept
    {
        m_pendingDelete = true;
    }

    bool IRHIResource::IsPendingDelete() const noexcept
    {
        return m_pendingDelete;
    }

    void IRHIResource::OnZeroRefCount() const
    {
        delete this;
    }

    void IRHIResource::ExecuteDeferredDelete() const
    {
        delete this;
    }

} // namespace NS::RHI
