/// @file D3D12CommandContext.h
/// @brief D3D12コマンドコンテキスト — IRHICommandContext / IRHIComputeContext 実装
#pragma once

#include "D3D12Barriers.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHICommandContext.h"
#include <vector>

namespace NS::D3D12RHI
{
    class D3D12CommandAllocator;
    class D3D12CommandList;
    class D3D12Device;

    //=========================================================================
    // D3D12CommandContext — IRHICommandContext実装
    //=========================================================================

    class D3D12CommandContext final : public NS::RHI::IRHICommandContext
    {
    public:
        D3D12CommandContext() = default;
        ~D3D12CommandContext() override = default;

        /// 初期化
        bool Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType);

        /// ネイティブコマンドリスト取得
        ID3D12GraphicsCommandList* GetD3DCommandList() const;

        //=====================================================================
        // IRHICommandContextBase — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::GPUMask GetGPUMask() const override { return NS::RHI::GPUMask(1); }
        NS::RHI::ERHIQueueType GetQueueType() const override { return queueType_; }
        NS::RHI::ERHIPipeline GetPipeline() const override;

        //=====================================================================
        // IRHICommandContextBase — ライフサイクル
        //=====================================================================

        void Begin(NS::RHI::IRHICommandAllocator* allocator) override;
        NS::RHI::IRHICommandList* Finish() override;
        void Reset() override;
        bool IsRecording() const override { return recording_; }

        //=====================================================================
        // IRHIComputeContext — ディスクリプタヒープ
        //=====================================================================

        NS::RHI::IRHIDescriptorHeap* GetCBVSRVUAVHeap() const override { return nullptr; }
        NS::RHI::IRHIDescriptorHeap* GetSamplerHeap() const override { return nullptr; }

        //=====================================================================
        // IRHICommandContext — レンダーパス
        //=====================================================================

        bool IsInRenderPass() const override { return inRenderPass_; }
        const NS::RHI::RHIRenderPassDesc* GetCurrentRenderPassDesc() const override { return nullptr; }
        uint32 GetCurrentSubpassIndex() const override { return 0; }
        bool GetRenderPassStatistics(NS::RHI::RHIRenderPassStatistics& outStats) const override;
        void ResetStatistics() override;

        //=====================================================================
        // バリアバッチャー
        //=====================================================================

        D3D12BarrierBatcher& GetBarrierBatcher() { return legacyBatcher_; }
        bool UseEnhancedBarriers() const { return useEnhancedBarriers_; }

        /// 蓄積バリアをフラッシュ
        void FlushBarriers();

        /// 一時リソースをコンテキスト寿命まで保持（GPU参照中の早期解放を防止）
        void DeferRelease(ComPtr<ID3D12Resource> resource) { pendingResources_.push_back(std::move(resource)); }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::ERHIQueueType queueType_ = NS::RHI::ERHIQueueType::Graphics;
        D3D12CommandList* commandList_ = nullptr;
        NS::RHI::IRHICommandAllocator* allocator_ = nullptr;
        bool recording_ = false;
        bool inRenderPass_ = false;
        bool useEnhancedBarriers_ = false;
        D3D12BarrierBatcher legacyBatcher_;
        std::vector<ComPtr<ID3D12Resource>> pendingResources_;
    };

    //=========================================================================
    // D3D12ComputeContext — IRHIComputeContext実装
    //=========================================================================

    class D3D12ComputeContext final : public NS::RHI::IRHIComputeContext
    {
    public:
        D3D12ComputeContext() = default;
        ~D3D12ComputeContext() override = default;

        /// 初期化
        bool Init(D3D12Device* device);

        /// ネイティブコマンドリスト取得
        ID3D12GraphicsCommandList* GetD3DCommandList() const;

        //=====================================================================
        // IRHICommandContextBase — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::GPUMask GetGPUMask() const override { return NS::RHI::GPUMask(1); }
        NS::RHI::ERHIQueueType GetQueueType() const override { return NS::RHI::ERHIQueueType::Compute; }
        NS::RHI::ERHIPipeline GetPipeline() const override;

        //=====================================================================
        // IRHICommandContextBase — ライフサイクル
        //=====================================================================

        void Begin(NS::RHI::IRHICommandAllocator* allocator) override;
        NS::RHI::IRHICommandList* Finish() override;
        void Reset() override;
        bool IsRecording() const override { return recording_; }

        //=====================================================================
        // IRHIComputeContext — ディスクリプタヒープ
        //=====================================================================

        NS::RHI::IRHIDescriptorHeap* GetCBVSRVUAVHeap() const override { return nullptr; }
        NS::RHI::IRHIDescriptorHeap* GetSamplerHeap() const override { return nullptr; }

        //=====================================================================
        // バリアバッチャー
        //=====================================================================

        D3D12BarrierBatcher& GetBarrierBatcher() { return legacyBatcher_; }
        bool UseEnhancedBarriers() const { return useEnhancedBarriers_; }

        /// 蓄積バリアをフラッシュ
        void FlushBarriers();

        /// 一時リソースをコンテキスト寿命まで保持（GPU参照中の早期解放を防止）
        void DeferRelease(ComPtr<ID3D12Resource> resource) { pendingResources_.push_back(std::move(resource)); }

    private:
        D3D12Device* device_ = nullptr;
        D3D12CommandList* commandList_ = nullptr;
        NS::RHI::IRHICommandAllocator* allocator_ = nullptr;
        bool recording_ = false;
        bool useEnhancedBarriers_ = false;
        D3D12BarrierBatcher legacyBatcher_;
        std::vector<ComPtr<ID3D12Resource>> pendingResources_;
    };

} // namespace NS::D3D12RHI
