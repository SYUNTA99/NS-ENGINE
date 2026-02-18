/// @file D3D12DynamicRHI.cpp
/// @brief D3D12 DynamicRHI — IDynamicRHI実装
#include "D3D12DynamicRHI.h"
#include "D3D12Buffer.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Descriptors.h"
#include "D3D12Dispatch.h"
#include "D3D12PipelineState.h"
#include "D3D12Queue.h"
#include "D3D12RootSignature.h"
#include "D3D12Sampler.h"
#include "D3D12Texture.h"
#include "D3D12View.h"
#include "RHI/Public/IRHIQueue.h"
#include "RHI/Public/RHIDispatchTable.h"

#include <cstdio>

namespace NS::D3D12RHI
{
    //=========================================================================
    // デストラクタ
    //=========================================================================

    D3D12DynamicRHI::~D3D12DynamicRHI()
    {
        if (initialized_)
        {
            Shutdown();
        }
    }

    //=========================================================================
    // ネイティブアクセス
    //=========================================================================

    D3D12Adapter* D3D12DynamicRHI::GetD3D12Adapter(uint32 index)
    {
        if (index < adapterCount_)
        {
            return &adapters_[index];
        }
        return nullptr;
    }

    //=========================================================================
    // 識別
    //=========================================================================

    NS::RHI::ERHIInterfaceType D3D12DynamicRHI::GetInterfaceType() const
    {
        return NS::RHI::ERHIInterfaceType::D3D12;
    }

    const char* D3D12DynamicRHI::GetName() const
    {
        return "D3D12";
    }

    NS::RHI::ERHIFeatureLevel D3D12DynamicRHI::GetFeatureLevel() const
    {
        if (adapterCount_ > 0)
        {
            return adapters_[selectedAdapterIndex_].GetDesc().maxFeatureLevel;
        }
        return NS::RHI::ERHIFeatureLevel::SM6;
    }

    //=========================================================================
    // ライフサイクル
    //=========================================================================

    bool D3D12DynamicRHI::Init()
    {
        LOG_INFO("[D3D12RHI] Initializing D3D12 backend...");

        // Debug構成でデバッグレイヤー有効化（デバイス作成前に呼ぶ）
        bool enableDebug = false;
#if !defined(NS_BUILD_SHIPPING)
        enableDebug = true;
        EnableDebugLayer(/* gpuBasedValidation */ false);
#endif

        // 1. Factory作成
        if (!factory_.Create(enableDebug))
        {
            LOG_ERROR("[D3D12RHI] Failed to create DXGI factory");
            return false;
        }

        // 2. Adapter列挙
        adapterCount_ = EnumerateAdapters(factory_.Get(), adapters_, kMaxAdapters);
        if (adapterCount_ == 0)
        {
            LOG_ERROR("[D3D12RHI] No D3D12-capable adapters found");
            return false;
        }

        // 最初のアダプタ（最高性能GPU）を選択
        selectedAdapterIndex_ = 0;

        // 3. Device作成
        if (!device_.Init(&adapters_[selectedAdapterIndex_], enableDebug))
        {
            LOG_ERROR("[D3D12RHI] Failed to create D3D12 device");
            return false;
        }

        // 4. Adapter←→Device接続 + Factory設定
        adapters_[selectedAdapterIndex_].SetDevice(&device_);
        device_.SetDXGIFactory(factory_.Get());

        // 5. DispatchTable登録
        RegisterD3D12DispatchTable(NS::RHI::GRHIDispatchTable);
        if (!NS::RHI::GRHIDispatchTable.IsValid())
        {
            LOG_ERROR("[D3D12RHI] Dispatch table validation failed - missing function pointers");
            return false;
        }

        // 6. デフォルトコンテキスト作成
        defaultContext_ = new D3D12CommandContext();
        if (!defaultContext_->Init(&device_, NS::RHI::ERHIQueueType::Graphics))
        {
            LOG_ERROR("[D3D12RHI] Failed to create default graphics context");
            delete defaultContext_;
            defaultContext_ = nullptr;
            return false;
        }

        defaultComputeContext_ = new D3D12ComputeContext();
        if (!defaultComputeContext_->Init(&device_))
        {
            LOG_ERROR("[D3D12RHI] Failed to create default compute context");
            delete defaultComputeContext_;
            defaultComputeContext_ = nullptr;
            delete defaultContext_;
            defaultContext_ = nullptr;
            return false;
        }

        initialized_ = true;
        frameNumber_ = 0;

        {
            char buf[256];
            std::snprintf(buf,
                          sizeof(buf),
                          "[D3D12RHI] D3D12 backend initialized (Adapter: %s)",
                          adapters_[selectedAdapterIndex_].GetDesc().deviceName.c_str());
            LOG_INFO(buf);
        }

        return true;
    }

