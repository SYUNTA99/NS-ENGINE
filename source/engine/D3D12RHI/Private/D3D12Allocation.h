/// @file D3D12Allocation.h
/// @brief D3D12 メモリアロケータ（Buddy + Pool + Transient）
#pragma once

#include "D3D12RHIPrivate.h"

#include "RHI/Public/RHITransientAllocator.h"

#include <mutex>
#include <shared_mutex>
#include <vector>

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12AllocationBlock — 割り当て結果
    //=========================================================================

    struct D3D12AllocationBlock
    {
        /// ヒープ内オフセット
        uint64 offset = 0;

        /// 割り当てサイズ
        uint64 size = 0;

        /// ヒープインデックス（アロケータ内）
        uint32 heapIndex = 0;

        /// Buddyアロケータのオーダー（Pool時は0）
        uint32 order = 0;

        bool IsValid() const { return size > 0; }
    };

    //=========================================================================
    // D3D12BuddyAllocator — 2のべき乗分割アロケータ
    //=========================================================================

    /// 単一ID3D12Heap上のBuddyアロケータ。
    /// 2のべき乗ブロック分割 + 隣接フリーブロック再帰マージ。
    class D3D12BuddyAllocator
    {
    public:
        /// D3D12 Placed Resource最小サイズ
        static constexpr uint64 kMinBlockSize = 64 * 1024; // 64KB

        D3D12BuddyAllocator() = default;
        ~D3D12BuddyAllocator();

        /// 初期化
        /// @param device D3D12デバイス
        /// @param heapType ヒープタイプ（DEFAULT/UPLOAD/READBACK）
        /// @param heapSize ヒープ総サイズ（2のべき乗必須）
        bool Init(ID3D12Device* device, D3D12_HEAP_TYPE heapType, uint64 heapSize);

        /// ブロック割り当て
        /// @return true=成功
        bool Allocate(uint64 size, uint64 alignment, D3D12AllocationBlock& outBlock);

        /// ブロック解放（遅延キューへ）
        void Deallocate(const D3D12AllocationBlock& block, uint64 fenceValue);

        /// 遅延解放処理（フェンス完了分のみ）
        void ProcessDeferredFrees(uint64 completedFenceValue);

        /// ヒープ取得
        ID3D12Heap* GetHeap() const { return heap_.Get(); }

        /// 使用量
        uint64 GetUsedSize() const { return usedSize_; }
        uint64 GetTotalSize() const { return heapSize_; }
        bool IsEmpty() const { return usedSize_ == 0; }

    private:
        ComPtr<ID3D12Heap> heap_;
        uint64 heapSize_ = 0;
        uint64 usedSize_ = 0;
        uint32 maxOrder_ = 0;

        /// フリーブロックリスト（オーダー別、オフセットの集合）
        std::vector<std::vector<uint64>> freeBlocks_;

        struct DeferredFree
        {
            uint64 offset;
            uint32 order;
            uint64 fenceValue;
        };
        std::vector<DeferredFree> deferredFrees_;

        mutable std::shared_mutex mutex_;

        /// オーダー計算（sizeに必要な最小オーダー）
        uint32 SizeToOrder(uint64 size) const;

        /// オーダーからブロックサイズ
        uint64 OrderToSize(uint32 order) const;

        /// バディオフセット取得
        uint64 GetBuddyOffset(uint64 offset, uint32 order) const;

        /// フリーブロック検索・除去
        bool RemoveFreeBlock(uint32 order, uint64 offset);

        /// ブロック解放（内部、マージ処理付き）
        void FreeBlock(uint64 offset, uint32 order);
    };

    //=========================================================================
    // D3D12PoolAllocator — 固定サイズブロックプール
    //=========================================================================

    /// 固定サイズブロックのフリーリスト管理。
    /// 容量不足時はヒープ追加。
    class D3D12PoolAllocator
    {
    public:
        D3D12PoolAllocator() = default;
        ~D3D12PoolAllocator() = default;

        /// 初期化
        /// @param device D3D12デバイス
        /// @param heapType ヒープタイプ
        /// @param blockSize 1ブロックのサイズ
        /// @param blocksPerHeap 1ヒープあたりのブロック数
        bool Init(ID3D12Device* device, D3D12_HEAP_TYPE heapType, uint64 blockSize, uint32 blocksPerHeap);

        /// ブロック割り当て
        bool Allocate(D3D12AllocationBlock& outBlock);

        /// ブロック解放（遅延キューへ）
        void Deallocate(const D3D12AllocationBlock& block, uint64 fenceValue);

        /// 遅延解放処理
        void ProcessDeferredFrees(uint64 completedFenceValue);

        /// ヒープ取得
        ID3D12Heap* GetHeap(uint32 heapIndex) const;

        /// ブロックサイズ
        uint64 GetBlockSize() const { return blockSize_; }

    private:
        struct PoolHeap
        {
            ComPtr<ID3D12Heap> heap;
            std::vector<uint32> freeList;
            uint32 allocatedCount = 0;
        };

        ID3D12Device* device_ = nullptr;
        D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
        uint64 blockSize_ = 0;
        uint32 blocksPerHeap_ = 0;

        std::vector<PoolHeap> heaps_;

        struct DeferredFree
        {
            uint32 heapIndex;
            uint32 blockIndex;
            uint64 fenceValue;
        };
        std::vector<DeferredFree> deferredFrees_;

        std::mutex mutex_;

        /// 新規ヒープ追加
        bool AddHeap();
    };

    //=========================================================================
    // D3D12TransientLinearAllocator — フレーム単位リニアアロケータ
    //=========================================================================

    /// 単一ID3D12Heap上のバンプポインタアロケータ。
    /// フレーム毎にリセットし、Placed Resourceに利用。
    class D3D12TransientLinearAllocator
    {
    public:
        D3D12TransientLinearAllocator() = default;
        ~D3D12TransientLinearAllocator();

        bool Init(ID3D12Device* device, D3D12_HEAP_TYPE heapType, uint64 heapSize,
                  D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);

        /// アライメント対応割り当て（バンプポインタ）
        bool Allocate(uint64 size, uint64 alignment, D3D12AllocationBlock& outBlock);

        /// フレーム開始時にリセット
        void Reset();

        ID3D12Heap* GetHeap() const { return heap_.Get(); }
        uint64 GetUsedSize() const { return currentOffset_; }
        uint64 GetTotalSize() const { return heapSize_; }
        bool IsEmpty() const { return currentOffset_ == 0; }

    private:
        ComPtr<ID3D12Heap> heap_;
        uint64 heapSize_ = 0;
        uint64 currentOffset_ = 0;
        std::mutex mutex_;
    };

    //=========================================================================
    // D3D12UploadRingBuffer — Upload Heapリングバッファ
    //=========================================================================

    /// GPU完了フェンス追跡による安全なアップロードメモリ再利用。
    /// 定数バッファ・動的頂点データ等のフレーム単位アップロードに使用。
    class D3D12UploadRingBuffer
    {
    public:
        struct Allocation
        {
            uint64 offset = 0;
            uint64 size = 0;
            void* cpuAddress = nullptr;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
            bool IsValid() const { return size > 0; }
        };

        D3D12UploadRingBuffer() = default;
        ~D3D12UploadRingBuffer();

        bool Init(ID3D12Device* device, uint64 bufferSize);
        void Shutdown();

        /// フレーム開始: 完了済みフェンス以前のメモリを解放
        void BeginFrame(uint64 completedFenceValue);
        /// フレーム終了: 現フレームの使用範囲を記録
        void EndFrame(uint64 fenceValue);

        /// リングバッファから割り当て
        Allocation Allocate(uint64 size, uint64 alignment = 256);

        ID3D12Resource* GetResource() const { return resource_.Get(); }
        uint64 GetTotalSize() const { return bufferSize_; }
        uint64 GetUsedSize() const;

    private:
        ComPtr<ID3D12Resource> resource_;
        uint64 bufferSize_ = 0;
        uint64 head_ = 0;
        uint64 tail_ = 0;
        void* mappedPtr_ = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuBaseAddress_ = 0;

        struct FrameRecord
        {
            uint64 endOffset;
            uint64 fenceValue;
        };
        std::vector<FrameRecord> frameRecords_;
        uint64 frameStartOffset_ = 0;

        std::mutex mutex_;
    };

    //=========================================================================
    // D3D12TransientResourceAllocator — IRHITransientResourceAllocator実装
    //=========================================================================

    class D3D12Device;

    /// Committed Resourceキャッシュによるフレーム間再利用。
    /// 同一サイズ・Usageのリソースを自動的に再利用しメモリ使用量を抑える。
    class D3D12TransientResourceAllocator : public NS::RHI::IRHITransientResourceAllocator
    {
    public:
        D3D12TransientResourceAllocator() = default;
        ~D3D12TransientResourceAllocator() override;

        bool Init(D3D12Device* device, const NS::RHI::RHITransientAllocatorDesc& desc);

        // IRHITransientResourceAllocator
        void BeginFrame() override;
        void EndFrame() override;

        NS::RHI::RHITransientBuffer AllocateBuffer(const NS::RHI::RHITransientBufferDesc& desc) override;
        NS::RHI::RHITransientTexture AllocateTexture(const NS::RHI::RHITransientTextureDesc& desc) override;

        void AcquireResources(NS::RHI::IRHICommandContext* context, uint32 passIndex) override;
        void ReleaseResources(NS::RHI::IRHICommandContext* context, uint32 passIndex) override;

        void AcquireResourcesForPipeline(NS::RHI::IRHICommandContext* context, uint32 passIndex,
                                         NS::RHI::ERHIPipeline pipeline) override;
        void ReleaseResourcesForPipeline(NS::RHI::IRHICommandContext* context, uint32 passIndex,
                                         NS::RHI::ERHIPipeline pipeline) override;

        void SetAllocationFences(const NS::RHI::RHITransientAllocationFences& fences) override;
        void SetAsyncComputeBudget(NS::RHI::ERHIAsyncComputeBudget budget) override;

        NS::RHI::RHITransientAllocatorStats GetStats() const override;
        void DumpMemoryUsage() const override;

    protected:
        NS::RHI::IRHIBuffer* GetBufferInternal(uint32 handle) const override;
        NS::RHI::IRHITexture* GetTextureInternal(uint32 handle) const override;

    private:
        D3D12Device* device_ = nullptr;
        uint64 maxHeapSize_ = 0;
        bool allowGrowth_ = true;

        /// バッファキャッシュ: サイズ+usage→再利用可能バッファ
        struct CachedBuffer
        {
            NS::RHI::IRHIBuffer* buffer = nullptr;
            uint64 size = 0;
            NS::RHI::ERHIBufferUsage usage = NS::RHI::ERHIBufferUsage::Default;
            bool inUse = false;
        };
        std::vector<CachedBuffer> bufferCache_;

        /// テクスチャキャッシュ
        struct CachedTexture
        {
            NS::RHI::IRHITexture* texture = nullptr;
            uint32 width = 0;
            uint32 height = 0;
            uint32 depth = 0;
            NS::RHI::ERHIPixelFormat format = NS::RHI::ERHIPixelFormat::Unknown;
            NS::RHI::ERHITextureUsage usage = NS::RHI::ERHITextureUsage::Default;
            bool inUse = false;
        };
        std::vector<CachedTexture> textureCache_;

        /// ハンドルインデックス→リソース（フレーム内有効）
        std::vector<NS::RHI::IRHIBuffer*> bufferHandles_;
        std::vector<NS::RHI::IRHITexture*> textureHandles_;

        NS::RHI::RHITransientAllocatorStats stats_ = {};
        NS::RHI::RHITransientAllocationFences fences_ = {};

        std::mutex mutex_;
    };

} // namespace NS::D3D12RHI
