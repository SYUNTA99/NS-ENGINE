/// @file RHIResidency.cpp
/// @brief GPUメモリ常駐管理・テクスチャストリーミング実装
#include "RHIResidency.h"
#include "IRHIDevice.h"
#include "IRHIFence.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIResidencyManager
    //=========================================================================

    bool RHIResidencyManager::Initialize(IRHIDevice* device,
                                         const RHIResidencyManagerConfig& config,
                                         IRHIFence* fence,
                                         IRHIQueue* queue)
    {
        m_device = device;
        m_config = config;
        m_fence = fence;
        m_queue = queue;
        m_currentFrame = 0;
        m_residentCount = 0;
        m_evictedCount = 0;

        // VRAM予算の取得はバックエンド依存
        m_budget = config.maxVideoMemoryUsage;
        m_currentUsage = 0;

        m_trackedCapacity = 256;
        m_trackedResources = new TrackedResource[m_trackedCapacity];
        m_trackedCount = 0;

        return true;
    }

    void RHIResidencyManager::Shutdown()
    {
        delete[] m_trackedResources;
        m_trackedResources = nullptr;
        m_trackedCount = 0;
        m_trackedCapacity = 0;
        m_residentCount = 0;
        m_evictedCount = 0;
        m_fence = nullptr;
        m_queue = nullptr;
        m_device = nullptr;
    }

    void RHIResidencyManager::BeginFrame(uint64 frameNumber)
    {
        m_currentFrame = frameNumber;
    }

    void RHIResidencyManager::EndFrame()
    {
        // 未使用フレーム数が閾値を超えたリソースの退避判定
        if (GetUsageRatio() > m_config.evictionThreshold)
        {
            PerformEviction();
        }
    }

    void RHIResidencyManager::RegisterResource(IRHIResidentResource* resource)
    {
        if (resource == nullptr)
        {
            return;
        }

        // 容量拡張
        if (m_trackedCount >= m_trackedCapacity)
        {
            uint32 const newCap = m_trackedCapacity * 2;
            auto* newArray = new TrackedResource[newCap];
            std::memcpy(newArray, m_trackedResources, sizeof(TrackedResource) * m_trackedCount);
            delete[] m_trackedResources;
            m_trackedResources = newArray;
            m_trackedCapacity = newCap;
        }

        auto& tracked = m_trackedResources[m_trackedCount++];
        tracked.resource = resource;
        tracked.lastUsedFrame = m_currentFrame;
        tracked.lastUsedFenceValue = 0;
        tracked.status = ERHIResidencyStatus::Resident;

        m_currentUsage += resource->GetSize();
        ++m_residentCount;
    }

    void RHIResidencyManager::UnregisterResource(IRHIResidentResource* resource)
    {
        if (resource == nullptr)
        {
            return;
        }

        for (uint32 i = 0; i < m_trackedCount; ++i)
        {
            if (m_trackedResources[i].resource == resource)
            {
                if (m_trackedResources[i].status == ERHIResidencyStatus::Resident)
                {
                    m_currentUsage -= resource->GetSize();
                    --m_residentCount;
                }
                else
                {
                    --m_evictedCount;
                }

                // 末尾と入れ替え
                m_trackedResources[i] = m_trackedResources[m_trackedCount - 1];
                --m_trackedCount;
                return;
            }
        }
    }

    void RHIResidencyManager::MarkUsed(IRHIResidentResource* resource, uint64 fenceValue)
    {
        if (resource == nullptr)
        {
            return;
        }

        for (uint32 i = 0; i < m_trackedCount; ++i)
        {
            if (m_trackedResources[i].resource == resource)
            {
                m_trackedResources[i].lastUsedFrame = m_currentFrame;
                m_trackedResources[i].lastUsedFenceValue = fenceValue;
                resource->SetLastUsed(m_currentFrame, fenceValue);
                return;
            }
        }
    }

    void RHIResidencyManager::MarkUsed(IRHIResidentResource* const* resources, uint32 count, uint64 fenceValue)
    {
        for (uint32 i = 0; i < count; ++i)
        {
            MarkUsed(resources[i], fenceValue);
        }
    }

    bool RHIResidencyManager::EnsureResident(IRHIResidentResource* resource)
    {
        return EnsureResident(&resource, 1);
    }

    bool RHIResidencyManager::EnsureResident(IRHIResidentResource* const* resources, uint32 count)
    {
        // 退避済みリソースの常駐化はバックエンド依存
        // （ID3D12Device::MakeResident等を呼ぶ）
        for (uint32 i = 0; i < count; ++i)
        {
            if (resources[i] == nullptr)
            {
                continue;
            }

            for (uint32 j = 0; j < m_trackedCount; ++j)
            {
                if (m_trackedResources[j].resource == resources[i] &&
                    m_trackedResources[j].status == ERHIResidencyStatus::Evicted)
                {
                    m_trackedResources[j].status = ERHIResidencyStatus::Resident;
                    m_currentUsage += resources[i]->GetSize();
                    ++m_residentCount;
                    --m_evictedCount;
                }
            }
        }
        return true;
    }

    void RHIResidencyManager::PerformEviction()
    {
        if (m_budget == 0 || GetUsageRatio() <= m_config.evictionTarget)
        {
            return;
        }

        uint64 const targetReduction =
            m_currentUsage - static_cast<uint64>(static_cast<double>(m_budget) * m_config.evictionTarget);

        TrackedResource* candidates[64];
        uint32 candidateCount = 0;
        SelectEvictionCandidates(candidates, candidateCount, targetReduction);

        // 退避実行はバックエンド依存（ID3D12Device::Evict等）
        for (uint32 i = 0; i < candidateCount; ++i)
        {
            candidates[i]->status = ERHIResidencyStatus::Evicted;
            m_currentUsage -= candidates[i]->resource->GetSize();
            --m_residentCount;
            ++m_evictedCount;
        }
    }

    bool RHIResidencyManager::EnqueueMakeResident(IRHIResidentResource* const* resources,
                                                  uint32 count,
                                                  IRHIFence* fenceToSignal,
                                                  uint64 fenceValue)
    {
        // 非同期常駐化はバックエンド依存
        (void)resources;
        (void)count;
        (void)fenceToSignal;
        (void)fenceValue;
        return true;
    }

    void RHIResidencyManager::SelectEvictionCandidates(TrackedResource** outCandidates,
                                                       uint32& outCount,
                                                       uint64 targetSize)
    {
        outCount = 0;
        uint64 accumulated = 0;

        // LRU: 最も古い使用フレームのリソースから退避候補に選択
        // 優先度が低いリソースを優先
        for (uint32 i = 0; i < m_trackedCount && accumulated < targetSize; ++i)
        {
            auto& tracked = m_trackedResources[i];

            if (tracked.status != ERHIResidencyStatus::Resident)
            {
                continue;
            }

            if (tracked.resource->GetResidencyPriority() >= ERHIResidencyPriority::Maximum)
            {
                continue;
            }

            uint64 const unusedFrames = m_currentFrame - tracked.lastUsedFrame;
            if (unusedFrames >= m_config.unusedFramesBeforeEvict)
            {
                if (outCount < 64)
                {
                    outCandidates[outCount++] = &tracked;
                    accumulated += tracked.resource->GetSize();
                }
            }
        }
    }

    //=========================================================================
    // RHITextureStreamingManager
    //=========================================================================

    bool RHITextureStreamingManager::Initialize(IRHIDevice* device,
                                                RHIResidencyManager* residencyManager,
                                                uint64 streamingBudget)
    {
        m_device = device;
        m_residencyManager = residencyManager;
        m_budget = streamingBudget;

        m_entryCapacity = 128;
        m_entries = new StreamingEntry[m_entryCapacity];
        m_entryCount = 0;

        return true;
    }

    void RHITextureStreamingManager::Shutdown()
    {
        delete[] m_entries;
        m_entries = nullptr;
        m_entryCount = 0;
        m_entryCapacity = 0;
        m_residencyManager = nullptr;
        m_device = nullptr;
    }

    void RHITextureStreamingManager::BeginFrame()
    {
        // フレーム開始時の優先度リセット
    }

    void RHITextureStreamingManager::EndFrame()
    {
        ProcessStreaming();
    }

    void RHITextureStreamingManager::RegisterResource(IRHIStreamingResource* resource)
    {
        if (resource == nullptr)
        {
            return;
        }

        if (m_entryCount >= m_entryCapacity)
        {
            uint32 const newCap = m_entryCapacity * 2;
            auto* newEntries = new StreamingEntry[newCap];
            std::memcpy(newEntries, m_entries, sizeof(StreamingEntry) * m_entryCount);
            delete[] m_entries;
            m_entries = newEntries;
            m_entryCapacity = newCap;
        }

        auto& entry = m_entries[m_entryCount++];
        entry.resource = resource;
        entry.distance = 0.0F;
        entry.priority = 0.0F;
    }

    void RHITextureStreamingManager::UnregisterResource(IRHIStreamingResource* resource)
    {
        if (resource == nullptr)
        {
            return;
        }

        for (uint32 i = 0; i < m_entryCount; ++i)
        {
            if (m_entries[i].resource == resource)
            {
                m_entries[i] = m_entries[m_entryCount - 1];
                --m_entryCount;
                return;
            }
        }
    }

    void RHITextureStreamingManager::UpdateResourceDistance(IRHIStreamingResource* resource, float distance)
    {
        if (resource == nullptr)
        {
            return;
        }

        for (uint32 i = 0; i < m_entryCount; ++i)
        {
            if (m_entries[i].resource == resource)
            {
                m_entries[i].distance = distance;
                // 距離から優先度を計算（近いほど高優先度）
                m_entries[i].priority = distance > 0.0F ? 1.0F / distance : 1000.0F;
                return;
            }
        }
    }

    void RHITextureStreamingManager::SetStreamingBudget(uint64 budget)
    {
        m_budget = budget;
    }

    void RHITextureStreamingManager::ForceLoad(IRHIStreamingResource* resource, ERHIStreamingLevel level)
    {
        if (resource == nullptr)
        {
            return;
        }

        resource->RequestStreamingLevel(level);
    }

    void RHITextureStreamingManager::ProcessStreaming()
    {
        // 優先度に基づいてストリーミングレベルを調整
        // 実際のストリーミング処理はバックエンド/ファイルシステム依存
        for (uint32 i = 0; i < m_entryCount; ++i)
        {
            auto& entry = m_entries[i];
            if (entry.resource == nullptr)
            {
                continue;
            }

            ERHIStreamingLevel const current = entry.resource->GetCurrentStreamingLevel();
            ERHIStreamingLevel const requested = entry.resource->GetRequestedStreamingLevel();

            if (current != requested && !entry.resource->IsStreamingComplete())
            {
                // ストリーミングリクエストは既にキューイング済み
            }
        }
    }

} // namespace NS::RHI
