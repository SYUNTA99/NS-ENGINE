/// @file D3D12CommandContext.cpp
/// @brief D3D12コマンドコンテキスト実装
#include "D3D12CommandContext.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandList.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12CommandContext
    //=========================================================================

    bool D3D12CommandContext::Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType)
    {
        device_ = device;
        queueType_ = queueType;
        useEnhancedBarriers_ = device->GetFeatures().enhancedBarriersSupported;
        return true;
    }

    ID3D12GraphicsCommandList* D3D12CommandContext::GetD3DCommandList() const
    {
        return commandList_ ? commandList_->GetD3DCommandList() : nullptr;
    }

    NS::RHI::IRHIDevice* D3D12CommandContext::GetDevice() const
    {
        return device_;
    }

    NS::RHI::ERHIPipeline D3D12CommandContext::GetPipeline() const
    {
        return queueType_ == NS::RHI::ERHIQueueType::Compute ? NS::RHI::ERHIPipeline::AsyncCompute
                                                             : NS::RHI::ERHIPipeline::Graphics;
    }

    void D3D12CommandContext::Begin(NS::RHI::IRHICommandAllocator* allocator)
    {
        if (!device_ || !allocator)
            return;

        allocator_ = allocator;
        legacyBatcher_.Reset();

        // コマンドリスト取得（プールから）
        commandList_ = static_cast<D3D12CommandList*>(device_->ObtainCommandList(allocator, nullptr));

        if (commandList_)
            recording_ = true;
    }

    NS::RHI::IRHICommandList* D3D12CommandContext::Finish()
    {
        if (!commandList_)
            return nullptr;

        // 未発行バリアをフラッシュ
        FlushBarriers();

        commandList_->Close();
        recording_ = false;
        inRenderPass_ = false;

        auto* result = commandList_;
        commandList_ = nullptr;
        allocator_ = nullptr;
        return result;
    }

    void D3D12CommandContext::Reset()
    {
        if (commandList_ && recording_)
        {
            commandList_->Close();
            device_->ReleaseCommandList(commandList_);
        }
        commandList_ = nullptr;
        allocator_ = nullptr;
        recording_ = false;
        inRenderPass_ = false;
    }

    bool D3D12CommandContext::GetRenderPassStatistics(NS::RHI::RHIRenderPassStatistics& /*outStats*/) const
    {
        return false;
    }

    void D3D12CommandContext::FlushBarriers()
    {
        auto* cmdList = GetD3DCommandList();
        if (!cmdList)
            return;

        legacyBatcher_.Flush(cmdList);
    }

    void D3D12CommandContext::ResetStatistics()
    {
        // 統計リセット — 将来のサブ計画で実装
    }

    //=========================================================================
    // D3D12ComputeContext
    //=========================================================================

    bool D3D12ComputeContext::Init(D3D12Device* device)
    {
        device_ = device;
        useEnhancedBarriers_ = device->GetFeatures().enhancedBarriersSupported;
        return true;
    }

    ID3D12GraphicsCommandList* D3D12ComputeContext::GetD3DCommandList() const
    {
        return commandList_ ? commandList_->GetD3DCommandList() : nullptr;
    }

    NS::RHI::IRHIDevice* D3D12ComputeContext::GetDevice() const
    {
        return device_;
    }

    NS::RHI::ERHIPipeline D3D12ComputeContext::GetPipeline() const
    {
        return NS::RHI::ERHIPipeline::AsyncCompute;
    }

    void D3D12ComputeContext::Begin(NS::RHI::IRHICommandAllocator* allocator)
    {
        if (!device_ || !allocator)
            return;

        allocator_ = allocator;
        legacyBatcher_.Reset();

        commandList_ = static_cast<D3D12CommandList*>(device_->ObtainCommandList(allocator, nullptr));

        if (commandList_)
            recording_ = true;
    }

    NS::RHI::IRHICommandList* D3D12ComputeContext::Finish()
    {
        if (!commandList_)
            return nullptr;

        // 未発行バリアをフラッシュ
        FlushBarriers();

        commandList_->Close();
        recording_ = false;

        auto* result = commandList_;
        commandList_ = nullptr;
        allocator_ = nullptr;
        return result;
    }

    void D3D12ComputeContext::FlushBarriers()
    {
        auto* cmdList = GetD3DCommandList();
        if (!cmdList)
            return;

        legacyBatcher_.Flush(cmdList);
    }

    void D3D12ComputeContext::Reset()
    {
        if (commandList_ && recording_)
        {
            commandList_->Close();
            device_->ReleaseCommandList(commandList_);
        }
        commandList_ = nullptr;
        allocator_ = nullptr;
        recording_ = false;
    }

} // namespace NS::D3D12RHI
