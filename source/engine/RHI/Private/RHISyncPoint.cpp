/// @file RHISyncPoint.cpp
/// @brief 蜷梧悄繝昴う繝ｳ繝医・繝輔Ξ繝ｼ繝蜷梧悄繝ｻ繝代う繝励Λ繧､繝ｳ蜷梧悄繝ｻ繧ｿ繧､繝繝ｩ繧､繝ｳ蜷梧悄螳溯｣・
#include "RHISyncPoint.h"
#include "IRHIDevice.h"
#include "IRHIQueue.h"

#include <chrono>
#include <thread>

namespace NS::RHI
{
    //=========================================================================
    // RHIFrameSync
    //=========================================================================

    bool RHIFrameSync::Initialize(IRHIDevice* device, uint32 numBufferedFrames)
    {
        m_device = device;
        if (numBufferedFrames == 0)
        {
            numBufferedFrames = 1;
        }
        m_numBufferedFrames = (numBufferedFrames <= kMaxBufferedFrames) ? numBufferedFrames : kMaxBufferedFrames;
        m_currentFrameIndex = 0;
        m_frameNumber = 0;

        for (unsigned long long& m_frameFenceValue : m_frameFenceValues)
        {
            m_frameFenceValue = 0;
        }

        RHIFenceDesc fenceDesc;
        fenceDesc.initialValue = 0;
        m_frameFence = device->CreateFence(fenceDesc, "FrameSyncFence");

        return m_frameFence != nullptr;
    }

    void RHIFrameSync::Shutdown()
    {
        if (m_frameFence)
        {
            WaitForAllFrames();
        }

        m_frameFence = nullptr;
        m_device = nullptr;
    }

    void RHIFrameSync::BeginFrame()
    {
        // 迴ｾ蝨ｨ縺ｮ繝輔Ξ繝ｼ繝繧ｹ繝ｭ繝・ヨ縺御ｽｿ逕ｨ荳ｭ縺ｪ繧牙ｮ御ｺ・ｒ蠕・ｩ・
        uint64 const fenceValue = m_frameFenceValues[m_currentFrameIndex];
        if (fenceValue > 0 && m_frameFence && !m_frameFence->IsCompleted(fenceValue))
        {
            m_frameFence->Wait(fenceValue, 30000);
        }
    }

    void RHIFrameSync::EndFrame(IRHIQueue* queue)
    {
        ++m_frameNumber;
        m_frameFenceValues[m_currentFrameIndex] = m_frameNumber;

        if ((queue != nullptr) && m_frameFence)
        {
            queue->Signal(m_frameFence.Get(), m_frameNumber);
        }

        m_currentFrameIndex = (m_numBufferedFrames > 0) ? static_cast<uint32>(m_frameNumber % m_numBufferedFrames) : 0;
    }

    uint64 RHIFrameSync::GetCompletedFrameNumber() const
    {
        if (!m_frameFence)
        {
            return 0;
        }
        return m_frameFence->GetCompletedValue();
    }

    uint32 RHIFrameSync::GetFramesInFlight() const
    {
        uint64 const completed = GetCompletedFrameNumber();
        return static_cast<uint32>(m_frameNumber - completed);
    }

    RHISyncPoint RHIFrameSync::GetCurrentFrameSyncPoint() const
    {
        RHISyncPoint sp;
        sp.fence = m_frameFence.Get();
        sp.value = m_frameFenceValues[m_currentFrameIndex];
        return sp;
    }

    RHISyncPoint RHIFrameSync::GetFrameSyncPoint(uint64 frameNumber) const
    {
        RHISyncPoint sp;
        sp.fence = m_frameFence.Get();
        sp.value = frameNumber;
        return sp;
    }

    bool RHIFrameSync::WaitForFrame(uint64 frameNumber, uint64 timeoutMs)
    {
        if (!m_frameFence)
        {
            return true;
        }
        if (m_frameFence->IsCompleted(frameNumber))
        {
            return true;
        }
        return m_frameFence->Wait(frameNumber, timeoutMs);
    }

    void RHIFrameSync::WaitForAllFrames()
    {
        if (!m_frameFence || m_frameNumber == 0)
        {
            return;
        }
        m_frameFence->Wait(m_frameNumber, 30000);
    }

    //=========================================================================
    // RHIPipelineSync
    //=========================================================================

    bool RHIPipelineSync::Initialize(IRHIDevice* device)
    {
        m_device = device;
        m_nextSyncValue = 1;

        RHIFenceDesc fenceDesc;
        fenceDesc.initialValue = 0;
        m_syncFence = device->CreateFence(fenceDesc, "PipelineSyncFence");

        ResetFrameGraph();
        return m_syncFence != nullptr;
    }

    void RHIPipelineSync::Shutdown()
    {
        m_syncFence = nullptr;
        m_device = nullptr;
    }

    RHISyncPoint RHIPipelineSync::InsertSyncPoint(IRHIQueue* fromQueue)
    {
        uint64 const value = m_nextSyncValue++;
        if ((fromQueue != nullptr) && m_syncFence)
        {
            fromQueue->Signal(m_syncFence.Get(), value);
        }

        RHISyncPoint sp;
        sp.fence = m_syncFence.Get();
        sp.value = value;
        return sp;
    }

