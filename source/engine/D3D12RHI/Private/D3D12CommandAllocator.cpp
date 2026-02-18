/// @file D3D12CommandAllocator.cpp
/// @brief D3D12コマンドアロケーター実装
#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"
#include "D3D12Fence.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12CommandAllocator
    //=========================================================================

    D3D12CommandAllocator::D3D12CommandAllocator() = default;

    bool D3D12CommandAllocator::Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType)
    {
        device_ = device;
        queueType_ = queueType;

        D3D12_COMMAND_LIST_TYPE type = ToD3D12CommandListType(queueType);

        HRESULT hr = device->GetD3DDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateCommandAllocator failed");
            return false;
        }

        return true;
    }

    NS::RHI::IRHIDevice* D3D12CommandAllocator::GetDevice() const
    {
        return device_;
    }

    void D3D12CommandAllocator::Reset()
    {
        if (allocator_)
        {
            allocator_->Reset();
        }
        waitFence_ = nullptr;
        waitFenceValue_ = 0;
    }

    bool D3D12CommandAllocator::IsInUse() const
    {
        if (!waitFence_)
        {
            return false;
        }

        auto* d3dFence = static_cast<D3D12Fence*>(waitFence_);
        return d3dFence->GetCompletedValue() < waitFenceValue_;
    }

    void D3D12CommandAllocator::SetWaitFence(NS::RHI::IRHIFence* fence, uint64 value)
    {
        waitFence_ = fence;
        waitFenceValue_ = value;
    }

    //=========================================================================
    // D3D12CommandAllocatorPool
    //=========================================================================

    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12Device* device) : device_(device) {}

    D3D12CommandAllocatorPool::~D3D12CommandAllocatorPool()
    {
        // 全アロケーターを解放
        for (uint32 i = 0; i < kQueueTypeCount; ++i)
        {
            for (auto* alloc : available_[i])
                delete alloc;
            available_[i].clear();
        }
        // inUse_のアロケーターも解放
        for (auto* alloc : inUse_)
            delete alloc;
        inUse_.clear();
    }

    NS::RHI::IRHICommandAllocator* D3D12CommandAllocatorPool::Obtain(NS::RHI::ERHIQueueType queueType)
    {
        // 注意: このプールは単一スレッドからのみ呼び出すこと（同期機構なし）
        uint32 idx = static_cast<uint32>(queueType);
        if (idx >= kQueueTypeCount)
        {
            return nullptr;
        }

        // 利用可能なアロケーターがあればそれを使う
        if (!available_[idx].empty())
        {
            auto* alloc = available_[idx].back();
            available_[idx].pop_back();
            alloc->Reset();
            inUse_.push_back(alloc);
            return alloc;
        }

        // なければ新規作成
        auto* alloc = new D3D12CommandAllocator();
        if (!alloc->Init(device_, queueType))
        {
            delete alloc;
            return nullptr;
        }
        inUse_.push_back(alloc);
        return alloc;
    }

    void D3D12CommandAllocatorPool::Release(NS::RHI::IRHICommandAllocator* allocator,
                                            NS::RHI::IRHIFence* fence,
                                            uint64 fenceValue)
    {
        if (!allocator)
        {
            return;
        }

        allocator->SetWaitFence(fence, fenceValue);

        // inUse_から除去してpending_へ
        for (auto it = inUse_.begin(); it != inUse_.end(); ++it)
        {
            if (*it == allocator)
            {
                inUse_.erase(it);
                break;
            }
        }
        pending_.push_back(static_cast<D3D12CommandAllocator*>(allocator));
    }

    uint32 D3D12CommandAllocatorPool::ProcessCompletedAllocators()
    {
        uint32 recycled = 0;

        for (auto it = pending_.begin(); it != pending_.end();)
        {
            auto* alloc = *it;
            if (!alloc->IsInUse())
            {
                uint32 idx = static_cast<uint32>(alloc->GetQueueType());
                available_[idx].push_back(alloc);
                it = pending_.erase(it);
                ++recycled;
            }
            else
            {
                ++it;
            }
        }

        return recycled;
    }

    uint32 D3D12CommandAllocatorPool::GetPooledCount(NS::RHI::ERHIQueueType queueType) const
    {
        uint32 idx = static_cast<uint32>(queueType);
        if (idx >= kQueueTypeCount)
        {
            return 0;
        }
        return static_cast<uint32>(available_[idx].size());
    }

    uint32 D3D12CommandAllocatorPool::GetInUseCount(NS::RHI::ERHIQueueType queueType) const
    {
        uint32 count = 0;
        for (auto* alloc : inUse_)
        {
            if (alloc->GetQueueType() == queueType)
                ++count;
        }
        for (auto* alloc : pending_)
        {
            if (alloc->GetQueueType() == queueType)
                ++count;
        }
        return count;
    }

} // namespace NS::D3D12RHI
