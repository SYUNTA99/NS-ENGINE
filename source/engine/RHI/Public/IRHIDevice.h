/// @file IRHIDevice.h
/// @brief 論理デバイスインターフェース
/// @details GPU機能へのアクセスとリソース管理を提供。
/// @see 01-20-device-core.md
#pragma once

#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHITypes.h"

namespace NS::RHI
{
    /// 前方宣言
    struct RHIDeviceLostInfo;
    struct RHIValidationConfig;
    struct RHIValidationStats;
    enum class ERHIValidationLevel : uint8;
    class RHIDeferredDeleteQueue;
    class IRHIDescriptorHeapManager;
    class IRHIOnlineDescriptorManager;
    class IRHIOfflineDescriptorManager;
    class IRHISamplerHeap;
    class IRHIBufferAllocator;
    class IRHITextureAllocator;
    class IRHIFastAllocator;
    class IRHIResidencyManager;
    class IRHIUploadHeap;
    class IRHIPipelineState;

    //=========================================================================
    // ERHIHeapType
    //=========================================================================

    /// ヒープタイプ
    enum class ERHIHeapType : uint8
    {
        Default,  // GPU専用（VRAM、最高速）
        Upload,   // CPU書き込み→GPU読み取り
        Readback, // GPU書き込み→CPU読み取り
        Custom,   // カスタム（細かい制御用）
    };

    //=========================================================================
    // RHIResourceAllocationInfo
    //=========================================================================

    /// リソース割り当て情報
    struct RHIResourceAllocationInfo
    {
        uint64 size = 0;      ///< 必要なメモリサイズ（バイト）
        uint64 alignment = 0; ///< アライメント要件（バイト）
    };

    //=========================================================================
    // RHIMemoryStats
    //=========================================================================

    /// メモリ統計
    struct RHIMemoryStats
    {
        uint64 allocatedDefault = 0;
        uint64 allocatedUpload = 0;
        uint64 allocatedReadback = 0;
        uint64 usedDefault = 0;
        uint64 usedUpload = 0;
        uint64 usedReadback = 0;
        uint64 textureMemory = 0;
        uint64 bufferMemory = 0;

        uint64 GetTotalAllocated() const { return allocatedDefault + allocatedUpload + allocatedReadback; }
        uint64 GetTotalUsed() const { return usedDefault + usedUpload + usedReadback; }
    };

    //=========================================================================
    // RHIMemoryBudget
    //=========================================================================

    /// メモリバジェット情報
    struct RHIMemoryBudget
    {
        uint64 budget = 0;       ///< 使用可能なメモリ量
        uint64 currentUsage = 0; ///< 現在の使用量

        /// 利用可能な残り
        uint64 GetAvailable() const { return budget > currentUsage ? budget - currentUsage : 0; }

        /// 使用率（0.0〜1.0）
        float GetUsageRatio() const { return budget > 0 ? static_cast<float>(currentUsage) / budget : 0.0f; }
    };

    //=========================================================================
    // RHIFormatSupport
    //=========================================================================

    /// フォーマットサポート情報
    struct RHIFormatSupport
    {
        bool buffer = false;
        bool texture = false;
        bool renderTarget = false;
        bool depthStencil = false;
        bool unorderedAccess = false;
        bool mipMapGeneration = false;
        bool multisample = false;
        ERHISampleCount maxSampleCount = ERHISampleCount::Count1;
    };

    //=========================================================================
    // IRHIDevice
    //=========================================================================

    /// 論理デバイスインターフェース
    /// GPU機能へのアクセスとリソース管理を提供
    class RHI_API IRHIDevice
    {
    public:
        virtual ~IRHIDevice() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 親アダプター取得
        virtual IRHIAdapter* GetAdapter() const = 0;

        /// このデバイスのGPUマスク取得
        virtual GPUMask GetGPUMask() const = 0;

        /// GPUインデックス取得（マルチGPU時）
        virtual uint32 GetGPUIndex() const = 0;

        /// プライマリデバイスか（GPU0）
        bool IsPrimaryDevice() const { return GetGPUIndex() == 0; }

