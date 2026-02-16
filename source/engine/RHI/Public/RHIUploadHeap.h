/// @file RHIUploadHeap.h
/// @brief CPU→GPUアップロードヒープ管理
/// @details ステージングバッファ、バッチアップロード、非同期転送、テクスチャローダーを提供。
/// @see 11-04-upload-heap.md
#pragma once

#include "RHIBufferAllocator.h"
#include "RHISyncPoint.h"

namespace NS { namespace RHI {
    class IRHIDevice;
    class IRHICommandContext;
    class IRHIComputePipelineState;
    class IRHIQueue;

    //=========================================================================
    // RHIBufferUploadRequest (11-04)
    //=========================================================================

    /// バッファアップロードリクエスト
    struct RHI_API RHIBufferUploadRequest
    {
        IRHIBuffer* destBuffer = nullptr; ///< 転送先バッファ
        uint64 destOffset = 0;            ///< 転送先オフセット
        const void* srcData = nullptr;    ///< ソースデータ
        uint64 size = 0;                  ///< データサイズ
    };

    //=========================================================================
    // RHITextureUploadRequest (11-04)
    //=========================================================================

    /// テクスチャアップロードリクエスト
    struct RHI_API RHITextureUploadRequest
    {
        IRHITexture* destTexture = nullptr; ///< 転送先テクスチャ
        uint32 destSubresource = 0;         ///< サブリソース
        uint32 destX = 0;                   ///< X オフセット（ピクセル）
        uint32 destY = 0;                   ///< Y オフセット
        uint32 destZ = 0;                   ///< Z オフセット
        const void* srcData = nullptr;      ///< ソースデータ
        uint32 srcRowPitch = 0;             ///< 行ピッチ（バイト）
        uint32 srcSlicePitch = 0;           ///< スライスピッチ（バイト）
        uint32 width = 0;                   ///< 幅（ピクセル）
        uint32 height = 0;                  ///< 高さ
        uint32 depth = 1;                   ///< 奥行き
    };

    //=========================================================================
    // RHIUploadHeap (11-04)
    //=========================================================================

    /// アップロードヒープ
    /// CPU→GPUデータ転送のためのステージングバッファ管理
    class RHI_API RHIUploadHeap
    {
    public:
        RHIUploadHeap() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint64 size, uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        /// @param frameIndex フレームインデックス
        /// @param completedFrame GPU完了済みフェンス値
        void BeginFrame(uint32 frameIndex, uint64 completedFrame);

        /// フレーム終了
        /// @param frameNumber フェンス値（EndFrame後にSignalすること）
        void EndFrame(uint64 frameNumber);

        //=====================================================================
        // 即時アップロード
        //=====================================================================

        /// バッファアップロード（即時）
        bool UploadBuffer(IRHICommandContext* context, const RHIBufferUploadRequest& request);

        /// テクスチャアップロード（即時）
        bool UploadTexture(IRHICommandContext* context, const RHITextureUploadRequest& request);

        //=====================================================================
        // ステージング割り当て
        //=====================================================================

        /// ステージング領域割り当て
        RHIBufferAllocation AllocateStaging(uint64 size, uint64 alignment = 0);

        /// テクスチャ用ステージング結果
        struct TextureStagingAllocation
        {
            RHIBufferAllocation allocation;
            uint32 rowPitch;
            uint32 slicePitch;
        };

        /// テクスチャ用ステージング割り当て
        TextureStagingAllocation AllocateTextureStaging(uint32 width, uint32 height, ERHIPixelFormat format);

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetSize() const { return m_ringBuffer.GetTotalSize(); }
        uint64 GetUsedSize() const { return m_ringBuffer.GetUsedSize(); }
        IRHIBuffer* GetBuffer() const { return m_ringBuffer.GetBuffer(); }

    private:
        IRHIDevice* m_device = nullptr;
        RHIRingBufferAllocator m_ringBuffer;
    };

    //=========================================================================
    // RHIUploadBatch (11-04)
    //=========================================================================

    /// アップロードバッチ
    /// 複数のアップロードリクエストをまとめて処理
    class RHI_API RHIUploadBatch
    {
    public:
        RHIUploadBatch() = default;

        /// 初期化
        bool Initialize(RHIUploadHeap* uploadHeap, uint32 maxRequests = 256);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // リクエスト追加
        //=====================================================================

        bool AddBuffer(const RHIBufferUploadRequest& request);
        bool AddTexture(const RHITextureUploadRequest& request);
        void Clear();

        //=====================================================================
        // 実行
        //=====================================================================

        /// バッチ実行
        /// @return アップロードしたリソース数
        uint32 Execute(IRHICommandContext* context);

        //=====================================================================
        // 情報
        //=====================================================================

        uint32 GetRequestCount() const { return m_bufferCount + m_textureCount; }
        uint64 GetTotalDataSize() const { return m_totalDataSize; }

