/// @file D3D12Device.h
/// @brief D3D12 論理デバイス
#pragma once

#include "D3D12Adapter.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIDevice.h"
#include "RHI/Public/RHIDeferredDelete.h"
#include "RHI/Public/RHIMeshShader.h"
#include "RHI/Public/RHIMultiGPU.h"
#include "RHI/Public/RHIRaytracingAS.h"
#include "RHI/Public/RHIReservedResource.h"
#include "RHI/Public/RHIVariableRateShading.h"
#include "RHI/Public/RHIWorkGraphTypes.h"

namespace NS::D3D12RHI
{
    class D3D12BindlessManager;
    class D3D12CommandAllocatorPool;
    class D3D12GPUProfiler;
    class D3D12CommandListPool;
    class D3D12Queue;
    class D3D12ResidencyManager;

    //=========================================================================
    // D3D12DeviceFeatures — CheckFeatureSupport結果キャッシュ
    //=========================================================================

    struct D3D12DeviceFeatures
    {
        // Options
        bool doublePrecisionFloatShaderOps = false;
        bool outputMergerLogicOp = false;
        bool rovSupported = false;
        bool conservativeRasterizationTier = false;
        uint32 resourceBindingTier = 0;
        uint32 tiledResourcesTier = 0;
        uint32 resourceHeapTier = 0;

        // Options5
        bool renderPassesTier = false;
        bool raytracingTier = false;
        uint32 raytracingTierValue = 0;

        // Options6
        uint32 vrsTier = 0;
        uint32 vrsTileSize = 0;
        bool vrsAdditionalShadingRatesSupported = false;

        // Options7
        bool meshShaderTier = false;
        bool samplerFeedbackTier = false;

        // Options12
        bool enhancedBarriersSupported = false;

        // Options16
        bool gpuUploadHeapSupported = false;

        // Options21
        bool workGraphsTier = false;

        // Wave operations
        bool waveOpsSupported = false;
        uint32 waveLaneCountMin = 0;
        uint32 waveLaneCountMax = 0;

        // Shader Model
        uint32 highestShaderModel = 0; // D3D_SHADER_MODEL value

        // Architecture
        bool isUMA = false;

        // Atomics
        bool int64ShaderOps = false;
    };

    //=========================================================================
    // DebugLayer設定
    //=========================================================================

    /// Debug Layer有効化（デバイス作成前に呼ぶ）
    void EnableDebugLayer(bool gpuBasedValidation);

    /// DRED設定（デバイス作成後に呼ぶ）
    void ConfigureDRED(ID3D12Device5* device);

    /// InfoQueueメッセージフィルタ設定
    void ConfigureInfoQueue(ID3D12Device* device);

    //=========================================================================
    // D3D12Device — IRHIDevice実装
    //=========================================================================

    class D3D12Device final : public NS::RHI::IRHIDevice
    {
    public:
        /// 初期化
        bool Init(D3D12Adapter* adapter, bool enableDebug);

        /// シャットダウン（キュー等のリソース解放）
        void Shutdown();

        /// ネイティブデバイス取得
        ID3D12Device* GetD3DDevice() const { return device_.Get(); }
        ID3D12Device5* GetD3DDevice5() const { return device5_.Get(); }

        /// Feature検出結果取得
        const D3D12DeviceFeatures& GetFeatures() const { return features_; }

        /// コマンドシグネチャ取得
        ID3D12CommandSignature* GetDrawIndirectSignature() const { return drawIndirectSig_.Get(); }
        ID3D12CommandSignature* GetDrawIndexedIndirectSignature() const { return drawIndexedIndirectSig_.Get(); }
        ID3D12CommandSignature* GetDispatchIndirectSignature() const { return dispatchIndirectSig_.Get(); }

