# 11-04: アップロードヒープ

## 目的

CPU→GPUデータ転送のためのアップロードヒープ管理を定義する。

## 参照ドキュメント

- 11-01-heap-types.md (ERHIHeapType::Upload)
- 11-02-buffer-allocator.md (リングバッファ)
- 02-01-command-context-base.md (IRHICommandContext)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIUploadHeap.h`

## TODO

### 1. アップロードリクエスト

```cpp
#pragma once

#include "RHITypes.h"
#include "IRHIBuffer.h"
#include "IRHITexture.h"

namespace NS::RHI
{
    /// バッファアップロードリクエスト
    struct RHI_API RHIBufferUploadRequest
    {
        /// 転送のバッファ
        IRHIBuffer* destBuffer = nullptr;

        /// 転送のオフセット
        uint64 destOffset = 0;

        /// ソースデータ
        const void* srcData = nullptr;

        /// データサイズ
        uint64 size = 0;
    };

    /// テクスチャアップロードリクエスト
    struct RHI_API RHITextureUploadRequest
    {
        /// 転送のテクスチャ
        IRHITexture* destTexture = nullptr;

        /// 転送のサブリソース
        uint32 destSubresource = 0;

        /// 転送のオフセット（ピクセル）。
        uint32 destX = 0;
        uint32 destY = 0;
        uint32 destZ = 0;

        /// ソースデータ
        const void* srcData = nullptr;

        /// 行ピッチ（バイト）
        uint32 srcRowPitch = 0;

        /// スライスピッチ（バイト）
        uint32 srcSlicePitch = 0;

        /// サイズ（ピクセル）。
        uint32 width = 0;
        uint32 height = 0;
        uint32 depth = 1;
    };
}
```

- [ ] RHIBufferUploadRequest 構造体
- [ ] RHITextureUploadRequest 構造体

### 2. アップロードヒープ

```cpp
namespace NS::RHI
{
    /// アップロードヒープ
    /// CPU→GPUデータ転送のためのステージングバッファ管理
    class RHI_API RHIUploadHeap
    {
    public:
        RHIUploadHeap() = default;

        /// 初期化
        /// @param device デバイス
        /// @param size ヒープサイズ
        /// @param numBufferedFrames バッファリングフレーム数
        bool Initialize(
            IRHIDevice* device,
            uint64 size,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        /// @param frameIndex 現在フレームインデックス（ダブル/トリプルバッファリング用）
        /// @param completedFrame GPU側で完了済みのフェンス値。
        ///        IRHIFence::GetCompletedValue()で取得した値を渡す。
        ///        この値以前のフレームで使用されたステージングメモリのみリサイクル可能。
        ///        フレームインデックスではなくフェンス値であることに注意。
        void BeginFrame(uint32 frameIndex, uint64 completedFrame);

        /// フレーム終了
        /// @param frameNumber このフレームに対応するフェンス値。
        ///        EndFrame後にIRHIQueue::Signal(fence, frameNumber)で発行すること。
        void EndFrame(uint64 frameNumber);

        //=====================================================================
        // 即時アップロード
        //=====================================================================

        /// バッファアップロード（即時）
        /// @param context コマンドコンテキスト
        /// @param request アップロードリクエスト
        bool UploadBuffer(
            IRHICommandContext* context,
            const RHIBufferUploadRequest& request);

        /// テクスチャアップロード（即時）
        bool UploadTexture(
            IRHICommandContext* context,
            const RHITextureUploadRequest& request);

        //=====================================================================
        // ステージング割り当て
        //=====================================================================

        /// ステージング領域割り当て
        /// @param size サイズ
        /// @param alignment アライメント
        /// @return 割り当て結果
        RHIBufferAllocation AllocateStaging(uint64 size, uint64 alignment = 0);

        /// テクスチャ用ステージング割り当て
        /// @param width 幅
        /// @param height 高さ
        /// @param format フォーマット
        /// @return 割り当て結果と行ピッチ
        struct TextureStagingAllocation {
            RHIBufferAllocation allocation;
            uint32 rowPitch;
            uint32 slicePitch;
        };
        TextureStagingAllocation AllocateTextureStaging(
            uint32 width,
            uint32 height,
            ERHIPixelFormat format);

        //=====================================================================
        // 情報
        //=====================================================================

        /// ヒープサイズ
        uint64 GetSize() const { return m_ringBuffer.GetTotalSize(); }

        /// 使用量
        uint64 GetUsedSize() const { return m_ringBuffer.GetUsedSize(); }

        /// アップロードバッファ取得
        IRHIBuffer* GetBuffer() const { return m_ringBuffer.GetBuffer(); }

    private:
        IRHIDevice* m_device = nullptr;
        RHIRingBufferAllocator m_ringBuffer;
    };
}
```

- [ ] RHIUploadHeap クラス
- [ ] TextureStagingAllocation 構造体

### 3. バッチアップロード

```cpp
namespace NS::RHI
{
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

        /// バッファアップロード追加
        bool AddBuffer(const RHIBufferUploadRequest& request);

        /// テクスチャアップロード追加
        bool AddTexture(const RHITextureUploadRequest& request);

        /// リクエストクリア
        void Clear();

        //=====================================================================
        // 実行
        //=====================================================================