        //=====================================================================
        // デバイス情報
        //=====================================================================

        /// タイムスタンプ周波数取得（Hz）
        virtual uint64 GetTimestampFrequency() const = 0;

        /// 現在のメモリバジェットを取得
        virtual RHIMemoryBudget GetMemoryBudget() const = 0;

        /// 定数バッファアライメント取得
        virtual uint32 GetConstantBufferAlignment() const = 0;

        /// テクスチャデータアライメント取得
        virtual uint32 GetTextureDataAlignment() const = 0;

        /// デバイスが有効か
        virtual bool IsValid() const = 0;

        /// デバイスロスト状態か
        virtual bool IsDeviceLost() const = 0;

        //=====================================================================
        // デフォルトビュー（Nullビュー）
        //=====================================================================

        /// Null SRV取得
        virtual IRHIShaderResourceView* GetNullSRV() const = 0;

        /// Null UAV取得
        virtual IRHIUnorderedAccessView* GetNullUAV() const = 0;

        /// Null CBV取得
        virtual IRHIConstantBufferView* GetNullCBV() const = 0;

        /// Null サンプラー取得
        virtual IRHISampler* GetNullSampler() const = 0;

        //=====================================================================
        // フォーマットサポートクエリ
        //=====================================================================

        /// フォーマットサポート情報取得
        virtual RHIFormatSupport GetFormatSupport(ERHIPixelFormat format) const = 0;

        /// レンダーターゲットとして使用可能か
        bool SupportsRenderTarget(ERHIPixelFormat format) const { return GetFormatSupport(format).renderTarget; }

        /// デプスステンシルとして使用可能か
        bool SupportsDepthStencil(ERHIPixelFormat format) const { return GetFormatSupport(format).depthStencil; }

        /// UAVとして使用可能か
        bool SupportsUAV(ERHIPixelFormat format) const { return GetFormatSupport(format).unorderedAccess; }

        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// GPU同期のコマンド完了待ち
        virtual void WaitIdle() = 0;

        /// 遅延削除キュー取得
        virtual RHIDeferredDeleteQueue* GetDeferredDeleteQueue() = 0;

        /// デバッグ名設定
        virtual void SetDebugName(const char* name) = 0;

        /// 現在のフレーム番号取得
        virtual uint64 GetCurrentFrameNumber() const = 0;

        //=====================================================================
        // キュー管理 (01-21)
        //=====================================================================

        /// 指定タイプのキュー数取得
        virtual uint32 GetQueueCount(ERHIQueueType type) const = 0;

        /// キュー取得
        virtual IRHIQueue* GetQueue(ERHIQueueType type, uint32 index = 0) const = 0;

        /// グラフィックスキュー取得（プライマリ）
        IRHIQueue* GetGraphicsQueue() const { return GetQueue(ERHIQueueType::Graphics); }

        /// 非同期コンピュートキュー取得
        IRHIQueue* GetComputeQueue() const
        {
            IRHIQueue* queue = GetQueue(ERHIQueueType::Compute);
            return queue ? queue : GetGraphicsQueue();
        }

        /// コピーキュー取得
        IRHIQueue* GetCopyQueue() const
        {
            IRHIQueue* queue = GetQueue(ERHIQueueType::Copy);
            return queue ? queue : GetGraphicsQueue();
        }

        /// 非同期コンピュートキューがあるか
        bool HasAsyncComputeQueue() const { return GetQueueCount(ERHIQueueType::Compute) > 0; }

        /// 専用コピーキューがあるか
        bool HasCopyQueue() const { return GetQueueCount(ERHIQueueType::Copy) > 0; }

        /// 指定キュータイプが利用可能か
        bool IsQueueTypeAvailable(ERHIQueueType type) const { return GetQueueCount(type) > 0; }

        /// キュー同期
        virtual void SignalQueue(IRHIQueue* queue, IRHIFence* fence, uint64 value) = 0;
        virtual void WaitQueue(IRHIQueue* queue, IRHIFence* fence, uint64 value) = 0;
        virtual void FlushQueue(IRHIQueue* queue) = 0;
        virtual void FlushAllQueues() = 0;