        // --- IRHIDevice ---
        NS::RHI::IRHIAdapter* GetAdapter() const override;
        NS::RHI::GPUMask GetGPUMask() const override { return NS::RHI::GPUMask(1); }
        uint32 GetGPUIndex() const override { return 0; }
        uint64 GetTimestampFrequency() const override { return timestampFreq_; }
        NS::RHI::RHIMemoryBudget GetMemoryBudget() const override;
        uint32 GetConstantBufferAlignment() const override { return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; }
        uint32 GetTextureDataAlignment() const override { return D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT; }
        bool IsValid() const override { return device_ != nullptr; }
        bool IsDeviceLost() const override { return deviceLost_; }
        NS::RHI::IRHIShaderResourceView* GetNullSRV() const override { return nullptr; }
        NS::RHI::IRHIUnorderedAccessView* GetNullUAV() const override { return nullptr; }
        NS::RHI::IRHIConstantBufferView* GetNullCBV() const override { return nullptr; }
        NS::RHI::IRHISampler* GetNullSampler() const override { return nullptr; }
        NS::RHI::RHIFormatSupport GetFormatSupport(NS::RHI::ERHIPixelFormat format) const override;
        void WaitIdle() override;
        NS::RHI::RHIDeferredDeleteQueue* GetDeferredDeleteQueue() override { return &deferredDeleteQueue_; }
        void SetDebugName(const char* name) override;
        uint64 GetCurrentFrameNumber() const override { return frameNumber_; }

        // Queue management
        uint32 GetQueueCount(NS::RHI::ERHIQueueType type) const override;
        NS::RHI::IRHIQueue* GetQueue(NS::RHI::ERHIQueueType type, uint32 index) const override;
        void SignalQueue(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value) override;
        void WaitQueue(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value) override;
        void FlushQueue(NS::RHI::IRHIQueue* queue) override;
        void FlushAllQueues() override;
        void InsertQueueBarrier(NS::RHI::IRHIQueue* src, NS::RHI::IRHIQueue* dst) override;

        /// D3D12キュー直接取得
        D3D12Queue* GetD3D12Queue(NS::RHI::ERHIQueueType type) const;

        // Context management
        NS::RHI::IRHICommandContext* GetImmediateContext() override { return nullptr; }
        NS::RHI::IRHICommandContext* ObtainContext(NS::RHI::ERHIQueueType queueType) override;
        NS::RHI::IRHIComputeContext* ObtainComputeContext() override;
        void ReleaseContext(NS::RHI::IRHICommandContext* context) override;
        void ReleaseContext(NS::RHI::IRHIComputeContext* context) override;
        /// フレーム開始時にフェンス完了済みアロケーターをリサイクル
        uint32 ProcessCompletedAllocators();

        NS::RHI::IRHICommandAllocator* ObtainCommandAllocator(NS::RHI::ERHIQueueType queueType) override;
        void ReleaseCommandAllocator(NS::RHI::IRHICommandAllocator* allocator,
                                     NS::RHI::IRHIFence* fence,
                                     uint64 fenceValue) override;
        void ReleaseCommandAllocatorImmediate(NS::RHI::IRHICommandAllocator* allocator) override;
        NS::RHI::IRHICommandList* ObtainCommandList(NS::RHI::IRHICommandAllocator* allocator,
                                                    NS::RHI::IRHIPipelineState* initialPSO) override;
        void ReleaseCommandList(NS::RHI::IRHICommandList* commandList) override;
        NS::RHI::IRHICommandList* FinalizeContext(NS::RHI::IRHICommandContext* context) override;
        NS::RHI::IRHICommandList* FinalizeContext(NS::RHI::IRHIComputeContext* context) override;
        void ResetContext(NS::RHI::IRHICommandContext* context, NS::RHI::IRHICommandAllocator* allocator) override;
        void ResetContext(NS::RHI::IRHIComputeContext* context, NS::RHI::IRHICommandAllocator* allocator) override;

