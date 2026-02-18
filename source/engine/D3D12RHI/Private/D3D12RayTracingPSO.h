/// @file D3D12RayTracingPSO.h
/// @brief D3D12 レイトレーシング Pipeline State Object
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIRaytracingPSO.h"
#include "RHI/Public/RHIRaytracingShader.h"

namespace NS::D3D12RHI
{
    class D3D12Device;
    class D3D12RootSignature;

    //=========================================================================
    // D3D12RaytracingPipelineState — IRHIRaytracingPipelineState実装
    //=========================================================================

    class D3D12RaytracingPipelineState final : public NS::RHI::IRHIRaytracingPipelineState
    {
    public:
        D3D12RaytracingPipelineState() = default;
        ~D3D12RaytracingPipelineState() override = default;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIRaytracingPipelineStateDesc& desc, const char* debugName);

        // --- IRHIRaytracingPipelineState ---
        NS::RHI::RHIShaderIdentifier GetShaderIdentifier(const char* exportName) const override;
        uint32 GetMaxPayloadSize() const override { return maxPayloadSize_; }
        uint32 GetMaxAttributeSize() const override { return maxAttributeSize_; }
        uint32 GetMaxRecursionDepth() const override { return maxRecursionDepth_; }
        NS::RHI::IRHIRootSignature* GetGlobalRootSignature() const override { return globalRootSignature_; }

        /// D3D12ネイティブ取得
        ID3D12StateObject* GetStateObject() const { return stateObject_.Get(); }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12StateObject> stateObject_;
        ComPtr<ID3D12StateObjectProperties> properties_;
        NS::RHI::IRHIRootSignature* globalRootSignature_ = nullptr;
        uint32 maxPayloadSize_ = 0;
        uint32 maxAttributeSize_ = 0;
        uint32 maxRecursionDepth_ = 0;
    };

} // namespace NS::D3D12RHI