        /// バッチ実行
        /// @param context コマンドコンテキスト
        /// @return アップロードしたリソース数
        uint32 Execute(IRHICommandContext* context);

        //=====================================================================
        // 情報
        //=====================================================================

        /// リクエスト数
        uint32 GetRequestCount() const { return m_bufferCount + m_textureCount; }

        /// 総データサイズ
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
}
```

- [ ] RHIUploadBatch クラス

### 4. 非同期アップロード

```cpp
namespace NS::RHI
{
    /// 非同期アップロードステータス
    enum class ERHIUploadStatus : uint8
    {
        Pending,        // 待機中
        InProgress,     // 転送中
        Completed,      // 完了
        Failed,         // 失敗
    };

    /// 非同期アップロードハンドル
    struct RHI_API RHIAsyncUploadHandle
    {
        uint64 id = 0;

        bool IsValid() const { return id != 0; }

        static RHIAsyncUploadHandle Invalid() { return {}; }
    };

    /// 非同期アップロードのマネージャー
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

        /// バッファ非同期アップロード
        RHIAsyncUploadHandle UploadBufferAsync(const RHIBufferUploadRequest& request);

        /// テクスチャ非同期アップロード
        RHIAsyncUploadHandle UploadTextureAsync(const RHITextureUploadRequest& request);

        /// ステータス確認
        ERHIUploadStatus GetStatus(RHIAsyncUploadHandle handle) const;

        /// 完了待ち。
        bool Wait(RHIAsyncUploadHandle handle, uint64 timeoutMs = UINT64_MAX);

        /// すべての待機中アップロードを完了待ち。
        void WaitAll();

        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// グラフィックスキューでの同期ポイント取得
        /// アップロード完了後グラフィックスで待機するため
        RHISyncPoint GetSyncPoint() const;

        /// グラフィックスキューで待つ。
        void WaitOnGraphicsQueue(IRHIQueue* graphicsQueue);

    private:
        IRHIDevice* m_device = nullptr;
        IRHIQueue* m_copyQueue = nullptr;
        RHIUploadHeap m_uploadHeap;
        RHIFenceRef m_fence;
        uint64 m_nextFenceValue = 1;
        uint64 m_nextHandleId = 1;

        struct PendingUpload {
            RHIAsyncUploadHandle handle;
            uint64 fenceValue;
            ERHIUploadStatus status;
        };
        PendingUpload* m_pendingUploads = nullptr;
        uint32 m_pendingCount = 0;
        uint32 m_pendingCapacity = 0;
    };
}
```

- [ ] ERHIUploadStatus 列挙型
- [ ] RHIAsyncUploadHandle 構造体
- [ ] RHIAsyncUploadManager クラス

### 5. テクスチャローダーヘルパー

```cpp
namespace NS::RHI
{
    /// ミップの自動生成のオプション
    enum class ERHIMipGeneration : uint8
    {
        None,           // ミップマップなし
        Precomputed,    // 事前計算済み（データに含まれる）。
        Runtime,        // ランタイム生成。
    };

    /// テクスチャロードオプション
    struct RHI_API RHITextureLoadOptions
    {
        /// ミップの自動生成。
        ERHIMipGeneration mipGeneration = ERHIMipGeneration::None;

        /// sRGBとして扱うか
        bool sRGB = false;

        /// 非同期ロード
        bool async = false;

        /// 圧縮フォーマットに変換
        bool compress = false;

        /// デバッグ名
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
        IRHITexture* LoadFromMemory(
            const void* data,
            uint64 dataSize,
            const RHITextureLoadOptions& options = {});

        /// 生データからテクスチャ作成
        IRHITexture* LoadFromRawData(
            const void* data,
            uint32 width,
            uint32 height,
            ERHIPixelFormat format,
            const RHITextureLoadOptions& options = {});

        /// 複数ミップレベルからテクスチャ作成
        IRHITexture* LoadFromMipData(
            const void* const* mipData,
            const uint32* mipRowPitches,
            uint32 mipCount,
            uint32 width,
            uint32 height,
            ERHIPixelFormat format,
            const RHITextureLoadOptions& options = {});

        //=====================================================================
        // ミップの自動生成。
        //=====================================================================

        /// ミップのオート生成（コンピュートシェーダー使用）。
        void GenerateMipmaps(
            IRHICommandContext* context,
            IRHITexture* texture);

    private:
        IRHIDevice* m_device = nullptr;
        RHIAsyncUploadManager* m_uploadManager = nullptr;
        RHIUploadHeap m_syncUploadHeap;

        // ミップの自動生成用コンピュートシェーダー
        IRHIComputePipelineState* m_mipGenPSO = nullptr;
    };
}
```

- [ ] ERHIMipGeneration 列挙型
- [ ] RHITextureLoadOptions 構造体
- [ ] RHITextureLoader クラス

## 枯渇時フォールバック

1. リングバッファ枯渇 → GPUフェンス待ち + フラッシュ後リトライ
2. 単一転送がヒープサイズ超過 → 分割転送（チャンク化）
3. システムメモリ不足 → RHIMemoryPressureHandler通知

## 検証方法

- [ ] バッファアップロード
- [ ] テクスチャアップロード
- [ ] バッチアップロード
- [ ] 非同期アップロード