    void RHIPipelineSync::WaitForSyncPoint(IRHIQueue* queue, const RHISyncPoint& syncPoint)
    {
        if ((queue != nullptr) && syncPoint.IsValid())
        {
            queue->Wait(syncPoint.fence, syncPoint.value);
        }
    }

    void RHIPipelineSync::SyncQueues(IRHIQueue* fromQueue, IRHIQueue* toQueue)
    {
        auto sp = InsertSyncPoint(fromQueue);
        WaitForSyncPoint(toQueue, sp);
    }

    bool RHIPipelineSync::ValidateNoCircularDependency(uint32 fromQueue, uint32 toQueue) const
    {
        if (fromQueue >= kMaxQueues || toQueue >= kMaxQueues)
        {
            return true;
        }

        // 蜊倡ｴ斐↑DFS: toQueue -> fromQueue 繝代せ縺悟ｭ伜惠縺吶ｋ縺狗｢ｺ隱・
        bool visited[kMaxQueues] = {};
        uint32 stack[kMaxQueues];
        uint32 stackSize = 0;

        stack[stackSize++] = toQueue;
        visited[toQueue] = true;

        while (stackSize > 0)
        {
            uint32 const current = stack[--stackSize];
            if (current == fromQueue)
            {
                return false; // 蠕ｪ迺ｰ讀懷・
            }

            for (uint32 i = 0; i < kMaxQueues; ++i)
            {
                if (m_syncGraph[current][i] > 0 && !visited[i])
                {
                    visited[i] = true;
                    stack[stackSize++] = i;
                }
            }
        }
        return true;
    }

    void RHIPipelineSync::ResetFrameGraph()
    {
        for (auto& i : m_syncGraph)
        {
            for (uint32 j = 0; j < kMaxQueues; ++j)
            {
                i[j] = 0;
            }
        }
    }

    //=========================================================================
    // RHISyncPointWaiter
    //=========================================================================

    bool RHISyncPointWaiter::WaitAll(uint64 timeoutMs)
    {
        for (uint32 i = 0; i < m_count; ++i)
        {
            if (!m_syncPoints[i].Wait(timeoutMs))
            {
                return false;
            }
        }
        return true;
    }

    int32 RHISyncPointWaiter::WaitAny(uint64 timeoutMs)
    {
        using clock = std::chrono::steady_clock;
        if (timeoutMs == UINT64_MAX)
        {
            for (;;)
            {
                for (uint32 i = 0; i < m_count; ++i)
                {
                    if (m_syncPoints[i].IsCompleted())
                    {
                        return static_cast<int32>(i);
                    }
                }
                std::this_thread::yield();
            }
        }

        uint64 clampedTimeoutMs = timeoutMs;
        auto const maxMsCount = static_cast<uint64>((std::chrono::milliseconds::max)().count());
        if (clampedTimeoutMs > maxMsCount)
        {
            clampedTimeoutMs = maxMsCount;
        }

        auto const deadline = clock::now() + std::chrono::milliseconds(clampedTimeoutMs);
        do
        {
            for (uint32 i = 0; i < m_count; ++i)
            {
                if (m_syncPoints[i].IsCompleted())
                {
                    return static_cast<int32>(i);
                }
            }
            std::this_thread::yield();
        } while (clock::now() < deadline);

        return -1;
    }

    bool RHISyncPointWaiter::AreAllCompleted() const
    {
        for (uint32 i = 0; i < m_count; ++i)
        {
            if (!m_syncPoints[i].IsCompleted())
            {
                return false;
            }
        }
        return true;
    }

    bool RHISyncPointWaiter::IsAnyCompleted() const
    {
        for (uint32 i = 0; i < m_count; ++i)
        {
            if (m_syncPoints[i].IsCompleted())
            {
                return true;
            }
        }
        return false;
    }

    //=========================================================================
    // RHITimelineSync
    //=========================================================================

    bool RHITimelineSync::Initialize(IRHIDevice* device)
    {
        RHIFenceDesc fenceDesc;
        fenceDesc.initialValue = 0;
        m_fence = device->CreateFence(fenceDesc, "TimelineSyncFence");
        m_nextValue = 1;
        return m_fence != nullptr;
    }

    void RHITimelineSync::Shutdown()
    {
        m_fence = nullptr;
    }

    uint64 RHITimelineSync::GetCurrentValue() const
    {
        return m_fence ? m_fence->GetCompletedValue() : 0;
    }

    uint64 RHITimelineSync::Signal(IRHIQueue* queue)
    {
        uint64 const value = m_nextValue++;
        if ((queue != nullptr) && m_fence)
        {
            queue->Signal(m_fence.Get(), value);
        }
        return value;
    }

    void RHITimelineSync::Wait(IRHIQueue* queue, uint64 value)
    {
        if ((queue != nullptr) && m_fence)
        {
            queue->Wait(m_fence.Get(), value);
        }
    }

    bool RHITimelineSync::WaitCPU(uint64 value, uint64 timeoutMs)
    {
        if (!m_fence)
        {
            return true;
        }
        return m_fence->Wait(value, timeoutMs);
    }

    RHISyncPoint RHITimelineSync::GetSyncPoint(uint64 value) const
    {
        RHISyncPoint sp;
        sp.fence = m_fence.Get();
        sp.value = value;
        return sp;
    }

} // namespace NS::RHI
