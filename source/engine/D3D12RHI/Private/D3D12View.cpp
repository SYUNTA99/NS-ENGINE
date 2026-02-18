/// @file D3D12View.cpp
/// @brief D3D12 ビュー実装（SRV/UAV/CBV/RTV/DSV）
#include "D3D12View.h"
#include "D3D12Buffer.h"
#include "D3D12Descriptors.h"
#include "D3D12Device.h"
#include "D3D12Texture.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // ヘルパー: テクスチャ次元→SRV ViewDimension
    //=========================================================================

    static D3D12_SRV_DIMENSION ConvertSRVDimension(NS::RHI::ERHITextureDimension dim)
    {
        using namespace NS::RHI;
        switch (dim)
        {
        case ERHITextureDimension::Texture1D:
            return D3D12_SRV_DIMENSION_TEXTURE1D;
        case ERHITextureDimension::Texture1DArray:
            return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        case ERHITextureDimension::Texture2D:
            return D3D12_SRV_DIMENSION_TEXTURE2D;
        case ERHITextureDimension::Texture2DArray:
            return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        case ERHITextureDimension::Texture2DMS:
            return D3D12_SRV_DIMENSION_TEXTURE2DMS;
        case ERHITextureDimension::Texture2DMSArray:
            return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        case ERHITextureDimension::Texture3D:
            return D3D12_SRV_DIMENSION_TEXTURE3D;
        case ERHITextureDimension::TextureCube:
            return D3D12_SRV_DIMENSION_TEXTURECUBE;
        case ERHITextureDimension::TextureCubeArray:
            return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        default:
            return D3D12_SRV_DIMENSION_TEXTURE2D;
        }
    }

    //=========================================================================
    // D3D12ShaderResourceView
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12ShaderResourceView::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12ShaderResourceView::InitFromBuffer(D3D12Device* device,
                                                 const NS::RHI::RHIBufferSRVDesc& desc,
                                                 const char* /*debugName*/)
    {
        if (!device || !desc.buffer)
            return false;

        device_ = device;
        resource_ = desc.buffer;
        isBufferView_ = true;

        auto* d3dBuf = static_cast<D3D12Buffer*>(desc.buffer);

        // SRV記述
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (desc.srvFormat)
        {
        case NS::RHI::ERHIBufferSRVFormat::Structured:
        {
            uint32 stride = desc.structureByteStride > 0 ? desc.structureByteStride : desc.buffer->GetStride();
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.Buffer.FirstElement = desc.firstElement;
            srvDesc.Buffer.NumElements =
                desc.numElements > 0 ? desc.numElements : static_cast<uint32>(desc.buffer->GetSize() / stride);
            srvDesc.Buffer.StructureByteStride = stride;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            break;
        }
        case NS::RHI::ERHIBufferSRVFormat::Raw:
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.Buffer.FirstElement = desc.firstElement;
            srvDesc.Buffer.NumElements =
                desc.numElements > 0 ? desc.numElements : static_cast<uint32>(desc.buffer->GetSize() / 4);
            srvDesc.Buffer.StructureByteStride = 0;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            break;
        case NS::RHI::ERHIBufferSRVFormat::Typed:
            srvDesc.Format = D3D12Texture::ConvertPixelFormat(desc.format);
            srvDesc.Buffer.FirstElement = desc.firstElement;
            srvDesc.Buffer.NumElements = desc.numElements;
            srvDesc.Buffer.StructureByteStride = 0;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            break;
        }

        // 暫定: 1個だけのヒープを作成してビュー作成
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateShaderResourceView(d3dBuf->GetD3DResource(), &srvDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach(); // ビューが生存する限りヒープも保持（暫定）

        return true;
    }

    bool D3D12ShaderResourceView::InitFromTexture(D3D12Device* device,
                                                  const NS::RHI::RHITextureSRVDesc& desc,
                                                  const char* /*debugName*/)
    {
        if (!device || !desc.texture)
            return false;

        device_ = device;
        resource_ = desc.texture;
        isBufferView_ = false;

        auto* d3dTex = static_cast<D3D12Texture*>(desc.texture);

        // SRV記述
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        DXGI_FORMAT format = desc.format != NS::RHI::ERHIPixelFormat::Unknown
                                 ? D3D12Texture::ConvertPixelFormat(desc.format)
                                 : D3D12Texture::ConvertPixelFormat(desc.texture->GetFormat());
        srvDesc.Format = format;

        D3D12_SRV_DIMENSION viewDim = ConvertSRVDimension(desc.dimension);
        srvDesc.ViewDimension = viewDim;

        uint32 mipLevels = desc.mipLevels > 0 ? desc.mipLevels : (desc.texture->GetMipLevels() - desc.mostDetailedMip);

        switch (viewDim)
        {
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            srvDesc.Texture2D.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.Texture2D.MipLevels = mipLevels;
            srvDesc.Texture2D.PlaneSlice = desc.planeSlice;
            srvDesc.Texture2D.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            srvDesc.Texture2DArray.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.Texture2DArray.MipLevels = mipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = desc.firstArraySlice;
            srvDesc.Texture2DArray.ArraySize = desc.arraySize > 0 ? desc.arraySize : desc.texture->GetArraySize();
            srvDesc.Texture2DArray.PlaneSlice = desc.planeSlice;
            srvDesc.Texture2DArray.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            srvDesc.TextureCube.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.TextureCube.MipLevels = mipLevels;
            srvDesc.TextureCube.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
            srvDesc.TextureCubeArray.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.TextureCubeArray.MipLevels = mipLevels;
            srvDesc.TextureCubeArray.First2DArrayFace = desc.firstArraySlice;
            srvDesc.TextureCubeArray.NumCubes = desc.arraySize > 0 ? desc.arraySize / 6 : 1;
            srvDesc.TextureCubeArray.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE3D:
            srvDesc.Texture3D.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.Texture3D.MipLevels = mipLevels;
            srvDesc.Texture3D.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1D:
            srvDesc.Texture1D.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.Texture1D.MipLevels = mipLevels;
            srvDesc.Texture1D.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
            srvDesc.Texture1DArray.MostDetailedMip = desc.mostDetailedMip;
            srvDesc.Texture1DArray.MipLevels = mipLevels;
            srvDesc.Texture1DArray.FirstArraySlice = desc.firstArraySlice;
            srvDesc.Texture1DArray.ArraySize = desc.arraySize > 0 ? desc.arraySize : 1;
            srvDesc.Texture1DArray.ResourceMinLODClamp = desc.minLODClamp;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DMS:
            break; // MSはパラメータなし
        case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
            srvDesc.Texture2DMSArray.FirstArraySlice = desc.firstArraySlice;
            srvDesc.Texture2DMSArray.ArraySize = desc.arraySize > 0 ? desc.arraySize : 1;
            break;
        default:
            break;
        }

        // 暫定: 1個ヒープでビュー作成
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateShaderResourceView(d3dTex->GetD3DResource(), &srvDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

    //=========================================================================
    // D3D12UnorderedAccessView
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12UnorderedAccessView::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12UnorderedAccessView::InitFromBuffer(D3D12Device* device,
                                                  const NS::RHI::RHIBufferUAVDesc& desc,
                                                  const char* /*debugName*/)
    {
        if (!device || !desc.buffer)
            return false;

        device_ = device;
        resource_ = desc.buffer;
        isBufferView_ = true;
        counterBuffer_ = desc.counterBuffer;
        counterOffset_ = desc.counterOffset;

        auto* d3dBuf = static_cast<D3D12Buffer*>(desc.buffer);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

        switch (desc.uavFormat)
        {
        case NS::RHI::ERHIBufferSRVFormat::Structured:
        {
            uint32 stride = desc.structureByteStride > 0 ? desc.structureByteStride : desc.buffer->GetStride();
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.Buffer.FirstElement = desc.firstElement;
            uavDesc.Buffer.NumElements =
                desc.numElements > 0 ? desc.numElements : static_cast<uint32>(desc.buffer->GetSize() / stride);
            uavDesc.Buffer.StructureByteStride = stride;
            uavDesc.Buffer.CounterOffsetInBytes = desc.counterOffset;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            break;
        }
        case NS::RHI::ERHIBufferSRVFormat::Raw:
            uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            uavDesc.Buffer.FirstElement = desc.firstElement;
            uavDesc.Buffer.NumElements =
                desc.numElements > 0 ? desc.numElements : static_cast<uint32>(desc.buffer->GetSize() / 4);
            uavDesc.Buffer.StructureByteStride = 0;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
            break;
        case NS::RHI::ERHIBufferSRVFormat::Typed:
            uavDesc.Format = D3D12Texture::ConvertPixelFormat(desc.format);
            uavDesc.Buffer.FirstElement = desc.firstElement;
            uavDesc.Buffer.NumElements = desc.numElements;
            uavDesc.Buffer.StructureByteStride = 0;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            break;
        }

        ID3D12Resource* counterResource = nullptr;
        if (desc.counterBuffer)
            counterResource = static_cast<D3D12Buffer*>(desc.counterBuffer)->GetD3DResource();

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateUnorderedAccessView(
            d3dBuf->GetD3DResource(), counterResource, &uavDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

    bool D3D12UnorderedAccessView::InitFromTexture(D3D12Device* device,
                                                   const NS::RHI::RHITextureUAVDesc& desc,
                                                   const char* /*debugName*/)
    {
        if (!device || !desc.texture)
            return false;

        device_ = device;
        resource_ = desc.texture;
        isBufferView_ = false;

        auto* d3dTex = static_cast<D3D12Texture*>(desc.texture);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        DXGI_FORMAT format = desc.format != NS::RHI::ERHIPixelFormat::Unknown
                                 ? D3D12Texture::ConvertPixelFormat(desc.format)
                                 : D3D12Texture::ConvertPixelFormat(desc.texture->GetFormat());
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = desc.mipSlice;
        uavDesc.Texture2D.PlaneSlice = desc.planeSlice;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateUnorderedAccessView(d3dTex->GetD3DResource(), nullptr, &uavDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

    //=========================================================================
    // D3D12RenderTargetView
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12RenderTargetView::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12RenderTargetView::Init(D3D12Device* device,
                                     const NS::RHI::RHIRenderTargetViewDesc& desc,
                                     const char* /*debugName*/)
    {
        if (!device || !desc.texture)
            return false;

        device_ = device;
        texture_ = desc.texture;
        format_ = desc.format != NS::RHI::ERHIPixelFormat::Unknown ? desc.format : desc.texture->GetFormat();
        mipSlice_ = desc.mipSlice;
        firstArraySlice_ = desc.firstArraySlice;
        arraySize_ = desc.arraySize > 0 ? desc.arraySize : 1;

        auto* d3dTex = static_cast<D3D12Texture*>(desc.texture);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = D3D12Texture::ConvertPixelFormat(format_);

        using namespace NS::RHI;
        switch (desc.dimension)
        {
        case ERHITextureDimension::Texture2D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = desc.mipSlice;
            rtvDesc.Texture2D.PlaneSlice = desc.planeSlice;
            break;
        case ERHITextureDimension::Texture2DArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = desc.mipSlice;
            rtvDesc.Texture2DArray.FirstArraySlice = desc.firstArraySlice;
            rtvDesc.Texture2DArray.ArraySize = arraySize_;
            rtvDesc.Texture2DArray.PlaneSlice = desc.planeSlice;
            break;
        case ERHITextureDimension::Texture2DMS:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            break;
        case ERHITextureDimension::Texture2DMSArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.FirstArraySlice = desc.firstArraySlice;
            rtvDesc.Texture2DMSArray.ArraySize = arraySize_;
            break;
        case ERHITextureDimension::Texture3D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = desc.mipSlice;
            rtvDesc.Texture3D.FirstWSlice = desc.firstWSlice;
            rtvDesc.Texture3D.WSize = desc.wSize > 0 ? desc.wSize : static_cast<uint32>(-1);
            break;
        case ERHITextureDimension::Texture1D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtvDesc.Texture1D.MipSlice = desc.mipSlice;
            break;
        case ERHITextureDimension::Texture1DArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtvDesc.Texture1DArray.MipSlice = desc.mipSlice;
            rtvDesc.Texture1DArray.FirstArraySlice = desc.firstArraySlice;
            rtvDesc.Texture1DArray.ArraySize = arraySize_;
            break;
        default:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = desc.mipSlice;
            break;
        }

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateRenderTargetView(d3dTex->GetD3DResource(), &rtvDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

    //=========================================================================
    // D3D12DepthStencilView
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12DepthStencilView::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12DepthStencilView::Init(D3D12Device* device,
                                     const NS::RHI::RHIDepthStencilViewDesc& desc,
                                     const char* /*debugName*/)
    {
        if (!device || !desc.texture)
            return false;

        device_ = device;
        texture_ = desc.texture;
        format_ = desc.format != NS::RHI::ERHIPixelFormat::Unknown ? desc.format : desc.texture->GetFormat();
        flags_ = desc.flags;
        mipSlice_ = desc.mipSlice;
        firstArraySlice_ = desc.firstArraySlice;
        arraySize_ = desc.arraySize > 0 ? desc.arraySize : 1;

        auto* d3dTex = static_cast<D3D12Texture*>(desc.texture);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = D3D12Texture::ConvertPixelFormat(format_);
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        if (EnumHasAnyFlags(desc.flags, NS::RHI::ERHIDSVFlags::ReadOnlyDepth))
            dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
        if (EnumHasAnyFlags(desc.flags, NS::RHI::ERHIDSVFlags::ReadOnlyStencil))
            dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

        using namespace NS::RHI;
        switch (desc.dimension)
        {
        case ERHITextureDimension::Texture2D:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = desc.mipSlice;
            break;
        case ERHITextureDimension::Texture2DArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = desc.mipSlice;
            dsvDesc.Texture2DArray.FirstArraySlice = desc.firstArraySlice;
            dsvDesc.Texture2DArray.ArraySize = arraySize_;
            break;
        case ERHITextureDimension::Texture2DMS:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            break;
        case ERHITextureDimension::Texture2DMSArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDesc.Texture2DMSArray.FirstArraySlice = desc.firstArraySlice;
            dsvDesc.Texture2DMSArray.ArraySize = arraySize_;
            break;
        case ERHITextureDimension::Texture1D:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsvDesc.Texture1D.MipSlice = desc.mipSlice;
            break;
        case ERHITextureDimension::Texture1DArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsvDesc.Texture1DArray.MipSlice = desc.mipSlice;
            dsvDesc.Texture1DArray.FirstArraySlice = desc.firstArraySlice;
            dsvDesc.Texture1DArray.ArraySize = arraySize_;
            break;
        default:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = desc.mipSlice;
            break;
        }

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateDepthStencilView(d3dTex->GetD3DResource(), &dsvDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

    //=========================================================================
    // D3D12ConstantBufferView
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12ConstantBufferView::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12ConstantBufferView::Init(D3D12Device* device,
                                       const NS::RHI::RHIConstantBufferViewDesc& desc,
                                       const char* /*debugName*/)
    {
        if (!device)
            return false;

        device_ = device;
        buffer_ = desc.buffer;
        offset_ = desc.offset;

        // GPUアドレスとサイズ決定
        uint64 gpuAddress = 0;
        uint64 viewSize = 0;

        if (desc.buffer)
        {
            auto* d3dBuf = static_cast<D3D12Buffer*>(desc.buffer);
            gpuAddress = d3dBuf->GetGPUVirtualAddress() + desc.offset;
            viewSize = desc.size > 0 ? desc.size : desc.buffer->GetSize() - desc.offset;
        }
        else
        {
            gpuAddress = desc.gpuAddress;
            viewSize = desc.size;
        }

        size_ = viewSize;
        m_gpuVirtualAddress = gpuAddress;

        // CBV記述
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = gpuAddress;
        cbvDesc.SizeInBytes = static_cast<UINT>((viewSize + 255) & ~255); // 256バイトアライメント

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
            return false;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateConstantBufferView(&cbvDesc, cpuHandle);

        m_cpuHandle.ptr = cpuHandle.ptr;
        heap.Detach();

        return true;
    }

} // namespace NS::D3D12RHI