        // Descriptor management — サブ計画19で実装
        NS::RHI::IRHIDescriptorHeap* CreateDescriptorHeap(const NS::RHI::RHIDescriptorHeapDesc& desc,
                                                          const char* debugName) override;
        uint32 GetMaxDescriptorCount(NS::RHI::ERHIDescriptorHeapType type) const override;
        NS::RHI::IRHIDescriptorHeapManager* GetDescriptorHeapManager() override { return nullptr; }
        NS::RHI::IRHIOnlineDescriptorManager* GetOnlineDescriptorManager() override { return nullptr; }
        NS::RHI::RHIDescriptorHandle AllocateOnlineDescriptors(NS::RHI::ERHIDescriptorHeapType, uint32) override
        {
            return {};
        }
        NS::RHI::IRHIOfflineDescriptorManager* GetOfflineDescriptorManager(NS::RHI::ERHIDescriptorHeapType) override
        {
            return nullptr;
        }
        NS::RHI::RHICPUDescriptorHandle AllocateOfflineDescriptor(NS::RHI::ERHIDescriptorHeapType) override
        {
            return {};
        }
        void FreeOfflineDescriptor(NS::RHI::ERHIDescriptorHeapType, NS::RHI::RHICPUDescriptorHandle) override {}
        NS::RHI::IRHISamplerHeap* GetGlobalSamplerHeap() override { return nullptr; }
        NS::RHI::RHIDescriptorHandle AllocateSamplerDescriptor(NS::RHI::IRHISampler*) override { return {}; }
        void CopyDescriptor(NS::RHI::RHICPUDescriptorHandle dst,
                            NS::RHI::RHICPUDescriptorHandle src,
                            NS::RHI::ERHIDescriptorHeapType type) override;
        void CopyDescriptors(NS::RHI::RHICPUDescriptorHandle dst,
                             NS::RHI::RHICPUDescriptorHandle src,
                             uint32 count,
                             NS::RHI::ERHIDescriptorHeapType type) override;
        uint32 GetDescriptorIncrementSize(NS::RHI::ERHIDescriptorHeapType type) const override;
        bool SupportsBindless() const override { return features_.resourceBindingTier >= 3; }
        NS::RHI::BindlessSRVIndex AllocateBindlessSRV(NS::RHI::IRHIShaderResourceView* view) override;
        NS::RHI::BindlessUAVIndex AllocateBindlessUAV(NS::RHI::IRHIUnorderedAccessView* view) override;
        void FreeBindlessSRV(NS::RHI::BindlessSRVIndex index) override;
        void FreeBindlessUAV(NS::RHI::BindlessUAVIndex index) override;

        /// Bindlessマネージャー取得
        D3D12BindlessManager* GetBindlessManager() const { return bindlessManager_; }
        NS::RHI::IRHIDescriptorHeap* GetBindlessSRVUAVHeap() const override;
        NS::RHI::IRHIDescriptorHeap* GetBindlessSamplerHeap() const override;

        // Memory management
        NS::RHI::IRHIBufferAllocator* GetDefaultBufferAllocator() override { return nullptr; }
        NS::RHI::IRHIBufferAllocator* GetBufferAllocator(NS::RHI::ERHIHeapType) override { return nullptr; }
        NS::RHI::IRHITextureAllocator* GetTextureAllocator() override { return nullptr; }
        NS::RHI::RHIResourceAllocationInfo GetTextureAllocationInfo(const NS::RHI::RHITextureDesc& desc) const override;
        NS::RHI::RHIResourceAllocationInfo GetBufferAllocationInfo(const NS::RHI::RHIBufferDesc& desc) const override;
        NS::RHI::IRHIFastAllocator* GetFastAllocator() override { return nullptr; }
        NS::RHI::IRHIUploadHeap* GetUploadHeap() override { return nullptr; }
        void ResetFrameAllocators() override {}
        NS::RHI::IRHIResidencyManager* GetResidencyManager() override { return nullptr; }
        void MakeResident(NS::RHI::IRHIResource* const* resources, uint32 count) override;
        void Evict(NS::RHI::IRHIResource* const* resources, uint32 count) override;
        void SetMemoryPressureCallback(MemoryPressureCallback callback) override;
        NS::RHI::RHIMemoryStats GetMemoryStats() const override;

        // Work graphs — サブ計画44-46で実装
        NS::RHI::IRHIWorkGraphPipeline* CreateWorkGraphPipeline(const NS::RHI::RHIWorkGraphPipelineDesc& desc) override;
        bool SupportsWorkGraphs() const override { return features_.workGraphsTier; }
        NS::RHI::RHIWorkGraphMemoryRequirements GetWorkGraphMemoryRequirements(
            NS::RHI::IRHIWorkGraphPipeline* pipeline) const override;

