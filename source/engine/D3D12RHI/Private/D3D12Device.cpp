/// @file D3D12Device.cpp
/// @brief D3D12 論理デバイス作成・Feature検出・デバッグ支援
#include "D3D12Device.h"
#include "D3D12AccelerationStructure.h"
#include "D3D12Allocation.h"
#include "D3D12Bindless.h"
#include "D3D12Buffer.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandContext.h"
#include "D3D12CommandList.h"
#include "D3D12Descriptors.h"
#include "D3D12DeviceLost.h"
#include "D3D12Fence.h"
#include "D3D12MeshShader.h"
#include "D3D12PipelineState.h"
#include "D3D12PipelineStateCache.h"
#include "D3D12Query.h"
#include "D3D12Queue.h"
#include "D3D12RayTracingPSO.h"
#include "D3D12RayTracingSBT.h"
#include "D3D12Residency.h"
#include "D3D12RootSignature.h"
#include "D3D12Sampler.h"
#include "D3D12Shader.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "D3D12View.h"
#include "D3D12WorkGraph.h"

#include <cstdio>

namespace NS::D3D12RHI
{
    //=========================================================================
    // Debug Layer
    //=========================================================================

    void EnableDebugLayer(bool gpuBasedValidation)
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            LOG_INFO("[D3D12RHI] Debug Layer enabled");

            if (gpuBasedValidation)
            {
                ComPtr<ID3D12Debug1> debug1;
                if (SUCCEEDED(debug.As(&debug1)))
                {
                    debug1->SetEnableGPUBasedValidation(TRUE);
                    LOG_INFO("[D3D12RHI] GPU Based Validation enabled");
                }
            }
        }
        else
        {
            LOG_WARN("[D3D12RHI] Debug Layer not available");
        }
    }

    //=========================================================================
    // DRED (Device Removed Extended Data)
    //=========================================================================

    void ConfigureDRED(ID3D12Device5* device)
    {
        if (!device)
            return;

        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
        {
            dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            LOG_INFO("[D3D12RHI] DRED enabled (AutoBreadcrumbs + PageFault)");
        }
    }

    //=========================================================================
    // InfoQueue
    //=========================================================================

    void ConfigureInfoQueue(ID3D12Device* device)
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            // 既知の無害メッセージを抑制
            D3D12_MESSAGE_ID denyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            };

            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(denyIds));
            filter.DenyList.pIDList = denyIds;
            infoQueue->PushStorageFilter(&filter);

            LOG_INFO("[D3D12RHI] InfoQueue configured (break on error/corruption)");
        }
    }

    //=========================================================================
    // D3D12Device::Init
    //=========================================================================

    bool D3D12Device::Init(D3D12Adapter* adapter, bool enableDebug)
    {
        adapter_ = adapter;
        debugEnabled_ = enableDebug;

        // Debug Layer（デバイス作成前）
        if (enableDebug)
        {
            EnableDebugLayer(true);
        }

        // D3D12CreateDevice
        HRESULT hr = D3D12CreateDevice(adapter->GetDXGIAdapter(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));

        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] D3D12CreateDevice failed");
            return false;
        }

        // ID3D12Device5 取得（DXR / DRED用）
        device_.As(&device5_);

        // デバッグ名設定
        SetDebugName("D3D12Device");

        // InfoQueue設定
        if (enableDebug)
        {
            ConfigureInfoQueue(device_.Get());
            if (device5_)
            {
                ConfigureDRED(device5_.Get());
            }
        }

        // Feature検出
        DetectFeatures();

        // コマンドシグネチャ作成
        if (!CreateCommandSignatures())
        {
            LOG_ERROR("[D3D12RHI] Failed to create command signatures");
            return false;
        }

        // コマンドキュー作成（Graphics/Compute/Copy各1つ）
        if (!CreateQueues())
        {
            LOG_ERROR("[D3D12RHI] Failed to create command queues");
            return false;
        }

        // コマンドアロケータープール作成
        allocatorPool_ = new D3D12CommandAllocatorPool(this);

        // コマンドリストプール作成
        commandListPool_ = new D3D12CommandListPool(this);

        // タイムスタンプ周波数（Graphicsキューから取得）
        if (queues_[0])
        {
            timestampFreq_ = queues_[0]->GetTimestampFrequency();
        }

        // ディスクリプタインクリメントサイズキャッシュ
        CacheDescriptorIncrementSizes();

        // Bindlessディスクリプタマネージャー（Tier3以上で有効）
        if (features_.resourceBindingTier >= 3)
        {
            bindlessManager_ = new D3D12BindlessManager();
            if (!bindlessManager_->Init(this))
            {
                LOG_ERROR("[D3D12RHI] Failed to init bindless manager");
                delete bindlessManager_;
                bindlessManager_ = nullptr;
            }
        }

        // GPUプロファイラー初期化
        gpuProfiler_ = new D3D12GPUProfiler();
        if (!gpuProfiler_->Init(this))
        {
            LOG_ERROR("[D3D12RHI] Failed to init GPU profiler");
            delete gpuProfiler_;
            gpuProfiler_ = nullptr;
        }

        // レジデンシーマネージャー初期化
        if (adapter_)
        {
            residencyManager_ = new D3D12ResidencyManager();
            if (!residencyManager_->Init(this, adapter_->GetDXGIAdapter()))
            {
                LOG_ERROR("[D3D12RHI] Failed to init residency manager");
                delete residencyManager_;
                residencyManager_ = nullptr;
            }
        }

        LOG_INFO("[D3D12RHI] D3D12 Device created successfully");
        return true;
    }

    //=========================================================================
    // Feature検出
    //=========================================================================

    void D3D12Device::DetectFeatures()
    {
        // D3D12_FEATURE_D3D12_OPTIONS
        D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
        {
            features_.doublePrecisionFloatShaderOps = options.DoublePrecisionFloatShaderOps;
            features_.outputMergerLogicOp = options.OutputMergerLogicOp;
            features_.rovSupported = options.ROVsSupported;
            features_.conservativeRasterizationTier =
                (options.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
            features_.resourceBindingTier = static_cast<uint32>(options.ResourceBindingTier);
            features_.tiledResourcesTier = static_cast<uint32>(options.TiledResourcesTier);
            features_.resourceHeapTier = static_cast<uint32>(options.ResourceHeapTier);
        }

        // D3D12_FEATURE_D3D12_OPTIONS5 (Render Passes, Raytracing)
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
        {
            features_.renderPassesTier = (options5.RenderPassesTier != D3D12_RENDER_PASS_TIER_0);
            features_.raytracingTier = (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
            features_.raytracingTierValue = static_cast<uint32>(options5.RaytracingTier);
        }

        // D3D12_FEATURE_D3D12_OPTIONS6 (Variable Rate Shading)
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6))))
        {
            features_.vrsTier = static_cast<uint32>(options6.VariableShadingRateTier);
            features_.vrsTileSize = options6.ShadingRateImageTileSize;
            features_.vrsAdditionalShadingRatesSupported = options6.AdditionalShadingRatesSupported;
        }

        // D3D12_FEATURE_D3D12_OPTIONS7 (Mesh Shaders, Sampler Feedback)
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
        {
            features_.meshShaderTier = (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED);
            features_.samplerFeedbackTier = (options7.SamplerFeedbackTier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED);
        }

        // D3D12_FEATURE_D3D12_OPTIONS12 (Enhanced Barriers)
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))))
        {
            features_.enhancedBarriersSupported = options12.EnhancedBarriersSupported;
        }

        // D3D12_FEATURE_D3D12_OPTIONS16 (GPU Upload Heaps)
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
        {
            features_.gpuUploadHeapSupported = options16.GPUUploadHeapSupported;
        }

