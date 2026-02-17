/// @file RHIUploadHeap.cpp
/// @brief アチE�Eロードヒープ�Eバッチ�E非同期�Eネ�Eジャー・チE��スチャローダー実裁E
#include "RHIUploadHeap.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include "IRHIFence.h"
#include "IRHITexture.h"
#include "RHIPixelFormat.h"
#include <cstddef>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIUploadHeap
    //=========================================================================

    bool RHIUploadHeap::Initialize(IRHIDevice* device, uint64 size, uint32 numBufferedFrames)
    {
        m_device = device;
        return m_ringBuffer.Initialize(device, size, numBufferedFrames, ERHIHeapType::Upload);
    }

    void RHIUploadHeap::Shutdown()
    {
        m_ringBuffer.Shutdown();
        m_device = nullptr;
    }

    void RHIUploadHeap::BeginFrame(uint32 frameIndex, uint64 completedFrame)
    {
        m_ringBuffer.BeginFrame(frameIndex, completedFrame);
    }

    void RHIUploadHeap::EndFrame(uint64 frameNumber)
    {
        m_ringBuffer.EndFrame(frameNumber);
    }

    bool RHIUploadHeap::UploadBuffer(IRHICommandContext* context, const RHIBufferUploadRequest& request)
    {
        if ((context == nullptr) || (request.destBuffer == nullptr) || (request.srcData == nullptr) ||
            request.size == 0)
        {
            return false;
        }

        RHIBufferAllocation const staging = AllocateStaging(request.size);
        if (!staging.IsValid())
        {
            return false;
        }

        // スチE�EジングバッファにチE�Eタコピ�E
        std::memcpy(staging.cpuAddress, request.srcData, request.size);

        // GPU側コピ�Eコマンド発衁E
        context->CopyBufferRegion(request.destBuffer, request.destOffset, staging.buffer, staging.offset, request.size);

        return true;
    }

    bool RHIUploadHeap::UploadTexture(IRHICommandContext* context, const RHITextureUploadRequest& request)
    {
        if ((context == nullptr) || (request.destTexture == nullptr) || (request.srcData == nullptr))
        {
            return false;
        }

        // チE��スチャ用スチE�Eジング割り当て
        TextureStagingAllocation const staging =
            AllocateTextureStaging(request.width, request.height, request.destTexture->GetFormat());
        if (!staging.allocation.IsValid())
        {
            return false;
        }

        // 行ごとにコピ�E�E�ピチE��が異なる場合�Eため�E�E
        const auto* srcBytes = static_cast<const uint8*>(request.srcData);
        auto* dstBytes = static_cast<uint8*>(staging.allocation.cpuAddress);

        uint32 const srcPitch = request.srcRowPitch > 0 ? request.srcRowPitch : staging.rowPitch;
        uint32 const copyPitch = srcPitch < staging.rowPitch ? srcPitch : staging.rowPitch;

        for (uint32 row = 0; row < request.height; ++row)
        {
            std::memcpy(dstBytes + (static_cast<size_t>(row) * staging.rowPitch),
                        srcBytes + (static_cast<size_t>(row) * srcPitch),
                        copyPitch);
        }

        // GPU側チE��スチャコピ�E発衁E
        context->CopyBufferToTexture(request.destTexture,
                                     request.destSubresource, // mip level
                                     0,                       // array slice
                                     Offset3D{static_cast<int32>(request.destX),
                                              static_cast<int32>(request.destY),
                                              static_cast<int32>(request.destZ)},
                                     staging.allocation.buffer,
                                     staging.allocation.offset,
                                     staging.rowPitch,
                                     staging.slicePitch > 0 ? staging.slicePitch : staging.rowPitch * request.height);

        return true;
    }

    RHIBufferAllocation RHIUploadHeap::AllocateStaging(uint64 size, uint64 alignment)
    {
        return m_ringBuffer.Allocate(size, alignment > 0 ? alignment : 256);
    }

    RHIUploadHeap::TextureStagingAllocation RHIUploadHeap::AllocateTextureStaging(uint32 width,
                                                                                  uint32 height,
                                                                                  ERHIPixelFormat format)
    {
        TextureStagingAllocation result;

        // フォーマット別のバイト数
        uint32 const bytesPerPixel = GetFormatBytesPerPixelOrBlock(format);
        if (bytesPerPixel == 0)
        {
            return result;
        }

        // D3D12のチE��スチャアチE�Eロード�E256バイトアライメント�E行ピチE��が忁E��E
        result.rowPitch = (width * bytesPerPixel + 255) & ~255U;
        result.slicePitch = result.rowPitch * height;

        result.allocation = m_ringBuffer.Allocate(result.slicePitch, 512);
        return result;
    }

    //=========================================================================
    // RHIUploadBatch
    //=========================================================================

    bool RHIUploadBatch::Initialize(RHIUploadHeap* uploadHeap, uint32 maxRequests)
    {
        m_uploadHeap = uploadHeap;
        m_maxRequests = maxRequests;

        m_bufferRequests = new RHIBufferUploadRequest[maxRequests];
        m_textureRequests = new RHITextureUploadRequest[maxRequests];
        m_bufferCount = 0;
        m_textureCount = 0;
        m_totalDataSize = 0;

        return true;
    }

    void RHIUploadBatch::Shutdown()
    {
        delete[] m_bufferRequests;
        delete[] m_textureRequests;
        m_bufferRequests = nullptr;
        m_textureRequests = nullptr;
        m_bufferCount = 0;
        m_textureCount = 0;
        m_maxRequests = 0;
        m_totalDataSize = 0;
        m_uploadHeap = nullptr;
    }

    bool RHIUploadBatch::AddBuffer(const RHIBufferUploadRequest& request)
    {
        if (m_bufferCount + m_textureCount >= m_maxRequests)
        {
            return false;
        }

        m_bufferRequests[m_bufferCount++] = request;
        m_totalDataSize += request.size;
        return true;
    }

    bool RHIUploadBatch::AddTexture(const RHITextureUploadRequest& request)
    {
        if (m_bufferCount + m_textureCount >= m_maxRequests)
        {
            return false;
        }

        m_textureRequests[m_textureCount++] = request;
        m_totalDataSize += static_cast<uint64>(request.srcRowPitch) * request.height * request.depth;
        return true;
    }

    void RHIUploadBatch::Clear()
    {
        m_bufferCount = 0;
        m_textureCount = 0;
        m_totalDataSize = 0;
    }

    uint32 RHIUploadBatch::Execute(IRHICommandContext* context)
    {
        if ((context == nullptr) || (m_uploadHeap == nullptr))
        {
            return 0;
        }

        uint32 uploaded = 0;

        for (uint32 i = 0; i < m_bufferCount; ++i)
        {
            if (m_uploadHeap->UploadBuffer(context, m_bufferRequests[i]))
            {
                ++uploaded;
            }
        }

        for (uint32 i = 0; i < m_textureCount; ++i)
        {
            if (m_uploadHeap->UploadTexture(context, m_textureRequests[i]))
            {
                ++uploaded;
            }
        }

        Clear();
        return uploaded;
    }

    //=========================================================================
    // RHIAsyncUploadManager
    //=========================================================================

    bool RHIAsyncUploadManager::Initialize(IRHIDevice* device, uint64 uploadHeapSize)
    {
        m_device = device;

        if (!m_uploadHeap.Initialize(device, uploadHeapSize))
        {
            return false;
        }

        m_fence = device->CreateFence(0, "AsyncUploadFence");
        m_nextFenceValue = 1;
        m_nextHandleId = 1;

        m_pendingCapacity = 64;
        m_pendingUploads = new PendingUpload[m_pendingCapacity];
        m_pendingCount = 0;

        // コピ�Eキュー取得�Eバックエンド依孁E
        m_copyQueue = nullptr;

        return true;
    }

    void RHIAsyncUploadManager::Shutdown()
    {
        WaitAll();

        delete[] m_pendingUploads;
        m_pendingUploads = nullptr;
        m_pendingCount = 0;
        m_pendingCapacity = 0;

        m_uploadHeap.Shutdown();
        m_fence = nullptr;
        m_copyQueue = nullptr;
        m_device = nullptr;
    }

    void RHIAsyncUploadManager::BeginFrame()
    {
        // 完亁E��たアチE�Eロードを除去
        uint64 const completedValue = m_fence ? m_fence->GetCompletedValue() : 0;

        uint32 writeIdx = 0;
        for (uint32 i = 0; i < m_pendingCount; ++i)
        {
            if (m_pendingUploads[i].fenceValue <= completedValue)
            {
                m_pendingUploads[i].status = ERHIUploadStatus::Completed;
            }
            else
            {
                m_pendingUploads[writeIdx++] = m_pendingUploads[i];
            }
        }
        m_pendingCount = writeIdx;
    }

    void RHIAsyncUploadManager::EndFrame()
    {
        // フレーム終亁E��の処琁E
    }

    RHIAsyncUploadHandle RHIAsyncUploadManager::UploadBufferAsync(const RHIBufferUploadRequest& request)
    {
        (void)request;

        RHIAsyncUploadHandle handle;
        handle.id = m_nextHandleId++;

        // TODO: 非同期コピーキューでの実転送（バックエンド依存）
        if (m_pendingCount < m_pendingCapacity)
        {
            auto& pending = m_pendingUploads[m_pendingCount++];
            pending.handle = handle;
            pending.fenceValue = 0;
            pending.status = ERHIUploadStatus::Pending;
        }

        return handle;
    }

    RHIAsyncUploadHandle RHIAsyncUploadManager::UploadTextureAsync(const RHITextureUploadRequest& request)
    {
        (void)request;

        RHIAsyncUploadHandle handle;
        handle.id = m_nextHandleId++;

        // TODO: 非同期コピーキューでの実転送（バックエンド依存）
        if (m_pendingCount < m_pendingCapacity)
        {
            auto& pending = m_pendingUploads[m_pendingCount++];
            pending.handle = handle;
            pending.fenceValue = 0;
            pending.status = ERHIUploadStatus::Pending;
        }

        return handle;
    }

    ERHIUploadStatus RHIAsyncUploadManager::GetStatus(RHIAsyncUploadHandle handle) const
    {
        for (uint32 i = 0; i < m_pendingCount; ++i)
        {
            if (m_pendingUploads[i].handle.id == handle.id)
            {
                return m_pendingUploads[i].status;
            }
        }
        return ERHIUploadStatus::Completed;
    }

    bool RHIAsyncUploadManager::Wait(RHIAsyncUploadHandle handle, uint64 timeoutMs)
    {
        for (uint32 i = 0; i < m_pendingCount; ++i)
        {
            if (m_pendingUploads[i].handle.id == handle.id)
            {
                if (m_pendingUploads[i].status == ERHIUploadStatus::Completed || m_pendingUploads[i].fenceValue == 0)
                {
                    return true;
                }
                if (m_fence)
                {
                    return m_fence->Wait(m_pendingUploads[i].fenceValue, timeoutMs);
                }
                return false;
            }
        }
        return true;
    }

    void RHIAsyncUploadManager::WaitAll()
    {
        uint64 maxFenceValue = 0;
        for (uint32 i = 0; i < m_pendingCount; ++i)
        {
            if (m_pendingUploads[i].status != ERHIUploadStatus::Completed &&
                m_pendingUploads[i].fenceValue > maxFenceValue)
            {
                maxFenceValue = m_pendingUploads[i].fenceValue;
            }
        }

        if (m_fence && maxFenceValue > 0)
        {
            m_fence->Wait(maxFenceValue, UINT64_MAX);
        }
        m_pendingCount = 0;
    }

    RHISyncPoint RHIAsyncUploadManager::GetSyncPoint() const
    {
        RHISyncPoint syncPoint;
        // 同期ポイント�E構築�Eバックエンド依孁E
        return syncPoint;
    }

    void RHIAsyncUploadManager::WaitOnGraphicsQueue(IRHIQueue* graphicsQueue)
    {
        // グラフィチE��スキューでコピ�Eキューのフェンスを征E��E
        (void)graphicsQueue;
    }

    //=========================================================================
    // RHITextureLoader
    //=========================================================================

    bool RHITextureLoader::Initialize(IRHIDevice* device, RHIAsyncUploadManager* uploadManager)
    {
        m_device = device;
        m_uploadManager = uploadManager;

        // 同期アチE�Eロード用ヒ�EチE
        if (!m_syncUploadHeap.Initialize(device, static_cast<uint64>(16 * 1024 * 1024)))
        {
            return false;
        }

        // ミップ生成PSOの作�Eはバックエンド依孁E
        m_mipGenPSO = nullptr;

        return true;
    }

    void RHITextureLoader::Shutdown()
    {
        m_syncUploadHeap.Shutdown();
        m_mipGenPSO = nullptr;
        m_uploadManager = nullptr;
        m_device = nullptr;
    }

    IRHITexture* RHITextureLoader::LoadFromMemory(const void* data,
                                                  uint64 dataSize,
                                                  const RHITextureLoadOptions& options)
    {
        // ファイルフォーマット�Eパ�EスはプラチE��フォーム依孁E
        // �E�EDS/PNG/JPEG等�EチE��ーダーが忁E��E��E
        (void)data;
        (void)dataSize;
        (void)options;
        return nullptr;
    }

    IRHITexture* RHITextureLoader::LoadFromRawData(
        const void* data, uint32 width, uint32 height, ERHIPixelFormat format, const RHITextureLoadOptions& options)
    {
        if ((m_device == nullptr) || (data == nullptr) || width == 0 || height == 0)
        {
            return nullptr;
        }

        RHITextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.mipLevels = 1;

        IRHITexture* texture = m_device->CreateTexture(desc, options.debugName);
        if (texture == nullptr)
        {
            return nullptr;
        }

        // TODO: Raw data → GPU アップロード（バックエンド依存）
        // 現在は未実装のため、テクスチャ内容は未初期化
        NS_ASSERT(false && "RHITextureLoader::LoadFromRawData - upload not yet implemented");
        return texture;
    }

    IRHITexture* RHITextureLoader::LoadFromMipData(const void* const* mipData,
                                                   const uint32* mipRowPitches,
                                                   uint32 mipCount,
                                                   uint32 width,
                                                   uint32 height,
                                                   ERHIPixelFormat format,
                                                   const RHITextureLoadOptions& options)
    {
        if ((m_device == nullptr) || (mipData == nullptr) || (mipRowPitches == nullptr) || mipCount == 0)
        {
            return nullptr;
        }

        RHITextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.mipLevels = mipCount;

        IRHITexture* texture = m_device->CreateTexture(desc, options.debugName);
        if (texture == nullptr)
        {
            return nullptr;
        }

        // TODO: Mip data → GPU アップロード（バックエンド依存）
        // 現在は未実装のため、テクスチャ内容は未初期化
        NS_ASSERT(false && "RHITextureLoader::LoadFromMipData - upload not yet implemented");
        return texture;
    }

    void RHITextureLoader::GenerateMipmaps(IRHICommandContext* context, IRHITexture* texture)
    {
        // コンピュートシェーダーによるミップ生成�Eバックエンド依孁E
        (void)context;
        (void)texture;
    }

} // namespace NS::RHI