        // Command signature
        NS::RHI::IRHICommandSignature* CreateCommandSignature(const NS::RHI::RHICommandSignatureDesc&,
                                                              const char*) override
        {
            return nullptr;
        }

        // Views — サブ計画20で実装
        NS::RHI::IRHIShaderResourceView* CreateShaderResourceView(const NS::RHI::RHIBufferSRVDesc& desc,
                                                                  const char* debugName) override;
        NS::RHI::IRHIShaderResourceView* CreateShaderResourceView(const NS::RHI::RHITextureSRVDesc& desc,
                                                                  const char* debugName) override;
        NS::RHI::IRHIUnorderedAccessView* CreateUnorderedAccessView(const NS::RHI::RHIBufferUAVDesc& desc,
                                                                    const char* debugName) override;
        NS::RHI::IRHIUnorderedAccessView* CreateUnorderedAccessView(const NS::RHI::RHITextureUAVDesc& desc,
                                                                    const char* debugName) override;
        NS::RHI::IRHIRenderTargetView* CreateRenderTargetView(const NS::RHI::RHIRenderTargetViewDesc& desc,
                                                              const char* debugName) override;
        NS::RHI::IRHIDepthStencilView* CreateDepthStencilView(const NS::RHI::RHIDepthStencilViewDesc& desc,
                                                              const char* debugName) override;
        NS::RHI::IRHIConstantBufferView* CreateConstantBufferView(const NS::RHI::RHIConstantBufferViewDesc& desc,
                                                                  const char* debugName) override;
        NS::RHI::IRHIShaderResourceView* GetNullBufferSRV(NS::RHI::ERHIBufferSRVFormat) override { return nullptr; }
        NS::RHI::IRHIShaderResourceView* GetNullTextureSRV(NS::RHI::ERHITextureDimension) override { return nullptr; }
        NS::RHI::IRHIUnorderedAccessView* GetNullBufferUAV() override { return nullptr; }
        NS::RHI::IRHIUnorderedAccessView* GetNullTextureUAV(NS::RHI::ERHITextureDimension) override { return nullptr; }
        NS::RHI::IRHIRenderTargetView* GetNullRTV() override { return nullptr; }
        NS::RHI::IRHIDepthStencilView* GetNullDSV() override { return nullptr; }

        // Shader — サブ計画24で実装
        NS::RHI::IRHIShader* CreateShader(const NS::RHI::RHIShaderDesc& desc, const char* debugName) override;
        NS::RHI::IRHIShaderLibrary* CreateShaderLibrary(const NS::RHI::RHIShaderLibraryDesc&, const char*) override
        {
            return nullptr;
        }

        // Root signature / PSO — サブ計画22-25で実装
        NS::RHI::IRHIRootSignature* CreateRootSignature(const NS::RHI::RHIRootSignatureDesc& desc,
                                                        const char* debugName) override;
        NS::RHI::IRHIRootSignature* CreateRootSignatureFromBlob(const NS::RHI::RHIShaderBytecode& blob,
                                                                const char* debugName) override;
        NS::RHI::IRHIInputLayout* CreateInputLayout(const NS::RHI::RHIInputLayoutDesc& desc,
                                                    const NS::RHI::RHIShaderBytecode& shaderBytecode,
                                                    const char* debugName) override;
        NS::RHI::IRHIGraphicsPipelineState* CreateGraphicsPipelineState(
            const NS::RHI::RHIGraphicsPipelineStateDesc& desc, const char* debugName) override;
        NS::RHI::IRHIGraphicsPipelineState* CreateGraphicsPipelineStateFromCache(
            const NS::RHI::RHIGraphicsPipelineStateDesc& desc,
            const NS::RHI::RHIShaderBytecode& cachedBlob,
            const char* debugName) override;
        NS::RHI::IRHIComputePipelineState* CreateComputePipelineState(const NS::RHI::RHIComputePipelineStateDesc& desc,
                                                                      const char* debugName) override;
        NS::RHI::IRHIComputePipelineState* CreateComputePipelineStateFromCache(
            const NS::RHI::RHIComputePipelineStateDesc& desc,
            const NS::RHI::RHIShaderBytecode& cachedBlob,
            const char* debugName) override;
        NS::RHI::IRHIPipelineStateCache* CreatePipelineStateCache() override;

