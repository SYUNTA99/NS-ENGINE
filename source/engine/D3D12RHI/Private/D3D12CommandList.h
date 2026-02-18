/// @file D3D12CommandList.h
/// @brief D3D12コマンドリスト — IRHICommandList実装
#pragma once

#include "D3D12Queue.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHICommandList.h"

#include <vector>

namespace NS::D3D12RHI
{
    class D3D12CommandAllocator;
    class D3D12Device;

    //=========================================================================
    // D3D12CommandList — IRHICommandList実装
    //=========================================================================

    class D3D12CommandList final : public NS::RHI::IRHICommandList
    {
    public:
        D3D12CommandList();
        ~D3D12CommandList() override = default;

        /// 初期化
        bool Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType, NS::RHI::ERHICommandListType listType);

        /// ネイティブコマンドリスト取得
        ID3D12GraphicsCommandList* GetD3DCommandList() const { return commandList_.Get(); }

        //=====================================================================
        // IRHICommandList — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIQueueType GetQueueType() const override { return queueType_; }
        NS::RHI::ERHICommandListState GetState() const override { return state_; }
        NS::RHI::ERHICommandListType GetListType() const override { return listType_; }

        //=====================================================================
        // IRHICommandList — ライフサイクル
        //=====================================================================

        void Reset(NS::RHI::IRHICommandAllocator* allocator, NS::RHI::IRHIPipelineState* initialPSO = nullptr) override;
        void Close() override;

        //=====================================================================
        // IRHICommandList — アロケーター
        //=====================================================================

        NS::RHI::IRHICommandAllocator* GetAllocator() const override { return allocator_; }
        uint64 GetUsedMemory() const override { return 0; }

        //=====================================================================
        // IRHICommandList — バンドル
        //=====================================================================

        void ExecuteBundle(NS::RHI::IRHICommandList* bundle) override;

        //=====================================================================
        // IRHICommandList — 統計
        //=====================================================================

        NS::RHI::RHICommandListStats GetStats() const override { return stats_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::ERHIQueueType queueType_ = NS::RHI::ERHIQueueType::Graphics;
        NS::RHI::ERHICommandListType listType_ = NS::RHI::ERHICommandListType::Direct;
        NS::RHI::ERHICommandListState state_ = NS::RHI::ERHICommandListState::Initial;
        ComPtr<ID3D12GraphicsCommandList> commandList_;
        NS::RHI::IRHICommandAllocator* allocator_ = nullptr;
        NS::RHI::RHICommandListStats stats_{};
    };

    //=========================================================================
    // D3D12CommandListPool — IRHICommandListPool実装
    //=========================================================================

    class D3D12CommandListPool final : public NS::RHI::IRHICommandListPool
    {
    public:
        explicit D3D12CommandListPool(D3D12Device* device);
        ~D3D12CommandListPool() override;

        NS::RHI::IRHICommandList* Obtain(NS::RHI::IRHICommandAllocator* allocator,
                                         NS::RHI::ERHICommandListType type) override;
        void Release(NS::RHI::IRHICommandList* commandList) override;
        uint32 GetPooledCount() const override;

    private:
        D3D12Device* device_ = nullptr;
        std::vector<D3D12CommandList*> available_;
        std::vector<D3D12CommandList*> inUse_;
    };

} // namespace NS::D3D12RHI
