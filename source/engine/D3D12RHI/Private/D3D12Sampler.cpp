/// @file D3D12Sampler.cpp
/// @brief D3D12 サンプラー実装
#include "D3D12Sampler.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    D3D12_FILTER ConvertFilter(NS::RHI::ERHIFilter minFilter,
                               NS::RHI::ERHIFilter magFilter,
                               NS::RHI::ERHIFilter mipFilter,
                               bool enableComparison)
    {
        using namespace NS::RHI;

        // Anisotropic check
        if (minFilter == ERHIFilter::Anisotropic || magFilter == ERHIFilter::Anisotropic)
        {
            return enableComparison ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
        }

        // D3D12_FILTER_TYPE: 0=POINT, 1=LINEAR
        UINT min = (minFilter == ERHIFilter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
        UINT mag = (magFilter == ERHIFilter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
        UINT mip = (mipFilter == ERHIFilter::Linear) ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
        UINT reduction =
            enableComparison ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;

        return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, reduction);
    }

    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(NS::RHI::ERHITextureAddressMode mode)
    {
        switch (mode)
        {
        case NS::RHI::ERHITextureAddressMode::Wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case NS::RHI::ERHITextureAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case NS::RHI::ERHITextureAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case NS::RHI::ERHITextureAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case NS::RHI::ERHITextureAddressMode::MirrorOnce:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    D3D12_COMPARISON_FUNC ConvertCompareFunc(NS::RHI::ERHICompareFunc func)
    {
        switch (func)
        {
        case NS::RHI::ERHICompareFunc::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case NS::RHI::ERHICompareFunc::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case NS::RHI::ERHICompareFunc::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case NS::RHI::ERHICompareFunc::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case NS::RHI::ERHICompareFunc::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case NS::RHI::ERHICompareFunc::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case NS::RHI::ERHICompareFunc::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case NS::RHI::ERHICompareFunc::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_NEVER;
        }
    }

    //=========================================================================
    // D3D12Sampler
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12Sampler::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    bool D3D12Sampler::Init(D3D12Device* device, const NS::RHI::RHISamplerDesc& desc, const char* /*debugName*/)
    {
        if (!device)
            return false;

        device_ = device;
        desc_ = desc;

        // RHISamplerDesc → D3D12_SAMPLER_DESC
        D3D12_SAMPLER_DESC d3dDesc{};
        d3dDesc.Filter = ConvertFilter(desc.minFilter, desc.magFilter, desc.mipFilter, desc.enableComparison);
        d3dDesc.AddressU = ConvertAddressMode(desc.addressU);
        d3dDesc.AddressV = ConvertAddressMode(desc.addressV);
        d3dDesc.AddressW = ConvertAddressMode(desc.addressW);
        d3dDesc.MipLODBias = desc.mipLODBias;
        d3dDesc.MaxAnisotropy = desc.maxAnisotropy;
        d3dDesc.ComparisonFunc = ConvertCompareFunc(desc.comparisonFunc);
        d3dDesc.MinLOD = desc.minLOD;
        d3dDesc.MaxLOD = desc.maxLOD;

        // ボーダーカラー
        if (desc.useCustomBorderColor)
        {
            d3dDesc.BorderColor[0] = desc.customBorderColor[0];
            d3dDesc.BorderColor[1] = desc.customBorderColor[1];
            d3dDesc.BorderColor[2] = desc.customBorderColor[2];
            d3dDesc.BorderColor[3] = desc.customBorderColor[3];
        }
        else
        {
            switch (desc.borderColor)
            {
            case NS::RHI::ERHIBorderColor::TransparentBlack:
                d3dDesc.BorderColor[0] = d3dDesc.BorderColor[1] = d3dDesc.BorderColor[2] = d3dDesc.BorderColor[3] =
                    0.0f;
                break;
            case NS::RHI::ERHIBorderColor::OpaqueBlack:
                d3dDesc.BorderColor[0] = d3dDesc.BorderColor[1] = d3dDesc.BorderColor[2] = 0.0f;
                d3dDesc.BorderColor[3] = 1.0f;
                break;
            case NS::RHI::ERHIBorderColor::OpaqueWhite:
                d3dDesc.BorderColor[0] = d3dDesc.BorderColor[1] = d3dDesc.BorderColor[2] = d3dDesc.BorderColor[3] =
                    1.0f;
                break;
            }
        }

        // 暫定: 1個サンプラーヒープ作成
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ComPtr<ID3D12DescriptorHeap> heap;
        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create sampler descriptor heap");
            return false;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
        device_->GetD3DDevice()->CreateSampler(&d3dDesc, cpuHandle);

        cpuHandle_.ptr = cpuHandle.ptr;
        heap.Detach(); // 暫定: ビューと同パターン

        return true;
    }

} // namespace NS::D3D12RHI