        // Mesh shader
        NS::RHI::IRHIMeshPipelineState* CreateMeshPipelineState(const NS::RHI::RHIMeshPipelineStateDesc& desc,
                                                                const char* debugName) override;
        NS::RHI::RHIMeshShaderCapabilities GetMeshShaderCapabilities() const override;

        // Transient
        NS::RHI::IRHITransientResourceAllocator* CreateTransientAllocator(
            const NS::RHI::RHITransientAllocatorDesc& desc) override;

        // VRS
        NS::RHI::RHIVRSCapabilities GetVRSCapabilities() const override;
        NS::RHI::IRHITexture* CreateVRSImage(const NS::RHI::RHIVRSImageDesc& desc) override;

        // Sampler — サブ計画21で実装
        NS::RHI::IRHISampler* CreateSampler(const NS::RHI::RHISamplerDesc& desc, const char* debugName) override;

        // Staging / Readback
        NS::RHI::IRHIStagingBuffer* CreateStagingBuffer(const NS::RHI::RHIStagingBufferDesc&) override
        {
            return nullptr;
        }
        NS::RHI::IRHIBufferReadback* CreateBufferReadback(const NS::RHI::RHIBufferReadbackDesc&) override
        {
            return nullptr;
        }
        NS::RHI::IRHITextureReadback* CreateTextureReadback(const NS::RHI::RHITextureReadbackDesc&) override
        {
            return nullptr;
        }

        // Profiler
        NS::RHI::IRHIGPUProfiler* GetGPUProfiler() override;
        bool IsGPUProfilingSupported() const override { return gpuProfiler_ != nullptr; }

        // Fence
        NS::RHI::IRHIFence* CreateFence(const NS::RHI::RHIFenceDesc& desc, const char* debugName) override;
        NS::RHI::IRHIFence* OpenSharedFence(void*, const char*) override { return nullptr; }

        // Device lost
        void SetDeviceLostCallback(NS::RHI::RHIDeviceLostCallback callback, void* userData) override;
        bool GetGPUCrashInfo(NS::RHI::RHIGPUCrashInfo& outInfo) override;
        void SetBreadcrumbBuffer(NS::RHI::RHIBreadcrumbBuffer* buffer) override;
        bool GetDeviceLostInfo(NS::RHI::RHIDeviceLostInfo& outInfo) const override;
        bool CheckDeviceRemoved();

        // Profiler config
        void ConfigureProfiler(const NS::RHI::RHIProfilerConfig&) override {}
        void BeginCapture(const char*) override {}
        void EndCapture() override {}
        bool IsCapturing() const override { return false; }
        NS::RHI::ERHIProfilerType GetAvailableProfiler() const override;

        // Reserved resource
        NS::RHI::RHIReservedResourceCapabilities GetReservedResourceCapabilities() const override { return {}; }
        NS::RHI::RHITextureTileInfo GetTextureTileInfo(const NS::RHI::RHITextureDesc&) const override { return {}; }

        // SwapChain — サブ計画26で実装
        NS::RHI::IRHISwapChain* CreateSwapChain(const NS::RHI::RHISwapChainDesc& desc,
                                                NS::RHI::IRHIQueue* presentQueue,
                                                const char* debugName) override;

        /// DXGIファクトリ設定（D3D12DynamicRHI初期化時に呼ぶ）
        void SetDXGIFactory(IDXGIFactory6* factory) { dxgiFactory_ = factory; }

        // Query
        NS::RHI::IRHIQueryHeap* CreateQueryHeap(const NS::RHI::RHIQueryHeapDesc& desc, const char* debugName) override;
        bool GetQueryData(NS::RHI::IRHIQueryHeap* queryHeap,
                          uint32 startIndex,
                          uint32 numQueries,
                          void* outData,
                          uint32 dataSize,
                          NS::RHI::ERHIQueryFlags flags) override;
        bool GetTimestampCalibration(uint64& gpuTimestamp, uint64& cpuTimestamp) const override;

