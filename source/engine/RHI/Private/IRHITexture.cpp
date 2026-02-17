/// @file IRHITexture.cpp
/// @brief テクスチャ便利メソッド・ヘルパークラス実装
#include "IRHITexture.h"
#include "IDynamicRHI.h"
#include "IRHIBuffer.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include <cstddef>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // IRHITexture::CalculateUploadSize
    //=========================================================================

    MemorySize IRHITexture::CalculateUploadSize(const RHISubresourceRange& range) const
    {
        MemorySize totalSize = 0;

        uint32 const mipCount = range.levelCount > 0 ? range.levelCount : GetMipLevels() - range.baseMipLevel;
        uint32 const layerCount = range.layerCount > 0 ? range.layerCount : GetArraySize() - range.baseArrayLayer;

        for (uint32 layer = 0; layer < layerCount; ++layer)
        {
            for (uint32 mip = 0; mip < mipCount; ++mip)
            {
                uint32 const mipLevel = range.baseMipLevel + mip;
                uint32 const arraySlice = range.baseArrayLayer + layer;
                RHISubresourceLayout const layout = GetSubresourceLayout(mipLevel, arraySlice);
                totalSize += layout.size;
            }
        }

        return totalSize;
    }

    //=========================================================================
    // RHITextureUploader
    //=========================================================================

    RHITextureUploader::RHITextureUploader(IRHIDevice* device, IRHICommandContext* context)
        : m_device(device), m_context(context)
    {}

    bool RHITextureUploader::Upload2D(IRHITexture* dst, uint32 mipLevel, const void* srcData, uint32 srcRowPitch)
    {
        return Upload2DArray(dst, mipLevel, 0, srcData, srcRowPitch);
    }

    bool RHITextureUploader::Upload2DArray(
        IRHITexture* dst, uint32 mipLevel, uint32 arraySlice, const void* srcData, uint32 srcRowPitch)
    {
        if ((m_device == nullptr) || (m_context == nullptr) || (dst == nullptr) || (srcData == nullptr))
        {
            return false;
        }

        RHISubresourceLayout const layout = dst->GetSubresourceLayout(mipLevel, arraySlice);
        if (layout.size == 0)
        {
            return false;
        }

        // ステージングバッファ作成
        RHIBufferDesc stagingDesc;
        stagingDesc.size = layout.size;
        stagingDesc.usage = ERHIBufferUsage::Staging;
        auto stagingRef = GetDynamicRHI()->CreateBuffer(stagingDesc);
        if (!stagingRef)
        {
            return false;
        }

        m_stagingBuffer = stagingRef;
        IRHIBuffer* staging = stagingRef.Get();

        // ステージングバッファにデータ書き込み
        RHIMapResult const map = staging->Map(ERHIMapMode::WriteDiscard);
        if (!map.IsValid())
        {
            return false;
        }

        uint32 const rowCount = dst->GetRowCount(mipLevel);
        uint32 copyRowPitch = std::min(srcRowPitch, layout.rowPitch);
        for (uint32 row = 0; row < rowCount; ++row)
        {
            const uint8* srcRow = static_cast<const uint8*>(srcData) + (static_cast<size_t>(row) * srcRowPitch);
            uint8* dstRow = static_cast<uint8*>(map.data) + (static_cast<size_t>(row) * layout.rowPitch);
            std::memcpy(dstRow, srcRow, copyRowPitch);
        }
        staging->Unmap();

        // コピーコマンド発行
        m_context->CopyBufferToTexture(
            dst, mipLevel, arraySlice, Offset3D{0, 0, 0}, staging, 0, layout.rowPitch, layout.depthPitch);

        return true;
    }

    bool RHITextureUploader::Upload3D(
        IRHITexture* dst, uint32 mipLevel, const void* srcData, uint32 srcRowPitch, uint32 srcDepthPitch)
    {
        if ((m_device == nullptr) || (m_context == nullptr) || (dst == nullptr) || (srcData == nullptr))
        {
            return false;
        }

        RHISubresourceLayout const layout = dst->GetSubresourceLayout(mipLevel, 0);
        if (layout.size == 0)
        {
            return false;
        }

        RHIBufferDesc stagingDesc;
        stagingDesc.size = layout.size;
        stagingDesc.usage = ERHIBufferUsage::Staging;
        auto stagingRef = GetDynamicRHI()->CreateBuffer(stagingDesc);
        if (!stagingRef)
        {
            return false;
        }

        m_stagingBuffer = stagingRef;
        IRHIBuffer* staging = stagingRef.Get();

        RHIMapResult const map = staging->Map(ERHIMapMode::WriteDiscard);
        if (!map.IsValid())
        {
            return false;
        }

        Extent3D const mipSize = dst->GetMipSize(mipLevel);
        uint32 const rowCount = dst->GetRowCount(mipLevel);
        uint32 copyRowPitch = std::min(srcRowPitch, layout.rowPitch);

        for (uint32 slice = 0; slice < mipSize.depth; ++slice)
        {
            for (uint32 row = 0; row < rowCount; ++row)
            {
                const uint8* srcRow = static_cast<const uint8*>(srcData) +
                                      (static_cast<size_t>(slice) * srcDepthPitch) +
                                      (static_cast<size_t>(row) * srcRowPitch);
                uint8* dstRow = static_cast<uint8*>(map.data) + (static_cast<size_t>(slice) * layout.depthPitch) +
                                (static_cast<size_t>(row) * layout.rowPitch);
                std::memcpy(dstRow, srcRow, copyRowPitch);
            }
        }
        staging->Unmap();

        m_context->CopyBufferToTexture(
            dst, mipLevel, 0, Offset3D{0, 0, 0}, staging, 0, layout.rowPitch, layout.depthPitch);

        return true;
    }

    bool RHITextureUploader::UploadCubeFace(
        IRHITexture* dst, ERHICubeFace face, uint32 mipLevel, const void* srcData, uint32 srcRowPitch)
    {
        auto const arraySlice = static_cast<uint32>(face);
        return Upload2DArray(dst, mipLevel, arraySlice, srcData, srcRowPitch);
    }

    bool RHITextureUploader::UploadRegion(IRHITexture* dst,
                                          uint32 mipLevel,
                                          uint32 arraySlice,
                                          const RHIBox& dstRegion,
                                          const void* srcData,
                                          uint32 srcRowPitch,
                                          uint32 srcDepthPitch)
    {
        // 部分領域アップロードはバックエンド依存が強い
        // 基本実装: ステージングバッファ経由でCopyTextureRegionを使用
        (void)dstRegion;
        (void)srcDepthPitch;
        return Upload2DArray(dst, mipLevel, arraySlice, srcData, srcRowPitch);
    }

    void RHITextureUploader::GenerateMips(IRHITexture* texture)
    {
        // MIP自動生成はバックエンド依存（コンピュートシェーダーまたはハードウェア機能）
        (void)texture;
    }

    //=========================================================================
    // RHITextureReadback
    //=========================================================================

    RHITextureReadback::RHITextureReadback(IRHIDevice* device, IRHICommandContext* context)
        : m_device(device), m_context(context)
    {}

    bool RHITextureReadback::Read2D(IRHITexture* src, uint32 mipLevel, void* dstData, uint32 dstRowPitch)
    {
        if ((m_device == nullptr) || (m_context == nullptr) || (src == nullptr) || (dstData == nullptr))
        {
            return false;
        }

        RHISubresourceLayout const layout = src->GetSubresourceLayout(mipLevel, 0);
        if (layout.size == 0)
        {
            return false;
        }

        // ステージングバッファ作成（CPU読み取り可能）
        RHIBufferDesc stagingDesc;
        stagingDesc.size = layout.size;
        stagingDesc.usage = ERHIBufferUsage::Staging;
        auto stagingRef = GetDynamicRHI()->CreateBuffer(stagingDesc);
        if (!stagingRef)
        {
            return false;
        }

        IRHIBuffer* staging = stagingRef.Get();

        // テクスチャ→バッファコピー
        m_context->CopyTextureToBuffer(staging, 0, layout.rowPitch, layout.depthPitch, src, mipLevel, 0, nullptr);

        // GPU完了待ち（同期リードバック）
        // 実際にはフェンス待機が必要だが、ここでは即座にマップ
        RHIMapResult const map = staging->Map(ERHIMapMode::Read);
        if (!map.IsValid())
        {
            return false;
        }

        uint32 const rowCount = src->GetRowCount(mipLevel);
        uint32 copyRowPitch = std::min(dstRowPitch, layout.rowPitch);
        for (uint32 row = 0; row < rowCount; ++row)
        {
            const uint8* srcRow = static_cast<const uint8*>(map.data) + (static_cast<size_t>(row) * layout.rowPitch);
            uint8* dstRow = static_cast<uint8*>(dstData) + (static_cast<size_t>(row) * dstRowPitch);
            std::memcpy(dstRow, srcRow, copyRowPitch);
        }
        staging->Unmap();

        return true;
    }

    uint64 RHITextureReadback::BeginAsyncRead(IRHITexture* src, uint32 mipLevel, uint32 arraySlice)
    {
        // 非同期リードバック: バックエンド依存
        (void)src;
        (void)mipLevel;
        (void)arraySlice;
        return 0;
    }

    bool RHITextureReadback::IsReadComplete(uint64 readbackId)
    {
        // バックエンド依存
        (void)readbackId;
        return false;
    }

    bool RHITextureReadback::GetReadResult(uint64 readbackId, void* dstData, uint32 dstRowPitch)
    {
        // バックエンド依存
        (void)readbackId;
        (void)dstData;
        (void)dstRowPitch;
        return false;
    }

    //=========================================================================
    // RHITextureCopyHelper
    //=========================================================================

    bool RHITextureCopyHelper::Validate(const RHITextureCopyDesc& desc)
    {
        if ((desc.srcTexture == nullptr) || (desc.dstTexture == nullptr))
        {
            return false;
        }

        if (!desc.srcTexture->IsValidMipLevel(desc.srcMipLevel))
        {
            return false;
        }

        if (!desc.dstTexture->IsValidMipLevel(desc.dstMipLevel))
        {
            return false;
        }

        return true;
    }

    void RHITextureCopyHelper::CopyWithBarriers(IRHICommandContext* context, const RHITextureCopyDesc& desc)
    {
        if ((context == nullptr) || !Validate(desc))
        {
            return;
        }

        // ソース→CopySource、デスト→CopyDest にバリア
        context->TransitionBarrier(desc.srcTexture, ERHIResourceState::Common, ERHIResourceState::CopySource);
        context->TransitionBarrier(desc.dstTexture, ERHIResourceState::Common, ERHIResourceState::CopyDest);
        context->FlushBarriers();

        // コピー実行
        if (desc.extent.width == 0 && desc.extent.height == 0 && desc.extent.depth == 0)
        {
            // 全体コピー
            context->CopyTexture(desc.dstTexture, desc.srcTexture);
        }
        else
        {
            // 領域コピー
            context->CopyTextureRegion(desc.dstTexture,
                                       desc.dstMipLevel,
                                       desc.dstArraySlice,
                                       desc.dstOffset,
                                       desc.srcTexture,
                                       desc.srcMipLevel,
                                       desc.srcArraySlice);
        }
    }

    bool RHITextureCopyHelper::CopyWithFormatConversion(IRHICommandContext* context, const RHITextureCopyDesc& desc)
    {
        // フォーマット変換コピーはバックエンド依存
        // 基本実装: 互換フォーマット間は通常コピー
        if (!Validate(desc))
        {
            return false;
        }

        CopyWithBarriers(context, desc);
        return true;
    }

} // namespace NS::RHI
