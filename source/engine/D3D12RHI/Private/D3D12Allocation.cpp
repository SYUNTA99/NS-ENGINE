/// @file D3D12Allocation.cpp
/// @brief D3D12 メモリアロケータ実装（Buddy + Pool + Transient）
#include "D3D12Allocation.h"

#include "D3D12Buffer.h"
#include "D3D12Device.h"
#include "D3D12Texture.h"

#include <algorithm>
#include <bit>

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12BuddyAllocator
    //=========================================================================

    D3D12BuddyAllocator::~D3D12BuddyAllocator()
    {
        heap_.Reset();
    }

    bool D3D12BuddyAllocator::Init(ID3D12Device* device, D3D12_HEAP_TYPE heapType, uint64 heapSize)
    {
        if (!device || heapSize == 0)
            return false;

        // ヒープサイズを2のべき乗に切り上げ
        heapSize = std::bit_ceil(heapSize);
        heapSize_ = heapSize;

        // 最大オーダー計算
        maxOrder_ = 0;
        {
            uint64 s = heapSize;
            while (s > kMinBlockSize)
            {
                s >>= 1;
                ++maxOrder_;
            }
        }

        // フリーブロックリスト初期化（全オーダー分）
        freeBlocks_.resize(maxOrder_ + 1);
        // 最大オーダー（ヒープ全体）を1ブロック追加
        freeBlocks_[maxOrder_].push_back(0);

        // ヒープ作成
        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = heapSize;
        heapDesc.Properties.Type = heapType;
        heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        HRESULT hr = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12BuddyAllocator] CreateHeap failed");
            return false;
        }

        return true;
    }

    uint32 D3D12BuddyAllocator::SizeToOrder(uint64 size) const
    {
        // 最小ブロックサイズ以上に切り上げ
        if (size <= kMinBlockSize)
            return 0;

        // 2のべき乗に切り上げてオーダー計算
        uint64 aligned = std::bit_ceil(size);
        uint32 order = 0;
        uint64 blockSize = kMinBlockSize;
        while (blockSize < aligned)
        {
            blockSize <<= 1;
            ++order;
        }
        return order;
    }

    uint64 D3D12BuddyAllocator::OrderToSize(uint32 order) const
    {
        return kMinBlockSize << order;
    }

    uint64 D3D12BuddyAllocator::GetBuddyOffset(uint64 offset, uint32 order) const
    {
        // バディ = offset XOR blockSize
        return offset ^ OrderToSize(order);
    }

    bool D3D12BuddyAllocator::RemoveFreeBlock(uint32 order, uint64 offset)
    {
        auto& blocks = freeBlocks_[order];
        auto it = std::find(blocks.begin(), blocks.end(), offset);
        if (it != blocks.end())
        {
            blocks.erase(it);
            return true;
        }
        return false;
    }

    bool D3D12BuddyAllocator::Allocate(uint64 size, uint64 alignment, D3D12AllocationBlock& outBlock)
    {
        std::unique_lock lock(mutex_);

        // アライメント対応: サイズをアライメント以上に切り上げ
        if (alignment > kMinBlockSize)
            size = std::max(size, alignment);

        uint32 order = SizeToOrder(size);
        if (order > maxOrder_)
            return false;

        // 要求オーダー以上のフリーブロック検索
        uint32 foundOrder = order;
        while (foundOrder <= maxOrder_ && freeBlocks_[foundOrder].empty())
            ++foundOrder;

        if (foundOrder > maxOrder_)
            return false; // 空き無し

        // フリーブロック取得
        uint64 offset = freeBlocks_[foundOrder].back();
        freeBlocks_[foundOrder].pop_back();

        // 必要に応じて分割
        while (foundOrder > order)
        {
            --foundOrder;
            // 上半分をフリーリストに追加
            uint64 buddyOffset = offset + OrderToSize(foundOrder);
            freeBlocks_[foundOrder].push_back(buddyOffset);
        }

        outBlock.offset = offset;
        outBlock.size = OrderToSize(order);
        outBlock.order = order;
        outBlock.heapIndex = 0;

        usedSize_ += outBlock.size;
        return true;
    }

    void D3D12BuddyAllocator::Deallocate(const D3D12AllocationBlock& block, uint64 fenceValue)
    {
        std::unique_lock lock(mutex_);

        DeferredFree df;
        df.offset = block.offset;
        df.order = block.order;
        df.fenceValue = fenceValue;
        deferredFrees_.push_back(df);
    }

    void D3D12BuddyAllocator::FreeBlock(uint64 offset, uint32 order)
    {
        usedSize_ -= OrderToSize(order);

        // バディマージ（再帰的に上位オーダーへ）
        while (order < maxOrder_)
        {
            uint64 buddyOffset = GetBuddyOffset(offset, order);

            // バディがフリーか確認
            if (!RemoveFreeBlock(order, buddyOffset))
                break; // バディは使用中 — マージ不可

            // マージ: 小さい方のオフセットで上位オーダーに
            offset = std::min(offset, buddyOffset);
            ++order;
        }

        freeBlocks_[order].push_back(offset);
    }

    void D3D12BuddyAllocator::ProcessDeferredFrees(uint64 completedFenceValue)
    {
        std::unique_lock lock(mutex_);

        auto it = deferredFrees_.begin();
        while (it != deferredFrees_.end())
        {
            if (it->fenceValue <= completedFenceValue)
            {
                FreeBlock(it->offset, it->order);
                it = deferredFrees_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    //=========================================================================
    // D3D12PoolAllocator
    //=========================================================================

    bool D3D12PoolAllocator::Init(ID3D12Device* device,
                                  D3D12_HEAP_TYPE heapType,
                                  uint64 blockSize,
                                  uint32 blocksPerHeap)
    {
        if (!device || blockSize == 0 || blocksPerHeap == 0)
            return false;

        device_ = device;
        heapType_ = heapType;
        blockSize_ = std::max(blockSize, static_cast<uint64>(D3D12BuddyAllocator::kMinBlockSize));
        blocksPerHeap_ = blocksPerHeap;

        return true;
    }

    bool D3D12PoolAllocator::AddHeap()
    {
        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = blockSize_ * blocksPerHeap_;
        heapDesc.Properties.Type = heapType_;
        heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        PoolHeap ph;
        HRESULT hr = device_->CreateHeap(&heapDesc, IID_PPV_ARGS(&ph.heap));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12PoolAllocator] CreateHeap failed");
            return false;
        }

        // フリーリスト初期化（全ブロックフリー）
        ph.freeList.reserve(blocksPerHeap_);
        for (uint32 i = blocksPerHeap_; i > 0; --i)
            ph.freeList.push_back(i - 1);

        heaps_.push_back(std::move(ph));
        return true;
    }

    bool D3D12PoolAllocator::Allocate(D3D12AllocationBlock& outBlock)
    {
        std::lock_guard lock(mutex_);

        // フリーブロックがあるヒープを検索
        for (uint32 i = 0; i < static_cast<uint32>(heaps_.size()); ++i)
        {
            auto& ph = heaps_[i];
            if (!ph.freeList.empty())
            {
                uint32 blockIndex = ph.freeList.back();
                ph.freeList.pop_back();
                ++ph.allocatedCount;

                outBlock.offset = static_cast<uint64>(blockIndex) * blockSize_;
                outBlock.size = blockSize_;
                outBlock.heapIndex = i;
                outBlock.order = 0;
                return true;
            }
        }

        // 全ヒープ満杯 — 新規ヒープ追加
        if (!AddHeap())
            return false;

        auto& ph = heaps_.back();
        uint32 blockIndex = ph.freeList.back();
        ph.freeList.pop_back();
        ++ph.allocatedCount;

        outBlock.offset = static_cast<uint64>(blockIndex) * blockSize_;
        outBlock.size = blockSize_;
        outBlock.heapIndex = static_cast<uint32>(heaps_.size() - 1);
        outBlock.order = 0;
        return true;
    }

    void D3D12PoolAllocator::Deallocate(const D3D12AllocationBlock& block, uint64 fenceValue)
    {
        std::lock_guard lock(mutex_);

        DeferredFree df;
        df.heapIndex = block.heapIndex;
        df.blockIndex = static_cast<uint32>(block.offset / blockSize_);
        df.fenceValue = fenceValue;
        deferredFrees_.push_back(df);
    }

    void D3D12PoolAllocator::ProcessDeferredFrees(uint64 completedFenceValue)
    {
        std::lock_guard lock(mutex_);

        auto it = deferredFrees_.begin();
        while (it != deferredFrees_.end())
        {
            if (it->fenceValue <= completedFenceValue)
            {
                if (it->heapIndex < heaps_.size())
                {
                    auto& ph = heaps_[it->heapIndex];
                    ph.freeList.push_back(it->blockIndex);
                    if (ph.allocatedCount > 0)
                        --ph.allocatedCount;
                }
                it = deferredFrees_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    ID3D12Heap* D3D12PoolAllocator::GetHeap(uint32 heapIndex) const
    {
        if (heapIndex < heaps_.size())
            return heaps_[heapIndex].heap.Get();
        return nullptr;
    }

    //=========================================================================
    // D3D12TransientLinearAllocator
    //=========================================================================

    D3D12TransientLinearAllocator::~D3D12TransientLinearAllocator()
    {
        heap_.Reset();
    }

    bool D3D12TransientLinearAllocator::Init(ID3D12Device* device,
                                             D3D12_HEAP_TYPE heapType,
                                             uint64 heapSize,
                                             D3D12_HEAP_FLAGS heapFlags)
    {
        if (!device || heapSize == 0)
            return false;

        heapSize_ = heapSize;

        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = heapSize;
        heapDesc.Properties.Type = heapType;
        heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heapDesc.Flags = heapFlags;

        HRESULT hr = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12TransientLinearAllocator] CreateHeap failed");
            return false;
        }

        return true;
    }

    bool D3D12TransientLinearAllocator::Allocate(uint64 size, uint64 alignment, D3D12AllocationBlock& outBlock)
    {
        std::lock_guard lock(mutex_);

        if (alignment == 0)
            alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        uint64 alignedOffset = (currentOffset_ + alignment - 1) & ~(alignment - 1);
        if (alignedOffset + size > heapSize_)
            return false;

        outBlock.offset = alignedOffset;
        outBlock.size = size;
        outBlock.heapIndex = 0;
        outBlock.order = 0;

        currentOffset_ = alignedOffset + size;
        return true;
    }

    void D3D12TransientLinearAllocator::Reset()
    {
        std::lock_guard lock(mutex_);
        currentOffset_ = 0;
    }

    //=========================================================================
    // D3D12UploadRingBuffer
    //=========================================================================

    D3D12UploadRingBuffer::~D3D12UploadRingBuffer()
    {
        Shutdown();
    }

    bool D3D12UploadRingBuffer::Init(ID3D12Device* device, uint64 bufferSize)
    {
        if (!device || bufferSize == 0)
            return false;

        bufferSize_ = bufferSize;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = bufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(&heapProps,
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &resourceDesc,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                     nullptr,
                                                     IID_PPV_ARGS(&resource_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12UploadRingBuffer] CreateCommittedResource failed");
            return false;
        }

        // 常時マップ
        D3D12_RANGE readRange = {0, 0};
        hr = resource_->Map(0, &readRange, &mappedPtr_);
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12UploadRingBuffer] Map failed");
            resource_.Reset();
            return false;
        }

        gpuBaseAddress_ = resource_->GetGPUVirtualAddress();
        return true;
    }

    void D3D12UploadRingBuffer::Shutdown()
    {
        if (resource_ && mappedPtr_)
        {
            resource_->Unmap(0, nullptr);
            mappedPtr_ = nullptr;
        }
        resource_.Reset();
        bufferSize_ = 0;
        head_ = 0;
        tail_ = 0;
        gpuBaseAddress_ = 0;
        frameRecords_.clear();
    }

    void D3D12UploadRingBuffer::BeginFrame(uint64 completedFenceValue)
    {
        std::lock_guard lock(mutex_);

        // 完了済みフレームを解放
        while (!frameRecords_.empty() && frameRecords_.front().fenceValue <= completedFenceValue)
        {
            tail_ = frameRecords_.front().endOffset;
            frameRecords_.erase(frameRecords_.begin());
        }

        if (frameRecords_.empty())
        {
            // 全フレーム完了 — リセット
            head_ = 0;
            tail_ = 0;
        }

        frameStartOffset_ = head_;
    }

    void D3D12UploadRingBuffer::EndFrame(uint64 fenceValue)
    {
        std::lock_guard lock(mutex_);

        if (head_ != frameStartOffset_)
        {
            FrameRecord record;
            record.endOffset = head_;
            record.fenceValue = fenceValue;
            frameRecords_.push_back(record);
        }
    }

    D3D12UploadRingBuffer::Allocation D3D12UploadRingBuffer::Allocate(uint64 size, uint64 alignment)
    {
        std::lock_guard lock(mutex_);

        Allocation result;

        if (alignment == 0)
            alignment = 1;

        uint64 alignedHead = (head_ + alignment - 1) & ~(alignment - 1);

        if (head_ >= tail_)
        {
            // 通常: head >= tail
            if (alignedHead + size <= bufferSize_)
            {
                result.offset = alignedHead;
                result.size = size;
                result.cpuAddress = static_cast<uint8*>(mappedPtr_) + alignedHead;
                result.gpuAddress = gpuBaseAddress_ + alignedHead;
                head_ = alignedHead + size;
                return result;
            }

            // ラップアラウンド試行
            alignedHead = 0;
            if (alignedHead + size <= tail_)
            {
                result.offset = alignedHead;
                result.size = size;
                result.cpuAddress = static_cast<uint8*>(mappedPtr_) + alignedHead;
                result.gpuAddress = gpuBaseAddress_ + alignedHead;
                head_ = alignedHead + size;
                return result;
            }

            return result; // 空き不足
        }

        // ラップ済み: head < tail
        if (alignedHead + size <= tail_)
        {
            result.offset = alignedHead;
            result.size = size;
            result.cpuAddress = static_cast<uint8*>(mappedPtr_) + alignedHead;
            result.gpuAddress = gpuBaseAddress_ + alignedHead;
            head_ = alignedHead + size;
            return result;
        }

        return result; // 空き不足
    }

    uint64 D3D12UploadRingBuffer::GetUsedSize() const
    {
        if (head_ >= tail_)
            return head_ - tail_;
        return bufferSize_ - tail_ + head_;
    }

    //=========================================================================
    // D3D12TransientResourceAllocator
    //=========================================================================

    D3D12TransientResourceAllocator::~D3D12TransientResourceAllocator()
    {
        for (auto& cb : bufferCache_)
        {
            if (cb.buffer)
                cb.buffer->Release();
        }
        for (auto& ct : textureCache_)
        {
            if (ct.texture)
                ct.texture->Release();
        }
    }

    bool D3D12TransientResourceAllocator::Init(D3D12Device* device, const NS::RHI::RHITransientAllocatorDesc& desc)
    {
        if (!device)
            return false;

        device_ = device;
        maxHeapSize_ = desc.maxHeapSize;
        allowGrowth_ = desc.allowGrowth;
        return true;
    }

    void D3D12TransientResourceAllocator::BeginFrame()
    {
        std::lock_guard lock(mutex_);

        // 全リソースをキャッシュに戻す（再利用可能に）
        for (auto& cb : bufferCache_)
            cb.inUse = false;
        for (auto& ct : textureCache_)
            ct.inUse = false;

        // ハンドル配列リセット
        bufferHandles_.clear();
        textureHandles_.clear();

        // Stats: フレーム単位リセット
        stats_.currentUsedMemory = 0;
        stats_.allocatedBuffers = 0;
        stats_.allocatedTextures = 0;
        stats_.reusedResources = 0;
    }

    void D3D12TransientResourceAllocator::EndFrame()
    {
        // フレーム終了処理（現状はスタブ、フェンス連携は将来拡張）
    }

    NS::RHI::RHITransientBuffer D3D12TransientResourceAllocator::AllocateBuffer(
        const NS::RHI::RHITransientBufferDesc& desc)
    {
        std::lock_guard lock(mutex_);

        NS::RHI::RHITransientBuffer result;
        NS::RHI::IRHIBuffer* buffer = nullptr;

        // キャッシュから同一サイズ・usageのバッファを探す
        for (auto& cb : bufferCache_)
        {
            if (!cb.inUse && cb.size == desc.size && cb.usage == desc.usage)
            {
                cb.inUse = true;
                buffer = cb.buffer;
                ++stats_.reusedResources;
                break;
            }
        }

        // キャッシュミス: 新規作成
        if (!buffer)
        {
            NS::RHI::RHIBufferDesc bufDesc;
            bufDesc.size = desc.size;
            bufDesc.usage = desc.usage;

            auto* d3dBuffer = new D3D12Buffer();
            if (!d3dBuffer->Init(device_, bufDesc, nullptr))
            {
                delete d3dBuffer;
                return result;
            }
            buffer = d3dBuffer;

            CachedBuffer cb;
            cb.buffer = buffer;
            cb.size = desc.size;
            cb.usage = desc.usage;
            cb.inUse = true;
            bufferCache_.push_back(cb);

            stats_.totalHeapSize += desc.size;
        }

        // ハンドル登録
        uint32 handle = static_cast<uint32>(bufferHandles_.size());
        bufferHandles_.push_back(buffer);

        // RHITransientBuffer構築（基底クラスのヘルパー経由）
        SetupBufferHandle(result, this, handle, desc);

        stats_.allocatedBuffers++;
        stats_.currentUsedMemory += desc.size;
        if (stats_.currentUsedMemory > stats_.peakUsedMemory)
            stats_.peakUsedMemory = stats_.currentUsedMemory;

        return result;
    }

    NS::RHI::RHITransientTexture D3D12TransientResourceAllocator::AllocateTexture(
        const NS::RHI::RHITransientTextureDesc& desc)
    {
        std::lock_guard lock(mutex_);

        NS::RHI::RHITransientTexture result;
        NS::RHI::IRHITexture* texture = nullptr;

        // キャッシュから同一パラメータのテクスチャを探す
        for (auto& ct : textureCache_)
        {
            if (!ct.inUse && ct.width == desc.width && ct.height == desc.height && ct.depth == desc.depth &&
                ct.format == desc.format && ct.usage == desc.usage)
            {
                ct.inUse = true;
                texture = ct.texture;
                ++stats_.reusedResources;
                break;
            }
        }

        // キャッシュミス: 新規作成
        if (!texture)
        {
            NS::RHI::RHITextureDesc texDesc;
            texDesc.width = desc.width;
            texDesc.height = desc.height;
            texDesc.depthOrArraySize = desc.depth;
            texDesc.format = desc.format;
            texDesc.usage = desc.usage;
            texDesc.mipLevels = desc.mipLevels;
            texDesc.sampleCount = static_cast<NS::RHI::ERHISampleCount>(desc.sampleCount);
            texDesc.dimension = desc.dimension;

            auto* d3dTexture = new D3D12Texture();
            if (!d3dTexture->Init(device_, texDesc))
            {
                delete d3dTexture;
                return result;
            }
            texture = d3dTexture;

            CachedTexture ct;
            ct.texture = texture;
            ct.width = desc.width;
            ct.height = desc.height;
            ct.depth = desc.depth;
            ct.format = desc.format;
            ct.usage = desc.usage;
            ct.inUse = true;
            textureCache_.push_back(ct);

            stats_.totalHeapSize += desc.EstimateMemorySize();
        }

        // ハンドル登録
        uint32 handle = static_cast<uint32>(textureHandles_.size());
        textureHandles_.push_back(texture);

        // RHITransientTexture構築（基底クラスのヘルパー経由）
        SetupTextureHandle(result, this, handle, desc);

        uint64 memSize = desc.EstimateMemorySize();
        stats_.allocatedTextures++;
        stats_.currentUsedMemory += memSize;
        if (stats_.currentUsedMemory > stats_.peakUsedMemory)
            stats_.peakUsedMemory = stats_.currentUsedMemory;

        return result;
    }

    void D3D12TransientResourceAllocator::AcquireResources(NS::RHI::IRHICommandContext*, uint32)
    {
        // エイリアシングバリア挿入（将来: Placed Resource使用時に必要）
    }

    void D3D12TransientResourceAllocator::ReleaseResources(NS::RHI::IRHICommandContext*, uint32)
    {
        // リソース解放（将来: パス終了時のバリア処理）
    }

    void D3D12TransientResourceAllocator::AcquireResourcesForPipeline(NS::RHI::IRHICommandContext*,
                                                                      uint32,
                                                                      NS::RHI::ERHIPipeline)
    {}

    void D3D12TransientResourceAllocator::ReleaseResourcesForPipeline(NS::RHI::IRHICommandContext*,
                                                                      uint32,
                                                                      NS::RHI::ERHIPipeline)
    {}

    void D3D12TransientResourceAllocator::SetAllocationFences(const NS::RHI::RHITransientAllocationFences& fences)
    {
        fences_ = fences;
    }

    void D3D12TransientResourceAllocator::SetAsyncComputeBudget(NS::RHI::ERHIAsyncComputeBudget)
    {
        // AsyncComputeバジェット（将来拡張）
    }

    NS::RHI::RHITransientAllocatorStats D3D12TransientResourceAllocator::GetStats() const
    {
        return stats_;
    }

    void D3D12TransientResourceAllocator::DumpMemoryUsage() const
    {
        char buf[256];
        snprintf(buf,
                 sizeof(buf),
                 "[D3D12TransientResourceAllocator] Heap: %llu MB, Used: %llu MB, Peak: %llu MB, "
                 "Buffers: %u, Textures: %u, Reused: %u",
                 stats_.totalHeapSize / (1024 * 1024),
                 stats_.currentUsedMemory / (1024 * 1024),
                 stats_.peakUsedMemory / (1024 * 1024),
                 stats_.allocatedBuffers,
                 stats_.allocatedTextures,
                 stats_.reusedResources);
        LOG_INFO(buf);
    }

    NS::RHI::IRHIBuffer* D3D12TransientResourceAllocator::GetBufferInternal(uint32 handle) const
    {
        if (handle < static_cast<uint32>(bufferHandles_.size()))
            return bufferHandles_[handle];
        return nullptr;
    }

    NS::RHI::IRHITexture* D3D12TransientResourceAllocator::GetTextureInternal(uint32 handle) const
    {
        if (handle < static_cast<uint32>(textureHandles_.size()))
            return textureHandles_[handle];
        return nullptr;
    }

} // namespace NS::D3D12RHI
