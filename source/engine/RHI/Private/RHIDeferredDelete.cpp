/// @file RHIDeferredDelete.cpp
/// @brief 遅延削除キュー実装
#include "RHIDeferredDelete.h"
#include "IRHIFence.h"
#include "IRHIResource.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIDeferredDeleteQueue
    //=========================================================================

    RHIDeferredDeleteQueue::RHIDeferredDeleteQueue()
    {
        m_entries.reserve(256);
    }

    RHIDeferredDeleteQueue::~RHIDeferredDeleteQueue()
    {
        FlushAll();
    }

    void RHIDeferredDeleteQueue::SetMaxDeferredFrames(uint32 frames)
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        m_maxDeferredFrames = frames;
    }

    void RHIDeferredDeleteQueue::SetCurrentFrame(uint64 frameNumber)
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        m_currentFrame = frameNumber;
    }

    void RHIDeferredDeleteQueue::SetMemoryPressureHandler(RHIMemoryPressureHandler* handler)
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        m_pressureHandler = handler;
    }

    void RHIDeferredDeleteQueue::SetPressureThreshold(uint32 threshold)
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        m_pressureThreshold = threshold;
    }

    void RHIDeferredDeleteQueue::Enqueue(const IRHIResource* resource, IRHIFence* fence, uint64 fenceValue)
    {
        std::lock_guard<std::mutex> const lock(m_mutex);

        RHIDeferredDeleteEntry entry;
        entry.resource = resource;
        entry.fence = fence;
        entry.fenceValue = fenceValue;
        entry.frameNumber = m_currentFrame;
        m_entries.push_back(entry);

        // メモリプレッシャー通知
        if ((m_pressureHandler != nullptr) && m_entries.size() >= m_pressureThreshold)
        {
            ERHIMemoryPressure level = ERHIMemoryPressure::Low;
            if (m_entries.size() >= static_cast<size_t>(m_pressureThreshold) * 4) {
                level = ERHIMemoryPressure::Critical;
            } else if (m_entries.size() >= static_cast<size_t>(m_pressureThreshold) * 2) {
                level = ERHIMemoryPressure::High;
            } else if (m_entries.size() >= m_pressureThreshold) {
                level = ERHIMemoryPressure::Medium;
}
            m_pressureHandler->NotifyPressureChange(level);
        }
    }

    void RHIDeferredDeleteQueue::EnqueueFrameDeferred(const IRHIResource* resource)
    {
        Enqueue(resource, nullptr, 0);
    }

    void RHIDeferredDeleteQueue::DeleteImmediate(const IRHIResource* resource)
    {
        if (resource != nullptr)
        {
            resource->Release();
        }
    }

    uint32 RHIDeferredDeleteQueue::ProcessCompletedDeletions()
    {
        std::lock_guard<std::mutex> const lock(m_mutex);

        uint32 deletedCount = 0;
        auto it = m_entries.begin();

        while (it != m_entries.end())
        {
            bool canDelete = false;

            if (it->fence != nullptr)
            {
                // フェンスベースの待機
                canDelete = it->fence->IsCompleted(it->fenceValue);
            }
            else
            {
                // フレームベースの待機
                canDelete = m_currentFrame >= it->frameNumber &&
                           (m_currentFrame - it->frameNumber) >= m_maxDeferredFrames;
            }

            if (canDelete)
            {
                if (it->resource != nullptr) {
                    it->resource->Release();
}
                it = m_entries.erase(it);
                ++deletedCount;
            }
            else
            {
                ++it;
            }
        }

        return deletedCount;
    }

    void RHIDeferredDeleteQueue::FlushAll()
    {
        std::lock_guard<std::mutex> const lock(m_mutex);

        for (auto& entry : m_entries)
        {
            if (entry.resource != nullptr) {
                entry.resource->Release();
}
        }
        m_entries.clear();
    }

    size_t RHIDeferredDeleteQueue::GetPendingCount() const
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        return m_entries.size();
    }

    size_t RHIDeferredDeleteQueue::GetPendingMemoryEstimate() const
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        // 各リソースのメモリサイズ取得はバックエンド依存
        return m_entries.size() * 4096; // 概算
    }

} // namespace NS::RHI
