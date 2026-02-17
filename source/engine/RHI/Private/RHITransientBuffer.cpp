/// @file RHITransientBuffer.cpp
/// @brief Transientバッファプール実装
#include "RHITransientBuffer.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHITransientBufferHandle
    //=========================================================================

    IRHIBuffer* RHITransientBufferHandle::GetBuffer() const
    {
        return m_acquiredBuffer;
    }

    //=========================================================================
    // RHITransientBufferPool
    //=========================================================================

    RHITransientBufferPool::RHITransientBufferPool(IRHIDevice* device) : m_device(device) {}

    RHITransientBufferPool::~RHITransientBufferPool()
    {
        Clear();
    }

    ERHIBufferUsage RHITransientBufferPool::TransientToBufferUsage(ERHITransientBufferUsage transientUsage)
    {
        ERHIBufferUsage result = ERHIBufferUsage::None;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Vertex))
            result |= ERHIBufferUsage::VertexBuffer;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Index))
            result |= ERHIBufferUsage::IndexBuffer;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Constant))
            result |= ERHIBufferUsage::ConstantBuffer;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Structured))
            result |= ERHIBufferUsage::StructuredBuffer | ERHIBufferUsage::ShaderResource;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Raw))
            result |= ERHIBufferUsage::ByteAddressBuffer;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::Indirect))
            result |= ERHIBufferUsage::IndirectArgs;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::CopySource))
            result |= ERHIBufferUsage::CopySource;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::CopyDest))
            result |= ERHIBufferUsage::CopyDest;
        if (EnumHasAnyFlags(transientUsage, ERHITransientBufferUsage::UAV))
            result |= ERHIBufferUsage::UnorderedAccess;
        return result;
    }

    RHIBufferRef RHITransientBufferPool::Acquire(const RHITransientBufferCreateInfo& info)
    {
        PoolKey const key{.size = info.size, .usage = info.usage};

        auto it = m_pools.find(key);
        if (it != m_pools.end() && !it->second.empty())
        {
            RHIBufferRef buffer = std::move(it->second.back());
            it->second.pop_back();
            m_bufferToKey[buffer.get()] = key;
            return buffer;
        }

        // 新規バッファ作成
        RHIBufferDesc desc;
        desc.size = info.size;
        desc.stride = info.structureByteStride;
        desc.usage = TransientToBufferUsage(info.usage);

        RHIBufferRef buffer(m_device->CreateBuffer(desc, info.debugName));
        if (buffer)
        {
            m_bufferToKey[buffer.get()] = key;
        }
        return buffer;
    }

    void RHITransientBufferPool::Release(RHIBufferRef buffer)
    {
        if (!buffer)
        {
            return;
        }

        // Acquire時に記録したキーを検索
        auto it = m_bufferToKey.find(buffer.get());
        if (it != m_bufferToKey.end())
        {
            PoolKey key = it->second;
            m_bufferToKey.erase(it);
            m_pendingRelease.emplace_back(key, std::move(buffer));
        }
        else
        {
            // 不明なバッファ: サイズのみでフォールバック
            uint64 const bufferSize = buffer->GetSize();
            PoolKey key{.size = bufferSize, .usage = ERHITransientBufferUsage::None};
            m_pendingRelease.emplace_back(key, std::move(buffer));
        }
    }

    void RHITransientBufferPool::OnFrameEnd()
    {
        for (auto& [key, buffer] : m_pendingRelease)
        {
            m_pools[key].push_back(std::move(buffer));
        }
        m_pendingRelease.clear();
    }

    void RHITransientBufferPool::Clear()
    {
        m_pools.clear();
        m_bufferToKey.clear();
        m_pendingRelease.clear();
    }

    uint32 RHITransientBufferPool::GetPooledBufferCount() const
    {
        uint32 count = 0;
        for (const auto& [key, buffers] : m_pools)
        {
            count += static_cast<uint32>(buffers.size());
        }
        return count;
    }

    uint64 RHITransientBufferPool::GetTotalPooledMemory() const
    {
        uint64 total = 0;
        for (const auto& [key, buffers] : m_pools)
        {
            total += key.size * buffers.size();
        }
        return total;
    }

} // namespace NS::RHI
