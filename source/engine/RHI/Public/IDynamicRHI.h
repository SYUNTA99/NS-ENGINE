/// @file IDynamicRHI.h
/// @brief RHI最上位インターフェース
/// @details プラットフォーム固有RHI（D3D12, Vulkan等）の基底。
///          ライフサイクル管理、アダプター/デバイスアクセス、機能クエリを提供。
/// @see 01-15-dynamic-rhi-core.md
#pragma once

#include "RHIFwd.h"
#include "RHIEnums.h"
#include "RHITypes.h"
#include "RHICheck.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIFeature: 機能フラグ
    //=========================================================================

    /// 機能フラグ
    enum class ERHIFeature : uint32
    {
        // シェーダー機能
        WaveOperations,
        RayTracing,
        MeshShaders,
        VariableRateShading,
        AmplificationShaders,
        ShaderModel6_6,
        ShaderModel6_7,

        // テクスチャ機能
        TextureCompressionBC,
        TextureCompressionASTC,
        Texture3D,
        MSAA_16X,
        SamplerFeedback,

        // バッファ機能
        StructuredBuffer,
        ByteAddressBuffer,
        TypedBuffer,

        // レンダリング機能
        Bindless,
        ConservativeRasterization,
        MultiDrawIndirect,
        DrawIndirectCount,
        RenderPass,
        DepthBoundsTest,

        // 高度な機能
        WorkGraphs,
        EnhancedBarriers,
        GPUUploadHeaps,
        ExecuteIndirect,
        AtomicInt64,
        Residency,

        Count
    };

    //=========================================================================
    // IDynamicRHI
    //=========================================================================

    /// RHI最上位インターフェース
    /// プラットフォーム固有RHI（D3D12, Vulkan等）の基底
    class RHI_API IDynamicRHI
    {
    public:
        virtual ~IDynamicRHI() = default;

        //=====================================================================
        // 識別
        //=====================================================================

        /// RHIバックエンド種別取得
        virtual ERHIInterfaceType GetInterfaceType() const = 0;

        /// RHI名取得（"D3D12", "Vulkan"等）
        virtual const char* GetName() const = 0;

        /// 現在の機能レベル取得
        virtual ERHIFeatureLevel GetFeatureLevel() const = 0;

        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// 初期化（アダプター/デバイス/キューを作成）
        virtual bool Init() = 0;

        /// 追加初期化（Init後、レンダリング開始前）
        virtual void PostInit() {}

        /// シャットダウン（全リソース解放、デバイス破棄）
        virtual void Shutdown() = 0;

        /// 毎フレーム更新
        virtual void Tick(float deltaTime) { (void)deltaTime; }

        /// フレーム終了処理
        virtual void EndFrame() {}

        /// RHIが有効か
        virtual bool IsInitialized() const = 0;

        //=====================================================================
        // アダプター/デバイスアクセス
        //=====================================================================

        /// 利用可能なアダプター数
        virtual uint32 GetAdapterCount() const = 0;

        /// アダプター取得
        virtual IRHIAdapter* GetAdapter(uint32 index) const = 0;

        /// 現在使用中のアダプター取得
        virtual IRHIAdapter* GetCurrentAdapter() const = 0;

        /// デフォルトデバイス取得
        virtual IRHIDevice* GetDefaultDevice() const = 0;

        /// GPUマスクからデバイス取得
        virtual IRHIDevice* GetDevice(GPUMask gpuMask) const = 0;

        //=====================================================================
        // 機能クエリ
        //=====================================================================

        /// 機能サポート状態取得
        virtual ERHIFeatureSupport GetFeatureSupport(ERHIFeature feature) const = 0;

        /// 拡張サポート確認
        virtual bool SupportsExtension(const char* extensionName) const = 0;

        //=====================================================================
        // 制限値取得
        //=====================================================================

        /// 最大テクスチャサイズ
        virtual uint32 GetMaxTextureSize() const = 0;

        /// 最大テクスチャ配列サイズ
        virtual uint32 GetMaxTextureArrayLayers() const = 0;

        /// 最大バッファサイズ
        virtual uint64 GetMaxBufferSize() const = 0;

        /// 最大コンスタントバッファサイズ
        virtual uint32 GetMaxConstantBufferSize() const = 0;

        /// 最大サンプルカウント
        virtual ERHISampleCount GetMaxSampleCount() const = 0;

        //=====================================================================
        // リソースファクトリ
        //=====================================================================

        /// バッファ作成
        virtual TRefCountPtr<IRHIBuffer> CreateBuffer(const RHIBufferDesc& desc,
                                                      const void* initialData = nullptr) = 0;

        /// テクスチャ作成
        virtual TRefCountPtr<IRHITexture> CreateTexture(const RHITextureDesc& desc) = 0;

        /// テクスチャ作成（初期データ付き）
        virtual TRefCountPtr<IRHITexture> CreateTextureWithData(
            const RHITextureDesc& desc, const RHISubresourceData* initialData,
            uint32 numSubresources) = 0;

        /// シェーダーリソースビュー作成
        virtual TRefCountPtr<IRHIShaderResourceView> CreateShaderResourceView(
            IRHIResource* resource, const RHISRVDesc& desc) = 0;

        /// アンオーダードアクセスビュー作成
        virtual TRefCountPtr<IRHIUnorderedAccessView> CreateUnorderedAccessView(
            IRHIResource* resource, const RHIUAVDesc& desc) = 0;

        /// レンダーターゲットビュー作成
        virtual TRefCountPtr<IRHIRenderTargetView> CreateRenderTargetView(
            IRHITexture* texture, const RHIRTVDesc& desc) = 0;

        /// デプスステンシルビュー作成
        virtual TRefCountPtr<IRHIDepthStencilView> CreateDepthStencilView(
            IRHITexture* texture, const RHIDSVDesc& desc) = 0;

        /// 定数バッファビュー作成
        virtual TRefCountPtr<IRHIConstantBufferView> CreateConstantBufferView(
            IRHIBuffer* buffer, const RHICBVDesc& desc) = 0;

        /// シェーダー作成
        virtual TRefCountPtr<IRHIShader> CreateShader(const RHIShaderDesc& desc) = 0;

        /// グラフィックスパイプラインステート作成
        virtual TRefCountPtr<IRHIGraphicsPipelineState> CreateGraphicsPipelineState(
            const RHIGraphicsPipelineStateDesc& desc) = 0;

        /// コンピュートパイプラインステート作成
        virtual TRefCountPtr<IRHIComputePipelineState> CreateComputePipelineState(
            const RHIComputePipelineStateDesc& desc) = 0;

        /// ルートシグネチャ作成
        virtual TRefCountPtr<IRHIRootSignature> CreateRootSignature(
            const RHIRootSignatureDesc& desc) = 0;

        /// サンプラー作成
        virtual TRefCountPtr<IRHISampler> CreateSampler(const RHISamplerDesc& desc) = 0;

        /// フェンス作成
        virtual TRefCountPtr<IRHIFence> CreateFence(uint64 initialValue = 0) = 0;

        /// スワップチェーン作成
        virtual TRefCountPtr<IRHISwapChain> CreateSwapChain(const RHISwapChainDesc& desc,
                                                            void* windowHandle) = 0;

        /// クエリヒープ作成
        virtual TRefCountPtr<IRHIQueryHeap> CreateQueryHeap(const RHIQueryHeapDesc& desc) = 0;

        /// ディスクリプタヒープ作成
        virtual TRefCountPtr<IRHIDescriptorHeap> CreateDescriptorHeap(
            const RHIDescriptorHeapDesc& desc) = 0;

        //=====================================================================
        // コマンドコンテキスト取得
        //=====================================================================

        /// デフォルトコンテキスト取得
        virtual IRHICommandContext* GetDefaultContext() = 0;

        /// パイプライン別コンテキスト取得
        virtual IRHICommandContext* GetCommandContext(ERHIPipeline pipeline) = 0;

        /// コンピュートコンテキスト取得（AsyncCompute用）
        virtual IRHIComputeContext* GetComputeContext() = 0;

        //=====================================================================
        // コマンドリスト管理
        //=====================================================================

        /// コマンドアロケーター取得
        virtual IRHICommandAllocator* ObtainCommandAllocator(ERHIQueueType queueType) = 0;

        /// コマンドアロケーター解放
        virtual void ReleaseCommandAllocator(IRHICommandAllocator* allocator, IRHIFence* fence,
                                             uint64 fenceValue) = 0;

        /// コマンドリスト取得
        virtual IRHICommandList* ObtainCommandList(IRHICommandAllocator* allocator) = 0;

        /// コマンドリスト解放
        virtual void ReleaseCommandList(IRHICommandList* commandList) = 0;

        //=====================================================================
        // コンテキスト終了
        //=====================================================================

        /// コンテキストを終了しコマンドリスト化
        virtual IRHICommandList* FinalizeContext(IRHICommandContext* context) = 0;

        /// コンテキストをリセット（再利用準備）
        virtual void ResetContext(IRHICommandContext* context) = 0;

        //=====================================================================
        // コマンド送信
        //=====================================================================

        /// コマンドリストをGPUに送信
        virtual void SubmitCommandLists(ERHIQueueType queueType,
                                        IRHICommandList* const* commandLists, uint32 count) = 0;

        /// 単一コマンドリスト送信
        void SubmitCommandList(ERHIQueueType queueType, IRHICommandList* commandList)
        {
            SubmitCommandLists(queueType, &commandList, 1);
        }

        /// 全コマンドの完了を待つ
        virtual void FlushCommands() = 0;

        /// 指定キューのコマンド完了を待つ
        virtual void FlushQueue(ERHIQueueType queueType) = 0;

        //=====================================================================
        // GPU同期
        //=====================================================================

        /// フェンスシグナル
        virtual void SignalFence(IRHIQueue* queue, IRHIFence* fence, uint64 value) = 0;

        /// フェンス待ち（GPU側）
        virtual void WaitFence(IRHIQueue* queue, IRHIFence* fence, uint64 value) = 0;

        /// フェンス待ち（CPU側）
        virtual bool WaitForFence(IRHIFence* fence, uint64 value,
                                  uint64 timeoutMs = UINT64_MAX) = 0;

        //=====================================================================
        // フレーム同期
        //=====================================================================

        /// フレーム開始
        virtual void BeginFrame() {}

        /// 現在のフレーム番号取得
        virtual uint64 GetCurrentFrameNumber() const = 0;
    };

    //=========================================================================
    // グローバルRHIインスタンス
    //=========================================================================

    /// グローバルRHIポインタ
    extern RHI_API IDynamicRHI* g_dynamicRHI;

    /// グローバルRHI取得
    inline IDynamicRHI* GetDynamicRHI()
    {
        RHI_CHECK(g_dynamicRHI != nullptr);
        return g_dynamicRHI;
    }

    /// RHIが利用可能か
    inline bool IsRHIAvailable() { return g_dynamicRHI != nullptr && g_dynamicRHI->IsInitialized(); }

    /// デフォルトデバイス取得ショートカット
    inline IRHIDevice* GetRHIDevice() { return GetDynamicRHI()->GetDefaultDevice(); }

}} // namespace NS::RHI