#ifdef D3D12_FEATURE_D3D12_OPTIONS21
        // D3D12_FEATURE_D3D12_OPTIONS21 (Work Graphs)
        D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21))))
        {
            features_.workGraphsTier = (options21.WorkGraphsTier != D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED);
        }
#endif

        // Wave operations
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1{};
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1))))
        {
            features_.waveOpsSupported = options1.WaveOps;
            features_.waveLaneCountMin = options1.WaveLaneCountMin;
            features_.waveLaneCountMax = options1.WaveLaneCountMax;
            features_.int64ShaderOps = options1.Int64ShaderOps;
        }

        // Shader Model
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
        {
            features_.highestShaderModel = static_cast<uint32>(shaderModel.HighestShaderModel);
        }

        // Architecture (UMA)
        D3D12_FEATURE_DATA_ARCHITECTURE arch{};
        arch.NodeIndex = 0;
        if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch))))
        {
            features_.isUMA = arch.UMA;
        }

        // ログ出力
        {
            char logBuf[512];
            std::snprintf(logBuf,
                          sizeof(logBuf),
                          "[D3D12RHI] Features: RT=%s, Mesh=%s, VRS=%s, Wave=%s(min=%u,max=%u), "
                          "EnhancedBarriers=%s, GPUUpload=%s, Bindless(Tier%u), SM=0x%X",
                          features_.raytracingTier ? "Yes" : "No",
                          features_.meshShaderTier ? "Yes" : "No",
                          features_.vrsTier > 0 ? "Yes" : "No",
                          features_.waveOpsSupported ? "Yes" : "No",
                          features_.waveLaneCountMin,
                          features_.waveLaneCountMax,
                          features_.enhancedBarriersSupported ? "Yes" : "No",
                          features_.gpuUploadHeapSupported ? "Yes" : "No",
                          features_.resourceBindingTier,
                          features_.highestShaderModel);
            LOG_INFO(logBuf);
        }
    }

    //=========================================================================
    // コマンドシグネチャ作成
    //=========================================================================

    bool D3D12Device::CreateCommandSignatures()
    {
        // DrawInstanced indirect
        {
            D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

            D3D12_COMMAND_SIGNATURE_DESC desc{};
            desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
            desc.NumArgumentDescs = 1;
            desc.pArgumentDescs = &argDesc;

            HRESULT hr = device_->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&drawIndirectSig_));
            if (FAILED(hr))
            {
                LOG_HRESULT(hr, "[D3D12RHI] CreateCommandSignature (Draw) failed");
                return false;
            }
        }

        // DrawIndexedInstanced indirect
        {
            D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            D3D12_COMMAND_SIGNATURE_DESC desc{};
            desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            desc.NumArgumentDescs = 1;
            desc.pArgumentDescs = &argDesc;

            HRESULT hr = device_->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&drawIndexedIndirectSig_));
            if (FAILED(hr))
            {
                LOG_HRESULT(hr, "[D3D12RHI] CreateCommandSignature (DrawIndexed) failed");
                return false;
            }
        }

        // Dispatch indirect
        {
            D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_COMMAND_SIGNATURE_DESC desc{};
            desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
            desc.NumArgumentDescs = 1;
            desc.pArgumentDescs = &argDesc;

            HRESULT hr = device_->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&dispatchIndirectSig_));
            if (FAILED(hr))
            {
                LOG_HRESULT(hr, "[D3D12RHI] CreateCommandSignature (Dispatch) failed");
                return false;
            }
        }

        return true;
    }

    //=========================================================================
    // IRHIDevice実装（非スタブ部分）
    //=========================================================================

    NS::RHI::IRHIAdapter* D3D12Device::GetAdapter() const
    {
        return adapter_;
    }

    NS::RHI::RHIMemoryBudget D3D12Device::GetMemoryBudget() const
    {
        NS::RHI::RHIMemoryBudget budget{};

        ComPtr<IDXGIAdapter3> adapter3;
        if (adapter_ && SUCCEEDED(adapter_->GetDXGIAdapter()->QueryInterface(IID_PPV_ARGS(&adapter3))))
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO info{};
            if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
            {
                budget.budget = info.Budget;
                budget.currentUsage = info.CurrentUsage;
            }
        }

        return budget;
    }

    void D3D12Device::SetDebugName(const char* name)
    {
        if (device_ && name)
        {
            wchar_t wname[128]{};
            for (int i = 0; i < 127 && name[i]; ++i)
            {
                wname[i] = static_cast<wchar_t>(name[i]);
            }
            device_->SetName(wname);
        }
    }

    NS::RHI::RHIFormatSupport D3D12Device::GetFormatSupport(NS::RHI::ERHIPixelFormat) const
    {
        // サブ計画18で実装（DXGI_FORMAT変換後）
        return {};
    }

    NS::RHI::ERHIProfilerType D3D12Device::GetAvailableProfiler() const
    {
        return {};
    }

    NS::RHI::ERHIFormatSupportFlags D3D12Device::GetFormatSupportFlags(NS::RHI::ERHIPixelFormat) const
    {
        return {};
    }

    NS::RHI::ERHIPixelFormat D3D12Device::ConvertFromNativeFormat(uint32) const
    {
        return {};
    }

    NS::RHI::ERHIValidationLevel D3D12Device::GetValidationLevel() const
    {
        return {};
    }

    //=========================================================================
    // Fence作成
    //=========================================================================

    NS::RHI::IRHIFence* D3D12Device::CreateFence(const NS::RHI::RHIFenceDesc& desc, const char* debugName)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        if (EnumHasAnyFlags(desc.flags, NS::RHI::RHIFenceDesc::Flags::Shared))
            flags |= D3D12_FENCE_FLAG_SHARED;
        if (EnumHasAnyFlags(desc.flags, NS::RHI::RHIFenceDesc::Flags::CrossAdapter))
            flags |= D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER;

        auto* fence = new D3D12Fence();
        if (!fence->Init(this, desc.initialValue, flags))
        {
            delete fence;
            return nullptr;
        }

        if (debugName)
            fence->SetDebugName(debugName);

        return fence;
    }

    //=========================================================================
    // D3D12Device::Shutdown
    //=========================================================================

    void D3D12Device::Shutdown()
    {
        FlushAllQueues();

        // 遅延削除キューを全て排出（GPU完了済み）
        deferredDeleteQueue_.FlushAll();

        // Bindlessマネージャー破棄
        if (bindlessManager_)
        {
            bindlessManager_->Shutdown();
            delete bindlessManager_;
            bindlessManager_ = nullptr;
        }

        if (gpuProfiler_)
        {
            gpuProfiler_->Shutdown();
            delete gpuProfiler_;
            gpuProfiler_ = nullptr;
        }

        if (residencyManager_)
        {
            residencyManager_->Shutdown();
            delete residencyManager_;
            residencyManager_ = nullptr;
        }

        delete commandListPool_;
        commandListPool_ = nullptr;

        delete allocatorPool_;
        allocatorPool_ = nullptr;

        DestroyQueues();
    }

    //=========================================================================
    // Queue作成・破棄
    //=========================================================================

    bool D3D12Device::CreateQueues()
    {
        const NS::RHI::ERHIQueueType queueTypes[] = {
            NS::RHI::ERHIQueueType::Graphics,
            NS::RHI::ERHIQueueType::Compute,
            NS::RHI::ERHIQueueType::Copy,
        };

        for (uint32 i = 0; i < kQueueTypeCount; ++i)
        {
            auto* queue = new D3D12Queue();
            if (!queue->Init(this, queueTypes[i], i))
            {
                delete queue;
                DestroyQueues();
                return false;
            }
            queues_[i] = queue;
        }

        return true;
    }

    void D3D12Device::DestroyQueues()
    {
        for (uint32 i = 0; i < kQueueTypeCount; ++i)
        {
            if (queues_[i])
            {
                queues_[i]->Shutdown();
                delete queues_[i];
                queues_[i] = nullptr;
            }
        }
    }

    //=========================================================================
    // Queue管理 — IRHIDevice実装
    //=========================================================================

    uint32 D3D12Device::GetQueueCount(NS::RHI::ERHIQueueType type) const
    {
        uint32 idx = static_cast<uint32>(type);
        if (idx < kQueueTypeCount && queues_[idx])
            return 1;
        return 0;
    }

    NS::RHI::IRHIQueue* D3D12Device::GetQueue(NS::RHI::ERHIQueueType type, uint32 index) const
    {
        if (index != 0)
            return nullptr;
        uint32 idx = static_cast<uint32>(type);
        if (idx < kQueueTypeCount)
            return queues_[idx];
        return nullptr;
    }

    D3D12Queue* D3D12Device::GetD3D12Queue(NS::RHI::ERHIQueueType type) const
    {
        uint32 idx = static_cast<uint32>(type);
        if (idx < kQueueTypeCount)
            return queues_[idx];
        return nullptr;
    }

    void D3D12Device::SignalQueue(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (queue)
            queue->Signal(fence, value);
    }

    void D3D12Device::WaitQueue(NS::RHI::IRHIQueue* queue, NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (queue)
            queue->Wait(fence, value);
    }

    void D3D12Device::FlushQueue(NS::RHI::IRHIQueue* queue)
    {
        if (queue)
            queue->Flush();
    }

    void D3D12Device::FlushAllQueues()
    {
        for (uint32 i = 0; i < kQueueTypeCount; ++i)
        {
            if (queues_[i])
                queues_[i]->Flush();
        }
    }

    void D3D12Device::WaitIdle()
    {
        FlushAllQueues();
    }

    void D3D12Device::InsertQueueBarrier(NS::RHI::IRHIQueue* src, NS::RHI::IRHIQueue* dst)
    {
        if (!src || !dst || src == dst)
            return;

        // srcキューでフェンスシグナル → dstキューでWait
        auto* srcQueue = static_cast<D3D12Queue*>(src);
        uint64 fenceValue = srcQueue->AdvanceFence();
        dst->WaitForQueue(src, fenceValue);
    }

    //=========================================================================
    // CommandAllocator管理
    //=========================================================================

    uint32 D3D12Device::ProcessCompletedAllocators()
    {
        if (allocatorPool_)
            return allocatorPool_->ProcessCompletedAllocators();
        return 0;
    }

    NS::RHI::IRHICommandAllocator* D3D12Device::ObtainCommandAllocator(NS::RHI::ERHIQueueType queueType)
    {
        if (allocatorPool_)
            return allocatorPool_->Obtain(queueType);
        return nullptr;
    }

    void D3D12Device::ReleaseCommandAllocator(NS::RHI::IRHICommandAllocator* allocator,
                                              NS::RHI::IRHIFence* fence,
                                              uint64 fenceValue)
    {
        if (allocatorPool_)
            allocatorPool_->Release(allocator, fence, fenceValue);
    }

    void D3D12Device::ReleaseCommandAllocatorImmediate(NS::RHI::IRHICommandAllocator* allocator)
    {
        if (!allocator)
            return;

        // 即時解放：リセットして直接プールに返却（フェンスなし）
        allocator->Reset();
        if (allocatorPool_)
            allocatorPool_->Release(allocator, nullptr, 0);
    }

    //=========================================================================
    // CommandList管理
    //=========================================================================

    NS::RHI::IRHICommandList* D3D12Device::ObtainCommandList(NS::RHI::IRHICommandAllocator* allocator,
                                                             NS::RHI::IRHIPipelineState* /*initialPSO*/)
    {
        if (commandListPool_ && allocator)
            return commandListPool_->Obtain(allocator, NS::RHI::ERHICommandListType::Direct);
        return nullptr;
    }

    void D3D12Device::ReleaseCommandList(NS::RHI::IRHICommandList* commandList)
    {
        if (commandListPool_)
            commandListPool_->Release(commandList);
    }

    //=========================================================================
    // Context管理
    //=========================================================================

    NS::RHI::IRHICommandContext* D3D12Device::ObtainContext(NS::RHI::ERHIQueueType queueType)
    {
        auto* ctx = new D3D12CommandContext();
        if (!ctx->Init(this, queueType))
        {
            delete ctx;
            return nullptr;
        }
        return ctx;
    }

    NS::RHI::IRHIComputeContext* D3D12Device::ObtainComputeContext()
    {
        auto* ctx = new D3D12ComputeContext();
        if (!ctx->Init(this))
        {
            delete ctx;
            return nullptr;
        }
        return ctx;
    }

    void D3D12Device::ReleaseContext(NS::RHI::IRHICommandContext* context)
    {
        delete static_cast<D3D12CommandContext*>(context);
    }

    void D3D12Device::ReleaseContext(NS::RHI::IRHIComputeContext* context)
    {
        delete static_cast<D3D12ComputeContext*>(context);
    }

    NS::RHI::IRHICommandList* D3D12Device::FinalizeContext(NS::RHI::IRHICommandContext* context)
    {
        if (!context)
            return nullptr;
        return context->Finish();
    }

    NS::RHI::IRHICommandList* D3D12Device::FinalizeContext(NS::RHI::IRHIComputeContext* context)
    {
        if (!context)
            return nullptr;
        return context->Finish();
    }

    void D3D12Device::ResetContext(NS::RHI::IRHICommandContext* context, NS::RHI::IRHICommandAllocator* allocator)
    {
        if (context)
        {
            context->Reset();
            if (allocator)
                context->Begin(allocator);
        }
    }

    void D3D12Device::ResetContext(NS::RHI::IRHIComputeContext* context, NS::RHI::IRHICommandAllocator* allocator)
    {
        if (context)
        {
            context->Reset();
            if (allocator)
                context->Begin(allocator);
        }
    }

    //=========================================================================
    // ディスクリプタ管理
    //=========================================================================

    void D3D12Device::CacheDescriptorIncrementSizes()
    {
        descriptorIncrementSize_[0] = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        descriptorIncrementSize_[1] = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        descriptorIncrementSize_[2] = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptorIncrementSize_[3] = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    NS::RHI::IRHIDescriptorHeap* D3D12Device::CreateDescriptorHeap(const NS::RHI::RHIDescriptorHeapDesc& desc,
                                                                   const char* debugName)
    {
        auto* heap = new D3D12DescriptorHeap();
        if (!heap->Init(this, desc, debugName))
        {
            delete heap;
            return nullptr;
        }
        return heap;
    }

    uint32 D3D12Device::GetMaxDescriptorCount(NS::RHI::ERHIDescriptorHeapType type) const
    {
        switch (type)
        {
        case NS::RHI::ERHIDescriptorHeapType::CBV_SRV_UAV:
            return 1000000;
        case NS::RHI::ERHIDescriptorHeapType::Sampler:
            return 2048;
        case NS::RHI::ERHIDescriptorHeapType::RTV:
            return 1024;
        case NS::RHI::ERHIDescriptorHeapType::DSV:
            return 1024;
        default:
            return 0;
        }
    }

    uint32 D3D12Device::GetDescriptorIncrementSize(NS::RHI::ERHIDescriptorHeapType type) const
    {
        uint32 idx = static_cast<uint32>(type);
        if (idx < 4)
            return descriptorIncrementSize_[idx];
        return 0;
    }

    void D3D12Device::CopyDescriptor(NS::RHI::RHICPUDescriptorHandle dst,
                                     NS::RHI::RHICPUDescriptorHandle src,
                                     NS::RHI::ERHIDescriptorHeapType type)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE d3dDst{dst.ptr};
        D3D12_CPU_DESCRIPTOR_HANDLE d3dSrc{src.ptr};
        device_->CopyDescriptorsSimple(1, d3dDst, d3dSrc, ConvertDescriptorHeapType(type));
    }

    void D3D12Device::CopyDescriptors(NS::RHI::RHICPUDescriptorHandle dst,
                                      NS::RHI::RHICPUDescriptorHandle src,
                                      uint32 count,
                                      NS::RHI::ERHIDescriptorHeapType type)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE d3dDst{dst.ptr};
        D3D12_CPU_DESCRIPTOR_HANDLE d3dSrc{src.ptr};
        device_->CopyDescriptorsSimple(count, d3dDst, d3dSrc, ConvertDescriptorHeapType(type));
    }

    //=========================================================================
    // Bindless
    //=========================================================================

    NS::RHI::BindlessSRVIndex D3D12Device::AllocateBindlessSRV(NS::RHI::IRHIShaderResourceView* view)
    {
        if (!bindlessManager_ || !view)
            return {};

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{view->GetCPUHandle().ptr};
        return bindlessManager_->AllocateSRV(cpuHandle);
    }

    NS::RHI::BindlessUAVIndex D3D12Device::AllocateBindlessUAV(NS::RHI::IRHIUnorderedAccessView* view)
    {
        if (!bindlessManager_ || !view)
            return {};

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{view->GetCPUHandle().ptr};
        return bindlessManager_->AllocateUAV(cpuHandle);
    }

    void D3D12Device::FreeBindlessSRV(NS::RHI::BindlessSRVIndex index)
    {
        if (bindlessManager_)
            bindlessManager_->FreeSRV(index);
    }

    void D3D12Device::FreeBindlessUAV(NS::RHI::BindlessUAVIndex index)
    {
        if (bindlessManager_)
            bindlessManager_->FreeUAV(index);
    }

    NS::RHI::IRHIDescriptorHeap* D3D12Device::GetBindlessSRVUAVHeap() const
    {
        if (!bindlessManager_)
            return nullptr;
        return bindlessManager_->GetResourceHeapWrapper();
    }

    NS::RHI::IRHIDescriptorHeap* D3D12Device::GetBindlessSamplerHeap() const
    {
        if (!bindlessManager_)
            return nullptr;
        return bindlessManager_->GetSamplerHeapWrapper();
    }

    //=========================================================================
    // クエリ
    //=========================================================================

    NS::RHI::IRHIQueryHeap* D3D12Device::CreateQueryHeap(const NS::RHI::RHIQueryHeapDesc& desc, const char* debugName)
    {
        auto* heap = new D3D12QueryHeap();
        if (!heap->Init(this, desc, debugName))
        {
            delete heap;
            return nullptr;
        }
        return heap;
    }

    bool D3D12Device::GetQueryData(NS::RHI::IRHIQueryHeap* queryHeap,
                                   uint32 startIndex,
                                   uint32 numQueries,
                                   void* outData,
                                   uint32 dataSize,
                                   NS::RHI::ERHIQueryFlags /*flags*/)
    {
        if (!queryHeap || !outData || numQueries == 0)
            return false;

        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);
        const void* mappedPtr = d3dHeap->GetMappedPtr();
        if (!mappedPtr)
            return false;

        uint32 resultSize = d3dHeap->GetQueryResultSize();
        uint32 totalSize = resultSize * numQueries;
        if (dataSize < totalSize)
            return false;

        // 永続Map済みReadbackバッファから直接コピー
        const uint8* srcPtr = static_cast<const uint8*>(mappedPtr) + static_cast<uint64>(startIndex) * resultSize;
        memcpy(outData, srcPtr, totalSize);
        return true;
    }

    NS::RHI::IRHIGPUProfiler* D3D12Device::GetGPUProfiler()
    {
        return gpuProfiler_;
    }

    bool D3D12Device::GetTimestampCalibration(uint64& gpuTimestamp, uint64& cpuTimestamp) const
    {
        // Graphicsキューからキャリブレーション取得
        if (!queues_[0])
            return false;

        auto* d3dQueue = queues_[0]->GetD3DCommandQueue();
        if (!d3dQueue)
            return false;

        HRESULT hr = d3dQueue->GetClockCalibration(&gpuTimestamp, &cpuTimestamp);
        return SUCCEEDED(hr);
    }

    //=========================================================================
    // レジデンシー管理
    //=========================================================================

    void D3D12Device::MakeResident(NS::RHI::IRHIResource* const* resources, uint32 count)
    {
        if (!residencyManager_ || !resources || count == 0)
            return;

        // IRHIResource → ID3D12Pageable変換
        // 最大64個のバッチ（スタック上）
        constexpr uint32 kMaxBatch = 64;
        ID3D12Pageable* pageables[kMaxBatch];
        uint32 pageableCount = 0;

        for (uint32 i = 0; i < count; ++i)
        {
            if (!resources[i])
                continue;

            ID3D12Resource* d3dResource = nullptr;
            auto type = resources[i]->GetResourceType();
            if (type == NS::RHI::ERHIResourceType::Buffer)
                d3dResource =
                    static_cast<D3D12Buffer*>(static_cast<NS::RHI::IRHIBuffer*>(resources[i]))->GetD3DResource();
            else if (type == NS::RHI::ERHIResourceType::Texture)
                d3dResource =
                    static_cast<D3D12Texture*>(static_cast<NS::RHI::IRHITexture*>(resources[i]))->GetD3DResource();

            if (d3dResource)
            {
                pageables[pageableCount++] = d3dResource;
                if (pageableCount >= kMaxBatch)
                {
                    residencyManager_->MakeResident(pageables, pageableCount);
                    pageableCount = 0;
                }
            }
        }

        if (pageableCount > 0)
            residencyManager_->MakeResident(pageables, pageableCount);
    }

    void D3D12Device::Evict(NS::RHI::IRHIResource* const* resources, uint32 count)
    {
        if (!residencyManager_ || !resources || count == 0)
            return;

        constexpr uint32 kMaxBatch = 64;
        ID3D12Pageable* pageables[kMaxBatch];
        uint32 pageableCount = 0;

        for (uint32 i = 0; i < count; ++i)
        {
            if (!resources[i])
                continue;

            ID3D12Resource* d3dResource = nullptr;
            auto type = resources[i]->GetResourceType();
            if (type == NS::RHI::ERHIResourceType::Buffer)
                d3dResource =
                    static_cast<D3D12Buffer*>(static_cast<NS::RHI::IRHIBuffer*>(resources[i]))->GetD3DResource();
            else if (type == NS::RHI::ERHIResourceType::Texture)
                d3dResource =
                    static_cast<D3D12Texture*>(static_cast<NS::RHI::IRHITexture*>(resources[i]))->GetD3DResource();

            if (d3dResource)
            {
                pageables[pageableCount++] = d3dResource;
                if (pageableCount >= kMaxBatch)
                {
                    residencyManager_->Evict(pageables, pageableCount);
                    pageableCount = 0;
                }
            }
        }

        if (pageableCount > 0)
            residencyManager_->Evict(pageables, pageableCount);
    }

    void D3D12Device::SetMemoryPressureCallback(MemoryPressureCallback callback)
    {
        if (residencyManager_)
            residencyManager_->SetMemoryPressureCallback(callback);
    }

    NS::RHI::RHIMemoryStats D3D12Device::GetMemoryStats() const
    {
        NS::RHI::RHIMemoryStats stats{};

        if (residencyManager_)
        {
            // IDXGIAdapter3から最新情報取得
            const_cast<D3D12ResidencyManager*>(residencyManager_)->UpdateMemoryBudget();

            stats.allocatedDefault = residencyManager_->GetDedicatedUsage();
            stats.allocatedUpload = residencyManager_->GetSharedUsage();
            stats.usedDefault = residencyManager_->GetDedicatedUsage();
            stats.usedUpload = residencyManager_->GetSharedUsage();
        }

        return stats;
    }

    //=========================================================================
    // デバイスロスト
    //=========================================================================

    void D3D12Device::SetDeviceLostCallback(NS::RHI::RHIDeviceLostCallback callback, void* userData)
    {
        deviceLostCallback_ = callback;
        deviceLostUserData_ = userData;
    }

    bool D3D12Device::GetGPUCrashInfo(NS::RHI::RHIGPUCrashInfo& outInfo)
    {
        if (!device_)
            return false;
        return D3D12DeviceLostHelper::GetCrashInfo(device_.Get(), outInfo);
    }

    void D3D12Device::SetBreadcrumbBuffer(NS::RHI::RHIBreadcrumbBuffer* buffer)
    {
        breadcrumbBuffer_ = buffer;
    }

    bool D3D12Device::GetDeviceLostInfo(NS::RHI::RHIDeviceLostInfo& outInfo) const
    {
        if (!device_)
            return false;
        return D3D12DeviceLostHelper::CheckDeviceLost(device_.Get(), outInfo);
    }

    bool D3D12Device::CheckDeviceRemoved()
    {
        if (!device_ || deviceLost_)
            return deviceLost_;

        HRESULT reason = device_->GetDeviceRemovedReason();
        if (reason == S_OK)
            return false;

        deviceLost_ = true;
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "[D3D12RHI] Device removed! HRESULT: 0x%08X", static_cast<unsigned>(reason));
            LOG_ERROR(buf);
        }

        // コールバック通知
        if (deviceLostCallback_)
        {
            NS::RHI::RHIGPUCrashInfo crashInfo{};
            D3D12DeviceLostHelper::GetCrashInfo(device_.Get(), crashInfo);
            deviceLostCallback_(crashInfo, deviceLostUserData_);
        }

        return true;
    }

    //=========================================================================
    // レイトレーシング加速構造
    //=========================================================================

    NS::RHI::IRHIAccelerationStructure* D3D12Device::CreateAccelerationStructure(
        const NS::RHI::RHIRaytracingAccelerationStructureDesc& desc, const char* debugName)
    {
        auto* as = new D3D12AccelerationStructure();
        if (!as->Init(this, desc, debugName))
        {
            delete as;
            return nullptr;
        }
        return as;
    }

    NS::RHI::RHIRaytracingAccelerationStructurePrebuildInfo D3D12Device::GetAccelerationStructurePrebuildInfo(
        const NS::RHI::RHIRaytracingAccelerationStructureBuildInputs& inputs) const
    {
        NS::RHI::RHIRaytracingAccelerationStructurePrebuildInfo result{};

        if (!device5_)
            return result;

        constexpr uint32 kMaxGeometries = 64;
        D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[kMaxGeometries];
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3dInputs{};
        ConvertBuildInputs(inputs, d3dInputs, geometryDescs, kMaxGeometries);

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3dInfo{};
        device5_->GetRaytracingAccelerationStructurePrebuildInfo(&d3dInputs, &d3dInfo);

        result.resultDataMaxSize = d3dInfo.ResultDataMaxSizeInBytes;
        result.scratchDataSize = d3dInfo.ScratchDataSizeInBytes;
        result.updateScratchDataSize = d3dInfo.UpdateScratchDataSizeInBytes;

        return result;
    }

    NS::RHI::RHIRaytracingCapabilities D3D12Device::GetRaytracingCapabilities() const
    {
        NS::RHI::RHIRaytracingCapabilities caps{};

        if (features_.raytracingTier)
        {
            if (features_.raytracingTierValue >= 11)
                caps.tier = NS::RHI::ERHIRaytracingTier::Tier1_1;
            else
                caps.tier = NS::RHI::ERHIRaytracingTier::Tier1_0;

            caps.maxInstanceCount = D3D12_RAYTRACING_MAX_INSTANCES_PER_TOP_LEVEL_ACCELERATION_STRUCTURE;
            caps.maxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
            caps.maxGeometryCount = D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
            caps.maxPrimitiveCount = D3D12_RAYTRACING_MAX_PRIMITIVES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE;
            caps.supportsInlineRaytracing = (features_.raytracingTierValue >= 11);
        }

        return caps;
    }

    NS::RHI::IRHIRaytracingPipelineState* D3D12Device::CreateRaytracingPipelineState(
        const NS::RHI::RHIRaytracingPipelineStateDesc& desc, const char* debugName)
    {
        auto* rtpso = new D3D12RaytracingPipelineState();
        if (!rtpso->Init(this, desc, debugName))
        {
            delete rtpso;
            return nullptr;
        }
        return rtpso;
    }

    NS::RHI::IRHIShaderBindingTable* D3D12Device::CreateShaderBindingTable(
        const NS::RHI::RHIShaderBindingTableDesc& desc, const char* debugName)
    {
        auto* sbt = new D3D12ShaderBindingTable();
        if (!sbt->Init(this, desc, debugName))
        {
            delete sbt;
            return nullptr;
        }
        return sbt;
    }

    //=========================================================================
    // ワークグラフ
    //=========================================================================

    NS::RHI::IRHIWorkGraphPipeline* D3D12Device::CreateWorkGraphPipeline(const NS::RHI::RHIWorkGraphPipelineDesc& desc)
    {
        auto* wg = new D3D12WorkGraphPipeline();
        if (!wg->Init(this, desc))
        {
            delete wg;
            return nullptr;
        }
        return wg;
    }

    NS::RHI::RHIWorkGraphMemoryRequirements D3D12Device::GetWorkGraphMemoryRequirements(
        NS::RHI::IRHIWorkGraphPipeline* pipeline) const
    {
        NS::RHI::RHIWorkGraphMemoryRequirements reqs{};
        if (!pipeline)
            return reqs;

        auto* wg = static_cast<D3D12WorkGraphPipeline*>(pipeline);
        reqs.maxSize = wg->GetBackingMemorySize();
        reqs.minSize = reqs.maxSize; // Work Graphs uses maxSize as requirement
        reqs.sizeGranularity = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        return reqs;
    }

    //=========================================================================
    // パイプラインステート作成
    //=========================================================================

    NS::RHI::IRHIInputLayout* D3D12Device::CreateInputLayout(const NS::RHI::RHIInputLayoutDesc& desc,
                                                             const NS::RHI::RHIShaderBytecode& /*shaderBytecode*/,
                                                             const char* debugName)
    {
        auto* layout = new D3D12InputLayout();
        if (!layout->Init(this, desc, debugName))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }

    NS::RHI::IRHIGraphicsPipelineState* D3D12Device::CreateGraphicsPipelineState(
        const NS::RHI::RHIGraphicsPipelineStateDesc& desc, const char* debugName)
    {
        auto* pso = new D3D12GraphicsPipelineState();
        if (!pso->Init(this, desc, debugName))
        {
            delete pso;
            return nullptr;
        }
        return pso;
    }

    NS::RHI::IRHIGraphicsPipelineState* D3D12Device::CreateGraphicsPipelineStateFromCache(
        const NS::RHI::RHIGraphicsPipelineStateDesc& desc,
        const NS::RHI::RHIShaderBytecode& /*cachedBlob*/,
        const char* debugName)
    {
        // キャッシュ無視で通常作成（PSOキャッシュはサブ計画25で実装）
        return CreateGraphicsPipelineState(desc, debugName);
    }

    NS::RHI::IRHIComputePipelineState* D3D12Device::CreateComputePipelineState(
        const NS::RHI::RHIComputePipelineStateDesc& desc, const char* debugName)
    {
        auto* pso = new D3D12ComputePipelineState();
        if (!pso->Init(this, desc, debugName))
        {
            delete pso;
            return nullptr;
        }
        return pso;
    }

    NS::RHI::IRHIComputePipelineState* D3D12Device::CreateComputePipelineStateFromCache(
        const NS::RHI::RHIComputePipelineStateDesc& desc,
        const NS::RHI::RHIShaderBytecode& /*cachedBlob*/,
        const char* debugName)
    {
        // キャッシュ無視で通常作成（PSOキャッシュはサブ計画25で実装）
        return CreateComputePipelineState(desc, debugName);
    }

    //=========================================================================
    // シェーダー作成
    //=========================================================================

    NS::RHI::IRHIShader* D3D12Device::CreateShader(const NS::RHI::RHIShaderDesc& desc, const char* debugName)
    {
        auto* shader = new D3D12Shader();
        if (!shader->Init(this, desc, debugName))
        {
            delete shader;
            return nullptr;
        }
        return shader;
    }

    //=========================================================================
    // PSOキャッシュ作成
    //=========================================================================

    NS::RHI::IRHIPipelineStateCache* D3D12Device::CreatePipelineStateCache()
    {
        return new D3D12PipelineStateCache();
    }

    //=========================================================================
    // メッシュシェーダーPSO作成
    //=========================================================================

    NS::RHI::IRHIMeshPipelineState* D3D12Device::CreateMeshPipelineState(const NS::RHI::RHIMeshPipelineStateDesc& desc,
                                                                         const char* debugName)
    {
        if (!features_.meshShaderTier)
        {
            LOG_ERROR("[D3D12RHI] Mesh shaders not supported on this device");
            return nullptr;
        }

        auto* pso = new D3D12MeshPipelineState();
        if (!pso->Init(this, desc, debugName))
        {
            delete pso;
            return nullptr;
        }
        return pso;
    }

    NS::RHI::RHIMeshShaderCapabilities D3D12Device::GetMeshShaderCapabilities() const
    {
        NS::RHI::RHIMeshShaderCapabilities caps{};
        if (!features_.meshShaderTier)
            return caps;

        caps.supported = true;
        caps.amplificationShaderSupported = true;
        caps.maxOutputVertices = 256;
        caps.maxOutputPrimitives = 256;
        caps.maxMeshWorkGroupSize = 128;
        caps.maxTaskWorkGroupSize = 128;
        return caps;
    }

    //=========================================================================
    // VRS
    //=========================================================================

    NS::RHI::RHIVRSCapabilities D3D12Device::GetVRSCapabilities() const
    {
        NS::RHI::RHIVRSCapabilities caps{};
        if (features_.vrsTier == 0)
            return caps;

        // Tier 1: per-draw shading rate
        caps.supportsPipelineVRS = true;
        caps.supportsLargerSizes = features_.vrsAdditionalShadingRatesSupported;

        if (features_.vrsTier >= 2)
        {
            // Tier 2: per-primitive + shading rate image
            caps.supportsImageVRS = true;
            caps.supportsPerPrimitiveVRS = true;
            caps.supportsComplexCombiners = true;
            caps.imageTileMinWidth = features_.vrsTileSize;
            caps.imageTileMinHeight = features_.vrsTileSize;
            caps.imageTileMaxWidth = features_.vrsTileSize;
            caps.imageTileMaxHeight = features_.vrsTileSize;
            caps.imageType = NS::RHI::ERHIVRSImageType::Palette;
            caps.imageFormat = NS::RHI::ERHIPixelFormat::R8_UINT;
        }

        return caps;
    }

    NS::RHI::IRHITexture* D3D12Device::CreateVRSImage(const NS::RHI::RHIVRSImageDesc& desc)
    {
        if (features_.vrsTier < 2 || features_.vrsTileSize == 0)
        {
            LOG_ERROR("[D3D12RHI] VRS Tier 2 required for shading rate image");
            return nullptr;
        }

        uint32 tileW = desc.tileWidth > 0 ? desc.tileWidth : features_.vrsTileSize;
        uint32 tileH = desc.tileHeight > 0 ? desc.tileHeight : features_.vrsTileSize;
        uint32 imageW = (desc.targetWidth + tileW - 1) / tileW;
        uint32 imageH = (desc.targetHeight + tileH - 1) / tileH;

        NS::RHI::RHITextureDesc texDesc{};
        texDesc.dimension = NS::RHI::ERHITextureDimension::Texture2D;
        texDesc.width = imageW;
        texDesc.height = imageH;
        texDesc.depthOrArraySize = 1;
        texDesc.mipLevels = 1;
        texDesc.format = NS::RHI::ERHIPixelFormat::R8_UINT;
        texDesc.usage = NS::RHI::ERHITextureUsage::UnorderedAccess;

        auto* texture = new D3D12Texture();
        if (!texture->Init(this, texDesc))
        {
            delete texture;
            return nullptr;
        }

        if (desc.debugName)
        {
            texture->SetDebugName(desc.debugName);
        }

        return texture;
    }

    //=========================================================================
    // SwapChain作成
    //=========================================================================

    NS::RHI::IRHISwapChain* D3D12Device::CreateSwapChain(const NS::RHI::RHISwapChainDesc& desc,
                                                         NS::RHI::IRHIQueue* presentQueue,
                                                         const char* debugName)
    {
        if (!presentQueue || !dxgiFactory_)
            return nullptr;

        auto* d3dQueue = static_cast<D3D12Queue*>(presentQueue);

        auto* swapChain = new D3D12SwapChain();
        if (!swapChain->Init(this, dxgiFactory_, d3dQueue, desc, debugName))
        {
            delete swapChain;
            return nullptr;
        }

        return swapChain;
    }

    //=========================================================================
    // サンプラー作成
    //=========================================================================

    NS::RHI::IRHISampler* D3D12Device::CreateSampler(const NS::RHI::RHISamplerDesc& desc, const char* debugName)
    {
        auto* sampler = new D3D12Sampler();
        if (!sampler->Init(this, desc, debugName))
        {
            delete sampler;
            return nullptr;
        }
        return sampler;
    }

    //=========================================================================
    // ルートシグネチャ作成
    //=========================================================================

    NS::RHI::IRHIRootSignature* D3D12Device::CreateRootSignature(const NS::RHI::RHIRootSignatureDesc& desc,
                                                                 const char* debugName)
    {
        auto* rootSig = new D3D12RootSignature();
        if (!rootSig->Init(this, desc, debugName))
        {
            delete rootSig;
            return nullptr;
        }
        return rootSig;
    }

    NS::RHI::IRHIRootSignature* D3D12Device::CreateRootSignatureFromBlob(const NS::RHI::RHIShaderBytecode& blob,
                                                                         const char* debugName)
    {
        auto* rootSig = new D3D12RootSignature();
        if (!rootSig->InitFromBlob(this, blob, debugName))
        {
            delete rootSig;
            return nullptr;
        }
        return rootSig;
    }

    //=========================================================================
    // ビュー作成
    //=========================================================================

    NS::RHI::IRHIShaderResourceView* D3D12Device::CreateShaderResourceView(const NS::RHI::RHIBufferSRVDesc& desc,
                                                                           const char* debugName)
    {
        auto* view = new D3D12ShaderResourceView();
        if (!view->InitFromBuffer(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIShaderResourceView* D3D12Device::CreateShaderResourceView(const NS::RHI::RHITextureSRVDesc& desc,
                                                                           const char* debugName)
    {
        auto* view = new D3D12ShaderResourceView();
        if (!view->InitFromTexture(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIUnorderedAccessView* D3D12Device::CreateUnorderedAccessView(const NS::RHI::RHIBufferUAVDesc& desc,
                                                                             const char* debugName)
    {
        auto* view = new D3D12UnorderedAccessView();
        if (!view->InitFromBuffer(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIUnorderedAccessView* D3D12Device::CreateUnorderedAccessView(const NS::RHI::RHITextureUAVDesc& desc,
                                                                             const char* debugName)
    {
        auto* view = new D3D12UnorderedAccessView();
        if (!view->InitFromTexture(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIRenderTargetView* D3D12Device::CreateRenderTargetView(const NS::RHI::RHIRenderTargetViewDesc& desc,
                                                                       const char* debugName)
    {
        auto* view = new D3D12RenderTargetView();
        if (!view->Init(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIDepthStencilView* D3D12Device::CreateDepthStencilView(const NS::RHI::RHIDepthStencilViewDesc& desc,
                                                                       const char* debugName)
    {
        auto* view = new D3D12DepthStencilView();
        if (!view->Init(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    NS::RHI::IRHIConstantBufferView* D3D12Device::CreateConstantBufferView(
        const NS::RHI::RHIConstantBufferViewDesc& desc, const char* debugName)
    {
        auto* view = new D3D12ConstantBufferView();
        if (!view->Init(this, desc, debugName))
        {
            delete view;
            return nullptr;
        }
        return view;
    }

    //=========================================================================
    // Memory Allocation
    //=========================================================================

    NS::RHI::RHIResourceAllocationInfo D3D12Device::GetBufferAllocationInfo(const NS::RHI::RHIBufferDesc& desc) const
    {
        D3D12_RESOURCE_DESC d3dDesc = {};
        d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d3dDesc.Width = desc.size;
        d3dDesc.Height = 1;
        d3dDesc.DepthOrArraySize = 1;
        d3dDesc.MipLevels = 1;
        d3dDesc.SampleDesc.Count = 1;
        d3dDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device_->GetResourceAllocationInfo(0, 1, &d3dDesc);

        NS::RHI::RHIResourceAllocationInfo info;
        info.size = allocInfo.SizeInBytes;
        info.alignment = allocInfo.Alignment;
        return info;
    }

    NS::RHI::RHIResourceAllocationInfo D3D12Device::GetTextureAllocationInfo(const NS::RHI::RHITextureDesc& desc) const
    {
        D3D12_RESOURCE_DESC d3dDesc = {};
        d3dDesc.Width = desc.width;
        d3dDesc.Height = desc.height;
        d3dDesc.DepthOrArraySize = static_cast<UINT16>(desc.depthOrArraySize);
        d3dDesc.MipLevels = static_cast<UINT16>(desc.mipLevels);
        d3dDesc.SampleDesc.Count = static_cast<UINT>(desc.sampleCount);
        d3dDesc.SampleDesc.Quality = 0;

        switch (desc.dimension)
        {
        case NS::RHI::ERHITextureDimension::Texture1D:
        case NS::RHI::ERHITextureDimension::Texture1DArray:
            d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case NS::RHI::ERHITextureDimension::Texture3D:
            d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        default:
            d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        }

        // 簡易フォーマット変換（一般的なフォーマットのみ）
        d3dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // デフォルト

        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device_->GetResourceAllocationInfo(0, 1, &d3dDesc);

        NS::RHI::RHIResourceAllocationInfo info;
        info.size = allocInfo.SizeInBytes;
        info.alignment = allocInfo.Alignment;
        return info;
    }

    NS::RHI::IRHITransientResourceAllocator* D3D12Device::CreateTransientAllocator(
        const NS::RHI::RHITransientAllocatorDesc& desc)
    {
        auto* allocator = new D3D12TransientResourceAllocator();
        if (!allocator->Init(this, desc))
        {
            delete allocator;
            return nullptr;
        }
        return allocator;
    }

} // namespace NS::D3D12RHI