        /// キュー間バリア挿入
        virtual void InsertQueueBarrier(IRHIQueue* srcQueue, IRHIQueue* dstQueue) = 0;

        //=====================================================================
        // コンテキスト管理 (01-22)
        //=====================================================================

        /// 即時コンテキスト取得
        virtual IRHICommandContext* GetImmediateContext() = 0;

        /// コンテキスト取得（プールから）
        virtual IRHICommandContext* ObtainContext(ERHIQueueType queueType) = 0;

        /// コンピュートコンテキスト取得
        virtual IRHIComputeContext* ObtainComputeContext() = 0;

        /// コンテキスト解放（プールに返却）
        virtual void ReleaseContext(IRHICommandContext* context) = 0;

        /// コンピュートコンテキスト解放
        virtual void ReleaseContext(IRHIComputeContext* context) = 0;

        /// コマンドアロケーター取得（プールから）
        virtual IRHICommandAllocator* ObtainCommandAllocator(ERHIQueueType queueType) = 0;

        /// コマンドアロケーター解放
        virtual void ReleaseCommandAllocator(IRHICommandAllocator* allocator, IRHIFence* fence, uint64 fenceValue) = 0;

        /// コマンドアロケーターを即座に解放
        virtual void ReleaseCommandAllocatorImmediate(IRHICommandAllocator* allocator) = 0;

        /// コマンドリスト取得（プールから）
        virtual IRHICommandList* ObtainCommandList(IRHICommandAllocator* allocator,
                                                   IRHIPipelineState* pipelineState = nullptr) = 0;

        /// コマンドリスト解放
        virtual void ReleaseCommandList(IRHICommandList* commandList) = 0;

        /// コンテキストを終了しコマンドリスト化
        virtual IRHICommandList* FinalizeContext(IRHICommandContext* context) = 0;

        /// コンピュートコンテキストを終了
        virtual IRHICommandList* FinalizeContext(IRHIComputeContext* context) = 0;

        /// コンテキストをリセット
        virtual void ResetContext(IRHICommandContext* context, IRHICommandAllocator* allocator) = 0;

        /// コンピュートコンテキストをリセット
        virtual void ResetContext(IRHIComputeContext* context, IRHICommandAllocator* allocator) = 0;

        //=====================================================================
        // ディスクリプタヒープ作成 (10-01)
        //=====================================================================

        /// ディスクリプタヒープ作成
        virtual IRHIDescriptorHeap* CreateDescriptorHeap(const RHIDescriptorHeapDesc& desc,
                                                         const char* debugName = nullptr) = 0;

        /// 最大ディスクリプタ数取得
        virtual uint32 GetMaxDescriptorCount(ERHIDescriptorHeapType type) const = 0;

        //=====================================================================
        // ディスクリプタ管理 (01-23)
        //=====================================================================

        /// ディスクリプタヒープマネージャー取得
        virtual IRHIDescriptorHeapManager* GetDescriptorHeapManager() = 0;

        /// オンラインディスクリプタマネージャー取得
        virtual IRHIOnlineDescriptorManager* GetOnlineDescriptorManager() = 0;

        /// オンラインディスクリプタを割り当て
        virtual RHIDescriptorHandle AllocateOnlineDescriptors(ERHIDescriptorHeapType heapType, uint32 count) = 0;

        /// オフラインディスクリプタマネージャー取得
        virtual IRHIOfflineDescriptorManager* GetOfflineDescriptorManager(ERHIDescriptorHeapType heapType) = 0;

        /// オフラインディスクリプタを割り当て
        virtual RHICPUDescriptorHandle AllocateOfflineDescriptor(ERHIDescriptorHeapType heapType) = 0;

        /// オフラインディスクリプタを解放
        virtual void FreeOfflineDescriptor(ERHIDescriptorHeapType heapType, RHICPUDescriptorHandle handle) = 0;

        /// グローバルサンプラーヒープ取得
        virtual IRHISamplerHeap* GetGlobalSamplerHeap() = 0;

        /// サンプラーディスクリプタを割り当て
        virtual RHIDescriptorHandle AllocateSamplerDescriptor(IRHISampler* sampler) = 0;

