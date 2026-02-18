/// @file D3D12Bindless.cpp
/// @brief D3D12 Bindlessディスクリプタ管理実装
#include "D3D12Bindless.h"

#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12BindlessDescriptorHeap
    //=========================================================================

    bool D3D12BindlessDescriptorHeap::Init(D3D12Device* device, uint32 numDescriptors)
    {
        if (!device || numDescriptors == 0)
            return false;

        device_ = device->GetD3DDevice();
        numDescriptors_ = numDescriptors;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12BindlessDescriptorHeap] CreateDescriptorHeap failed");
            return false;
        }

        incrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpuStart_ = heap_->GetCPUDescriptorHandleForHeapStart();
        gpuStart_ = heap_->GetGPUDescriptorHandleForHeapStart();
        nextFreshIndex_ = 0;

        return true;
    }

    NS::RHI::BindlessIndex D3D12BindlessDescriptorHeap::Allocate()
    {
        std::lock_guard lock(mutex_);

        uint32 index;
        if (!freeList_.empty())
        {
            index = freeList_.back();
            freeList_.pop_back();
        }
        else
        {
            if (nextFreshIndex_ >= numDescriptors_)
            {
                LOG_ERROR("[D3D12BindlessDescriptorHeap] Out of descriptors");
                return {};
            }
            index = nextFreshIndex_++;
        }

        return NS::RHI::BindlessIndex(index);
    }

    void D3D12BindlessDescriptorHeap::Free(NS::RHI::BindlessIndex index)
    {
        if (!index.IsValid())
            return;

        std::lock_guard lock(mutex_);
        freeList_.push_back(index.index);
    }

    void D3D12BindlessDescriptorHeap::CopyToIndex(NS::RHI::BindlessIndex index,
                                                  D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle)
    {
        if (!index.IsValid() || !device_)
            return;

        D3D12_CPU_DESCRIPTOR_HANDLE dst = cpuStart_;
        dst.ptr += static_cast<SIZE_T>(index.index) * incrementSize_;
        device_->CopyDescriptorsSimple(1, dst, srcCpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorHeap::GetGPUHandle(NS::RHI::BindlessIndex index) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuStart_;
        handle.ptr += static_cast<UINT64>(index.index) * incrementSize_;
        return handle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorHeap::GetCPUHandle(NS::RHI::BindlessIndex index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuStart_;
        handle.ptr += static_cast<SIZE_T>(index.index) * incrementSize_;
        return handle;
    }

    //=========================================================================
    // D3D12BindlessSamplerHeap
    //=========================================================================

    bool D3D12BindlessSamplerHeap::Init(D3D12Device* device, uint32 numSamplers)
    {
        if (!device || numSamplers == 0)
            return false;

        device_ = device->GetD3DDevice();
        numSamplers_ = numSamplers;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.NumDescriptors = numSamplers;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12BindlessSamplerHeap] CreateDescriptorHeap failed");
            return false;
        }

        incrementSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        cpuStart_ = heap_->GetCPUDescriptorHandleForHeapStart();
        gpuStart_ = heap_->GetGPUDescriptorHandleForHeapStart();
        nextFreshIndex_ = 0;

        return true;
    }

    NS::RHI::BindlessSamplerIndex D3D12BindlessSamplerHeap::Allocate()
    {
        std::lock_guard lock(mutex_);

        uint32 index;
        if (!freeList_.empty())
        {
            index = freeList_.back();
            freeList_.pop_back();
        }
        else
        {
            if (nextFreshIndex_ >= numSamplers_)
            {
                LOG_ERROR("[D3D12BindlessSamplerHeap] Out of sampler descriptors");
                return {};
            }
            index = nextFreshIndex_++;
        }

        return NS::RHI::BindlessSamplerIndex(index);
    }

    void D3D12BindlessSamplerHeap::Free(NS::RHI::BindlessSamplerIndex index)
    {
        if (!index.IsValid())
            return;

        std::lock_guard lock(mutex_);
        freeList_.push_back(index.index);
    }

    void D3D12BindlessSamplerHeap::CopyToIndex(NS::RHI::BindlessSamplerIndex index,
                                               D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle)
    {
        if (!index.IsValid() || !device_)
            return;

        D3D12_CPU_DESCRIPTOR_HANDLE dst = cpuStart_;
        dst.ptr += static_cast<SIZE_T>(index.index) * incrementSize_;
        device_->CopyDescriptorsSimple(1, dst, srcCpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessSamplerHeap::GetGPUHandle(NS::RHI::BindlessSamplerIndex index) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuStart_;
        handle.ptr += static_cast<UINT64>(index.index) * incrementSize_;
        return handle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12BindlessSamplerHeap::GetCPUHandle(NS::RHI::BindlessSamplerIndex index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuStart_;
        handle.ptr += static_cast<SIZE_T>(index.index) * incrementSize_;
        return handle;
    }

    //=========================================================================
    // D3D12BindlessManager
    //=========================================================================

    bool D3D12BindlessManager::Init(D3D12Device* device)
    {
        if (!device)
            return false;

        device_ = device;

        if (!resourceHeap_.Init(device, D3D12BindlessDescriptorHeap::kMaxDescriptors))
        {
            LOG_ERROR("[D3D12BindlessManager] Resource heap init failed");
            return false;
        }

        if (!samplerHeap_.Init(device, D3D12BindlessSamplerHeap::kMaxSamplers))
        {
            LOG_ERROR("[D3D12BindlessManager] Sampler heap init failed");
            return false;
        }

        // IRHIDescriptorHeapラッパー初期化（RHIBindlessResourceTable用）
        resourceHeapWrapper_.InitFromExisting(
            device, resourceHeap_.GetD3DHeap(),
            NS::RHI::ERHIDescriptorHeapType::CBV_SRV_UAV,
            D3D12BindlessDescriptorHeap::kMaxDescriptors, true);

        samplerHeapWrapper_.InitFromExisting(
            device, samplerHeap_.GetD3DHeap(),
            NS::RHI::ERHIDescriptorHeapType::Sampler,
            D3D12BindlessSamplerHeap::kMaxSamplers, true);

        return true;
    }

    void D3D12BindlessManager::Shutdown()
    {
        // ComPtrが自動解放
    }

    NS::RHI::BindlessSRVIndex D3D12BindlessManager::AllocateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle)
    {
        NS::RHI::BindlessIndex idx = resourceHeap_.Allocate();
        if (!idx.IsValid())
            return {};

        resourceHeap_.CopyToIndex(idx, srvCpuHandle);
        return NS::RHI::BindlessSRVIndex(idx.index);
    }

    NS::RHI::BindlessUAVIndex D3D12BindlessManager::AllocateUAV(D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle)
    {
        NS::RHI::BindlessIndex idx = resourceHeap_.Allocate();
        if (!idx.IsValid())
            return {};

        resourceHeap_.CopyToIndex(idx, uavCpuHandle);
        return NS::RHI::BindlessUAVIndex(idx.index);
    }

    void D3D12BindlessManager::FreeSRV(NS::RHI::BindlessSRVIndex index)
    {
        resourceHeap_.Free(NS::RHI::BindlessIndex(index.index));
    }

    void D3D12BindlessManager::FreeUAV(NS::RHI::BindlessUAVIndex index)
    {
        resourceHeap_.Free(NS::RHI::BindlessIndex(index.index));
    }

    void D3D12BindlessManager::SetHeapsOnCommandList(ID3D12GraphicsCommandList* cmdList)
    {
        ID3D12DescriptorHeap* heaps[] = {
            resourceHeap_.GetD3DHeap(),
            samplerHeap_.GetD3DHeap(),
        };

        uint32 heapCount = 0;
        if (heaps[0])
            ++heapCount;
        if (heaps[1])
            ++heapCount;

        if (heapCount > 0)
            cmdList->SetDescriptorHeaps(heapCount, heaps);
    }

} // namespace NS::D3D12RHI
