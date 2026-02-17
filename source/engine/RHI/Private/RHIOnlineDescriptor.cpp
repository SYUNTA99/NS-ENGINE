/// @file RHIOnlineDescriptor.cpp
/// @brief オンラインディスクリプタ管理実装
#include "RHIOnlineDescriptor.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include "IRHISampler.h"
#include "IRHIViews.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIOnlineDescriptorHeap
    //=========================================================================

    bool RHIOnlineDescriptorHeap::Initialize(IRHIDevice* device,
                                             ERHIDescriptorHeapType type,
                                             uint32 numDescriptors,
                                             uint32 numBufferedFrames)
    {
        m_device = device;
        m_type = type;
        m_totalCount = numDescriptors;
        m_headIndex = 0;
        m_tailIndex = 0;
        m_numBufferedFrames = numBufferedFrames;
        m_currentFrame = 0;

        RHIDescriptorHeapDesc desc;
        desc.type = type;
        desc.numDescriptors = numDescriptors;
        desc.flags = ERHIDescriptorHeapFlags::ShaderVisible;

        m_heap = device->CreateDescriptorHeap(desc, "OnlineDescriptorHeap");
        if (!m_heap)
        {
            return false;
        }

        m_frameMarkers = new FrameMarker[numBufferedFrames];
        std::memset(m_frameMarkers, 0, sizeof(FrameMarker) * numBufferedFrames);

        return true;
    }

    void RHIOnlineDescriptorHeap::Shutdown()
    {
        delete[] m_frameMarkers;
        m_frameMarkers = nullptr;
        m_heap = nullptr;
        m_device = nullptr;
        m_totalCount = 0;
        m_headIndex = 0;
        m_tailIndex = 0;
    }

    void RHIOnlineDescriptorHeap::BeginFrame(uint64 frameNumber)
    {
        m_currentFrame = (m_currentFrame + 1) % m_numBufferedFrames;

        // 古いフレームのディスクリプタを解放
        auto& marker = m_frameMarkers[m_currentFrame];
        if (marker.frameNumber > 0)
        {
            m_tailIndex = marker.headIndex;
            marker.frameNumber = 0;
        }

        m_frameMarkers[m_currentFrame].frameNumber = frameNumber;
        m_frameMarkers[m_currentFrame].headIndex = m_headIndex;
    }

    void RHIOnlineDescriptorHeap::EndFrame()
    {
        // フレーム終了時のheadを記録
        m_frameMarkers[m_currentFrame].headIndex = m_headIndex;
    }

    RHIDescriptorAllocation RHIOnlineDescriptorHeap::Allocate(uint32 count)
    {
        RHIDescriptorAllocation result;
        if (!m_heap || count == 0)
        {
            return result;
        }

        uint32 newHead = m_headIndex + count;

        // リングバッファのラップアラウンドチェック
        if (newHead > m_totalCount)
        {
            // ラップアラウンド
            m_headIndex = 0;
            newHead = count;
        }

        // tailとの衝突チェック
        if (m_headIndex < m_tailIndex && newHead > m_tailIndex)
        {
            return result;
        }
        // ラップアラウンド後: head==0かつtail==0の場合も衝突
        if (m_headIndex == m_tailIndex && m_headIndex == 0 && newHead > 0 && m_totalCount > 0 &&
            newHead >= m_totalCount)
        {
            return result;
        }

        result.heap = m_heap.Get();
        result.heapIndex = m_headIndex;
        result.count = count;
        result.cpuHandle = m_heap->GetCPUDescriptorHandle(m_headIndex);
        result.gpuHandle = m_heap->GetGPUDescriptorHandle(m_headIndex);

        m_headIndex = newHead;
        return result;
    }

    uint32 RHIOnlineDescriptorHeap::GetAvailableCount() const
    {
        if (m_headIndex >= m_tailIndex)
        {
            return m_totalCount - (m_headIndex - m_tailIndex);
        }
        return m_tailIndex - m_headIndex;
    }

    uint32 RHIOnlineDescriptorHeap::GetTotalCount() const
    {
        return m_totalCount;
    }

    //=========================================================================
    // RHIOnlineDescriptorManager
    //=========================================================================

    bool RHIOnlineDescriptorManager::Initialize(IRHIDevice* device,
                                                uint32 cbvSrvUavCount,
                                                uint32 samplerCount,
                                                uint32 numBufferedFrames)
    {
        if (!m_cbvSrvUavHeap.Initialize(device, ERHIDescriptorHeapType::CBV_SRV_UAV, cbvSrvUavCount, numBufferedFrames))
        {
            return false;
        }

        if (!m_samplerHeap.Initialize(device, ERHIDescriptorHeapType::Sampler, samplerCount, numBufferedFrames))
        {
            m_cbvSrvUavHeap.Shutdown();
            return false;
        }

        return true;
    }

    void RHIOnlineDescriptorManager::Shutdown()
    {
        m_samplerHeap.Shutdown();
        m_cbvSrvUavHeap.Shutdown();
    }

    void RHIOnlineDescriptorManager::BeginFrame(uint64 frameNumber)
    {
        m_cbvSrvUavHeap.BeginFrame(frameNumber);
        m_samplerHeap.BeginFrame(frameNumber);
    }

    void RHIOnlineDescriptorManager::EndFrame()
    {
        m_cbvSrvUavHeap.EndFrame();
        m_samplerHeap.EndFrame();
    }

    void RHIOnlineDescriptorManager::BindToContext(IRHICommandContext* context)
    {
        if (context == nullptr)
        {
            return;
        }

        // コンテキストにヒープを設定
        context->SetDescriptorHeaps(m_cbvSrvUavHeap.GetHeap(), m_samplerHeap.GetHeap());
    }

    //=========================================================================
    // RHIDescriptorStaging
    //=========================================================================

    bool RHIDescriptorStaging::Initialize(IRHIDevice* device, RHIOnlineDescriptorManager* onlineManager)
    {
        m_device = device;
        m_onlineManager = onlineManager;

        m_batchCapacity = 64;
        m_batchEntries = new BatchEntry[m_batchCapacity];
        m_batchCount = 0;

        return true;
    }

    void RHIDescriptorStaging::Shutdown()
    {
        delete[] m_batchEntries;
        m_batchEntries = nullptr;
        m_batchCount = 0;
        m_batchCapacity = 0;
        m_onlineManager = nullptr;
        m_device = nullptr;
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::Stage(RHICPUDescriptorHandle srcHandle, ERHIDescriptorHeapType type)
    {
        return StageRange(srcHandle, 1, type);
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::StageRange(RHICPUDescriptorHandle srcHandle,
                                                            uint32 count,
                                                            ERHIDescriptorHeapType type)
    {
        RHIGPUDescriptorHandle result = {};
        if ((m_device == nullptr) || (m_onlineManager == nullptr))
        {
            return result;
        }

        RHIDescriptorAllocation alloc;
        if (type == ERHIDescriptorHeapType::Sampler)
        {
            alloc = m_onlineManager->AllocateSampler(count);
        }
        else
        {
            alloc = m_onlineManager->AllocateCBVSRVUAV(count);
        }

        if (!alloc.IsValid())
        {
            return result;
        }

        // オフライン→オンラインへディスクリプタコピー
        m_device->CopyDescriptors(alloc.cpuHandle, srcHandle, count, type);

        return alloc.gpuHandle;
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::StageSRV(IRHIShaderResourceView* srv)
    {
        if (srv == nullptr)
        {
            return {};
        }
        return Stage(srv->GetCPUHandle(), ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::StageUAV(IRHIUnorderedAccessView* uav)
    {
        if (uav == nullptr)
        {
            return {};
        }
        return Stage(uav->GetCPUHandle(), ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::StageCBV(IRHIConstantBufferView* cbv)
    {
        if (cbv == nullptr)
        {
            return {};
        }
        return Stage(cbv->GetCPUHandle(), ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::StageSampler(IRHISampler* sampler)
    {
        if (sampler == nullptr)
        {
            return {};
        }
        return Stage(sampler->GetCPUDescriptorHandle(), ERHIDescriptorHeapType::Sampler);
    }

    void RHIDescriptorStaging::BeginBatch()
    {
        m_batchCount = 0;
    }

    void RHIDescriptorStaging::AddToBatch(RHICPUDescriptorHandle srcHandle, ERHIDescriptorHeapType type)
    {
        if (m_batchCount >= m_batchCapacity)
        {
            uint32 const newCap = m_batchCapacity * 2;
            auto* newEntries = new BatchEntry[newCap];
            std::memcpy(newEntries, m_batchEntries, sizeof(BatchEntry) * m_batchCount);
            delete[] m_batchEntries;
            m_batchEntries = newEntries;
            m_batchCapacity = newCap;
        }

        m_batchEntries[m_batchCount++] = {.srcHandle = srcHandle, .type = type};
    }

    RHIGPUDescriptorHandle RHIDescriptorStaging::EndBatch()
    {
        if (m_batchCount == 0 || (m_onlineManager == nullptr))
        {
            return {};
        }

        // 全バッチエントリをまとめてオンラインヒープにコピー
        RHIDescriptorAllocation const alloc = m_onlineManager->AllocateCBVSRVUAV(m_batchCount);
        if (!alloc.IsValid())
        {
            return {};
        }

        for (uint32 i = 0; i < m_batchCount; ++i)
        {
            RHICPUDescriptorHandle const destHandle = alloc.GetCPUHandle(i);
            m_device->CopyDescriptors(destHandle, m_batchEntries[i].srcHandle, 1, m_batchEntries[i].type);
        }

        m_batchCount = 0;
        return alloc.gpuHandle;
    }

} // namespace NS::RHI