        /// ディスクリプタコピー
        virtual void CopyDescriptor(RHICPUDescriptorHandle dstHandle,
                                    RHICPUDescriptorHandle srcHandle,
                                    ERHIDescriptorHeapType heapType) = 0;

        /// ディスクリプタ複数コピー
        virtual void CopyDescriptors(RHICPUDescriptorHandle dstStart,
                                     RHICPUDescriptorHandle srcStart,
                                     uint32 count,
                                     ERHIDescriptorHeapType heapType) = 0;

        /// ディスクリプタインクリメントサイズ取得
        virtual uint32 GetDescriptorIncrementSize(ERHIDescriptorHeapType heapType) const = 0;

        /// Bindlessがサポートされているか
        virtual bool SupportsBindless() const = 0;

        /// BindlessSRVインデックスを割り当て
        virtual BindlessSRVIndex AllocateBindlessSRV(IRHIShaderResourceView* view) = 0;

        /// BindlessUAVインデックスを割り当て
        virtual BindlessUAVIndex AllocateBindlessUAV(IRHIUnorderedAccessView* view) = 0;

        /// Bindlessインデックスを解放
        virtual void FreeBindlessSRV(BindlessSRVIndex index) = 0;
        virtual void FreeBindlessUAV(BindlessUAVIndex index) = 0;

        //=====================================================================
        // メモリ管理 (01-24)
        //=====================================================================

        /// デフォルトバッファアロケーター取得
        virtual IRHIBufferAllocator* GetDefaultBufferAllocator() = 0;

        /// 指定ヒープタイプのバッファアロケーター取得
        virtual IRHIBufferAllocator* GetBufferAllocator(ERHIHeapType heapType) = 0;

        /// テクスチャアロケーター取得
        virtual IRHITextureAllocator* GetTextureAllocator() = 0;

        /// テクスチャメモリ要件取得
        virtual RHIResourceAllocationInfo GetTextureAllocationInfo(const RHITextureDesc& desc) const = 0;

        /// バッファメモリ要件取得
        virtual RHIResourceAllocationInfo GetBufferAllocationInfo(const RHIBufferDesc& desc) const = 0;

        /// 高速アロケーター取得
        virtual IRHIFastAllocator* GetFastAllocator() = 0;

        /// アップロードヒープ取得
        virtual IRHIUploadHeap* GetUploadHeap() = 0;

        /// フレーム終了時のアロケーターリセット
        virtual void ResetFrameAllocators() = 0;

        /// Residencyマネージャー取得
        virtual IRHIResidencyManager* GetResidencyManager() = 0;

        /// リソースを常駐状態にする
        virtual void MakeResident(IRHIResource* const* resources, uint32 count) = 0;

        /// リソースを非常駐状態にする
        virtual void Evict(IRHIResource* const* resources, uint32 count) = 0;

        /// メモリ圧迫時のコールバック設定
        using MemoryPressureCallback = void (*)(uint64 bytesNeeded);
        virtual void SetMemoryPressureCallback(MemoryPressureCallback callback) = 0;

        /// 現在のメモリ使用量取得
        virtual RHIMemoryStats GetMemoryStats() const = 0;

        //=====================================================================
        // ワークグラフ (02-06)
        //=====================================================================

        /// ワークグラフパイプライン作成
        virtual IRHIWorkGraphPipeline* CreateWorkGraphPipeline(const RHIWorkGraphPipelineDesc& desc) = 0;

        /// ワークグラフサポート確認
        virtual bool SupportsWorkGraphs() const = 0;

        /// バッキングメモリ要件取得
        virtual RHIWorkGraphMemoryRequirements GetWorkGraphMemoryRequirements(
            IRHIWorkGraphPipeline* pipeline) const = 0;

        //=====================================================================
        // コマンドシグネチャ作成 (21-04)
        //=====================================================================

        /// コマンドシグネチャ作成
        virtual IRHICommandSignature* CreateCommandSignature(const RHICommandSignatureDesc& desc,
                                                             const char* debugName = nullptr) = 0;

        //=====================================================================
        // リソースビュー作成 (05-01〜05-05)
        //=====================================================================

