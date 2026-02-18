/// @file D3D12Descriptors.h
/// @brief D3D12 ディスクリプタヒープ + アロケータ
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIDescriptorHeap.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12DescriptorHeap — IRHIDescriptorHeap実装
    //=========================================================================

    class D3D12DescriptorHeap final : public NS::RHI::IRHIDescriptorHeap
    {
    public:
        D3D12DescriptorHeap();
        ~D3D12DescriptorHeap() override = default;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIDescriptorHeapDesc& desc, const char* debugName);

        /// 既存ネイティブヒープをラップして初期化
        bool InitFromExisting(D3D12Device* device, ID3D12DescriptorHeap* existingHeap,
                              NS::RHI::ERHIDescriptorHeapType type, uint32 numDescriptors, bool shaderVisible);

        // --- IRHIDescriptorHeap ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIDescriptorHeapType GetType() const override { return type_; }
        uint32 GetNumDescriptors() const override { return numDescriptors_; }
        bool IsShaderVisible() const override { return shaderVisible_; }
        uint32 GetDescriptorIncrementSize() const override { return incrementSize_; }
        NS::RHI::RHICPUDescriptorHandle GetCPUDescriptorHandleForHeapStart() const override;
        NS::RHI::RHIGPUDescriptorHandle GetGPUDescriptorHandleForHeapStart() const override;

        /// ネイティブヒープ取得
        ID3D12DescriptorHeap* GetD3DHeap() const { return heap_.Get(); }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12DescriptorHeap> heap_;
        NS::RHI::ERHIDescriptorHeapType type_ = NS::RHI::ERHIDescriptorHeapType::CBV_SRV_UAV;
        uint32 numDescriptors_ = 0;
        uint32 incrementSize_ = 0;
        bool shaderVisible_ = false;
    };

    //=========================================================================
    // ヘルパー: ERHIDescriptorHeapType → D3D12_DESCRIPTOR_HEAP_TYPE
    //=========================================================================

    inline D3D12_DESCRIPTOR_HEAP_TYPE ConvertDescriptorHeapType(NS::RHI::ERHIDescriptorHeapType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIDescriptorHeapType::CBV_SRV_UAV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        case NS::RHI::ERHIDescriptorHeapType::Sampler:
            return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        case NS::RHI::ERHIDescriptorHeapType::RTV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        case NS::RHI::ERHIDescriptorHeapType::DSV:
            return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        default:
            return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        }
    }

} // namespace NS::D3D12RHI
