/// @file D3D12DynamicRHI.h
/// @brief D3D12 DynamicRHI — IDynamicRHI実装
#pragma once

#include "D3D12Adapter.h"
#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IDynamicRHI.h"

// TRefCountPtr<T>のデストラクタに完全型が必要
// IDynamicRHIの戻り値型として使用される全インターフェース
#include "RHI/Public/IRHIBuffer.h"
#include "RHI/Public/IRHIFence.h"
#include "RHI/Public/IRHIPipelineState.h"
#include "RHI/Public/IRHIRootSignature.h"
#include "RHI/Public/IRHISampler.h"
#include "RHI/Public/IRHIShader.h"
#include "RHI/Public/IRHISwapChain.h"
#include "RHI/Public/IRHITexture.h"
#include "RHI/Public/IRHIViews.h"
#include "RHI/Public/RHIDescriptorHeap.h"
#include "RHI/Public/RHIQuery.h"

namespace NS::D3D12RHI
{
    static constexpr uint32 kMaxAdapters = 4;

    //=========================================================================
    // D3D12DynamicRHI — IDynamicRHI実装
    //=========================================================================

    class D3D12DynamicRHI final : public NS::RHI::IDynamicRHI
    {
    public:
        D3D12DynamicRHI() = default;
        ~D3D12DynamicRHI() override;

        //=====================================================================
        // ネイティブアクセス
        //=====================================================================

        D3D12Device* GetD3D12Device() { return &device_; }
        const D3D12Device* GetD3D12Device() const { return &device_; }

        D3D12Adapter* GetD3D12Adapter(uint32 index);
        const D3D12Factory& GetFactory() const { return factory_; }

        //=====================================================================
        // IDynamicRHI — 識別
        //=====================================================================

        NS::RHI::ERHIInterfaceType GetInterfaceType() const override;
        const char* GetName() const override;
        NS::RHI::ERHIFeatureLevel GetFeatureLevel() const override;

        //=====================================================================
        // IDynamicRHI — ライフサイクル
        //=====================================================================

        bool Init() override;
        void PostInit() override;
        void Shutdown() override;
        void Tick(float deltaTime) override;
        void EndFrame() override;
        bool IsInitialized() const override;

        //=====================================================================
        // IDynamicRHI — アダプター/デバイスアクセス
        //=====================================================================

        uint32 GetAdapterCount() const override;
        NS::RHI::IRHIAdapter* GetAdapter(uint32 index) const override;
        NS::RHI::IRHIAdapter* GetCurrentAdapter() const override;
        NS::RHI::IRHIDevice* GetDefaultDevice() const override;
        NS::RHI::IRHIDevice* GetDevice(NS::RHI::GPUMask gpuMask) const override;

        //=====================================================================
        // IDynamicRHI — 機能クエリ
        //=====================================================================

        NS::RHI::ERHIFeatureSupport GetFeatureSupport(NS::RHI::ERHIFeature feature) const override;
        bool SupportsExtension(const char* extensionName) const override;

        //=====================================================================
        // IDynamicRHI — 制限値
        //=====================================================================

        uint32 GetMaxTextureSize() const override;
        uint32 GetMaxTextureArrayLayers() const override;
        uint64 GetMaxBufferSize() const override;
        uint32 GetMaxConstantBufferSize() const override;
        NS::RHI::ERHISampleCount GetMaxSampleCount() const override;

        //=====================================================================
        // IDynamicRHI — リソースファクトリ（サブ計画16-27で実装）
        //=====================================================================

