/// @file RHIOfflineDescriptor.cpp
/// @brief オフラインディスクリプタ管理実装
#include "RHIOfflineDescriptor.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIOfflineDescriptorHeap
    //=========================================================================

    bool RHIOfflineDescriptorHeap::Initialize(IRHIDevice* device, ERHIDescriptorHeapType type, uint32 numDescriptors)
    {
        m_device = device;
        m_type = type;

        RHIDescriptorHeapDesc desc;
        desc.type = type;
        desc.numDescriptors = numDescriptors;
        desc.flags = ERHIDescriptorHeapFlags::None; // CPU専用

        m_heap = device->CreateDescriptorHeap(desc, "OfflineDescriptorHeap");
        if (!m_heap)
        {
            return false;
        }

        return m_allocator.Initialize(m_heap.Get());
    }

    void RHIOfflineDescriptorHeap::Shutdown()
    {
        m_allocator.Shutdown();
        m_heap = nullptr;
        m_device = nullptr;
    }

    RHIDescriptorAllocation RHIOfflineDescriptorHeap::Allocate(uint32 count)
    {
        return m_allocator.Allocate(count);
    }

    void RHIOfflineDescriptorHeap::Free(const RHIDescriptorAllocation& allocation)
    {
        m_allocator.Free(allocation);
    }

    //=========================================================================
    // RHIOfflineDescriptorManager
    //=========================================================================

    bool RHIOfflineDescriptorManager::Initialize(
        IRHIDevice* device, uint32 cbvSrvUavCount, uint32 samplerCount, uint32 rtvCount, uint32 dsvCount)
    {
        if (!m_cbvSrvUavHeap.Initialize(device, ERHIDescriptorHeapType::CBV_SRV_UAV, cbvSrvUavCount))
        {
            return false;
        }

        if (!m_samplerHeap.Initialize(device, ERHIDescriptorHeapType::Sampler, samplerCount))
        {
            m_cbvSrvUavHeap.Shutdown();
            return false;
        }

        if (!m_rtvHeap.Initialize(device, ERHIDescriptorHeapType::RTV, rtvCount))
        {
            m_samplerHeap.Shutdown();
            m_cbvSrvUavHeap.Shutdown();
            return false;
        }

        if (!m_dsvHeap.Initialize(device, ERHIDescriptorHeapType::DSV, dsvCount))
        {
            m_rtvHeap.Shutdown();
            m_samplerHeap.Shutdown();
            m_cbvSrvUavHeap.Shutdown();
            return false;
        }

        return true;
    }

    void RHIOfflineDescriptorManager::Shutdown()
    {
        m_dsvHeap.Shutdown();
        m_rtvHeap.Shutdown();
        m_samplerHeap.Shutdown();
        m_cbvSrvUavHeap.Shutdown();
    }

} // namespace NS::RHI
