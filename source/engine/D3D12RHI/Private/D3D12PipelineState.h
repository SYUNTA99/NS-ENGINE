/// @file D3D12PipelineState.h
/// @brief D3D12 パイプラインステート実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIPipelineState.h"
#include "RHI/Public/RHIPipelineState.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    D3D12_BLEND ConvertBlendFactor(NS::RHI::ERHIBlendFactor factor);
    D3D12_BLEND_OP ConvertBlendOp(NS::RHI::ERHIBlendOp op);
    D3D12_LOGIC_OP ConvertLogicOp(NS::RHI::ERHILogicOp op);
    D3D12_FILL_MODE ConvertFillMode(NS::RHI::ERHIFillMode mode);
    D3D12_CULL_MODE ConvertCullMode(NS::RHI::ERHICullMode mode);
    D3D12_STENCIL_OP ConvertStencilOp(NS::RHI::ERHIStencilOp op);
    D3D12_COMPARISON_FUNC ConvertCompareFunc(NS::RHI::ERHICompareFunc func);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(NS::RHI::ERHIPrimitiveTopologyType type);
    DXGI_FORMAT ConvertVertexFormat(NS::RHI::ERHIVertexFormat format);
    D3D12_BLEND_DESC ConvertBlendState(const NS::RHI::RHIBlendStateDesc& src);
    D3D12_RASTERIZER_DESC ConvertRasterizerState(const NS::RHI::RHIRasterizerStateDesc& src);
    D3D12_DEPTH_STENCIL_DESC ConvertDepthStencilState(const NS::RHI::RHIDepthStencilStateDesc& src);

    //=========================================================================
    // D3D12GraphicsPipelineState
    //=========================================================================

    class D3D12GraphicsPipelineState final : public NS::RHI::IRHIGraphicsPipelineState
    {
    public:
        ~D3D12GraphicsPipelineState() override = default;

        bool Init(D3D12Device* device, const NS::RHI::RHIGraphicsPipelineStateDesc& desc, const char* debugName);

        ID3D12PipelineState* GetD3DPipelineState() const { return pso_.Get(); }

        // --- IRHIGraphicsPipelineState ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHIRootSignature* GetRootSignature() const override { return rootSignature_; }
        NS::RHI::ERHIPrimitiveTopologyType GetPrimitiveTopologyType() const override { return topologyType_; }
        NS::RHI::IRHIShader* GetVertexShader() const override { return vertexShader_; }
        NS::RHI::IRHIShader* GetPixelShader() const override { return pixelShader_; }
        NS::RHI::RHIShaderBytecode GetCachedBlob() const override;

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12PipelineState> pso_;
        NS::RHI::IRHIRootSignature* rootSignature_ = nullptr;
        NS::RHI::IRHIShader* vertexShader_ = nullptr;
        NS::RHI::IRHIShader* pixelShader_ = nullptr;
        NS::RHI::ERHIPrimitiveTopologyType topologyType_ = NS::RHI::ERHIPrimitiveTopologyType::Triangle;
    };

    //=========================================================================
    // D3D12ComputePipelineState
    //=========================================================================

    class D3D12ComputePipelineState final : public NS::RHI::IRHIComputePipelineState
    {
    public:
        ~D3D12ComputePipelineState() override = default;

        bool Init(D3D12Device* device, const NS::RHI::RHIComputePipelineStateDesc& desc, const char* debugName);

        ID3D12PipelineState* GetD3DPipelineState() const { return pso_.Get(); }

        // --- IRHIComputePipelineState ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::IRHIRootSignature* GetRootSignature() const override { return rootSignature_; }
        NS::RHI::IRHIShader* GetComputeShader() const override { return computeShader_; }
        void GetThreadGroupSize(uint32& outX, uint32& outY, uint32& outZ) const override;
        NS::RHI::RHIShaderBytecode GetCachedBlob() const override;

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12PipelineState> pso_;
        NS::RHI::IRHIRootSignature* rootSignature_ = nullptr;
        NS::RHI::IRHIShader* computeShader_ = nullptr;
        uint32 threadGroupX_ = 1;
        uint32 threadGroupY_ = 1;
        uint32 threadGroupZ_ = 1;
    };

    //=========================================================================
    // D3D12InputLayout
    //=========================================================================

    class D3D12InputLayout final : public NS::RHI::IRHIInputLayout
    {
    public:
        ~D3D12InputLayout() override = default;

        bool Init(D3D12Device* device, const NS::RHI::RHIInputLayoutDesc& desc, const char* debugName);

        // --- IRHIInputLayout ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        uint32 GetElementCount() const override { return elementCount_; }
        bool GetElement(uint32 index, NS::RHI::RHIInputElementDesc& outElement) const override;
        uint32 GetStride(uint32 slot) const override;

    private:
        D3D12Device* device_ = nullptr;
        static constexpr uint32 kMaxElements = 32;
        NS::RHI::RHIInputElementDesc elements_[kMaxElements]{};
        uint32 elementCount_ = 0;
    };

} // namespace NS::D3D12RHI
