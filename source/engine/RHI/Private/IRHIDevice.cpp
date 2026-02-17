/// @file IRHIDevice.cpp
/// @brief IRHIDevice 便利メソッド実装
#include "IRHIDevice.h"
#include "IRHIBuffer.h"
#include "IRHIFence.h"
#include "IRHIPipelineState.h"
#include "IRHIShader.h"
#include "IRHITexture.h"
#include "IRHIViews.h"
#include "RHIValidation.h"

namespace NS::RHI
{
    //=========================================================================
    // デフォルトビュー作成
    //=========================================================================

    IRHIShaderResourceView* IRHIDevice::CreateDefaultSRV(IRHITexture* texture, const char* debugName)
    {
        return CreateShaderResourceView(RHITextureSRVDesc::Default(texture), debugName);
    }

    IRHIShaderResourceView* IRHIDevice::CreateDefaultSRV(IRHIBuffer* buffer, const char* debugName)
    {
        return CreateShaderResourceView(RHIBufferSRVDesc::Structured(buffer), debugName);
    }

    IRHIUnorderedAccessView* IRHIDevice::CreateDefaultUAV(IRHIBuffer* buffer, const char* debugName)
    {
        return CreateUnorderedAccessView(RHIBufferUAVDesc::Structured(buffer), debugName);
    }

    IRHIUnorderedAccessView* IRHIDevice::CreateDefaultUAV(IRHITexture* texture, uint32 mipSlice, const char* debugName)
    {
        return CreateUnorderedAccessView(RHITextureUAVDesc::Default(texture, mipSlice), debugName);
    }

    IRHIRenderTargetView* IRHIDevice::CreateDefaultRTV(IRHITexture* texture, const char* debugName)
    {
        return CreateRenderTargetView(RHIRenderTargetViewDesc::Texture2D(texture), debugName);
    }

    IRHIDepthStencilView* IRHIDevice::CreateDefaultDSV(IRHITexture* texture, const char* debugName)
    {
        return CreateDepthStencilView(RHIDepthStencilViewDesc::Texture2D(texture), debugName);
    }

    IRHIDepthStencilView* IRHIDevice::CreateReadOnlyDSV(IRHITexture* texture, const char* debugName)
    {
        return CreateDepthStencilView(RHIDepthStencilViewDesc::Texture2DReadOnly(texture), debugName);
    }

    IRHIConstantBufferView* IRHIDevice::CreateDefaultCBV(IRHIBuffer* buffer, const char* debugName)
    {
        return CreateConstantBufferView(RHIConstantBufferViewDesc::FromBuffer(buffer), debugName);
    }

    //=========================================================================
    // シェーダー作成便利関数
    //=========================================================================

    IRHIShader* IRHIDevice::CreateVertexShader(const RHIShaderBytecode& bytecode,
                                               const char* entryPoint,
                                               const char* debugName)
    {
        return CreateShader(RHIShaderDesc::Vertex(bytecode, entryPoint), debugName);
    }

    IRHIShader* IRHIDevice::CreatePixelShader(const RHIShaderBytecode& bytecode,
                                              const char* entryPoint,
                                              const char* debugName)
    {
        return CreateShader(RHIShaderDesc::Pixel(bytecode, entryPoint), debugName);
    }

    IRHIShader* IRHIDevice::CreateComputeShader(const RHIShaderBytecode& bytecode,
                                                const char* entryPoint,
                                                const char* debugName)
    {
        return CreateShader(RHIShaderDesc::Compute(bytecode, entryPoint), debugName);
    }

    //=========================================================================
    // パイプライン作成便利関数
    //=========================================================================

    IRHIComputePipelineState* IRHIDevice::CreateComputePipelineState(IRHIShader* computeShader,
                                                                     IRHIRootSignature* rootSignature,
                                                                     const char* debugName)
    {
        return CreateComputePipelineState(RHIComputePipelineStateDesc::Create(computeShader, rootSignature), debugName);
    }

    //=========================================================================
    // フェンス作成便利関数
    //=========================================================================

    IRHIFence* IRHIDevice::CreateFence(uint64 initialValue, const char* debugName)
    {
        RHIFenceDesc desc;
        desc.initialValue = initialValue;
        return CreateFence(desc, debugName);
    }

    //=========================================================================
    // 検証
    //=========================================================================

    bool IRHIDevice::IsValidationEnabled() const
    {
        return GetValidationLevel() != ERHIValidationLevel::Disabled;
    }

} // namespace NS::RHI