    void D3D12DynamicRHI::PostInit()
    {
        // Queue/SwapChain等の追加初期化はサブ計画07-27で実装
    }

    void D3D12DynamicRHI::Shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        LOG_INFO("[D3D12RHI] Shutting down D3D12 backend...");

        // 1. 初期化フラグをまずクリア
        initialized_ = false;

        // 2. デフォルトコンテキスト破棄（Device Shutdown前に実行）
        delete defaultComputeContext_;
        defaultComputeContext_ = nullptr;
        delete defaultContext_;
        defaultContext_ = nullptr;

        // 3. GPU待機 + キュー破棄
        device_.Shutdown();

        // 4. Adapter←→Device切断
        adapters_[selectedAdapterIndex_].SetDevice(nullptr);

        // 5. Device破棄（ComPtrの自動解放に任せる）

        // 6. Factory破棄（同上）

        LOG_INFO("[D3D12RHI] D3D12 backend shutdown complete");
    }

    void D3D12DynamicRHI::Tick(float deltaTime)
    {
        (void)deltaTime;
    }

    void D3D12DynamicRHI::EndFrame()
    {
        // Graphicsキューにフレーム完了フェンスをSignal
        auto* graphicsQueue = device_.GetD3D12Queue(NS::RHI::ERHIQueueType::Graphics);
        if (graphicsQueue)
        {
            uint64 fenceValue = graphicsQueue->AdvanceFence();
            uint32 frameIndex = static_cast<uint32>(frameNumber_ % kMaxFrameLatency);
            frameFenceValues_[frameIndex] = fenceValue;
        }

        ++frameNumber_;
    }

    bool D3D12DynamicRHI::IsInitialized() const
    {
        return initialized_;
    }

    //=========================================================================
    // アダプター/デバイスアクセス
    //=========================================================================

    uint32 D3D12DynamicRHI::GetAdapterCount() const
    {
        return adapterCount_;
    }

    NS::RHI::IRHIAdapter* D3D12DynamicRHI::GetAdapter(uint32 index) const
    {
        if (index < adapterCount_)
        {
            return const_cast<D3D12Adapter*>(&adapters_[index]);
        }
        return nullptr;
    }

    NS::RHI::IRHIAdapter* D3D12DynamicRHI::GetCurrentAdapter() const
    {
        return const_cast<D3D12Adapter*>(&adapters_[selectedAdapterIndex_]);
    }

    NS::RHI::IRHIDevice* D3D12DynamicRHI::GetDefaultDevice() const
    {
        return const_cast<D3D12Device*>(&device_);
    }

    NS::RHI::IRHIDevice* D3D12DynamicRHI::GetDevice(NS::RHI::GPUMask gpuMask) const
    {
        // シングルGPU: gpuMaskに関わらずデフォルトデバイス返却
        (void)gpuMask;
        return const_cast<D3D12Device*>(&device_);
    }

    //=========================================================================
    // 機能クエリ
    //=========================================================================

    NS::RHI::ERHIFeatureSupport D3D12DynamicRHI::GetFeatureSupport(NS::RHI::ERHIFeature feature) const
    {
        if (!initialized_)
        {
            return NS::RHI::ERHIFeatureSupport::Unsupported;
        }
        return QueryFeatureFromDevice(feature);
    }

    bool D3D12DynamicRHI::SupportsExtension(const char* extensionName) const
    {
        (void)extensionName;
        return false;
    }

    NS::RHI::ERHIFeatureSupport D3D12DynamicRHI::QueryFeatureFromDevice(NS::RHI::ERHIFeature feature) const
    {
        using NS::RHI::ERHIFeature;
        using NS::RHI::ERHIFeatureSupport;

        const auto& f = device_.GetFeatures();

        switch (feature)
        {
        // D3D12 12_0 以上で保証
        case ERHIFeature::TextureCompressionBC:
        case ERHIFeature::StructuredBuffer:
        case ERHIFeature::ByteAddressBuffer:
        case ERHIFeature::TypedBuffer:
        case ERHIFeature::MultiDrawIndirect:
        case ERHIFeature::DrawIndirectCount:
        case ERHIFeature::DepthBoundsTest:
        case ERHIFeature::ExecuteIndirect:
        case ERHIFeature::Texture3D:
            return ERHIFeatureSupport::RuntimeGuaranteed;

        // CheckFeatureSupport結果依存
        case ERHIFeature::WaveOperations:
            return f.waveOpsSupported ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::RayTracing:
            return f.raytracingTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::MeshShaders:
            return f.meshShaderTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::VariableRateShading:
            // VRS判定はD3D12DeviceFeaturesに追加予定。暫定RuntimeDependent
            return ERHIFeatureSupport::RuntimeDependent;

        case ERHIFeature::AmplificationShaders:
            return f.meshShaderTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::ShaderModel6_6:
            return (f.highestShaderModel >= D3D_SHADER_MODEL_6_6) ? ERHIFeatureSupport::RuntimeGuaranteed
                                                                  : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::ShaderModel6_7:
            return (f.highestShaderModel >= D3D_SHADER_MODEL_6_7) ? ERHIFeatureSupport::RuntimeGuaranteed
                                                                  : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::SamplerFeedback:
            return f.samplerFeedbackTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::Bindless:
            return (f.resourceBindingTier >= 3) ? ERHIFeatureSupport::RuntimeGuaranteed
                                                : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::ConservativeRasterization:
            return f.conservativeRasterizationTier ? ERHIFeatureSupport::RuntimeGuaranteed
                                                   : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::RenderPass:
            return f.renderPassesTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::WorkGraphs:
            return f.workGraphsTier ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::EnhancedBarriers:
            return f.enhancedBarriersSupported ? ERHIFeatureSupport::RuntimeGuaranteed
                                               : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::GPUUploadHeaps:
            return f.gpuUploadHeapSupported ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::AtomicInt64:
            return f.int64ShaderOps ? ERHIFeatureSupport::RuntimeGuaranteed : ERHIFeatureSupport::Unsupported;

        case ERHIFeature::Residency:
            return ERHIFeatureSupport::RuntimeGuaranteed; // D3D12は常にレジデンシーAPI利用可能

        case ERHIFeature::MSAA_16X:
            return ERHIFeatureSupport::RuntimeDependent; // フォーマット依存

        case ERHIFeature::TextureCompressionASTC:
            return ERHIFeatureSupport::Unsupported; // D3D12はASTC非対応

        default:
            return ERHIFeatureSupport::Unsupported;
        }
    }

    //=========================================================================
    // 制限値
    //=========================================================================

    uint32 D3D12DynamicRHI::GetMaxTextureSize() const
    {
        return 16384;
    }

    uint32 D3D12DynamicRHI::GetMaxTextureArrayLayers() const
    {
        return 2048;
    }

    uint64 D3D12DynamicRHI::GetMaxBufferSize() const
    {
        return D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ULL * 1024ULL;
    }

    uint32 D3D12DynamicRHI::GetMaxConstantBufferSize() const
    {
        return D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    }

    NS::RHI::ERHISampleCount D3D12DynamicRHI::GetMaxSampleCount() const
    {
        return NS::RHI::ERHISampleCount::Count8;
    }

    //=========================================================================
    // リソースファクトリ — スタブ（サブ計画16-27で実装）
    //=========================================================================

    NS::RHI::TRefCountPtr<NS::RHI::IRHIBuffer> D3D12DynamicRHI::CreateBuffer(const NS::RHI::RHIBufferDesc& desc,
                                                                             const void* initialData)
    {
        auto* buffer = new D3D12Buffer();
        if (!buffer->Init(&device_, desc, initialData))
        {
            delete buffer;
            return nullptr;
        }
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIBuffer>(buffer);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHITexture> D3D12DynamicRHI::CreateTexture(const NS::RHI::RHITextureDesc& desc)
    {
        auto* texture = new D3D12Texture();
        if (!texture->Init(&device_, desc))
        {
            delete texture;
            return nullptr;
        }
        return NS::RHI::TRefCountPtr<NS::RHI::IRHITexture>(texture);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHITexture> D3D12DynamicRHI::CreateTextureWithData(
        const NS::RHI::RHITextureDesc& desc, const NS::RHI::RHISubresourceData*, uint32)
    {
        // 初期データアップロードはサブ計画18で実装
        // 現時点ではテクスチャ作成のみ
        return CreateTexture(desc);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIShaderResourceView> D3D12DynamicRHI::CreateShaderResourceView(
        NS::RHI::IRHIResource*, const NS::RHI::RHISRVDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIUnorderedAccessView> D3D12DynamicRHI::CreateUnorderedAccessView(
        NS::RHI::IRHIResource*, const NS::RHI::RHIUAVDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIRenderTargetView> D3D12DynamicRHI::CreateRenderTargetView(
        NS::RHI::IRHITexture*, const NS::RHI::RHIRTVDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIDepthStencilView> D3D12DynamicRHI::CreateDepthStencilView(
        NS::RHI::IRHITexture*, const NS::RHI::RHIDSVDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIConstantBufferView> D3D12DynamicRHI::CreateConstantBufferView(
        NS::RHI::IRHIBuffer*, const NS::RHI::RHICBVDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIShader> D3D12DynamicRHI::CreateShader(const NS::RHI::RHIShaderDesc& desc)
    {
        auto* shader = device_.CreateShader(desc, desc.debugName);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIShader>(shader);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIGraphicsPipelineState> D3D12DynamicRHI::CreateGraphicsPipelineState(
        const NS::RHI::RHIGraphicsPipelineStateDesc& desc)
    {
        auto* pso = device_.CreateGraphicsPipelineState(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIGraphicsPipelineState>(pso);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIComputePipelineState> D3D12DynamicRHI::CreateComputePipelineState(
        const NS::RHI::RHIComputePipelineStateDesc& desc)
    {
        auto* pso = device_.CreateComputePipelineState(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIComputePipelineState>(pso);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIRootSignature> D3D12DynamicRHI::CreateRootSignature(
        const NS::RHI::RHIRootSignatureDesc& desc)
    {
        auto* rootSig = device_.CreateRootSignature(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIRootSignature>(rootSig);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHISampler> D3D12DynamicRHI::CreateSampler(const NS::RHI::RHISamplerDesc& desc)
    {
        auto* sampler = device_.CreateSampler(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHISampler>(sampler);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIFence> D3D12DynamicRHI::CreateFence(uint64 initialValue)
    {
        NS::RHI::RHIFenceDesc desc;
        desc.initialValue = initialValue;
        NS::RHI::IRHIFence* fence = device_.CreateFence(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIFence>(fence);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHISwapChain> D3D12DynamicRHI::CreateSwapChain(
        const NS::RHI::RHISwapChainDesc& desc, void* /*windowHandle*/)
    {
        // desc.windowHandle を使用（windowHandleパラメータは互換性のため残す）
        auto* graphicsQueue = device_.GetD3D12Queue(NS::RHI::ERHIQueueType::Graphics);
        if (!graphicsQueue)
            return nullptr;

        NS::RHI::IRHISwapChain* swapChain = device_.CreateSwapChain(desc, graphicsQueue, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHISwapChain>(swapChain);
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIQueryHeap> D3D12DynamicRHI::CreateQueryHeap(const NS::RHI::RHIQueryHeapDesc&)
    {
        return nullptr;
    }

    NS::RHI::TRefCountPtr<NS::RHI::IRHIDescriptorHeap> D3D12DynamicRHI::CreateDescriptorHeap(
        const NS::RHI::RHIDescriptorHeapDesc& desc)
    {
        auto* heap = device_.CreateDescriptorHeap(desc, nullptr);
        return NS::RHI::TRefCountPtr<NS::RHI::IRHIDescriptorHeap>(heap);
    }

    //=========================================================================
    // コマンドコンテキスト
    //=========================================================================

    NS::RHI::IRHICommandContext* D3D12DynamicRHI::GetDefaultContext()
    {
        return defaultContext_;
    }

    NS::RHI::IRHICommandContext* D3D12DynamicRHI::GetCommandContext(NS::RHI::ERHIPipeline pipeline)
    {
        if (pipeline == NS::RHI::ERHIPipeline::AsyncCompute)
        {
            return nullptr; // ComputeContextはGetComputeContext()経由
        }
        return defaultContext_;
    }

    NS::RHI::IRHIComputeContext* D3D12DynamicRHI::GetComputeContext()
    {
        return defaultComputeContext_;
    }

    //=========================================================================
    // コマンドリスト管理
    //=========================================================================

    NS::RHI::IRHICommandAllocator* D3D12DynamicRHI::ObtainCommandAllocator(NS::RHI::ERHIQueueType queueType)
    {
        return device_.ObtainCommandAllocator(queueType);
    }

    void D3D12DynamicRHI::ReleaseCommandAllocator(NS::RHI::IRHICommandAllocator* allocator,
                                                  NS::RHI::IRHIFence* fence,
                                                  uint64 fenceValue)
    {
        device_.ReleaseCommandAllocator(allocator, fence, fenceValue);
    }

    NS::RHI::IRHICommandList* D3D12DynamicRHI::ObtainCommandList(NS::RHI::IRHICommandAllocator* allocator)
    {
        return device_.ObtainCommandList(allocator, nullptr);
    }

    void D3D12DynamicRHI::ReleaseCommandList(NS::RHI::IRHICommandList* commandList)
    {
        device_.ReleaseCommandList(commandList);
    }

    //=========================================================================
    // コンテキスト終了
    //=========================================================================

    NS::RHI::IRHICommandList* D3D12DynamicRHI::FinalizeContext(NS::RHI::IRHICommandContext* context)
    {
        return device_.FinalizeContext(context);
    }

    void D3D12DynamicRHI::ResetContext(NS::RHI::IRHICommandContext* context)
    {
        device_.ResetContext(context, nullptr);
    }

    //=========================================================================
    // コマンド送信
    //=========================================================================

    void D3D12DynamicRHI::SubmitCommandLists(NS::RHI::ERHIQueueType queueType,
                                             NS::RHI::IRHICommandList* const* commandLists,
                                             uint32 count)
    {
        auto* queue = device_.GetD3D12Queue(queueType);
        if (queue && commandLists && count > 0)
        {
            queue->ExecuteCommandLists(commandLists, count);
        }
    }

    void D3D12DynamicRHI::FlushCommands()
    {
        device_.FlushAllQueues();
    }

    void D3D12DynamicRHI::FlushQueue(NS::RHI::ERHIQueueType queueType)
    {
        auto* queue = device_.GetD3D12Queue(queueType);
        if (queue)
        {
            queue->Flush();
        }
    }

    //=========================================================================
    // GPU同期
    //=========================================================================

    void D3D12DynamicRHI::SignalFence(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (queue && fence)
            queue->Signal(fence, value);
    }

    void D3D12DynamicRHI::WaitFence(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (queue && fence)
            queue->Wait(fence, value);
    }

    bool D3D12DynamicRHI::WaitForFence(NS::RHI::IRHIFence* fence, uint64 value, uint64 timeoutMs)
    {
        if (!fence)
            return false;
        return fence->Wait(value, timeoutMs);
    }

    //=========================================================================
    // フレーム同期
    //=========================================================================

    void D3D12DynamicRHI::BeginFrame()
    {
        // フレームフェンス待ち: N-2フレーム前のGPU処理が完了するまで待機
        uint32 frameIndex = static_cast<uint32>(frameNumber_ % kMaxFrameLatency);
        auto* graphicsQueue = device_.GetD3D12Queue(NS::RHI::ERHIQueueType::Graphics);
        if (graphicsQueue && frameFenceValues_[frameIndex] > 0)
        {
            graphicsQueue->WaitForFence(frameFenceValues_[frameIndex], UINT32_MAX);
        }

        // 完了したアロケーターをリサイクル
        device_.ProcessCompletedAllocators();

        // 遅延削除キュー処理
        auto* deleteQueue = device_.GetDeferredDeleteQueue();
        if (deleteQueue)
        {
            deleteQueue->SetCurrentFrame(frameNumber_);
            deleteQueue->ProcessCompletedDeletions();
        }
    }

    uint64 D3D12DynamicRHI::GetCurrentFrameNumber() const
    {
        return frameNumber_;
    }

} // namespace NS::D3D12RHI
