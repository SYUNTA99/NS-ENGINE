/// @file D3D12View.h
/// @brief D3D12 ビュー実装（SRV/UAV/CBV/RTV/DSV）
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIViews.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12ShaderResourceView
    //=========================================================================

    class D3D12ShaderResourceView final : public NS::RHI::IRHIShaderResourceView
    {
    public:
        bool InitFromBuffer(D3D12Device* device, const NS::RHI::RHIBufferSRVDesc& desc, const char* debugName);
        bool InitFromTexture(D3D12Device* device, const NS::RHI::RHITextureSRVDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHIResource* GetResource() const override { return resource_; }
        bool IsBufferView() const override { return isBufferView_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHIResource* resource_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        bool isBufferView_ = false;
    };

    //=========================================================================
    // D3D12UnorderedAccessView
    //=========================================================================

    class D3D12UnorderedAccessView final : public NS::RHI::IRHIUnorderedAccessView
    {
    public:
        bool InitFromBuffer(D3D12Device* device, const NS::RHI::RHIBufferUAVDesc& desc, const char* debugName);
        bool InitFromTexture(D3D12Device* device, const NS::RHI::RHITextureUAVDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHIResource* GetResource() const override { return resource_; }
        bool IsBufferView() const override { return isBufferView_; }
        bool HasCounter() const override { return counterBuffer_ != nullptr; }
        NS::RHI::IRHIBuffer* GetCounterResource() const override { return counterBuffer_; }
        uint64 GetCounterOffset() const override { return counterOffset_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHIResource* resource_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        NS::RHI::IRHIBuffer* counterBuffer_ = nullptr;
        uint64 counterOffset_ = 0;
        bool isBufferView_ = false;
    };

    //=========================================================================
    // D3D12RenderTargetView
    //=========================================================================

    class D3D12RenderTargetView final : public NS::RHI::IRHIRenderTargetView
    {
    public:
        bool Init(D3D12Device* device, const NS::RHI::RHIRenderTargetViewDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHITexture* GetTexture() const override { return texture_; }
        uint32 GetMipSlice() const override { return mipSlice_; }
        uint32 GetFirstArraySlice() const override { return firstArraySlice_; }
        uint32 GetArraySize() const override { return arraySize_; }
        NS::RHI::ERHIPixelFormat GetFormat() const override { return format_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHITexture* texture_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        NS::RHI::ERHIPixelFormat format_ = NS::RHI::ERHIPixelFormat::Unknown;
        uint32 mipSlice_ = 0;
        uint32 firstArraySlice_ = 0;
        uint32 arraySize_ = 0;
    };

    //=========================================================================
    // D3D12DepthStencilView
    //=========================================================================

    class D3D12DepthStencilView final : public NS::RHI::IRHIDepthStencilView
    {
    public:
        bool Init(D3D12Device* device, const NS::RHI::RHIDepthStencilViewDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHITexture* GetTexture() const override { return texture_; }
        uint32 GetMipSlice() const override { return mipSlice_; }
        uint32 GetFirstArraySlice() const override { return firstArraySlice_; }
        uint32 GetArraySize() const override { return arraySize_; }
        NS::RHI::ERHIPixelFormat GetFormat() const override { return format_; }
        NS::RHI::ERHIDSVFlags GetFlags() const override { return flags_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHITexture* texture_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        NS::RHI::ERHIPixelFormat format_ = NS::RHI::ERHIPixelFormat::Unknown;
        NS::RHI::ERHIDSVFlags flags_ = NS::RHI::ERHIDSVFlags::None;
        uint32 mipSlice_ = 0;
        uint32 firstArraySlice_ = 0;
        uint32 arraySize_ = 0;
    };

    //=========================================================================
    // D3D12ConstantBufferView
    //=========================================================================

    class D3D12ConstantBufferView final : public NS::RHI::IRHIConstantBufferView
    {
    public:
        bool Init(D3D12Device* device, const NS::RHI::RHIConstantBufferViewDesc& desc, const char* debugName);

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHIBuffer* GetBuffer() const override { return buffer_; }
        NS::RHI::MemoryOffset GetOffset() const override { return offset_; }
        NS::RHI::MemorySize GetSize() const override { return size_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHIBuffer* buffer_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        NS::RHI::MemoryOffset offset_ = 0;
        NS::RHI::MemorySize size_ = 0;
    };

} // namespace NS::D3D12RHI