        NS::RHI::TRefCountPtr<NS::RHI::IRHIBuffer> CreateBuffer(const NS::RHI::RHIBufferDesc& desc,
                                                                const void* initialData) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHITexture> CreateTexture(const NS::RHI::RHITextureDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHITexture> CreateTextureWithData(
            const NS::RHI::RHITextureDesc& desc,
            const NS::RHI::RHISubresourceData* initialData,
            uint32 numSubresources) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIShaderResourceView> CreateShaderResourceView(
            NS::RHI::IRHIResource* resource, const NS::RHI::RHISRVDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIUnorderedAccessView> CreateUnorderedAccessView(
            NS::RHI::IRHIResource* resource, const NS::RHI::RHIUAVDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIRenderTargetView> CreateRenderTargetView(
            NS::RHI::IRHITexture* texture, const NS::RHI::RHIRTVDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIDepthStencilView> CreateDepthStencilView(
            NS::RHI::IRHITexture* texture, const NS::RHI::RHIDSVDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIConstantBufferView> CreateConstantBufferView(
            NS::RHI::IRHIBuffer* buffer, const NS::RHI::RHICBVDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIShader> CreateShader(const NS::RHI::RHIShaderDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIGraphicsPipelineState> CreateGraphicsPipelineState(
            const NS::RHI::RHIGraphicsPipelineStateDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIComputePipelineState> CreateComputePipelineState(
            const NS::RHI::RHIComputePipelineStateDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIRootSignature> CreateRootSignature(
            const NS::RHI::RHIRootSignatureDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHISampler> CreateSampler(const NS::RHI::RHISamplerDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIFence> CreateFence(uint64 initialValue) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHISwapChain> CreateSwapChain(const NS::RHI::RHISwapChainDesc& desc,
                                                                      void* windowHandle) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIQueryHeap> CreateQueryHeap(const NS::RHI::RHIQueryHeapDesc& desc) override;
        NS::RHI::TRefCountPtr<NS::RHI::IRHIDescriptorHeap> CreateDescriptorHeap(
            const NS::RHI::RHIDescriptorHeapDesc& desc) override;

        //=====================================================================
        // IDynamicRHI — コマンドコンテキスト（サブ計画12で実装）
        //=====================================================================

        NS::RHI::IRHICommandContext* GetDefaultContext() override;
        NS::RHI::IRHICommandContext* GetCommandContext(NS::RHI::ERHIPipeline pipeline) override;
        NS::RHI::IRHIComputeContext* GetComputeContext() override;

        //=====================================================================
        // IDynamicRHI — コマンドリスト管理（サブ計画11-14で実装）
        //=====================================================================

        NS::RHI::IRHICommandAllocator* ObtainCommandAllocator(NS::RHI::ERHIQueueType queueType) override;
        void ReleaseCommandAllocator(NS::RHI::IRHICommandAllocator* allocator,
                                     NS::RHI::IRHIFence* fence,
                                     uint64 fenceValue) override;
        NS::RHI::IRHICommandList* ObtainCommandList(NS::RHI::IRHICommandAllocator* allocator) override;
        void ReleaseCommandList(NS::RHI::IRHICommandList* commandList) override;

        //=====================================================================
        // IDynamicRHI — コンテキスト終了（サブ計画12で実装）
        //=====================================================================

        NS::RHI::IRHICommandList* FinalizeContext(NS::RHI::IRHICommandContext* context) override;
        void ResetContext(NS::RHI::IRHICommandContext* context) override;

        //=====================================================================
        // IDynamicRHI — コマンド送信（サブ計画09で実装）
        //=====================================================================

        void SubmitCommandLists(NS::RHI::ERHIQueueType queueType,
                                NS::RHI::IRHICommandList* const* commandLists,
                                uint32 count) override;
        void FlushCommands() override;
        void FlushQueue(NS::RHI::ERHIQueueType queueType) override;

        //=====================================================================
        // IDynamicRHI — GPU同期（サブ計画08で実装）
        //=====================================================================

        void SignalFence(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value) override;
        void WaitFence(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value) override;
        bool WaitForFence(NS::RHI::IRHIFence* fence, uint64 value, uint64 timeoutMs) override;

        //=====================================================================
        // IDynamicRHI — フレーム同期
        //=====================================================================

        void BeginFrame() override;
        uint64 GetCurrentFrameNumber() const override;

    private:
        D3D12Factory factory_;
        D3D12Adapter adapters_[kMaxAdapters];
        uint32 adapterCount_ = 0;
        uint32 selectedAdapterIndex_ = 0;
        D3D12Device device_;
        bool initialized_ = false;
        uint64 frameNumber_ = 0;

        // デフォルトコンテキスト
        D3D12CommandContext* defaultContext_ = nullptr;
        D3D12ComputeContext* defaultComputeContext_ = nullptr;

        // フレームフェンス（N-buffering同期）
        static constexpr uint32 kMaxFrameLatency = 3;
        uint64 frameFenceValues_[kMaxFrameLatency] = {};

        /// Feature検出結果に基づくERHIFeature→ERHIFeatureSupportマッピング
        NS::RHI::ERHIFeatureSupport QueryFeatureFromDevice(NS::RHI::ERHIFeature feature) const;
    };

} // namespace NS::D3D12RHI