        /// バッファSRV作成
        virtual IRHIShaderResourceView* CreateShaderResourceView(const RHIBufferSRVDesc& desc,
                                                                 const char* debugName = nullptr) = 0;

        /// テクスチャSRV作成
        virtual IRHIShaderResourceView* CreateShaderResourceView(const RHITextureSRVDesc& desc,
                                                                 const char* debugName = nullptr) = 0;

        /// バッファUAV作成
        virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(const RHIBufferUAVDesc& desc,
                                                                   const char* debugName = nullptr) = 0;

        /// テクスチャUAV作成
        virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(const RHITextureUAVDesc& desc,
                                                                   const char* debugName = nullptr) = 0;

        /// レンダーターゲットビュー作成
        virtual IRHIRenderTargetView* CreateRenderTargetView(const RHIRenderTargetViewDesc& desc,
                                                             const char* debugName = nullptr) = 0;

        /// デプスステンシルビュー作成
        virtual IRHIDepthStencilView* CreateDepthStencilView(const RHIDepthStencilViewDesc& desc,
                                                             const char* debugName = nullptr) = 0;

        /// 定数バッファビュー作成
        virtual IRHIConstantBufferView* CreateConstantBufferView(const RHIConstantBufferViewDesc& desc,
                                                                 const char* debugName = nullptr) = 0;

        //=====================================================================
        // ビュー作成便利関数 (05-01〜05-05)
        // ※ 実装は IRHIViews.h の型定義に依存するため非インライン
        //=====================================================================

        /// テクスチャからデフォルトSRV作成
        IRHIShaderResourceView* CreateDefaultSRV(IRHITexture* texture, const char* debugName = nullptr);

        /// バッファからデフォルトSRV作成
        IRHIShaderResourceView* CreateDefaultSRV(IRHIBuffer* buffer, const char* debugName = nullptr);

        /// バッファからデフォルトUAV作成
        IRHIUnorderedAccessView* CreateDefaultUAV(IRHIBuffer* buffer, const char* debugName = nullptr);

        /// テクスチャからデフォルトUAV作成
        IRHIUnorderedAccessView* CreateDefaultUAV(IRHITexture* texture,
                                                  uint32 mipSlice = 0,
                                                  const char* debugName = nullptr);

        /// テクスチャからデフォルトRTV作成
        IRHIRenderTargetView* CreateDefaultRTV(IRHITexture* texture, const char* debugName = nullptr);

        /// テクスチャからデフォルトDSV作成
        IRHIDepthStencilView* CreateDefaultDSV(IRHITexture* texture, const char* debugName = nullptr);

        /// 読み取り専用DSV作成
        IRHIDepthStencilView* CreateReadOnlyDSV(IRHITexture* texture, const char* debugName = nullptr);

        /// バッファからデフォルトCBV作成
        IRHIConstantBufferView* CreateDefaultCBV(IRHIBuffer* buffer, const char* debugName = nullptr);

        //=====================================================================
        // Nullビュー（型指定版） (05-01〜05-04)
        //=====================================================================

        /// Null バッファSRV取得
        virtual IRHIShaderResourceView* GetNullBufferSRV(
            ERHIBufferSRVFormat format = ERHIBufferSRVFormat::Structured) = 0;

        /// Null テクスチャSRV取得
        virtual IRHIShaderResourceView* GetNullTextureSRV(
            ERHITextureDimension dimension = ERHITextureDimension::Texture2D) = 0;

        /// Null バッファUAV取得
        virtual IRHIUnorderedAccessView* GetNullBufferUAV() = 0;

        /// Null テクスチャUAV取得
        virtual IRHIUnorderedAccessView* GetNullTextureUAV(
            ERHITextureDimension dimension = ERHITextureDimension::Texture2D) = 0;

        /// Null RTV取得
        virtual IRHIRenderTargetView* GetNullRTV() = 0;

        /// Null DSV取得
        virtual IRHIDepthStencilView* GetNullDSV() = 0;

        //=====================================================================
        // シェーダー作成 (06-02)
        //=====================================================================