        // Format support
        NS::RHI::ERHIFormatSupportFlags GetFormatSupportFlags(NS::RHI::ERHIPixelFormat) const override;
        NS::RHI::RHIMSAASupportInfo GetMSAASupport(NS::RHI::ERHIPixelFormat, bool) const override { return {}; }
        uint32 ConvertToNativeFormat(NS::RHI::ERHIPixelFormat) const override { return 0; }
        NS::RHI::ERHIPixelFormat ConvertFromNativeFormat(uint32) const override;

        // Validation
        void ConfigureValidation(const NS::RHI::RHIValidationConfig&) override {}
        NS::RHI::ERHIValidationLevel GetValidationLevel() const override;
        void SuppressValidationMessage(uint32) override {}
        void UnsuppressValidationMessage(uint32) override {}
        NS::RHI::RHIValidationStats GetValidationStats() const override { return {}; }
        void ResetValidationStats() override {}

        // Ray tracing — サブ計画40-43で実装
        NS::RHI::IRHIAccelerationStructure* CreateAccelerationStructure(
            const NS::RHI::RHIRaytracingAccelerationStructureDesc& desc, const char* debugName) override;
        NS::RHI::RHIRaytracingAccelerationStructurePrebuildInfo GetAccelerationStructurePrebuildInfo(
            const NS::RHI::RHIRaytracingAccelerationStructureBuildInputs& inputs) const override;
        NS::RHI::RHIRaytracingCapabilities GetRaytracingCapabilities() const override;
        NS::RHI::IRHIShaderBindingTable* CreateShaderBindingTable(const NS::RHI::RHIShaderBindingTableDesc& desc,
                                                                  const char* debugName) override;
        NS::RHI::IRHIRaytracingPipelineState* CreateRaytracingPipelineState(
            const NS::RHI::RHIRaytracingPipelineStateDesc& desc, const char* debugName) override;

        // Multi-GPU
        NS::RHI::RHIMultiGPUCapabilities GetMultiGPUCapabilities() const override { return {}; }
        uint32 GetNodeCount() const override { return 1; }

    private:
        D3D12Adapter* adapter_ = nullptr;
        IDXGIFactory6* dxgiFactory_ = nullptr; // 非所有: D3D12DynamicRHIが管理
        ComPtr<ID3D12Device> device_;
        ComPtr<ID3D12Device5> device5_;
        D3D12DeviceFeatures features_{};
        uint64 timestampFreq_ = 0;
        uint64 frameNumber_ = 0;
        bool deviceLost_ = false;
        bool debugEnabled_ = false;

        // デバイスロストコールバック
        NS::RHI::RHIDeviceLostCallback deviceLostCallback_ = nullptr;
        void* deviceLostUserData_ = nullptr;
        NS::RHI::RHIBreadcrumbBuffer* breadcrumbBuffer_ = nullptr;

        // コマンドシグネチャ
        ComPtr<ID3D12CommandSignature> drawIndirectSig_;
        ComPtr<ID3D12CommandSignature> drawIndexedIndirectSig_;
        ComPtr<ID3D12CommandSignature> dispatchIndirectSig_;

        // キュー（Graphics/Compute/Copy各1つ）
        D3D12Queue* queues_[kQueueTypeCount] = {};

        // コマンドアロケータープール
        D3D12CommandAllocatorPool* allocatorPool_ = nullptr;

        // コマンドリストプール
        D3D12CommandListPool* commandListPool_ = nullptr;

        // 遅延削除キュー
        NS::RHI::RHIDeferredDeleteQueue deferredDeleteQueue_;

        // ディスクリプタインクリメントサイズキャッシュ
        uint32 descriptorIncrementSize_[4] = {}; // CBV_SRV_UAV, Sampler, RTV, DSV

        // Bindlessディスクリプタマネージャー
        D3D12BindlessManager* bindlessManager_ = nullptr;
        D3D12GPUProfiler* gpuProfiler_ = nullptr;

        // レジデンシーマネージャー
        D3D12ResidencyManager* residencyManager_ = nullptr;

        void DetectFeatures();
        void CacheDescriptorIncrementSizes();
        bool CreateCommandSignatures();
        bool CreateQueues();
        void DestroyQueues();
    };

} // namespace NS::D3D12RHI
