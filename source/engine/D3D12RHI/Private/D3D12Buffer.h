/// @file D3D12Buffer.h
/// @brief D3D12 バッファ — IRHIBuffer実装
#pragma once

#include "D3D12Resource.h"
#include "RHI/Public/IRHIBuffer.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12Buffer — IRHIBuffer実装
    //=========================================================================

    class D3D12Buffer final : public NS::RHI::IRHIBuffer
    {
    public:
        D3D12Buffer();
        ~D3D12Buffer() override;

        /// バッファ作成
        bool Init(D3D12Device* device, const NS::RHI::RHIBufferDesc& desc, const void* initialData);

        /// ネイティブリソース取得
        D3D12GpuResource& GetGpuResource() { return gpuResource_; }
        const D3D12GpuResource& GetGpuResource() const { return gpuResource_; }
        ID3D12Resource* GetD3DResource() const { return gpuResource_.GetD3DResource(); }

        //=====================================================================
        // IRHIBuffer — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIBufferUsage GetUsage() const override { return usage_; }

        //=====================================================================
        // IRHIBuffer — メモリ情報
        //=====================================================================

        NS::RHI::RHIBufferMemoryInfo GetMemoryInfo() const override;

        //=====================================================================
        // IRHIBuffer — Map/Unmap
        //=====================================================================

        NS::RHI::RHIMapResult Map(NS::RHI::ERHIMapMode mode,
                                  NS::RHI::MemoryOffset offset,
                                  NS::RHI::MemorySize size) override;
        void Unmap(NS::RHI::MemoryOffset offset, NS::RHI::MemorySize size) override;
        bool IsMapped() const override { return gpuResource_.IsMapped(); }

        //=====================================================================
        // IRHIResource — デバッグ
        //=====================================================================

        void SetDebugName(const char* name) override;

    private:
        D3D12Device* device_ = nullptr;
        D3D12GpuResource gpuResource_;
        NS::RHI::ERHIBufferUsage usage_ = NS::RHI::ERHIBufferUsage::None;
        D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;

        /// ERHIBufferUsage → D3D12_HEAP_TYPE判定
        static D3D12_HEAP_TYPE DetermineHeapType(NS::RHI::ERHIBufferUsage usage);

        /// ERHIBufferUsage → D3D12_RESOURCE_FLAGS変換
        static D3D12_RESOURCE_FLAGS ConvertResourceFlags(NS::RHI::ERHIBufferUsage usage);
    };

} // namespace NS::D3D12RHI
