/// @file D3D12CommandList.cpp
/// @brief D3D12コマンドリスト実装
#include "D3D12CommandList.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12CommandList
    //=========================================================================

    D3D12CommandList::D3D12CommandList() = default;

    bool D3D12CommandList::Init(D3D12Device* device,
                                NS::RHI::ERHIQueueType queueType,
                                NS::RHI::ERHICommandListType listType)
    {
        device_ = device;
        queueType_ = queueType;
        listType_ = listType;

        D3D12_COMMAND_LIST_TYPE d3dType = ToD3D12CommandListType(queueType);
        if (listType == NS::RHI::ERHICommandListType::Bundle)
            d3dType = D3D12_COMMAND_LIST_TYPE_BUNDLE;

        // CreateCommandList1（ID3D12Device4+）: クローズ状態で作成
        auto* device5 = device->GetD3DDevice5();
        if (!device5)
        {
            LOG_ERROR("[D3D12RHI] CreateCommandList1 requires ID3D12Device4+");
            return false;
        }

        HRESULT hr = device5->CreateCommandList1(
            0, d3dType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList_));

        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateCommandList1 failed");
            return false;
        }

        state_ = NS::RHI::ERHICommandListState::Closed;
        return true;
    }

    NS::RHI::IRHIDevice* D3D12CommandList::GetDevice() const
    {
        return device_;
    }

    void D3D12CommandList::Reset(NS::RHI::IRHICommandAllocator* allocator, NS::RHI::IRHIPipelineState* /*initialPSO*/)
    {
        if (!commandList_ || !allocator)
            return;

        auto* d3dAllocator = static_cast<D3D12CommandAllocator*>(allocator);

        // PSO引数はサブ計画23で実装（PSO作成後）
        HRESULT hr = commandList_->Reset(d3dAllocator->GetD3DAllocator(), nullptr);
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CommandList::Reset failed");
            return;
        }

        allocator_ = allocator;
        state_ = NS::RHI::ERHICommandListState::Recording;
        stats_ = {};
    }

    void D3D12CommandList::Close()
    {
        if (!commandList_)
            return;

        HRESULT hr = commandList_->Close();
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CommandList::Close failed");
            return;
        }

        state_ = NS::RHI::ERHICommandListState::Closed;
    }

    void D3D12CommandList::ExecuteBundle(NS::RHI::IRHICommandList* bundle)
    {
        if (!commandList_ || !bundle)
            return;

        auto* d3dBundle = static_cast<D3D12CommandList*>(bundle);
        commandList_->ExecuteBundle(d3dBundle->GetD3DCommandList());
    }

    //=========================================================================
    // D3D12CommandListPool
    //=========================================================================

    D3D12CommandListPool::D3D12CommandListPool(D3D12Device* device) : device_(device) {}

    D3D12CommandListPool::~D3D12CommandListPool()
    {
        for (auto* list : available_)
            delete list;
        available_.clear();

        for (auto* list : inUse_)
            delete list;
        inUse_.clear();
    }

    NS::RHI::IRHICommandList* D3D12CommandListPool::Obtain(NS::RHI::IRHICommandAllocator* allocator,
                                                           NS::RHI::ERHICommandListType type)
    {
        // プール内で互換リストを検索
        for (auto it = available_.begin(); it != available_.end(); ++it)
        {
            auto* list = *it;
            if (list->GetQueueType() == allocator->GetQueueType() && list->GetListType() == type)
            {
                available_.erase(it);
                list->Reset(allocator);
                inUse_.push_back(list);
                return list;
            }
        }

        // なければ新規作成
        auto* list = new D3D12CommandList();
        if (!list->Init(device_, allocator->GetQueueType(), type))
        {
            delete list;
            return nullptr;
        }
        list->Reset(allocator);
        inUse_.push_back(list);
        return list;
    }

    void D3D12CommandListPool::Release(NS::RHI::IRHICommandList* commandList)
    {
        if (!commandList)
            return;

        for (auto it = inUse_.begin(); it != inUse_.end(); ++it)
        {
            if (*it == commandList)
            {
                inUse_.erase(it);
                break;
            }
        }
        available_.push_back(static_cast<D3D12CommandList*>(commandList));
    }

    uint32 D3D12CommandListPool::GetPooledCount() const
    {
        return static_cast<uint32>(available_.size());
    }

} // namespace NS::D3D12RHI
