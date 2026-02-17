/// @file RHITransientAllocator.cpp
/// @brief Transientリソースアロケーター実装
#include "RHITransientAllocator.h"

namespace NS::RHI
{
    //=========================================================================
    // RHITransientTextureDesc
    //=========================================================================

    uint64 RHITransientTextureDesc::EstimateMemorySize() const
    {
        // 簡易概算: 各ミップレベルのサイズ合計 × サンプル数
        // 実際のアライメント要件・フォーマット別bppはバックエンド依存
        constexpr uint32 kDefaultBytesPerPixel = 4;

        uint64 totalSize = 0;
        uint32 mipW = width;
        uint32 mipH = height;
        uint32 mipD = depth;

        for (uint32 mip = 0; mip < mipLevels; ++mip)
        {
            totalSize += static_cast<uint64>(mipW) * mipH * mipD * kDefaultBytesPerPixel;
            mipW = (mipW > 1) ? mipW / 2 : 1;
            mipH = (mipH > 1) ? mipH / 2 : 1;
            mipD = (mipD > 1) ? mipD / 2 : 1;
        }

        totalSize *= sampleCount;
        return totalSize;
    }

    //=========================================================================
    // RHITransientBuffer
    //=========================================================================

    IRHIBuffer* RHITransientBuffer::GetBuffer() const
    {
        if (m_allocator == nullptr) {
            return nullptr;
}

        // バックエンド実装: アロケーターがハンドルからバッファを解決
        // GetBufferInternalはprotectedのため、アロケーター側で直接設定するパスが必要
        return nullptr;
    }

    //=========================================================================
    // RHITransientTexture
    //=========================================================================

    IRHITexture* RHITransientTexture::GetTexture() const
    {
        if (m_allocator == nullptr) {
            return nullptr;
}

        // バックエンド実装: アロケーターがハンドルからテクスチャを解決
        return nullptr;
    }

} // namespace NS::RHI
