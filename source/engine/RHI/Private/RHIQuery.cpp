/// @file RHIQuery.cpp
/// @brief クエリアロケーター実装
#include "RHIQuery.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIQueryAllocator
    //=========================================================================

    bool RHIQueryAllocator::Initialize(IRHIDevice* device,
                                       ERHIQueryType type,
                                       uint32 queriesPerFrame,
                                       uint32 numBufferedFrames)
    {
        m_device = device;
        m_type = type;
        m_queriesPerFrame = queriesPerFrame;
        m_numFrames = numBufferedFrames;

        m_frameData = new FrameData[numBufferedFrames];

        for (uint32 i = 0; i < numBufferedFrames; ++i)
        {
            // クエリヒープ作成
            RHIQueryHeapDesc heapDesc;
            heapDesc.type = type;
            heapDesc.count = queriesPerFrame;
            m_frameData[i].heap = device->CreateQueryHeap(heapDesc, "QueryHeap");

            // 結果バッファ作成
            uint32 resultSize = queriesPerFrame * sizeof(uint64); // Timestamp/Occlusionは64bit
            RHIBufferDesc bufferDesc;
            bufferDesc.size = resultSize;
            bufferDesc.usage = ERHIBufferUsage::None;
            bufferDesc.debugName = "QueryResultBuffer";
            m_frameData[i].resultBuffer = device->CreateBuffer(bufferDesc);

            m_frameData[i].allocatedCount = 0;
            m_frameData[i].resolved = false;
        }

        return true;
    }

    void RHIQueryAllocator::Shutdown()
    {
        if (m_frameData != nullptr)
        {
            for (uint32 i = 0; i < m_numFrames; ++i)
            {
                m_frameData[i].heap = nullptr;
                m_frameData[i].resultBuffer = nullptr;
            }
            delete[] m_frameData;
            m_frameData = nullptr;
        }

        m_device = nullptr;
        m_numFrames = 0;
    }

    void RHIQueryAllocator::BeginFrame(uint32 frameIndex)
    {
        m_currentFrame = frameIndex % m_numFrames;
        m_frameData[m_currentFrame].allocatedCount = 0;
        m_frameData[m_currentFrame].resolved = false;
    }

    void RHIQueryAllocator::EndFrame()
    {
        // 結果の解決はコマンドリストでResolveQueryData呼び出しが必要
        // ここではフラグ設定のみ
        m_frameData[m_currentFrame].resolved = true;
    }

    RHIQueryAllocation RHIQueryAllocator::Allocate(uint32 count)
    {
        auto& frame = m_frameData[m_currentFrame];
        if (frame.allocatedCount + count > m_queriesPerFrame)
        {
            return {};
        }

        RHIQueryAllocation alloc;
        alloc.heap = frame.heap.Get();
        alloc.startIndex = frame.allocatedCount;
        alloc.count = count;

        frame.allocatedCount += count;
        return alloc;
    }

    uint32 RHIQueryAllocator::GetAvailableCount() const
    {
        return m_queriesPerFrame - m_frameData[m_currentFrame].allocatedCount;
    }

    bool RHIQueryAllocator::AreResultsReady(uint32 frameIndex) const
    {
        uint32 const index = frameIndex % m_numFrames;
        return m_frameData[index].resolved;
    }

    IRHIBuffer* RHIQueryAllocator::GetResultBuffer(uint32 frameIndex) const
    {
        uint32 const index = frameIndex % m_numFrames;
        return m_frameData[index].resultBuffer.Get();
    }

} // namespace NS::RHI
