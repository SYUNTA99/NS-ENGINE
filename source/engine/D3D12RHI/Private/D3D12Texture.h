/// @file D3D12Texture.h
/// @brief D3D12 テクスチャ — IRHITexture実装
#pragma once

#include "D3D12Resource.h"
#include "RHI/Public/IRHITexture.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12Texture — IRHITexture実装
    //=========================================================================

    class D3D12Texture final : public NS::RHI::IRHITexture
    {
    public:
        D3D12Texture();
        ~D3D12Texture() override;

        /// テクスチャ作成
        bool Init(D3D12Device* device, const NS::RHI::RHITextureDesc& desc);

        /// 既存のID3D12Resourceからテクスチャを初期化（SwapChainバックバッファ用）
        bool InitFromExisting(D3D12Device* device,
                              ComPtr<ID3D12Resource> resource,
                              NS::RHI::ERHIPixelFormat format,
                              NS::RHI::ERHIResourceState initialState);

        /// ネイティブリソース取得
        D3D12GpuResource& GetGpuResource() { return gpuResource_; }
        const D3D12GpuResource& GetGpuResource() const { return gpuResource_; }
        ID3D12Resource* GetD3DResource() const { return gpuResource_.GetD3DResource(); }

        //=====================================================================
        // IRHITexture — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHITextureDimension GetDimension() const override { return dimension_; }
        uint32 GetWidth() const override { return width_; }
        uint32 GetHeight() const override { return height_; }
        uint32 GetDepth() const override { return depth_; }
        uint32 GetArraySize() const override { return arraySize_; }

        //=====================================================================
        // IRHITexture — フォーマット
        //=====================================================================

        NS::RHI::ERHIPixelFormat GetFormat() const override { return format_; }

        //=====================================================================
        // IRHITexture — ミップマップ
        //=====================================================================

        uint32 GetMipLevels() const override { return mipLevels_; }

        //=====================================================================
        // IRHITexture — マルチサンプリング
        //=====================================================================

        NS::RHI::ERHISampleCount GetSampleCount() const override { return sampleCount_; }
        uint32 GetSampleQuality() const override { return sampleQuality_; }

        //=====================================================================
        // IRHITexture — 使用フラグ/メモリ
        //=====================================================================

        NS::RHI::ERHITextureUsage GetUsage() const override { return usage_; }
        NS::RHI::RHITextureMemoryInfo GetMemoryInfo() const override;
        NS::RHI::RHIClearValue GetClearValue() const override { return clearValue_; }

        //=====================================================================
        // IRHITexture — サブリソースレイアウト
        //=====================================================================

        NS::RHI::RHISubresourceLayout GetSubresourceLayout(uint32 mipLevel, uint32 arraySlice = 0) const override;

        //=====================================================================
        // IRHITexture — Map/Unmap
        //=====================================================================

        NS::RHI::RHITextureMapResult Map(uint32 mipLevel, uint32 arraySlice, NS::RHI::ERHIMapMode mode) override;
        void Unmap(uint32 mipLevel, uint32 arraySlice) override;

        //=====================================================================
        // IRHIResource — デバッグ
        //=====================================================================

        void SetDebugName(const char* name) override;

        //=====================================================================
        // 静的ヘルパー
        //=====================================================================

        /// ERHIPixelFormat → DXGI_FORMAT変換
        static DXGI_FORMAT ConvertPixelFormat(NS::RHI::ERHIPixelFormat format);

        /// ERHITextureDimension → D3D12_RESOURCE_DIMENSION変換
        static D3D12_RESOURCE_DIMENSION ConvertDimension(NS::RHI::ERHITextureDimension dim);

        /// ERHITextureUsage → D3D12_RESOURCE_FLAGS変換
        static D3D12_RESOURCE_FLAGS ConvertTextureFlags(NS::RHI::ERHITextureUsage usage);

    private:
        D3D12Device* device_ = nullptr;
        D3D12GpuResource gpuResource_;

        // キャッシュされたプロパティ
        NS::RHI::ERHITextureDimension dimension_ = NS::RHI::ERHITextureDimension::Texture2D;
        uint32 width_ = 0;
        uint32 height_ = 0;
        uint32 depth_ = 1;
        uint32 arraySize_ = 1;
        NS::RHI::ERHIPixelFormat format_ = NS::RHI::ERHIPixelFormat::Unknown;
        uint32 mipLevels_ = 1;
        NS::RHI::ERHISampleCount sampleCount_ = NS::RHI::ERHISampleCount::Count1;
        uint32 sampleQuality_ = 0;
        NS::RHI::ERHITextureUsage usage_ = NS::RHI::ERHITextureUsage::None;
        NS::RHI::RHIClearValue clearValue_;
        D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
    };

} // namespace NS::D3D12RHI
