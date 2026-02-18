/// @file D3D12Texture.cpp
/// @brief D3D12 テクスチャ — IRHITexture実装
#include "D3D12Texture.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // コンストラクタ / デストラクタ
    //=========================================================================

    D3D12Texture::D3D12Texture() = default;

    D3D12Texture::~D3D12Texture()
    {
        gpuResource_.Release();
        device_ = nullptr;
    }

    //=========================================================================
    // 初期化
    //=========================================================================

    bool D3D12Texture::InitFromExisting(D3D12Device* device,
                                        ComPtr<ID3D12Resource> resource,
                                        NS::RHI::ERHIPixelFormat format,
                                        NS::RHI::ERHIResourceState initialState)
    {
        if (!device || !resource)
            return false;

        device_ = device;

        // リソースデスクリプタから情報を抽出
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        width_ = static_cast<uint32>(desc.Width);
        height_ = desc.Height;
        depth_ = 1;
        arraySize_ = desc.DepthOrArraySize;
        format_ = format;
        mipLevels_ = desc.MipLevels;
        sampleCount_ = static_cast<NS::RHI::ERHISampleCount>(desc.SampleDesc.Count);
        sampleQuality_ = desc.SampleDesc.Quality;
        dimension_ = NS::RHI::ERHITextureDimension::Texture2D;
        usage_ = NS::RHI::ERHITextureUsage::RenderTarget;
        heapType_ = D3D12_HEAP_TYPE_DEFAULT;

        gpuResource_.InitFromExisting(device, std::move(resource), D3D12_HEAP_TYPE_DEFAULT, initialState, 1);

        return true;
    }

    bool D3D12Texture::Init(D3D12Device* device, const NS::RHI::RHITextureDesc& desc)
    {
        if (!device)
        {
            return false;
        }

        device_ = device;
        dimension_ = desc.dimension;
        width_ = desc.width;
        height_ = desc.height;
        format_ = desc.format;
        mipLevels_ = desc.mipLevels > 0 ? desc.mipLevels : 1;
        sampleCount_ = desc.sampleCount;
        sampleQuality_ = desc.sampleQuality;
        usage_ = desc.usage;
        clearValue_ = desc.clearValue;

        // Dimension別にdepth/arraySize設定
        using NS::RHI::ERHITextureDimension;
        switch (dimension_)
        {
        case ERHITextureDimension::Texture3D:
            depth_ = desc.depthOrArraySize;
            arraySize_ = 1;
            break;
        case ERHITextureDimension::TextureCube:
            depth_ = 1;
            arraySize_ = 6;
            break;
        case ERHITextureDimension::TextureCubeArray:
            depth_ = 1;
            arraySize_ = desc.depthOrArraySize * 6;
            break;
        case ERHITextureDimension::Texture1DArray:
        case ERHITextureDimension::Texture2DArray:
        case ERHITextureDimension::Texture2DMSArray:
            depth_ = 1;
            arraySize_ = desc.depthOrArraySize;
            break;
        default:
            depth_ = 1;
            arraySize_ = 1;
            break;
        }

        // ヒープタイプ判定
        using NS::RHI::ERHITextureUsage;
        if (EnumHasAnyFlags(usage_, ERHITextureUsage::CPUWritable))
        {
            heapType_ = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (EnumHasAnyFlags(usage_, ERHITextureUsage::CPUReadable))
        {
            heapType_ = D3D12_HEAP_TYPE_READBACK;
        }
        else
        {
            heapType_ = D3D12_HEAP_TYPE_DEFAULT;
        }

        // ヒーププロパティ
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = heapType_;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // リソースデスクリプタ
        DXGI_FORMAT dxgiFormat = ConvertPixelFormat(format_);
        D3D12_RESOURCE_DIMENSION d3dDimension = ConvertDimension(dimension_);
        D3D12_RESOURCE_FLAGS resourceFlags = ConvertTextureFlags(usage_);

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = d3dDimension;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = width_;
        resourceDesc.Height = height_;
        resourceDesc.MipLevels = static_cast<UINT16>(mipLevels_);
        resourceDesc.Format = dxgiFormat;
        resourceDesc.SampleDesc.Count = static_cast<UINT>(sampleCount_);
        resourceDesc.SampleDesc.Quality = sampleQuality_;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = resourceFlags;

        // DepthOrArraySize設定
        if (d3dDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        {
            resourceDesc.DepthOrArraySize = static_cast<UINT16>(depth_);
        }
        else
        {
            resourceDesc.DepthOrArraySize = static_cast<UINT16>(arraySize_);
        }

        // 1Dテクスチャは高さ1
        if (d3dDimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        {
            resourceDesc.Height = 1;
        }

        // 初期状態決定
        NS::RHI::ERHIResourceState initialState = NS::RHI::ERHIResourceState::Common;
        if (heapType_ == D3D12_HEAP_TYPE_UPLOAD)
        {
            initialState = NS::RHI::ERHIResourceState::GenericRead;
        }
        else if (heapType_ == D3D12_HEAP_TYPE_READBACK)
        {
            initialState = NS::RHI::ERHIResourceState::CopyDest;
        }
        else if (EnumHasAnyFlags(usage_, ERHITextureUsage::DepthStencil))
        {
            initialState = NS::RHI::ERHIResourceState::DepthWrite;
        }
        else if (EnumHasAnyFlags(usage_, ERHITextureUsage::RenderTarget))
        {
            initialState = NS::RHI::ERHIResourceState::RenderTarget;
        }

        // クリア値（RenderTarget / DepthStencil のみ）
        D3D12_CLEAR_VALUE d3dClearValue{};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;
        if (EnumHasAnyFlags(usage_, ERHITextureUsage::RenderTarget))
        {
            d3dClearValue.Format = dxgiFormat;
            d3dClearValue.Color[0] = clearValue_.color[0];
            d3dClearValue.Color[1] = clearValue_.color[1];
            d3dClearValue.Color[2] = clearValue_.color[2];
            d3dClearValue.Color[3] = clearValue_.color[3];
            pClearValue = &d3dClearValue;
        }
        else if (EnumHasAnyFlags(usage_, ERHITextureUsage::DepthStencil))
        {
            d3dClearValue.Format = dxgiFormat;
            d3dClearValue.DepthStencil.Depth = clearValue_.depth;
            d3dClearValue.DepthStencil.Stencil = clearValue_.stencil;
            pClearValue = &d3dClearValue;
        }

        // CommittedResource作成（D3D12GpuResource経由）
        if (!gpuResource_.InitCommitted(
                device_, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, initialState, pClearValue))
        {
            LOG_ERROR("[D3D12RHI] Failed to create texture resource");
            return false;
        }

        // デバッグ名
        if (desc.debugName)
        {
            SetDebugName(desc.debugName);
        }

        return true;
    }

    //=========================================================================
    // IRHITexture — 基本プロパティ
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12Texture::GetDevice() const
    {
        return device_;
    }

    //=========================================================================
    // IRHITexture — メモリ情報
    //=========================================================================

    NS::RHI::RHITextureMemoryInfo D3D12Texture::GetMemoryInfo() const
    {
        NS::RHI::RHITextureMemoryInfo info;
        info.usableSize = 0;
        info.alignment = 0;

        switch (heapType_)
        {
        case D3D12_HEAP_TYPE_UPLOAD:
            info.heapType = NS::RHI::ERHIHeapType::Upload;
            break;
        case D3D12_HEAP_TYPE_READBACK:
            info.heapType = NS::RHI::ERHIHeapType::Readback;
            break;
        default:
            info.heapType = NS::RHI::ERHIHeapType::Default;
            break;
        }

        if (gpuResource_.IsValid())
        {
            D3D12_RESOURCE_DESC resDesc = gpuResource_.GetDesc();
            auto allocInfo = device_->GetD3DDevice()->GetResourceAllocationInfo(0, 1, &resDesc);
            info.allocatedSize = static_cast<NS::RHI::MemorySize>(allocInfo.SizeInBytes);
            info.alignment = static_cast<uint32>(allocInfo.Alignment);
        }

        return info;
    }

    //=========================================================================
    // IRHITexture — サブリソースレイアウト
    //=========================================================================

    NS::RHI::RHISubresourceLayout D3D12Texture::GetSubresourceLayout(uint32 mipLevel, uint32 arraySlice) const
    {
        NS::RHI::RHISubresourceLayout layout;

        if (!gpuResource_.IsValid())
            return layout;

        uint32 subresource = mipLevel + arraySlice * mipLevels_;

        D3D12_RESOURCE_DESC desc = gpuResource_.GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        UINT64 totalBytes = 0;

        device_->GetD3DDevice()->GetCopyableFootprints(
            &desc, subresource, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

        layout.offset = static_cast<NS::RHI::MemoryOffset>(footprint.Offset);
        layout.size = static_cast<NS::RHI::MemorySize>(totalBytes);
        layout.rowPitch = footprint.Footprint.RowPitch;
        layout.depthPitch = footprint.Footprint.RowPitch * numRows;
        layout.width = footprint.Footprint.Width;
        layout.height = footprint.Footprint.Height;
        layout.depth = footprint.Footprint.Depth;

        return layout;
    }

    //=========================================================================
    // IRHITexture — Map/Unmap
    //=========================================================================

    NS::RHI::RHITextureMapResult D3D12Texture::Map(uint32 mipLevel, uint32 arraySlice, NS::RHI::ERHIMapMode mode)
    {
        NS::RHI::RHITextureMapResult result;

        if (!gpuResource_.IsValid())
            return result;

        uint32 subresource = mipLevel + arraySlice * mipLevels_;

        D3D12_RANGE readRange{};
        if (!NS::RHI::MapModeHasRead(mode))
        {
            // 書き込み専用: 空の読み取り範囲
            readRange.Begin = 0;
            readRange.End = 0;
        }
        else
        {
            // 読み取りあり: 全範囲
            readRange.Begin = 0;
            readRange.End = SIZE_T(-1);
        }

        void* mapped = nullptr;
        HRESULT hr = gpuResource_.GetD3DResource()->Map(subresource, &readRange, &mapped);
        if (FAILED(hr) || !mapped)
        {
            return result;
        }

        // レイアウト情報取得
        auto layout = GetSubresourceLayout(mipLevel, arraySlice);

        result.data = mapped;
        result.rowPitch = layout.rowPitch;
        result.depthPitch = layout.depthPitch;
        result.size = layout.size;

        return result;
    }

    void D3D12Texture::Unmap(uint32 mipLevel, uint32 arraySlice)
    {
        if (!gpuResource_.IsValid())
            return;

        uint32 subresource = mipLevel + arraySlice * mipLevels_;
        gpuResource_.GetD3DResource()->Unmap(subresource, nullptr);
    }

    //=========================================================================
    // IRHIResource — デバッグ
    //=========================================================================

    void D3D12Texture::SetDebugName(const char* name)
    {
        gpuResource_.SetDebugName(name);
    }

    //=========================================================================
    // ERHIPixelFormat → DXGI_FORMAT変換
    //=========================================================================

    DXGI_FORMAT D3D12Texture::ConvertPixelFormat(NS::RHI::ERHIPixelFormat format)
    {
        using NS::RHI::ERHIPixelFormat;
        switch (format)
        {
        // R（1チャンネル）
        case ERHIPixelFormat::R8_UNORM:
            return DXGI_FORMAT_R8_UNORM;
        case ERHIPixelFormat::R8_SNORM:
            return DXGI_FORMAT_R8_SNORM;
        case ERHIPixelFormat::R8_UINT:
            return DXGI_FORMAT_R8_UINT;
        case ERHIPixelFormat::R8_SINT:
            return DXGI_FORMAT_R8_SINT;
        case ERHIPixelFormat::R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case ERHIPixelFormat::R16_SNORM:
            return DXGI_FORMAT_R16_SNORM;
        case ERHIPixelFormat::R16_UINT:
            return DXGI_FORMAT_R16_UINT;
        case ERHIPixelFormat::R16_SINT:
            return DXGI_FORMAT_R16_SINT;
        case ERHIPixelFormat::R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case ERHIPixelFormat::R32_UINT:
            return DXGI_FORMAT_R32_UINT;
        case ERHIPixelFormat::R32_SINT:
            return DXGI_FORMAT_R32_SINT;
        case ERHIPixelFormat::R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

        // RG（2チャンネル）
        case ERHIPixelFormat::R8G8_UNORM:
            return DXGI_FORMAT_R8G8_UNORM;
        case ERHIPixelFormat::R8G8_SNORM:
            return DXGI_FORMAT_R8G8_SNORM;
        case ERHIPixelFormat::R8G8_UINT:
            return DXGI_FORMAT_R8G8_UINT;
        case ERHIPixelFormat::R8G8_SINT:
            return DXGI_FORMAT_R8G8_SINT;
        case ERHIPixelFormat::R16G16_UNORM:
            return DXGI_FORMAT_R16G16_UNORM;
        case ERHIPixelFormat::R16G16_SNORM:
            return DXGI_FORMAT_R16G16_SNORM;
        case ERHIPixelFormat::R16G16_UINT:
            return DXGI_FORMAT_R16G16_UINT;
        case ERHIPixelFormat::R16G16_SINT:
            return DXGI_FORMAT_R16G16_SINT;
        case ERHIPixelFormat::R16G16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case ERHIPixelFormat::R32G32_UINT:
            return DXGI_FORMAT_R32G32_UINT;
        case ERHIPixelFormat::R32G32_SINT:
            return DXGI_FORMAT_R32G32_SINT;
        case ERHIPixelFormat::R32G32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;

        // RGB（3チャンネル）
        case ERHIPixelFormat::R32G32B32_UINT:
            return DXGI_FORMAT_R32G32B32_UINT;
        case ERHIPixelFormat::R32G32B32_SINT:
            return DXGI_FORMAT_R32G32B32_SINT;
        case ERHIPixelFormat::R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case ERHIPixelFormat::R11G11B10_FLOAT:
            return DXGI_FORMAT_R11G11B10_FLOAT;

        // RGBA（4チャンネル）
        case ERHIPixelFormat::R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case ERHIPixelFormat::R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case ERHIPixelFormat::R8G8B8A8_SNORM:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case ERHIPixelFormat::R8G8B8A8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case ERHIPixelFormat::R8G8B8A8_SINT:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case ERHIPixelFormat::B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case ERHIPixelFormat::B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case ERHIPixelFormat::R10G10B10A2_UNORM:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case ERHIPixelFormat::R10G10B10A2_UINT:
            return DXGI_FORMAT_R10G10B10A2_UINT;
        case ERHIPixelFormat::R16G16B16A16_UNORM:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case ERHIPixelFormat::R16G16B16A16_SNORM:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case ERHIPixelFormat::R16G16B16A16_UINT:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case ERHIPixelFormat::R16G16B16A16_SINT:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case ERHIPixelFormat::R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case ERHIPixelFormat::R32G32B32A32_UINT:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case ERHIPixelFormat::R32G32B32A32_SINT:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case ERHIPixelFormat::R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        // デプス/ステンシル
        case ERHIPixelFormat::D16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case ERHIPixelFormat::D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case ERHIPixelFormat::D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        case ERHIPixelFormat::D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        // 圧縮フォーマット（BC）
        case ERHIPixelFormat::BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM;
        case ERHIPixelFormat::BC1_UNORM_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case ERHIPixelFormat::BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM;
        case ERHIPixelFormat::BC2_UNORM_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case ERHIPixelFormat::BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM;
        case ERHIPixelFormat::BC3_UNORM_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case ERHIPixelFormat::BC4_UNORM:
            return DXGI_FORMAT_BC4_UNORM;
        case ERHIPixelFormat::BC4_SNORM:
            return DXGI_FORMAT_BC4_SNORM;
        case ERHIPixelFormat::BC5_UNORM:
            return DXGI_FORMAT_BC5_UNORM;
        case ERHIPixelFormat::BC5_SNORM:
            return DXGI_FORMAT_BC5_SNORM;
        case ERHIPixelFormat::BC6H_UF16:
            return DXGI_FORMAT_BC6H_UF16;
        case ERHIPixelFormat::BC6H_SF16:
            return DXGI_FORMAT_BC6H_SF16;
        case ERHIPixelFormat::BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM;
        case ERHIPixelFormat::BC7_UNORM_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        // 特殊
        case ERHIPixelFormat::R9G9B9E5_SHAREDEXP:
            return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //=========================================================================
    // ERHITextureDimension → D3D12_RESOURCE_DIMENSION変換
    //=========================================================================

    D3D12_RESOURCE_DIMENSION D3D12Texture::ConvertDimension(NS::RHI::ERHITextureDimension dim)
    {
        using NS::RHI::ERHITextureDimension;
        switch (dim)
        {
        case ERHITextureDimension::Texture1D:
        case ERHITextureDimension::Texture1DArray:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

        case ERHITextureDimension::Texture3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

        case ERHITextureDimension::Texture2D:
        case ERHITextureDimension::Texture2DArray:
        case ERHITextureDimension::Texture2DMS:
        case ERHITextureDimension::Texture2DMSArray:
        case ERHITextureDimension::TextureCube:
        case ERHITextureDimension::TextureCubeArray:
        default:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        }
    }

    //=========================================================================
    // ERHITextureUsage → D3D12_RESOURCE_FLAGS変換
    //=========================================================================

    D3D12_RESOURCE_FLAGS D3D12Texture::ConvertTextureFlags(NS::RHI::ERHITextureUsage usage)
    {
        using NS::RHI::ERHITextureUsage;

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if (EnumHasAnyFlags(usage, ERHITextureUsage::RenderTarget))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (EnumHasAnyFlags(usage, ERHITextureUsage::DepthStencil))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            // DepthStencilでSRVが不要な場合はDENY_SHADER_RESOURCEを設定
            if (!EnumHasAnyFlags(usage, ERHITextureUsage::ShaderResource))
            {
                flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
        }

        if (EnumHasAnyFlags(usage, ERHITextureUsage::UnorderedAccess))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        return flags;
    }

} // namespace NS::D3D12RHI