        /// シェーダー作成
        virtual IRHIShader* CreateShader(const RHIShaderDesc& desc, const char* debugName = nullptr) = 0;

        /// 頂点シェーダー作成（便利関数）
        /// ※ RHIShaderDesc に依存するため非インライン
        IRHIShader* CreateVertexShader(const RHIShaderBytecode& bytecode,
                                       const char* entryPoint = "VSMain",
                                       const char* debugName = nullptr);

        /// ピクセルシェーダー作成（便利関数）
        IRHIShader* CreatePixelShader(const RHIShaderBytecode& bytecode,
                                      const char* entryPoint = "PSMain",
                                      const char* debugName = nullptr);

        /// コンピュートシェーダー作成（便利関数）
        IRHIShader* CreateComputeShader(const RHIShaderBytecode& bytecode,
                                        const char* entryPoint = "CSMain",
                                        const char* debugName = nullptr);

        //=====================================================================
        // シェーダーライブラリ作成 (06-04)
        //=====================================================================

        /// シェーダーライブラリ作成
        virtual IRHIShaderLibrary* CreateShaderLibrary(const RHIShaderLibraryDesc& desc,
                                                       const char* debugName = nullptr) = 0;

        //=====================================================================
        // ルートシグネチャ作成 (08-03)
        //=====================================================================

        /// ルートシグネチャ作成
        virtual IRHIRootSignature* CreateRootSignature(const RHIRootSignatureDesc& desc,
                                                       const char* debugName = nullptr) = 0;

        /// シリアライズ済みBlobからルートシグネチャ作成
        virtual IRHIRootSignature* CreateRootSignatureFromBlob(const RHIShaderBytecode& blob,
                                                               const char* debugName = nullptr) = 0;

        //=====================================================================
        // 入力レイアウト作成 (07-04)
        //=====================================================================

        /// 入力レイアウト作成
        virtual IRHIInputLayout* CreateInputLayout(const RHIInputLayoutDesc& desc,
                                                   const RHIShaderBytecode& vsBytecode,
                                                   const char* debugName = nullptr) = 0;

        //=====================================================================
        // パイプラインステート作成 (07-05, 07-06)
        //=====================================================================

        /// グラフィックスパイプラインステート作成
        virtual IRHIGraphicsPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc,
                                                                       const char* debugName = nullptr) = 0;

        /// キャッシュからグラフィックスPSO作成
        virtual IRHIGraphicsPipelineState* CreateGraphicsPipelineStateFromCache(
            const RHIGraphicsPipelineStateDesc& desc,
            const RHIShaderBytecode& cachedBlob,
            const char* debugName = nullptr) = 0;