    private:
        RHIUploadHeap* m_uploadHeap = nullptr;

        RHIBufferUploadRequest* m_bufferRequests = nullptr;
        uint32 m_bufferCount = 0;

        RHITextureUploadRequest* m_textureRequests = nullptr;
        uint32 m_textureCount = 0;

        uint32 m_maxRequests = 0;
        uint64 m_totalDataSize = 0;
    };

    //=========================================================================
    // RHIAsyncUploadManager (11-04)
    //=========================================================================

    /// 非同期アップロードステータス
    enum class ERHIUploadStatus : uint8
    {
        Pending,    ///< 待機中
        InProgress, ///< 転送中
        Completed,  ///< 完了
        Failed,     ///< 失敗
    };

    /// 非同期アップロードハンドル
    struct RHI_API RHIAsyncUploadHandle
    {
        uint64 id = 0;

        bool IsValid() const { return id != 0; }
        static RHIAsyncUploadHandle Invalid() { return {}; }
    };

    /// 非同期アップロードマネージャー
    /// コピーキューを使用した非同期データ転送
    class RHI_API RHIAsyncUploadManager
    {
    public:
        RHIAsyncUploadManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint64 uploadHeapSize = 64 * 1024 * 1024);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // 非同期アップロード
        //=====================================================================

        RHIAsyncUploadHandle UploadBufferAsync(const RHIBufferUploadRequest& request);
        RHIAsyncUploadHandle UploadTextureAsync(const RHITextureUploadRequest& request);

        /// ステータス確認
        ERHIUploadStatus GetStatus(RHIAsyncUploadHandle handle) const;

        /// 完了待ち
        bool Wait(RHIAsyncUploadHandle handle, uint64 timeoutMs = UINT64_MAX);

        /// すべての待機中アップロードを完了待ち
        void WaitAll();

        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// グラフィックスキューでの同期ポイント取得
        RHISyncPoint GetSyncPoint() const;

        /// グラフィックスキューで待機
        void WaitOnGraphicsQueue(IRHIQueue* graphicsQueue);

    private:
        IRHIDevice* m_device = nullptr;
        IRHIQueue* m_copyQueue = nullptr;
        RHIUploadHeap m_uploadHeap;
        RHIFenceRef m_fence;
        uint64 m_nextFenceValue = 1;
        uint64 m_nextHandleId = 1;

        struct PendingUpload
        {
            RHIAsyncUploadHandle handle;
            uint64 fenceValue;
            ERHIUploadStatus status;
        };
        PendingUpload* m_pendingUploads = nullptr;
        uint32 m_pendingCount = 0;
        uint32 m_pendingCapacity = 0;
    };

    //=========================================================================
    // RHITextureLoader (11-04)
    //=========================================================================

    /// ミップ生成モード
    enum class ERHIMipGeneration : uint8
    {
        None,        ///< ミップマップなし
        Precomputed, ///< 事前計算済み（データに含まれる）
        Runtime,     ///< ランタイム生成
    };

    /// テクスチャロードオプション
    struct RHI_API RHITextureLoadOptions
    {
        ERHIMipGeneration mipGeneration = ERHIMipGeneration::None;
        bool sRGB = false;
        bool async = false;
        bool compress = false;
        const char* debugName = nullptr;
    };

    /// テクスチャローダー
    class RHI_API RHITextureLoader
    {
    public:
        RHITextureLoader() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, RHIAsyncUploadManager* uploadManager);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ロード
        //=====================================================================

        /// メモリからテクスチャ作成
        IRHITexture* LoadFromMemory(const void* data, uint64 dataSize, const RHITextureLoadOptions& options = {});

        /// 生データからテクスチャ作成
        IRHITexture* LoadFromRawData(const void* data,
                                     uint32 width,
                                     uint32 height,
                                     ERHIPixelFormat format,
                                     const RHITextureLoadOptions& options = {});

        /// 複数ミップレベルからテクスチャ作成
        IRHITexture* LoadFromMipData(const void* const* mipData,
                                     const uint32* mipRowPitches,
                                     uint32 mipCount,
                                     uint32 width,
                                     uint32 height,
                                     ERHIPixelFormat format,
                                     const RHITextureLoadOptions& options = {});

        //=====================================================================
        // ミップ生成
        //=====================================================================

        /// ミップ自動生成（コンピュートシェーダー使用）
        void GenerateMipmaps(IRHICommandContext* context, IRHITexture* texture);

    private:
        IRHIDevice* m_device = nullptr;
        RHIAsyncUploadManager* m_uploadManager = nullptr;
        RHIUploadHeap m_syncUploadHeap;
        IRHIComputePipelineState* m_mipGenPSO = nullptr;
    };

}} // namespace NS::RHI
