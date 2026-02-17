/// @file RHIBindless.cpp
/// @brief Bindlessディスクリプタ管理実装
#include "RHIBindless.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include "IRHISampler.h"
#include "IRHIViews.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIBindlessDescriptorHeap
    //=========================================================================

    bool RHIBindlessDescriptorHeap::Initialize(IRHIDevice* device, uint32 numDescriptors)
    {
        m_device = device;

        RHIDescriptorHeapDesc desc;
        desc.type = ERHIDescriptorHeapType::CBV_SRV_UAV;
        desc.numDescriptors = numDescriptors;
        desc.flags = ERHIDescriptorHeapFlags::ShaderVisible;

        m_heap = device->CreateDescriptorHeap(desc, "BindlessDescriptorHeap");
        if (!m_heap)
        {
            return false;
        }

        return m_allocator.Initialize(m_heap.Get());
    }

    void RHIBindlessDescriptorHeap::Shutdown()
    {
        m_allocator.Shutdown();
        m_heap = nullptr;
        m_device = nullptr;
    }

    BindlessIndex RHIBindlessDescriptorHeap::Allocate()
    {
        RHIDescriptorAllocation const alloc = m_allocator.Allocate(1);
        return alloc.IsValid() ? BindlessIndex{alloc.heapIndex} : BindlessIndex{};
    }

    BindlessIndex RHIBindlessDescriptorHeap::AllocateRange(uint32 count)
    {
        RHIDescriptorAllocation const alloc = m_allocator.Allocate(count);
        return alloc.IsValid() ? BindlessIndex{alloc.heapIndex} : BindlessIndex{};
    }

    void RHIBindlessDescriptorHeap::Free(BindlessIndex index)
    {
        RHIDescriptorAllocation alloc;
        alloc.heap = m_heap.Get();
        alloc.heapIndex = index.index;
        alloc.count = 1;
        m_allocator.Free(alloc);
    }

    void RHIBindlessDescriptorHeap::FreeRange(BindlessIndex startIndex, uint32 count)
    {
        RHIDescriptorAllocation alloc;
        alloc.heap = m_heap.Get();
        alloc.heapIndex = startIndex.index;
        alloc.count = count;
        m_allocator.Free(alloc);
    }

    void RHIBindlessDescriptorHeap::SetSRV(BindlessIndex index, IRHIShaderResourceView* srv)
    {
        if ((m_device == nullptr) || (srv == nullptr))
        {
            return;
        }
        RHICPUDescriptorHandle dest = m_heap->GetCPUDescriptorHandle(index.index);
        m_device->CopyDescriptors(dest, srv->GetCPUHandle(), 1, ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    void RHIBindlessDescriptorHeap::SetUAV(BindlessIndex index, IRHIUnorderedAccessView* uav)
    {
        if ((m_device == nullptr) || (uav == nullptr))
        {
            return;
        }
        RHICPUDescriptorHandle dest = m_heap->GetCPUDescriptorHandle(index.index);
        m_device->CopyDescriptors(dest, uav->GetCPUHandle(), 1, ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    void RHIBindlessDescriptorHeap::SetCBV(BindlessIndex index, IRHIConstantBufferView* cbv)
    {
        if ((m_device == nullptr) || (cbv == nullptr))
        {
            return;
        }
        RHICPUDescriptorHandle dest = m_heap->GetCPUDescriptorHandle(index.index);
        m_device->CopyDescriptors(dest, cbv->GetCPUHandle(), 1, ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    void RHIBindlessDescriptorHeap::CopyDescriptor(BindlessIndex destIndex, RHICPUDescriptorHandle srcHandle)
    {
        if (m_device == nullptr)
        {
            return;
        }
        RHICPUDescriptorHandle dest = m_heap->GetCPUDescriptorHandle(destIndex.index);
        m_device->CopyDescriptors(dest, srcHandle, 1, ERHIDescriptorHeapType::CBV_SRV_UAV);
    }

    RHIGPUDescriptorHandle RHIBindlessDescriptorHeap::GetGPUHandle(BindlessIndex index) const
    {
        if (!m_heap)
        {
            return {};
        }
        return m_heap->GetGPUDescriptorHandle(index.index);
    }

    uint32 RHIBindlessDescriptorHeap::GetAvailableCount() const
    {
        return m_allocator.GetAvailableCount();
    }

    uint32 RHIBindlessDescriptorHeap::GetTotalCount() const
    {
        return m_allocator.GetTotalCount();
    }

    //=========================================================================
    // RHIBindlessSamplerHeap
    //=========================================================================

    bool RHIBindlessSamplerHeap::Initialize(IRHIDevice* device, uint32 numSamplers)
    {
        m_device = device;

        RHIDescriptorHeapDesc desc;
        desc.type = ERHIDescriptorHeapType::Sampler;
        desc.numDescriptors = numSamplers;
        desc.flags = ERHIDescriptorHeapFlags::ShaderVisible;

        m_heap = device->CreateDescriptorHeap(desc, "BindlessSamplerHeap");
        if (!m_heap)
        {
            return false;
        }

        return m_allocator.Initialize(m_heap.Get());
    }

    void RHIBindlessSamplerHeap::Shutdown()
    {
        m_allocator.Shutdown();
        m_heap = nullptr;
        m_device = nullptr;
    }

    BindlessSamplerIndex RHIBindlessSamplerHeap::RegisterSampler(IRHISampler* sampler)
    {
        if ((m_device == nullptr) || (sampler == nullptr))
        {
            return {};
        }

        RHIDescriptorAllocation alloc = m_allocator.Allocate(1);
        if (!alloc.IsValid())
        {
            return {};
        }

        m_device->CopyDescriptors(
            alloc.cpuHandle, sampler->GetCPUDescriptorHandle(), 1, ERHIDescriptorHeapType::Sampler);

        return BindlessSamplerIndex{alloc.heapIndex};
    }

    void RHIBindlessSamplerHeap::UnregisterSampler(BindlessSamplerIndex index)
    {
        RHIDescriptorAllocation alloc;
        alloc.heap = m_heap.Get();
        alloc.heapIndex = index.index;
        alloc.count = 1;
        m_allocator.Free(alloc);
    }

    RHIGPUDescriptorHandle RHIBindlessSamplerHeap::GetGPUHandle(BindlessSamplerIndex index) const
    {
        if (!m_heap)
        {
            return {};
        }
        return m_heap->GetGPUDescriptorHandle(index.index);
    }

    //=========================================================================
    // RHIBindlessResourceManager
    //=========================================================================

    bool RHIBindlessResourceManager::Initialize(IRHIDevice* device)
    {
        m_device = device;

        if (!m_descriptorHeap.Initialize(device))
        {
            return false;
        }

        if (!m_samplerHeap.Initialize(device))
        {
            m_descriptorHeap.Shutdown();
            return false;
        }

        return true;
    }

    void RHIBindlessResourceManager::Shutdown()
    {
        m_samplerHeap.Shutdown();
        m_descriptorHeap.Shutdown();
        m_device = nullptr;
    }

    BindlessSRVIndex RHIBindlessResourceManager::RegisterTextureSRV(IRHITexture* texture, const RHITextureSRVDesc& desc)
    {
        // テクスチャからSRVを作成してBindlessヒープに登録
        (void)texture;
        (void)desc;
        return BindlessSRVIndex{m_descriptorHeap.Allocate().index};
    }

    BindlessSRVIndex RHIBindlessResourceManager::RegisterBufferSRV(IRHIBuffer* buffer, const RHIBufferSRVDesc& desc)
    {
        (void)buffer;
        (void)desc;
        return BindlessSRVIndex{m_descriptorHeap.Allocate().index};
    }

    BindlessUAVIndex RHIBindlessResourceManager::RegisterTextureUAV(IRHITexture* texture, const RHITextureUAVDesc& desc)
    {
        (void)texture;
        (void)desc;
        return BindlessUAVIndex{m_descriptorHeap.Allocate().index};
    }

    BindlessUAVIndex RHIBindlessResourceManager::RegisterBufferUAV(IRHIBuffer* buffer, const RHIBufferUAVDesc& desc)
    {
        (void)buffer;
        (void)desc;
        return BindlessUAVIndex{m_descriptorHeap.Allocate().index};
    }

    BindlessSamplerIndex RHIBindlessResourceManager::RegisterSampler(IRHISampler* sampler)
    {
        return m_samplerHeap.RegisterSampler(sampler);
    }

    void RHIBindlessResourceManager::Unregister(BindlessIndex index)
    {
        m_descriptorHeap.Free(index);
    }

    void RHIBindlessResourceManager::UnregisterResource(IRHIResource* resource)
    {
        // リソースに関連する全Bindlessインデックスを解放
        // 実際にはリソース→インデックスのマッピングが必要
        (void)resource;
    }

    void RHIBindlessResourceManager::BindToContext(IRHICommandContext* context)
    {
        if (context == nullptr)
        {
            return;
        }

        context->SetDescriptorHeaps(m_descriptorHeap.GetHeap(), m_samplerHeap.GetHeap());
    }

} // namespace NS::RHI
