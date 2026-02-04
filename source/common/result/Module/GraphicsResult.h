/// @file GraphicsResult.h
/// @brief グラフィックスエラーコード
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS
{

    /// グラフィックスエラー
    namespace GraphicsResult
    {

        //=========================================================================
        // デバイス関連 (1-99)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, result::ModuleId::Graphics, 1, 100);
        NS_DEFINE_ERROR_RESULT(ResultDeviceCreationFailed, result::ModuleId::Graphics, 1);
        NS_DEFINE_ERROR_RESULT(ResultDeviceLost, result::ModuleId::Graphics, 2);
        NS_DEFINE_ERROR_RESULT(ResultDeviceRemoved, result::ModuleId::Graphics, 3);
        NS_DEFINE_ERROR_RESULT(ResultDeviceNotSupported, result::ModuleId::Graphics, 4);
        NS_DEFINE_ERROR_RESULT(ResultFeatureNotSupported, result::ModuleId::Graphics, 5);
        NS_DEFINE_ERROR_RESULT(ResultDriverVersionMismatch, result::ModuleId::Graphics, 6);
        NS_DEFINE_ERROR_RESULT(ResultAdapterNotFound, result::ModuleId::Graphics, 7);
        NS_DEFINE_ERROR_RESULT(ResultDisplayModeNotSupported, result::ModuleId::Graphics, 8);

        //=========================================================================
        // リソース関連 (100-199)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultResourceError, result::ModuleId::Graphics, 100, 200);
        NS_DEFINE_ERROR_RESULT(ResultTextureCreationFailed, result::ModuleId::Graphics, 100);
        NS_DEFINE_ERROR_RESULT(ResultBufferCreationFailed, result::ModuleId::Graphics, 101);
        NS_DEFINE_ERROR_RESULT(ResultShaderCreationFailed, result::ModuleId::Graphics, 102);
        NS_DEFINE_ERROR_RESULT(ResultPipelineCreationFailed, result::ModuleId::Graphics, 103);
        NS_DEFINE_ERROR_RESULT(ResultRenderTargetCreationFailed, result::ModuleId::Graphics, 104);
        NS_DEFINE_ERROR_RESULT(ResultSamplerCreationFailed, result::ModuleId::Graphics, 105);
        NS_DEFINE_ERROR_RESULT(ResultResourceMapFailed, result::ModuleId::Graphics, 106);
        NS_DEFINE_ERROR_RESULT(ResultResourceUnmapFailed, result::ModuleId::Graphics, 107);
        NS_DEFINE_ERROR_RESULT(ResultResourceUpdateFailed, result::ModuleId::Graphics, 108);
        NS_DEFINE_ERROR_RESULT(ResultResourceBindFailed, result::ModuleId::Graphics, 109);
        NS_DEFINE_ERROR_RESULT(ResultInvalidResourceState, result::ModuleId::Graphics, 110);
        NS_DEFINE_ERROR_RESULT(ResultResourceNotResident, result::ModuleId::Graphics, 111);

        //=========================================================================
        // シェーダー関連 (200-299)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultShaderError, result::ModuleId::Graphics, 200, 300);
        NS_DEFINE_ERROR_RESULT(ResultShaderCompilationFailed, result::ModuleId::Graphics, 200);
        NS_DEFINE_ERROR_RESULT(ResultShaderLinkFailed, result::ModuleId::Graphics, 201);
        NS_DEFINE_ERROR_RESULT(ResultShaderReflectionFailed, result::ModuleId::Graphics, 202);
        NS_DEFINE_ERROR_RESULT(ResultInvalidShaderBytecode, result::ModuleId::Graphics, 203);
        NS_DEFINE_ERROR_RESULT(ResultShaderSignatureMismatch, result::ModuleId::Graphics, 204);
        NS_DEFINE_ERROR_RESULT(ResultConstantBufferMismatch, result::ModuleId::Graphics, 205);
        NS_DEFINE_ERROR_RESULT(ResultRootSignatureError, result::ModuleId::Graphics, 206);

        //=========================================================================
        // レンダリング関連 (300-399)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultRenderingError, result::ModuleId::Graphics, 300, 400);
        NS_DEFINE_ERROR_RESULT(ResultSwapChainCreationFailed, result::ModuleId::Graphics, 300);
        NS_DEFINE_ERROR_RESULT(ResultSwapChainResizeFailed, result::ModuleId::Graphics, 301);
        NS_DEFINE_ERROR_RESULT(ResultPresentFailed, result::ModuleId::Graphics, 302);
        NS_DEFINE_ERROR_RESULT(ResultFrameSkipped, result::ModuleId::Graphics, 303);
        NS_DEFINE_ERROR_RESULT(ResultCommandListError, result::ModuleId::Graphics, 304);
        NS_DEFINE_ERROR_RESULT(ResultCommandQueueError, result::ModuleId::Graphics, 305);
        NS_DEFINE_ERROR_RESULT(ResultFenceError, result::ModuleId::Graphics, 306);
        NS_DEFINE_ERROR_RESULT(ResultGpuTimeout, result::ModuleId::Graphics, 307);
        NS_DEFINE_ERROR_RESULT(ResultGpuHang, result::ModuleId::Graphics, 308);

        //=========================================================================
        // 検証関連 (400-499)
        //=========================================================================

        NS_DEFINE_ERROR_RANGE_RESULT(ResultValidationError, result::ModuleId::Graphics, 400, 500);
        NS_DEFINE_ERROR_RESULT(ResultInvalidPipelineState, result::ModuleId::Graphics, 400);
        NS_DEFINE_ERROR_RESULT(ResultInvalidRenderState, result::ModuleId::Graphics, 401);
        NS_DEFINE_ERROR_RESULT(ResultInvalidDescriptor, result::ModuleId::Graphics, 402);
        NS_DEFINE_ERROR_RESULT(ResultDescriptorHeapFull, result::ModuleId::Graphics, 403);
        NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, result::ModuleId::Graphics, 404);
        NS_DEFINE_ERROR_RESULT(ResultInvalidDimension, result::ModuleId::Graphics, 405);

    } // namespace GraphicsResult

} // namespace NS
