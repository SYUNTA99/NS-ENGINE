/// @file RHIMeshPipelineState.cpp
/// @brief メッシュレットパイプラインプリセット実装
#include "RHIMeshPipelineState.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIMeshletPipelinePresets
    //=========================================================================

    RHIMeshPipelineStateDesc RHIMeshletPipelinePresets::CreateOpaque(IRHIMeshShader* meshShader,
                                                                     IRHIShader* pixelShader,
                                                                     IRHIRootSignature* rootSig)
    {
        RHIMeshPipelineStateDesc desc;
        desc.amplificationShader = nullptr;
        desc.meshShader = meshShader;
        desc.pixelShader = pixelShader;
        desc.rootSignature = rootSig;

        // 不透明デフォルト: ブレンド無効、バックフェースカリング、深度書き込み有効
        desc.blendState = {};
        desc.rasterizerState = {};
        desc.depthStencilState = {};
        desc.depthStencilState.depthTestEnable = true;
        desc.depthStencilState.depthWriteEnable = true;

        desc.numRenderTargets = 1;
        desc.rtvFormats[0] = ERHIPixelFormat::R8G8B8A8_UNORM;
        desc.dsvFormat = ERHIPixelFormat::D32_FLOAT;
        desc.sampleCount = 1;
        desc.debugName = "Opaque_MeshletPSO";

        return desc;
    }

    RHIMeshPipelineStateDesc RHIMeshletPipelinePresets::CreateWithLODSelection(IRHIAmplificationShader* lodSelectShader,
                                                                               IRHIMeshShader* meshShader,
                                                                               IRHIShader* pixelShader,
                                                                               IRHIRootSignature* rootSig)
    {
        RHIMeshPipelineStateDesc desc = CreateOpaque(meshShader, pixelShader, rootSig);
        desc.amplificationShader = lodSelectShader;
        desc.debugName = "LODSelect_MeshletPSO";
        return desc;
    }

    RHIMeshPipelineStateDesc RHIMeshletPipelinePresets::CreateWithGPUCulling(IRHIAmplificationShader* cullingShader,
                                                                             IRHIMeshShader* meshShader,
                                                                             IRHIShader* pixelShader,
                                                                             IRHIRootSignature* rootSig)
    {
        RHIMeshPipelineStateDesc desc = CreateOpaque(meshShader, pixelShader, rootSig);
        desc.amplificationShader = cullingShader;
        desc.debugName = "GPUCull_MeshletPSO";
        return desc;
    }

} // namespace NS::RHI
