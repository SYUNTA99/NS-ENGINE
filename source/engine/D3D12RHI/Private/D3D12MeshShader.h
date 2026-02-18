/// @file D3D12MeshShader.h
/// @brief D3D12 メッシュシェーダーパイプラインステート
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIMeshPipelineState.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12MeshPipelineState — IRHIMeshPipelineState実装
    //=========================================================================

    class D3D12MeshPipelineState final : public NS::RHI::IRHIMeshPipelineState
    {
    public:
        ~D3D12MeshPipelineState() override = default;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIMeshPipelineStateDesc& desc, const char* debugName);

        /// D3D12ネイティブ取得
        ID3D12PipelineState* GetD3DPipelineState() const { return pso_.Get(); }

        // --- IRHIMeshPipelineState ---
        NS::RHI::IRHIAmplificationShader* GetAmplificationShader() const override { return amplificationShader_; }
        NS::RHI::IRHIMeshShader* GetMeshShader() const override { return meshShader_; }
        NS::RHI::IRHIShader* GetPixelShader() const override { return pixelShader_; }
        NS::RHI::IRHIRootSignature* GetRootSignature() const override { return rootSignature_; }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12PipelineState> pso_;
        NS::RHI::IRHIAmplificationShader* amplificationShader_ = nullptr;
        NS::RHI::IRHIMeshShader* meshShader_ = nullptr;
        NS::RHI::IRHIShader* pixelShader_ = nullptr;
        NS::RHI::IRHIRootSignature* rootSignature_ = nullptr;
    };

} // namespace NS::D3D12RHI
