/// @file RHICommandSignature.cpp
/// @brief コマンドシグネチャ・GPU駆動レンダリング実装
#include "RHICommandSignature.h"
#include "IRHIBuffer.h"
#include "IRHICommandContext.h"
#include "IRHIComputeContext.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIIndirectArgument
    //=========================================================================

    uint32 RHIIndirectArgument::GetByteSize() const
    {
        switch (type)
        {
        case ERHIIndirectArgumentType::Draw:
            return 16; // RHIDrawArguments
        case ERHIIndirectArgumentType::DrawIndexed:
            return 20; // RHIDrawIndexedArguments
        case ERHIIndirectArgumentType::Dispatch:
        case ERHIIndirectArgumentType::DispatchMesh:
        case ERHIIndirectArgumentType::DispatchRays:
            return 12; // 3 x uint32
        case ERHIIndirectArgumentType::VertexBufferView:
            return 16; // GPU VA (8) + size (4) + stride (4)
        case ERHIIndirectArgumentType::IndexBufferView:
            return 16; // GPU VA (8) + size (4) + format (4)
        case ERHIIndirectArgumentType::Constant:
            return constant.num32BitValues * 4;
        case ERHIIndirectArgumentType::ConstantBufferView:
        case ERHIIndirectArgumentType::ShaderResourceView:
        case ERHIIndirectArgumentType::UnorderedAccessView:
            return 8; // GPU VA
        default:
            return 0;
        }
    }

    //=========================================================================
    // RHIStandardCommandSignatures
    //=========================================================================

    namespace
    {
        IRHICommandSignature* s_drawIndexedSignature = nullptr;
        IRHICommandSignature* s_dispatchSignature = nullptr;
        IRHICommandSignature* s_dispatchMeshSignature = nullptr;
    } // namespace

    IRHICommandSignature* RHIStandardCommandSignatures::GetDrawIndexed(IRHIDevice* device)
    {
        if ((s_drawIndexedSignature == nullptr) && (device != nullptr))
        {
            auto desc = RHICommandSignatureBuilder().AddDrawIndexed().SetDebugName("Std_DrawIndexed").Build();
            s_drawIndexedSignature = device->CreateCommandSignature(desc);
        }
        return s_drawIndexedSignature;
    }

    IRHICommandSignature* RHIStandardCommandSignatures::GetDispatch(IRHIDevice* device)
    {
        if ((s_dispatchSignature == nullptr) && (device != nullptr))
        {
            auto desc = RHICommandSignatureBuilder().AddDispatch().SetDebugName("Std_Dispatch").Build();
            s_dispatchSignature = device->CreateCommandSignature(desc);
        }
        return s_dispatchSignature;
    }

    IRHICommandSignature* RHIStandardCommandSignatures::GetDispatchMesh(IRHIDevice* device)
    {
        if ((s_dispatchMeshSignature == nullptr) && (device != nullptr))
        {
            auto desc = RHICommandSignatureBuilder().AddDispatchMesh().SetDebugName("Std_DispatchMesh").Build();
            s_dispatchMeshSignature = device->CreateCommandSignature(desc);
        }
        return s_dispatchMeshSignature;
    }

    //=========================================================================
    // RHIGPUDrivenBatch
    //=========================================================================

    RHIGPUDrivenBatch::RHIGPUDrivenBatch(IRHIDevice* device, uint32 maxDraws) : m_maxDraws(maxDraws)
    {
        // 描画データバッファ
        RHIBufferDesc drawDataDesc;
        drawDataDesc.size = static_cast<uint64>(maxDraws) * sizeof(PerDrawData);
        drawDataDesc.usage = ERHIBufferUsage::ShaderResource;
        drawDataDesc.debugName = "GPUDrivenBatch_DrawData";
        m_drawDataBuffer = device->CreateBuffer(drawDataDesc);

        // Indirect引数バッファ（DrawIndexed = 20 bytes per draw）
        RHIBufferDesc argDesc;
        argDesc.size = static_cast<uint64>(maxDraws) * 20;
        argDesc.usage = ERHIBufferUsage::IndirectArgs;
        argDesc.debugName = "GPUDrivenBatch_Args";
        m_argumentBuffer = device->CreateBuffer(argDesc);

        // カウントバッファ
        RHIBufferDesc countDesc;
        countDesc.size = sizeof(uint32);
        countDesc.usage = ERHIBufferUsage::IndirectArgs;
        countDesc.debugName = "GPUDrivenBatch_Count";
        m_countBuffer = device->CreateBuffer(countDesc);
    }

    void RHIGPUDrivenBatch::UploadDrawData(const PerDrawData* data, uint32 count)
    {
        (void)data;
        (void)count;
        // バッファへのアップロードはバックエンド依存
        // m_drawDataBuffer->Map() + memcpy + Unmap()
    }

    void RHIGPUDrivenBatch::ExecuteCulling(IRHIComputeContext* context,
                                           IRHIBuffer* visibilityBuffer,
                                           IRHIBuffer* instanceBuffer)
    {
        (void)context;
        (void)visibilityBuffer;
        (void)instanceBuffer;
        // カリングコンピュートシェーダーをディスパッチ
        // 入力: m_drawDataBuffer + visibilityBuffer
        // 出力: m_argumentBuffer + m_countBuffer
    }

    void RHIGPUDrivenBatch::ExecuteDraws(IRHICommandContext* context, IRHICommandSignature* signature)
    {
        (void)context;
        (void)signature;
        // context->ExecuteIndirect(signature, m_maxDraws, m_argumentBuffer, 0, m_countBuffer, 0);
    }

    //=========================================================================
    // RHIMeshletGPURenderer
    //=========================================================================

    RHIMeshletGPURenderer::RHIMeshletGPURenderer(IRHIDevice* device, uint32 maxMeshlets) : m_maxMeshlets(maxMeshlets)
    {
        // メッシュレットデータバッファ
        RHIBufferDesc meshletDesc;
        meshletDesc.size = static_cast<uint64>(maxMeshlets) * 64; // メッシュレットあたり64 bytes想定
        meshletDesc.usage = ERHIBufferUsage::ShaderResource;
        meshletDesc.debugName = "MeshletGPU_Meshlets";
        m_meshletBuffer = device->CreateBuffer(meshletDesc);

        // 可視メッシュレットバッファ
        RHIBufferDesc visibleDesc;
        visibleDesc.size = static_cast<uint64>(maxMeshlets) * sizeof(uint32);
        visibleDesc.usage = ERHIBufferUsage::UnorderedAccess;
        visibleDesc.debugName = "MeshletGPU_Visible";
        m_visibleMeshletBuffer = device->CreateBuffer(visibleDesc);

        // Indirect引数バッファ
        RHIBufferDesc argsDesc;
        argsDesc.size = 12; // DispatchMesh引数 (3 x uint32)
        argsDesc.usage = ERHIBufferUsage::IndirectArgs;
        argsDesc.debugName = "MeshletGPU_IndirectArgs";
        m_indirectArgsBuffer = device->CreateBuffer(argsDesc);

        // 統計バッファ
        RHIBufferDesc statsDesc;
        statsDesc.size = sizeof(uint32) * 4; // visible count + culled count + etc
        statsDesc.usage = ERHIBufferUsage::UnorderedAccess;
        statsDesc.debugName = "MeshletGPU_Stats";
        m_statsBuffer = device->CreateBuffer(statsDesc);
    }

    void RHIMeshletGPURenderer::ExecuteTwoPassCulling(IRHIComputeContext* context,
                                                      const float* viewProjMatrix,
                                                      IRHITexture* hierarchicalZ)
    {
        (void)context;
        (void)viewProjMatrix;
        (void)hierarchicalZ;
        // Pass 1: フラスタムカリング（メッシュレットBBoxをViewProj変換して判定）
        // Pass 2: オクルージョンカリング（HiZバッファとの比較）
        // 結果: m_visibleMeshletBuffer + m_indirectArgsBuffer (DispatchMesh引数)
    }

    void RHIMeshletGPURenderer::ExecuteDraws(IRHICommandContext* context)
    {
        (void)context;
        // context->DispatchMeshIndirect(m_dispatchMeshSignature, m_indirectArgsBuffer, 0);
    }

    uint32 RHIMeshletGPURenderer::GetVisibleMeshletCount() 
    {
        // 統計バッファからの読み取りはバックエンド依存
        return 0;
    }

} // namespace NS::RHI
