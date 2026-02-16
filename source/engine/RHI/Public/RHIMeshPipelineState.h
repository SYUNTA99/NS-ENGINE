/// @file RHIMeshPipelineState.h
/// @brief メッシュシェーダーパイプラインステートオブジェクト
/// @details メッシュPSO記述子、インターフェース、ビルダー、標準プリセットを提供。
/// @see 22-03-mesh-pso.md
#pragma once

#include "IRHIMeshShader.h"
#include "IRHIResource.h"
#include "RHIMacros.h"
#include "RHIPipelineState.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"

#include <algorithm>

namespace NS { namespace RHI {

    //=========================================================================
    // RHIMeshPipelineStateDesc (22-03)
    //=========================================================================

    /// メッシュパイプラインステート記述
    struct RHI_API RHIMeshPipelineStateDesc
    {
        //=====================================================================
        // シェーダー
        //=====================================================================

        IRHIAmplificationShader* amplificationShader = nullptr; ///< オプション
        IRHIMeshShader* meshShader = nullptr;                   ///< 必須
        /// @note pixelShaderのGetFrequency()がPixelであることをデバッグビルドで検証
        IRHIShader* pixelShader = nullptr; ///< 必須

        //=====================================================================
        // ルートシグネチャ
        //=====================================================================

        IRHIRootSignature* rootSignature = nullptr;

        //=====================================================================
        // レンダーステート
        //=====================================================================

        RHIBlendStateDesc blendState;
        RHIRasterizerStateDesc rasterizerState;
        RHIDepthStencilStateDesc depthStencilState;

        //=====================================================================
        // レンダーターゲットフォーマット
        //=====================================================================

        uint32 numRenderTargets = 1;
        ERHIPixelFormat rtvFormats[8] = {ERHIPixelFormat::RGBA8_UNORM};
        ERHIPixelFormat dsvFormat = ERHIPixelFormat::D32_Float;
        uint32 sampleCount = 1;

        //=====================================================================
        // デバッグ
        //=====================================================================

        const char* debugName = nullptr;
    };

    //=========================================================================
    // IRHIMeshPipelineState (22-03)
    //=========================================================================

    /// メッシュパイプラインステートインターフェース
    class RHI_API IRHIMeshPipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(MeshPipelineState)

        virtual ~IRHIMeshPipelineState() = default;

        /// 増幅シェーダー取得
        virtual IRHIAmplificationShader* GetAmplificationShader() const = 0;

        /// メッシュシェーダー取得
        virtual IRHIMeshShader* GetMeshShader() const = 0;

        /// ピクセルシェーダー取得
        virtual IRHIShader* GetPixelShader() const = 0;

        /// ルートシグネチャ取得
        virtual IRHIRootSignature* GetRootSignature() const = 0;
    };

    using RHIMeshPipelineStateRef = TRefCountPtr<IRHIMeshPipelineState>;

    //=========================================================================
    // RHIMeshPipelineStateBuilder (22-03)
    //=========================================================================

    /// メッシュパイプラインステートビルダー
    class RHI_API RHIMeshPipelineStateBuilder
    {
    public:
        RHIMeshPipelineStateBuilder& SetAmplificationShader(IRHIAmplificationShader* shader)
        {
            m_desc.amplificationShader = shader;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetMeshShader(IRHIMeshShader* shader)
        {
            m_desc.meshShader = shader;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetPixelShader(IRHIShader* shader)
        {
            m_desc.pixelShader = shader;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetRootSignature(IRHIRootSignature* rootSig)
        {
            m_desc.rootSignature = rootSig;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetBlendState(const RHIBlendStateDesc& state)
        {
            m_desc.blendState = state;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetRasterizerState(const RHIRasterizerStateDesc& state)
        {
            m_desc.rasterizerState = state;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetDepthStencilState(const RHIDepthStencilStateDesc& state)
        {
            m_desc.depthStencilState = state;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetRenderTargetFormat(uint32 index, ERHIPixelFormat format)
        {
            m_desc.rtvFormats[index] = format;
            m_desc.numRenderTargets = std::max(m_desc.numRenderTargets, index + 1);
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetDepthStencilFormat(ERHIPixelFormat format)
        {
            m_desc.dsvFormat = format;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetSampleCount(uint32 count)
        {
            m_desc.sampleCount = count;
            return *this;
        }

        RHIMeshPipelineStateBuilder& SetDebugName(const char* name)
        {
            m_desc.debugName = name;
            return *this;
        }

        const RHIMeshPipelineStateDesc& Build() const { return m_desc; }

    private:
        RHIMeshPipelineStateDesc m_desc;
    };

    //=========================================================================
    // RHIMeshletPipelinePresets (22-03)
    //=========================================================================

    /// メッシュレット描画用標準パイプライン設定
    namespace RHIMeshletPipelinePresets
    {
        /// 不透明メッシュレット描画
        RHI_API RHIMeshPipelineStateDesc CreateOpaque(IRHIMeshShader* meshShader,
                                                      IRHIShader* pixelShader,
                                                      IRHIRootSignature* rootSig);

        /// LOD選択付きメッシュレット描画
        RHI_API RHIMeshPipelineStateDesc CreateWithLODSelection(IRHIAmplificationShader* lodSelectShader,
                                                                IRHIMeshShader* meshShader,
                                                                IRHIShader* pixelShader,
                                                                IRHIRootSignature* rootSig);

        /// GPUカリング付きメッシュレット描画
        RHI_API RHIMeshPipelineStateDesc CreateWithGPUCulling(IRHIAmplificationShader* cullingShader,
                                                              IRHIMeshShader* meshShader,
                                                              IRHIShader* pixelShader,
                                                              IRHIRootSignature* rootSig);
    } // namespace RHIMeshletPipelinePresets

}} // namespace NS::RHI
