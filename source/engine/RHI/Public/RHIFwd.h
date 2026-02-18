/// @file RHIFwd.h
/// @brief RHI全体の前方宣言
/// @details 全RHIインターフェースと構造体の前方宣言を提供する。
///          循環依存を防ぎ、コンパイル時間を短縮するために使用。
/// @see 01-01-fwd-macros.md
#pragma once

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // スマートポインタ
        //=========================================================================

        template <typename T> class TRefCountPtr;

        //=========================================================================
        // リソース基底
        //=========================================================================

        class IRHIResource;

        //=========================================================================
        // バッファ・テクスチャ
        //=========================================================================

        class IRHIBuffer;
        class IRHITexture;

        //=========================================================================
        // ビュー
        //=========================================================================

        class IRHIShaderResourceView;
        class IRHIUnorderedAccessView;
        class IRHIRenderTargetView;
        class IRHIDepthStencilView;
        class IRHIConstantBufferView;

        //=========================================================================
        // サンプラー
        //=========================================================================

        class IRHISampler;

        //=========================================================================
        // シェーダー・パイプライン
        //=========================================================================

        class IRHIShader;
        class IRHIPipelineState;
        class IRHIGraphicsPipelineState;
        class IRHIComputePipelineState;
        class IRHIRootSignature;
        class IRHIShaderLibrary;

        //=========================================================================
        // コマンド
        //=========================================================================

        class IRHICommandContextBase;
        class IRHICommandContext;
        class IRHIComputeContext;
        class IRHIUploadContext;
        class IRHIImmediateContext;
        class IRHICommandList;
        class IRHICommandAllocator;
        class IRHICommandAllocatorPool;
        class IRHICommandListPool;

        //=========================================================================
        // 同期
        //=========================================================================

        class IRHIFence;
        class IRHISyncPoint;

        //=========================================================================
        // デバイス階層
        //=========================================================================

        class IRHIQueue;
        class IRHIDevice;
        class IRHIAdapter;
        class IDynamicRHI;

        //=========================================================================
        // スワップチェーン
        //=========================================================================

        class IRHISwapChain;

        //=========================================================================
        // クエリ (14-01〜14-04)
        //=========================================================================

        class IRHIQueryHeap;
        struct RHIQueryHeapDesc;
        struct RHIOcclusionQueryResult;
        struct RHIPipelineStatisticsResult;
        struct RHIStreamOutputStatisticsResult;
        struct RHIQueryAllocation;
        class RHIQueryAllocator;
        struct RHITimestampResult;
        struct RHITimestampInterval;
        class RHIGPUTimer;
        class RHIScopedGPUTimer;
        struct RHIFrameTimelineEntry;
        class RHIFrameTimeline;
        struct RHIOcclusionResult;
        struct RHIOcclusionQueryId;
        class RHIOcclusionQueryManager;
        class RHIConditionalRendering;
        class RHIHiZBuffer;

        //=========================================================================
        // ディスクリプタ
        //=========================================================================

        class IRHIDescriptorHeap;

        //=========================================================================
        // 記述子（Desc構造体）
        //=========================================================================

        struct RHIBufferDesc;
        struct RHITextureDesc;
        struct RHISubresourceData;
        struct RHIShaderDesc;
        struct RHIGraphicsPipelineStateDesc;
        struct RHIComputePipelineStateDesc;
        struct RHISamplerDesc;
        struct RHISwapChainDesc;

        // IDynamicRHIファクトリ用統合記述子
        struct RHISRVDesc;
        struct RHIUAVDesc;
        struct RHIRTVDesc;
        struct RHIDSVDesc;
        struct RHICBVDesc;

        //=========================================================================
        // ビュー記述子 (05-01〜05-05)
        //=========================================================================

        struct RHIBufferSRVDesc;
        struct RHITextureSRVDesc;
        struct RHIBufferUAVDesc;
        struct RHITextureUAVDesc;
        struct RHIRenderTargetViewDesc;
        struct RHIDepthStencilViewDesc;
        struct RHIConstantBufferViewDesc;

        //=========================================================================
        // アダプター情報
        //=========================================================================

        struct RHIAdapterDesc;

        //=========================================================================
        // レンダーパス (13-01〜13-03)
        //=========================================================================

        struct RHIRenderPassInfo;
        struct RHIRenderTargetAttachment;
        struct RHIDepthStencilAttachment;
        struct RHIRenderPassDesc;
        struct RHIExtendedLoadStoreDesc;
        struct RHISubpassDesc;
        struct RHISubpassDependency;
        struct RHIRenderPassStatistics;
        class RHIScopedRenderPass;

        //=========================================================================
        // リソース状態・バリア (16-01〜16-03)
        //=========================================================================

        struct RHISubresourceState;
        class RHIResourceStateMap;
        struct RHITransitionBarrier;
        struct RHIUAVBarrier;
        struct RHIAliasingBarrier;
        class RHIBarrierBatch;
        class RHISplitBarrier;
        class RHISplitBarrierBatch;
        struct RHIEnhancedBarrierDesc;
        class RHIResourceStateTracker;
        class RHIGlobalResourceStateManager;
        class RHIAutoBarrierContext;
        class RHIResourceStateValidator;

        //=========================================================================
        // ピクセルフォーマット (15-01〜15-03)
        //=========================================================================

        struct RHIFormatInfo;
        struct RHIBlockSize;
        struct RHIMSAASupportInfo;

        //=========================================================================
        // ステート記述子
        //=========================================================================

        struct RHIBlendStateDesc;
        struct RHIRasterizerStateDesc;
        struct RHIDepthStencilStateDesc;

        //=========================================================================
        // ワークグラフ
        //=========================================================================

        class IRHIWorkGraphPipeline;
        struct RHIWorkGraphPipelineDesc;
        struct RHIWorkGraphDispatchDesc;
        struct RHIWorkGraphMemoryRequirements;
        struct RHIWorkGraphBackingMemory;

        //=========================================================================
        // ルートシグネチャ関連 (08-01〜08-03)
        //=========================================================================

        struct RHIRootConstantsDesc;
        struct RHIRootDescriptor;
        struct RHIRootParameter;
        struct RHIDescriptorRange;
        struct RHIDescriptorTableDesc;
        class RHIDescriptorTableBuilder;
        struct RHIStaticSamplerDesc;
        struct RHIRootSignatureDesc;
        class RHIRootSignatureBuilder;

        //=========================================================================
        // バインディングレイアウト (08-04)
        //=========================================================================

        struct RHIBindingSlot;
        struct RHIBindingSetDesc;
        struct RHIBindingLayoutDesc;
        class RHIBindingLayoutConverter;
        class RHIBindingSet;

        //=========================================================================
        // パイプラインステート関連 (07-01〜07-06)
        //=========================================================================

        class IRHIInputLayout;
        class IRHIPipelineStateCache;
        class RHIAsyncComputeHelper;
        struct RHIInputElementDesc;
        struct RHIInputLayoutDesc;
        struct RHIRenderTargetFormats;

        //=========================================================================
        // Variable Rate Shading (07-07)
        //=========================================================================

        struct RHIVRSCapabilities;
        struct RHIVRSImageDesc;

        //=========================================================================
        // シェーダー関連 (06-01〜06-04)
        //=========================================================================

        struct RHIShaderBytecode;
        struct RHIShaderModel;
        struct RHIShaderHash;
        struct RHIShaderCompileOptions;
        class IRHIShaderCache;
        struct RHIShaderLibraryDesc;
        struct RHIPermutationKey;
        struct RHIPermutationDimension;
        class RHIShaderPermutationSet;
        class RHIShaderManager;
        class IRHIShaderReflection;

        //=========================================================================
        // GPUプロファイラ (05-06)
        //=========================================================================

        class IRHIGPUProfiler;

        //=========================================================================
        // フェンス・同期 (09-01〜09-02)
        //=========================================================================

        struct RHIFenceDesc;
        class RHIFenceValueTracker;
        struct RHISyncPoint;
        class RHIFrameSync;
        class RHIPipelineSync;
        class RHISyncPointWaiter;
        class RHITimelineSync;

        //=========================================================================
        // GPUイベント (09-03)
        //=========================================================================

        struct RHIEventColor;
        class RHIScopedEvent;
        struct RHIBreadcrumbEntry;
        class RHIBreadcrumbBuffer;
        struct RHIGPUCrashInfo;
        struct RHIProfilerConfig;

        //=========================================================================
        // ディスクリプタヒープ (10-01)
        //=========================================================================

        struct RHIDescriptorHeapDesc;
        struct RHIDescriptorAllocation;
        class RHIDescriptorHeapAllocator;

        //=========================================================================
        // オンラインディスクリプタ (10-02)
        //=========================================================================

        class RHIOnlineDescriptorHeap;
        class RHIOnlineDescriptorManager;
        class RHIDescriptorStaging;

        //=========================================================================
        // オフラインディスクリプタ (10-03)
        //=========================================================================

        class RHIOfflineDescriptorHeap;
        class RHIOfflineDescriptorManager;
        struct RHIViewCacheKey;

        //=========================================================================
        // Bindless (10-04)
        //=========================================================================

        class RHIBindlessDescriptorHeap;
        class RHIBindlessSamplerHeap;
        struct RHIBindlessResourceInfo;
        class RHIBindlessResourceManager;

        //=========================================================================
        // ヒープ・メモリ型 (11-01)
        //=========================================================================

        struct RHIHeapDesc;
        class IRHIHeap;

        //=========================================================================
        // バッファアロケーター (11-02)
        //=========================================================================

        struct RHIBufferAllocation;
        class RHILinearBufferAllocator;
        class RHIRingBufferAllocator;
        struct RHIBufferPoolConfig;
        class RHIBufferPool;
        class RHIMultiSizeBufferPool;
        class RHIConstantBufferAllocator;
        class RHIDynamicBufferManager;

        //=========================================================================
        // テクスチャアロケーター (11-03)
        //=========================================================================

        struct RHITextureAllocation;
        struct RHITexturePoolConfig;
        class RHITexturePool;
        struct RHITransientTextureRequest;
        class RHITransientTextureAllocator;
        struct RHIRenderTargetKey;
        class RHIRenderTargetPool;
        struct RHIAtlasRegion;
        class RHITextureAtlasAllocator;

        //=========================================================================
        // アップロードヒープ (11-04)
        //=========================================================================

        struct RHIBufferUploadRequest;
        struct RHITextureUploadRequest;
        class RHIUploadHeap;
        class RHIUploadBatch;
        struct RHIAsyncUploadHandle;
        class RHIAsyncUploadManager;
        struct RHITextureLoadOptions;
        class RHITextureLoader;

        //=========================================================================
        // 常駐管理 (11-05)
        //=========================================================================

        class IRHIResidentResource;
        struct RHIResidencyManagerConfig;
        class RHIResidencyManager;
        class IRHIStreamingResource;
        class RHITextureStreamingManager;

        //=========================================================================
        // Reserved Resource (11-06)
        //=========================================================================

        struct RHIReservedResourceCapabilities;
        struct RHICommitResourceInfo;
        struct RHITextureCommitRegion;
        struct RHITextureTileInfo;

        //=========================================================================
        // プラットフォーム回避策 (11-07)
        //=========================================================================

        struct RHIPlatformWorkarounds;

        //=========================================================================
        // スワップチェーン (12-01〜12-03)
        //=========================================================================

        struct RHISwapChainDesc;
        struct RHIDisplayMode;
        struct RHIFullscreenDesc;
        struct RHIOutputInfo;
        struct RHISwapChainResizeDesc;
        struct RHIPresentParams;
        struct RHIFrameStatistics;
        class RHIMultiSwapChainPresenter;

        //=========================================================================
        // HDR (12-04)
        //=========================================================================

        struct RHIHDRMetadata;
        struct RHIColorPrimaries;
        struct RHIHDROutputCapabilities;
        struct RHIToneMappingSettings;
        class RHIHDRHelper;
        struct RHIAutoHDRSettings;

        //=========================================================================
        // デバイスロスト (17-02)
        //=========================================================================

        struct RHIDeviceLostInfo;
        class RHIDeviceLostHandler;
        struct RHIDeviceRecoveryOptions;
        class RHIDeviceRecoveryManager;

        //=========================================================================
        // 検証レイヤー (17-03)
        //=========================================================================

        struct RHIValidationMessage;
        struct RHIValidationConfig;
        struct RHIValidationStats;

        //=========================================================================
        // Breadcrumbs (17-04)
        //=========================================================================

        struct RHIBreadcrumbData;
        struct RHIBreadcrumbNode;
        class RHIBreadcrumbAllocator;
        class RHIBreadcrumbState;
        class RHIBreadcrumbScope;

        //=========================================================================
        // サンプラー (18-01〜18-02)
        //=========================================================================

        class RHISamplerBuilder;
        class RHISamplerCache;
        class RHISamplerManager;

        //=========================================================================
        // ステージングバッファ・リードバック (20-01〜20-04)
        //=========================================================================

        struct RHIStagingBufferDesc;
        class IRHIStagingBuffer;
        class RHIScopedStagingMap;
        class RHIStagingBufferDescBuilder;
        struct RHIBufferReadbackDesc;
        class IRHIBufferReadback;
        struct RHITextureReadbackDesc;
        class IRHITextureReadback;
        class RHIScreenCapture;
        class RHITextureDebugViewer;
        class RHIOcclusionQueryReadback;

        //=========================================================================
        // Indirect引数・コマンドシグネチャ (21-01〜21-05)
        //=========================================================================

        struct RHIDrawArguments;
        struct RHIDrawIndexedArguments;
        struct RHIDispatchArguments;
        struct RHIDispatchMeshArguments;
        struct RHIDispatchRaysArguments;
        struct RHIMultiDrawArguments;
        struct RHIIndirectArgument;
        struct RHICommandSignatureDesc;
        class IRHICommandSignature;
        class RHICommandSignatureBuilder;
        class RHIGPUDrivenBatch;
        class RHIMeshletGPURenderer;

        //=========================================================================
        // メッシュシェーダー (22-01〜22-04)
        //=========================================================================

        struct RHIMeshShaderCapabilities;
        class IRHIMeshShader;
        class IRHIAmplificationShader;
        struct RHIMeshlet;
        struct RHIMeshletBounds;
        struct RHIMeshletData;
        struct RHIAmplificationShaderDesc;
        struct RHIAmplificationDispatchInfo;
        class RHIAmplificationMeshPipeline;
        struct RHIMeshPipelineStateDesc;
        class IRHIMeshPipelineState;
        class RHIMeshPipelineStateBuilder;
        struct RHIMeshDispatchArgs;
        struct RHIMeshDispatchLimits;
        struct RHIMeshletBatch;
        class RHIMeshletDrawManager;

        //=========================================================================
        // Transientリソース (23-01〜23-04)
        //=========================================================================

        struct RHITransientResourceLifetime;
        struct RHITransientBufferDesc;
        struct RHITransientTextureDesc;
        class RHITransientBuffer;
        class RHITransientTexture;
        struct RHITransientAllocatorStats;
        struct RHITransientAllocationFences;
        class IRHITransientResourceAllocator;
        struct RHITransientAllocatorDesc;
        struct RHITransientBufferCreateInfo;
        class RHITransientBufferHandle;
        class RHITransientBufferPool;
        struct RHITransientTextureCreateInfo;
        class RHITransientTextureHandle;
        class RHIAliasingBarrierBatch;
        class RHIAliasingGroup;
        class RHIAliasingManager;

        //=========================================================================
        // Uniform/シェーダーパラメータ (24-01〜24-04)
        //=========================================================================

        struct RHIUniformElement;
        class RHIUniformBufferLayout;
        class RHIUniformBufferLayoutBuilder;
        struct RHIShaderParameterBinding;
        class RHIShaderParameterMap;
        class RHIMaterialParameterSet;
        struct RHIResourceTableEntry;
        class RHIResourceTable;
        class RHIMaterialResourceTable;
        class RHIGlobalResourceTable;
        class RHIBindlessResourceTable;
        struct RHIResourceCollectionMember;
        class RHIResourceCollection;
        struct RHIBoundShaderStateKey;
        struct RHIBoundShaderStateDesc;
        class RHIBoundShaderState;
        class RHIBoundShaderStateCache;
        class RHIBoundShaderStateBuilder;

        //=========================================================================
        // レイトレーシング (19-01〜19-03)
        //=========================================================================

        enum class ERHIRaytracingGeometryType : uint8;
        enum class ERHIRaytracingGeometryFlags : uint32;
        enum class ERHIRaytracingInstanceFlags : uint32;
        enum class ERHIRaytracingAccelerationStructureType : uint8;
        enum class ERHIRaytracingBuildFlags : uint32;
        enum class ERHIRaytracingCopyMode : uint8;
        enum class ERHIRaytracingTier : uint8;
        struct RHIRaytracingGeometryTrianglesDesc;
        struct RHIRaytracingGeometryAABBsDesc;
        struct RHIRaytracingGeometryDesc;
        struct RHIRaytracingInstanceDesc;
        struct RHIRaytracingAccelerationStructureBuildInputs;
        struct RHIRaytracingAccelerationStructurePrebuildInfo;
        struct RHIRaytracingAccelerationStructureDesc;
        class IRHIAccelerationStructure;
        struct RHIRaytracingCapabilities;
        struct RHIAccelerationStructureBuildDesc;
        struct RHIShaderIdentifier;
        struct RHIShaderRecord;
        struct RHIHitGroupDesc;
        struct RHIShaderTableRegion;
        struct RHIShaderBindingTableDesc;
        class IRHIShaderBindingTable;
        struct RHIDispatchRaysDesc;
        struct RHIDXILLibraryDesc;
        struct RHIRaytracingShaderConfig;
        struct RHIRaytracingPipelineConfig;
        struct RHILocalRootSignatureAssociation;
        struct RHIRaytracingPipelineStateDesc;
        class IRHIRaytracingPipelineState;
        class RHIRaytracingPipelineStateBuilder;

        //=========================================================================
        // マルチGPU (19-04)
        //=========================================================================

        using ERHIGPUNode = uint32;
        struct RHINodeAffinityMask;
        struct RHIMultiGPUCapabilities;
        struct RHICrossNodeResourceDesc;
        struct RHICrossNodeCopyDesc;
        struct RHICrossNodeFenceSync;
        class IRHIMultiGPUDevice;
        class RHIAlternateFrameRenderer;
        class RHISplitFrameRenderer;

        //=========================================================================
        // 統計 (25-01〜25-03)
        //=========================================================================

        struct RHIDrawCallStats;
        struct RHIStateChangeStats;
        struct RHIBindingStats;
        struct RHIBarrierStats;
        struct RHIMemoryOpStats;
        struct RHICommandListStats;
        class IRHICommandListStatsCollector;
        struct RHIFrameStats;
        class RHIFrameStatsTracker;
        struct RHICategoryMemoryStats;
        struct RHIHeapMemoryStats;
        struct RHIMemoryStats;
        struct RHIResourceMemoryInfo;
        class IRHIMemoryTracker;
        class RHIMemoryMonitor;
        struct RHIPSOInstanceStats;
        struct RHIPSOCacheStats;
        class IRHIPSOCacheTracker;
        struct RHIPSOWarmupProgress;
        class RHIPSOWarmupManager;

    } // namespace RHI
} // namespace NS
