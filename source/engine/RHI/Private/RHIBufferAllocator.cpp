/// @file RHIBufferAllocator.cpp
/// @brief バッファメモリアロケーター実装
#include "RHIBufferAllocator.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHILinearBufferAllocator
    //=========================================================================

    bool RHILinearBufferAllocator::Initialize(IRHIDevice* device, uint64 size, ERHIHeapType heapType)
    {
        m_device = device;
        m_totalSize = size;
        m_currentOffset = 0;

        RHIBufferDesc desc;
        desc.size = size;
        desc.heapType = heapType;

        m_buffer = device->CreateBuffer(desc, "LinearAllocator");
        if (!m_buffer)
        {
            return false;
        }

        if (heapType == ERHIHeapType::Upload)
        {
            RHIMapResult const map = m_buffer->Map(ERHIMapMode::WriteDiscard, 0, 0);
            m_mappedPtr = map.data;
        }

        return true;
    }

    void RHILinearBufferAllocator::Shutdown()
    {
        if (m_buffer && (m_mappedPtr != nullptr))
        {
            m_buffer->Unmap(0, 0);
            m_mappedPtr = nullptr;
        }

        m_buffer = nullptr;
        m_device = nullptr;
        m_totalSize = 0;
        m_currentOffset = 0;
    }

    RHIBufferAllocation RHILinearBufferAllocator::Allocate(uint64 size, uint64 alignment)
    {
        RHIBufferAllocation result;

        if (alignment == 0)
        {
            alignment = 1;
        }

        uint64 const alignedOffset = (m_currentOffset + alignment - 1) & ~(alignment - 1);

        if (alignedOffset + size > m_totalSize)
        {
            return result;
        }

        result.buffer = m_buffer.Get();
        result.offset = alignedOffset;
        result.size = size;
        result.gpuAddress = m_buffer->GetGPUVirtualAddress() + alignedOffset;
        result.cpuAddress = (m_mappedPtr != nullptr) ? static_cast<uint8*>(m_mappedPtr) + alignedOffset : nullptr;

        m_currentOffset = alignedOffset + size;
        return result;
    }

    void RHILinearBufferAllocator::Reset()
    {
        m_currentOffset = 0;
    }

    //=========================================================================
    // RHIRingBufferAllocator
    //=========================================================================

    bool RHIRingBufferAllocator::Initialize(IRHIDevice* device, uint64 size, uint32 numFrames, ERHIHeapType heapType)
    {
        m_device = device;
        m_totalSize = size;
        m_numFrames = numFrames;
        m_head = 0;
        m_tail = 0;

        RHIBufferDesc desc;
        desc.size = size;
        desc.heapType = heapType;

        m_buffer = device->CreateBuffer(desc, "RingAllocator");
        if (!m_buffer)
        {
            return false;
        }

        m_frameAllocations = new FrameAllocation[numFrames];
        std::memset(m_frameAllocations, 0, sizeof(FrameAllocation) * numFrames);

        if (heapType == ERHIHeapType::Upload)
        {
            RHIMapResult const map = m_buffer->Map(ERHIMapMode::WriteDiscard, 0, 0);
            m_mappedPtr = map.data;
        }

        return true;
    }

    void RHIRingBufferAllocator::Shutdown()
    {
        if (m_buffer && (m_mappedPtr != nullptr))
        {
            m_buffer->Unmap(0, 0);
            m_mappedPtr = nullptr;
        }

        delete[] m_frameAllocations;
        m_frameAllocations = nullptr;
        m_buffer = nullptr;
        m_device = nullptr;
        m_totalSize = 0;
        m_head = 0;
        m_tail = 0;
    }

    void RHIRingBufferAllocator::BeginFrame(uint32 frameIndex, uint64 completedFrame)
    {
        m_currentFrame = frameIndex % m_numFrames;

        // 完了フレームの領域を解放
        for (uint32 i = 0; i < m_numFrames; ++i)
        {
            if (m_frameAllocations[i].frameNumber > 0 && m_frameAllocations[i].frameNumber <= completedFrame)
            {
                m_frameAllocations[i].frameNumber = 0;
            }
        }

        // tailを最古の未完了フレームのオフセットに更新
        uint64 oldestOffset = m_head;
        bool foundActive = false;
        for (uint32 i = 0; i < m_numFrames; ++i)
        {
            if (m_frameAllocations[i].frameNumber > 0)
            {
                if (!foundActive || m_frameAllocations[i].offset < oldestOffset)
                {
                    oldestOffset = m_frameAllocations[i].offset;
                    foundActive = true;
                }
            }
        }
        m_tail = foundActive ? oldestOffset : m_head;
    }

    void RHIRingBufferAllocator::EndFrame(uint64 frameNumber)
    {
        m_frameAllocations[m_currentFrame].frameNumber = frameNumber;
        m_frameAllocations[m_currentFrame].offset = m_head;
    }

    RHIBufferAllocation RHIRingBufferAllocator::Allocate(uint64 size, uint64 alignment)
    {
        RHIBufferAllocation result;

        if (alignment == 0)
        {
            alignment = 1;
        }

        uint64 alignedHead = (m_head + alignment - 1) & ~(alignment - 1);

        if (alignedHead + size > m_totalSize)
        {
            // ラップアラウンド試行
            alignedHead = (alignment - 1) & ~(alignment - 1);
            if (alignedHead + size > m_tail)
            {
                return result;
            }
            m_head = alignedHead;
        }
        else if (m_head < m_tail && alignedHead + size > m_tail)
        {
            return result;
        }

        result.buffer = m_buffer.Get();
        result.offset = alignedHead;
        result.size = size;
        result.gpuAddress = m_buffer->GetGPUVirtualAddress() + alignedHead;
        result.cpuAddress = (m_mappedPtr != nullptr) ? static_cast<uint8*>(m_mappedPtr) + alignedHead : nullptr;

        m_head = alignedHead + size;
        return result;
    }

    uint64 RHIRingBufferAllocator::GetUsedSize() const
    {
        if (m_head >= m_tail)
        {
            return m_head - m_tail;
        }
        return m_totalSize - m_tail + m_head;
    }

    //=========================================================================
    // RHIBufferPool
    //=========================================================================

    bool RHIBufferPool::Initialize(IRHIDevice* device, const RHIBufferPoolConfig& config)
    {
        m_device = device;
        m_config = config;

        uint32 const capacity = config.initialBlockCount > 0 ? config.initialBlockCount : 16;
        m_freeList = new IRHIBuffer*[capacity];
        m_freeCapacity = capacity;
        m_freeCount = 0;
        m_totalCount = 0;

        // 初期ブロック事前確保
        for (uint32 i = 0; i < config.initialBlockCount; ++i)
        {
            RHIBufferDesc desc;
            desc.size = config.blockSize;
            desc.heapType = config.heapType;
            desc.usage = config.usage;

            IRHIBuffer* buffer = device->CreateBuffer(desc, "PoolBuffer");
            if (buffer != nullptr)
            {
                m_freeList[m_freeCount++] = buffer;
                ++m_totalCount;
            }
        }

        return true;
    }

    void RHIBufferPool::Shutdown()
    {
        for (uint32 i = 0; i < m_freeCount; ++i)
        {
            if (m_freeList[i] != nullptr)
            {
                m_freeList[i]->Release();
            }
        }

        delete[] m_freeList;
        m_freeList = nullptr;
        m_freeCount = 0;
        m_freeCapacity = 0;
        m_totalCount = 0;
        m_device = nullptr;
    }

    IRHIBuffer* RHIBufferPool::Acquire()
    {
        if (m_freeCount > 0)
        {
            return m_freeList[--m_freeCount];
        }

        if (m_config.maxBlockCount > 0 && m_totalCount >= m_config.maxBlockCount)
        {
            return nullptr;
        }

        RHIBufferDesc desc;
        desc.size = m_config.blockSize;
        desc.heapType = m_config.heapType;
        desc.usage = m_config.usage;

        IRHIBuffer* buffer = m_device->CreateBuffer(desc, "PoolBuffer");
        if (buffer != nullptr)
        {
            ++m_totalCount;
        }

        return buffer;
    }

    void RHIBufferPool::Release(IRHIBuffer* buffer)
    {
        if (buffer == nullptr)
        {
            return;
        }

        if (m_freeCount >= m_freeCapacity)
        {
            uint32 const newCapacity = m_freeCapacity * 2;
            auto** newList = new IRHIBuffer*[newCapacity];
            std::memcpy(newList, m_freeList, sizeof(IRHIBuffer*) * m_freeCount);
            delete[] m_freeList;
            m_freeList = newList;
            m_freeCapacity = newCapacity;
        }

        m_freeList[m_freeCount++] = buffer;
    }

    //=========================================================================
    // RHIMultiSizeBufferPool
    //=========================================================================

    bool RHIMultiSizeBufferPool::Initialize(IRHIDevice* device,
                                            const uint64* sizes,
                                            uint32 count,
                                            ERHIHeapType heapType)
    {
        m_device = device;
        m_poolCount = count;
        m_pools = new RHIBufferPool[count];

        for (uint32 i = 0; i < count; ++i)
        {
            RHIBufferPoolConfig config;
            config.blockSize = sizes[i];
            config.heapType = heapType;
            config.initialBlockCount = 4;

            if (!m_pools[i].Initialize(device, config))
            {
                Shutdown();
                return false;
            }
        }

        return true;
    }

    void RHIMultiSizeBufferPool::Shutdown()
    {
        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            m_pools[i].Shutdown();
        }

        delete[] m_pools;
        m_pools = nullptr;
        m_poolCount = 0;
        m_device = nullptr;
    }

    IRHIBuffer* RHIMultiSizeBufferPool::Acquire(uint64 minSize)
    {
        // 要求サイズ以上の最小プールから取得
        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            if (m_pools[i].GetBlockSize() >= minSize)
            {
                return m_pools[i].Acquire();
            }
        }
        return nullptr;
    }

    void RHIMultiSizeBufferPool::Release(IRHIBuffer* buffer)
    {
        if (buffer == nullptr)
        {
            return;
        }

        uint64 const bufferSize = buffer->GetSize();

        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            if (m_pools[i].GetBlockSize() == bufferSize)
            {
                m_pools[i].Release(buffer);
                return;
            }
        }

        // 一致するプールがない場合はそのままリリース
        buffer->Release();
    }

    //=========================================================================
    // RHIConstantBufferAllocator
    //=========================================================================

    bool RHIConstantBufferAllocator::Initialize(IRHIDevice* device, uint64 size)
    {
        return m_ringBuffer.Initialize(device, size, 3, ERHIHeapType::Upload);
    }

    void RHIConstantBufferAllocator::Shutdown()
    {
        m_ringBuffer.Shutdown();
    }

    void RHIConstantBufferAllocator::BeginFrame(uint32 frameIndex)
    {
        m_currentFrameIndex = frameIndex;
        m_ringBuffer.BeginFrame(frameIndex, frameIndex > 0 ? frameIndex - 1 : 0);
    }

    void RHIConstantBufferAllocator::EndFrame()
    {
        m_ringBuffer.EndFrame(m_currentFrameIndex);
    }

    RHIBufferAllocation RHIConstantBufferAllocator::Allocate(uint64 size)
    {
        uint64 const alignedSize = (size + kCBVAlignment - 1) & ~(kCBVAlignment - 1);
        return m_ringBuffer.Allocate(alignedSize, kCBVAlignment);
    }

    //=========================================================================
    // RHIDynamicBufferManager
    //=========================================================================

    bool RHIDynamicBufferManager::Initialize(IRHIDevice* device, uint64 uploadBufferSize, uint64 constantBufferSize)
    {
        if (!m_uploadAllocator.Initialize(device, uploadBufferSize, 3, ERHIHeapType::Upload))
        {
            return false;
        }

        if (!m_constantAllocator.Initialize(device, constantBufferSize))
        {
            m_uploadAllocator.Shutdown();
            return false;
        }

        return true;
    }

    void RHIDynamicBufferManager::Shutdown()
    {
        m_constantAllocator.Shutdown();
        m_uploadAllocator.Shutdown();
    }

    void RHIDynamicBufferManager::BeginFrame(uint32 frameIndex, uint64 completedFrame)
    {
        m_uploadAllocator.BeginFrame(frameIndex, completedFrame);
        m_constantAllocator.BeginFrame(frameIndex);
    }

    void RHIDynamicBufferManager::EndFrame(uint64 frameNumber)
    {
        m_uploadAllocator.EndFrame(frameNumber);
        m_constantAllocator.EndFrame();
    }

} // namespace NS::RHI
