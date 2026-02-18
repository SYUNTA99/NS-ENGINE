/// @file D3D12CommandAllocator.h
/// @brief D3D12コマンドアロケーター — IRHICommandAllocator実装
#pragma once

#include "D3D12Queue.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHICommandAllocator.h"

#include <vector>

namespace NS::D3D12RHI
{
    class D3D12Device;
    class D3D12Fence;

    //=========================================================================
    // D3D12CommandAllocator — IRHICommandAllocator実装
    //=========================================================================

    class D3D12CommandAllocator final : public NS::RHI::IRHICommandAllocator
    {
    public:
        D3D12CommandAllocator();
        ~D3D12CommandAllocator() override = default;

        /// 初期化
        bool Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType);

        /// ネイティブアロケーター取得
        ID3D12CommandAllocator* GetD3DAllocator() const { return allocator_.Get(); }

        //=====================================================================
        // IRHICommandAllocator — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIQueueType GetQueueType() const override { return queueType_; }

        //=====================================================================
        // IRHICommandAllocator — ライフサイクル
        //=====================================================================

        void Reset() override;
        bool IsInUse() const override;

        //=====================================================================
        // IRHICommandAllocator — メモリ情報
        //=====================================================================

        uint64 GetAllocatedMemory() const override { return 0; }
        uint64 GetUsedMemory() const override { return 0; }

        //=====================================================================
        // IRHICommandAllocator — 待機フェンス
        //=====================================================================

        void SetWaitFence(NS::RHI::IRHIFence* fence, uint64 value) override;
        NS::RHI::IRHIFence* GetWaitFence() const override { return waitFence_; }
        uint64 GetWaitFenceValue() const override { return waitFenceValue_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::ERHIQueueType queueType_ = NS::RHI::ERHIQueueType::Graphics;
        ComPtr<ID3D12CommandAllocator> allocator_;

        // 待機フェンス（参照のみ、非所有）
        NS::RHI::IRHIFence* waitFence_ = nullptr;
        uint64 waitFenceValue_ = 0;
    };

    //=========================================================================
    // D3D12CommandAllocatorPool — IRHICommandAllocatorPool実装
    //=========================================================================

    class D3D12CommandAllocatorPool final : public NS::RHI::IRHICommandAllocatorPool
    {
    public:
        explicit D3D12CommandAllocatorPool(D3D12Device* device);
        ~D3D12CommandAllocatorPool() override;

        NS::RHI::IRHICommandAllocator* Obtain(NS::RHI::ERHIQueueType queueType) override;
        void Release(NS::RHI::IRHICommandAllocator* allocator, NS::RHI::IRHIFence* fence, uint64 fenceValue) override;
        uint32 ProcessCompletedAllocators() override;
        uint32 GetPooledCount(NS::RHI::ERHIQueueType queueType) const override;
        uint32 GetInUseCount(NS::RHI::ERHIQueueType queueType) const override;

    private:
        D3D12Device* device_ = nullptr;
        std::vector<D3D12CommandAllocator*> available_[kQueueTypeCount];
        std::vector<D3D12CommandAllocator*> inUse_;
        std::vector<D3D12CommandAllocator*> pending_;
    };

} // namespace NS::D3D12RHI
