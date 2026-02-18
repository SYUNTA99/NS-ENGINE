/// @file D3D12Sampler.h
/// @brief D3D12 サンプラー実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHISampler.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    /// ERHIFilter → D3D12_FILTER変換（min/mag/mip + comparison組み合わせ）
    D3D12_FILTER ConvertFilter(NS::RHI::ERHIFilter minFilter,
                               NS::RHI::ERHIFilter magFilter,
                               NS::RHI::ERHIFilter mipFilter,
                               bool enableComparison);

    /// ERHITextureAddressMode → D3D12_TEXTURE_ADDRESS_MODE
    D3D12_TEXTURE_ADDRESS_MODE ConvertAddressMode(NS::RHI::ERHITextureAddressMode mode);

    /// ERHICompareFunc → D3D12_COMPARISON_FUNC
    D3D12_COMPARISON_FUNC ConvertCompareFunc(NS::RHI::ERHICompareFunc func);

    //=========================================================================
    // D3D12Sampler
    //=========================================================================

    class D3D12Sampler final : public NS::RHI::IRHISampler
    {
    public:
        bool Init(D3D12Device* device, const NS::RHI::RHISamplerDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        const NS::RHI::RHISamplerDesc& GetDesc() const override { return desc_; }
        NS::RHI::RHICPUDescriptorHandle GetCPUDescriptorHandle() const override { return cpuHandle_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::RHISamplerDesc desc_{};
        NS::RHI::RHICPUDescriptorHandle cpuHandle_{};
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    };

} // namespace NS::D3D12RHI
