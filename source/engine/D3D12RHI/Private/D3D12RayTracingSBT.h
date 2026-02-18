/// @file D3D12RayTracingSBT.h
/// @brief D3D12 シェーダーバインディングテーブル
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIRaytracingShader.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12ShaderBindingTable — IRHIShaderBindingTable実装
    //=========================================================================

    class D3D12ShaderBindingTable final : public NS::RHI::IRHIShaderBindingTable
    {
    public:
        D3D12ShaderBindingTable() = default;
        ~D3D12ShaderBindingTable() override;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIShaderBindingTableDesc& desc, const char* debugName);

        // --- IRHIShaderBindingTable ---
        NS::RHI::RHIShaderTableRegion GetRayGenRegion() const override;
        NS::RHI::RHIShaderTableRegion GetMissRegion() const override;
        NS::RHI::RHIShaderTableRegion GetHitGroupRegion() const override;
        NS::RHI::RHIShaderTableRegion GetCallableRegion() const override;

        void SetRayGenRecord(uint32 index, const NS::RHI::RHIShaderRecord& record) override;
        void SetMissRecord(uint32 index, const NS::RHI::RHIShaderRecord& record) override;
        void SetHitGroupRecord(uint32 index, const NS::RHI::RHIShaderRecord& record) override;
        void SetCallableRecord(uint32 index, const NS::RHI::RHIShaderRecord& record) override;

        NS::RHI::IRHIBuffer* GetBuffer() const override { return nullptr; }
        uint64 GetTotalSize() const override { return totalSize_; }

    private:
        void WriteRecord(uint64 regionOffset, uint32 index, const NS::RHI::RHIShaderRecord& record);

        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12Resource> buffer_;
        uint8* mappedData_ = nullptr;
        uint64 gpuBaseAddress_ = 0;
        uint32 recordStride_ = 0;
        uint64 totalSize_ = 0;

        // 各領域のオフセットとレコード数
        uint64 rayGenOffset_ = 0;
        uint32 rayGenCount_ = 0;
        uint64 missOffset_ = 0;
        uint32 missCount_ = 0;
        uint64 hitGroupOffset_ = 0;
        uint32 hitGroupCount_ = 0;
        uint64 callableOffset_ = 0;
        uint32 callableCount_ = 0;
    };

} // namespace NS::D3D12RHI