        /// コンピュートパイプラインステート作成
        virtual IRHIComputePipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc,
                                                                     const char* debugName = nullptr) = 0;

        /// キャッシュからコンピュートPSO作成
        virtual IRHIComputePipelineState* CreateComputePipelineStateFromCache(const RHIComputePipelineStateDesc& desc,
                                                                              const RHIShaderBytecode& cachedBlob,
                                                                              const char* debugName = nullptr) = 0;

        /// コンピュートPSO作成（便利関数）
        /// ※ RHIComputePipelineStateDesc に依存するため非インライン
        IRHIComputePipelineState* CreateComputePipelineState(IRHIShader* computeShader,
                                                             IRHIRootSignature* rootSignature = nullptr,
                                                             const char* debugName = nullptr);

        /// PSOキャッシュ作成
        virtual IRHIPipelineStateCache* CreatePipelineStateCache() = 0;

        //=====================================================================
        // メッシュシェーダーパイプライン (22-03)
        //=====================================================================

        /// メッシュパイプラインステート作成
        virtual IRHIMeshPipelineState* CreateMeshPipelineState(const RHIMeshPipelineStateDesc& desc,
                                                               const char* debugName = nullptr) = 0;

        /// メッシュシェーダーケイパビリティ取得
        virtual RHIMeshShaderCapabilities GetMeshShaderCapabilities() const = 0;

        //=====================================================================
        // Transientリソースアロケーター (23-01)
        //=====================================================================

        /// Transientリソースアロケーター作成
        virtual IRHITransientResourceAllocator* CreateTransientAllocator(const RHITransientAllocatorDesc& desc) = 0;

        //=====================================================================
        // Variable Rate Shading (07-07)
        //=====================================================================

        /// VRSケイパビリティ取得
        virtual RHIVRSCapabilities GetVRSCapabilities() const = 0;

        /// VRSイメージ作成
        virtual IRHITexture* CreateVRSImage(const RHIVRSImageDesc& desc) = 0;

        //=====================================================================
        // サンプラー作成 (18-02)
        //=====================================================================

        /// サンプラー作成
        virtual IRHISampler* CreateSampler(const RHISamplerDesc& desc, const char* debugName = nullptr) = 0;

        //=====================================================================
        // ステージングバッファ・リードバック (20-01〜20-04)
        //=====================================================================

        /// ステージングバッファ作成
        virtual IRHIStagingBuffer* CreateStagingBuffer(const RHIStagingBufferDesc& desc) = 0;

        /// バッファリードバック作成
        virtual IRHIBufferReadback* CreateBufferReadback(const RHIBufferReadbackDesc& desc) = 0;

        /// テクスチャリードバック作成
        virtual IRHITextureReadback* CreateTextureReadback(const RHITextureReadbackDesc& desc) = 0;

        //=====================================================================
        // GPUプロファイラ (05-06)
        //=====================================================================

        /// GPUプロファイラ取得
        virtual IRHIGPUProfiler* GetGPUProfiler() = 0;

        /// プロファイラが利用可能か
        virtual bool IsGPUProfilingSupported() const = 0;

        //=====================================================================
        // フェンス作成 (09-01)
        //=====================================================================

        /// フェンス作成
        virtual IRHIFence* CreateFence(const RHIFenceDesc& desc, const char* debugName = nullptr) = 0;

        /// 初期値指定でフェンス作成（便利関数）
        IRHIFence* CreateFence(uint64 initialValue = 0, const char* debugName = nullptr);

        /// 共有フェンスを開く
        virtual IRHIFence* OpenSharedFence(void* sharedHandle, const char* debugName = nullptr) = 0;

        //=====================================================================
        // デバイスロスト診断 (09-03)
        //=====================================================================

        /// デバイスロストコールバック設定
        virtual void SetDeviceLostCallback(RHIDeviceLostCallback callback, void* userData = nullptr) = 0;

        /// GPUクラッシュ情報を取得（デバイスロスト後に呼び出す）
        virtual bool GetGPUCrashInfo(RHIGPUCrashInfo& outInfo) = 0;

        /// ブレッドクラムバッファ設定
        virtual void SetBreadcrumbBuffer(RHIBreadcrumbBuffer* buffer) = 0;

        //=====================================================================
        // プロファイラ統合 (09-03)
        //=====================================================================

        /// プロファイラ設定
        virtual void ConfigureProfiler(const RHIProfilerConfig& config) = 0;

        /// キャプチャ開始
        virtual void BeginCapture(const char* captureName = nullptr) = 0;

        /// キャプチャ終了
        virtual void EndCapture() = 0;

        /// キャプチャ中か
        virtual bool IsCapturing() const = 0;

        /// 利用可能なプロファイラ取得
        virtual ERHIProfilerType GetAvailableProfiler() const = 0;

        //=====================================================================
        // Reserved Resource (11-06)
        //=====================================================================

        /// Reserved Resourceケイパビリティ取得
        virtual RHIReservedResourceCapabilities GetReservedResourceCapabilities() const = 0;

        /// テクスチャタイル情報取得
        virtual RHITextureTileInfo GetTextureTileInfo(const RHITextureDesc& desc) const = 0;

        //=====================================================================
        // スワップチェーン作成 (12-02)
        //=====================================================================

        /// スワップチェーン作成
        virtual IRHISwapChain* CreateSwapChain(const RHISwapChainDesc& desc,
                                               IRHIQueue* presentQueue,
                                               const char* debugName = nullptr) = 0;

        //=====================================================================
        // クエリ (14-02)
        //=====================================================================

        /// クエリヒープ作成
        virtual IRHIQueryHeap* CreateQueryHeap(const RHIQueryHeapDesc& desc, const char* debugName = nullptr) = 0;

        /// クエリデータ取得（CPU側）
        virtual bool GetQueryData(IRHIQueryHeap* queryHeap,
                                  uint32 startIndex,
                                  uint32 queryCount,
                                  void* destData,
                                  uint32 destStride,
                                  ERHIQueryFlags flags = ERHIQueryFlags::None) = 0;

        /// GPUタイムスタンプキャリブレーション取得
        virtual bool GetTimestampCalibration(uint64& outGPUTimestamp, uint64& outCPUTimestamp) const = 0;

        //=====================================================================
        // デバイスロスト診断 (17-02)
        //=====================================================================

        /// デバイスロスト情報取得
        virtual bool GetDeviceLostInfo(RHIDeviceLostInfo& outInfo) const = 0;

        //=====================================================================
        // フォーマット詳細サポート (15-02)
        //=====================================================================

        /// 詳細フォーマットサポートフラグ取得
        virtual ERHIFormatSupportFlags GetFormatSupportFlags(ERHIPixelFormat format) const = 0;

        /// MSAAサポート情報取得
        virtual RHIMSAASupportInfo GetMSAASupport(ERHIPixelFormat format, bool renderTarget = true) const = 0;

        //=====================================================================
        // プラットフォームフォーマット変換 (15-03)
        //=====================================================================

        /// ネイティブフォーマットに変換（D3D12: DXGI_FORMAT, Vulkan: VkFormat等）
        virtual uint32 ConvertToNativeFormat(ERHIPixelFormat format) const = 0;

        /// ネイティブフォーマットから変換
        virtual ERHIPixelFormat ConvertFromNativeFormat(uint32 nativeFormat) const = 0;

        //=====================================================================
        // 検証レイヤー (17-03)
        //=====================================================================

        /// 検証設定
        virtual void ConfigureValidation(const RHIValidationConfig& config) = 0;

        /// 検証レベル取得
        virtual ERHIValidationLevel GetValidationLevel() const = 0;

        /// 検証が有効か
        bool IsValidationEnabled() const { return GetValidationLevel() != ERHIValidationLevel::Disabled; }

        /// メッセージ抑制追加
        virtual void SuppressValidationMessage(uint32 messageId) = 0;

        /// メッセージ抑制解除
        virtual void UnsuppressValidationMessage(uint32 messageId) = 0;

        /// 検証統計取得
        virtual RHIValidationStats GetValidationStats() const = 0;

        /// 検証統計リセット
        virtual void ResetValidationStats() = 0;

        //=====================================================================
        // レイトレーシング (19-01〜19-03)
        //=====================================================================

        /// 加速構造作成
        virtual IRHIAccelerationStructure* CreateAccelerationStructure(
            const RHIRaytracingAccelerationStructureDesc& desc,
            const char* debugName = nullptr) = 0;

        /// 加速構造プレビルド情報取得
        virtual RHIRaytracingAccelerationStructurePrebuildInfo GetAccelerationStructurePrebuildInfo(
            const RHIRaytracingAccelerationStructureBuildInputs& inputs) const = 0;

        /// レイトレーシングケイパビリティ取得
        virtual RHIRaytracingCapabilities GetRaytracingCapabilities() const = 0;

        /// シェーダーバインディングテーブル作成
        virtual IRHIShaderBindingTable* CreateShaderBindingTable(const RHIShaderBindingTableDesc& desc,
                                                                  const char* debugName = nullptr) = 0;

        /// レイトレーシングPSO作成
        virtual IRHIRaytracingPipelineState* CreateRaytracingPipelineState(
            const RHIRaytracingPipelineStateDesc& desc,
            const char* debugName = nullptr) = 0;

        //=====================================================================
        // マルチGPU (19-04)
        //=====================================================================

        /// マルチGPUケイパビリティ取得
        virtual RHIMultiGPUCapabilities GetMultiGPUCapabilities() const = 0;

        /// GPUノード数取得
        virtual uint32 GetNodeCount() const = 0;
    };

} // namespace NS::RHI
