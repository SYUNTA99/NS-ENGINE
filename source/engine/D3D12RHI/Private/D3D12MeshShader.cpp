/// @file D3D12MeshShader.cpp
/// @brief D3D12 メッシュシェーダーパイプラインステート実装
#include "D3D12MeshShader.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12Texture.h"

#include <string>

namespace NS::D3D12RHI
{
    //=========================================================================
    // Pipeline State Stream サブオブジェクトテンプレート
    //=========================================================================

    // D3D12 Pipeline State Stream の各サブオブジェクトは
    // { D3D12_PIPELINE_STATE_SUBOBJECT_TYPE, T } の alignas(void*) 構造体
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
    template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubType, typename T>
    struct alignas(void*) PSOSubobject
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = SubType;
        T inner{};

        PSOSubobject() = default;
        PSOSubobject& operator=(const T& val)
        {
            inner = val;
            return *this;
        }
    };
#pragma warning(pop)

    bool D3D12MeshPipelineState::Init(D3D12Device* device, const NS::RHI::RHIMeshPipelineStateDesc& desc,
                                       const char* debugName)
    {
        if (!device)
            return false;

        device_ = device;
        amplificationShader_ = desc.amplificationShader;
        meshShader_ = desc.meshShader;
        pixelShader_ = desc.pixelShader;
        rootSignature_ = desc.rootSignature;

        if (!meshShader_ || !pixelShader_ || !rootSignature_)
        {
            LOG_ERROR("[D3D12RHI] MeshPipelineState requires meshShader, pixelShader, and rootSignature");
            return false;
        }

        auto* d3dDevice = device->GetD3DDevice5();
        if (!d3dDevice)
        {
            LOG_ERROR("[D3D12RHI] ID3D12Device5 required for mesh shader PSO");
            return false;
        }

        // MS/AS/PSバイトコード取得
        auto msBytecode = meshShader_->GetBytecode();
        auto psBytecode = pixelShader_->GetBytecode();

        auto* d3dRS = static_cast<D3D12RootSignature*>(rootSignature_);

        // 既存ヘルパーでステート変換
        D3D12_BLEND_DESC blendDesc = ConvertBlendState(desc.blendState);
        D3D12_RASTERIZER_DESC rastDesc = ConvertRasterizerState(desc.rasterizerState);
        D3D12_DEPTH_STENCIL_DESC dsDesc = ConvertDepthStencilState(desc.depthStencilState);

        // RTV/DSVフォーマット
        D3D12_RT_FORMAT_ARRAY rtvFormats{};
        rtvFormats.NumRenderTargets = desc.numRenderTargets;
        for (uint32 i = 0; i < desc.numRenderTargets; ++i)
            rtvFormats.RTFormats[i] = D3D12Texture::ConvertPixelFormat(desc.rtvFormats[i]);

        DXGI_FORMAT dsvFormat = D3D12Texture::ConvertPixelFormat(desc.dsvFormat);

        DXGI_SAMPLE_DESC sampleDesc{};
        sampleDesc.Count = desc.sampleCount;
        sampleDesc.Quality = 0;

        // Pipeline State Stream 構築（手動サブオブジェクト定義）
        using SubRS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
        using SubMS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>;
        using SubAS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>;
        using SubPS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
        using SubBlend = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
        using SubRast = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
        using SubDS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
        using SubRTV = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
        using SubDSV = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
        using SubSample = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;

        HRESULT hr;

        if (amplificationShader_)
        {
            auto asBytecode = amplificationShader_->GetBytecode();

            struct
            {
                SubRS rs;
                SubAS as;
                SubMS ms;
                SubPS ps;
                SubBlend blend;
                SubRast rast;
                SubDS ds;
                SubRTV rtv;
                SubDSV dsv;
                SubSample sample;
            } stream{};

            stream.rs = d3dRS->GetD3DRootSignature();
            stream.as = D3D12_SHADER_BYTECODE{asBytecode.data, asBytecode.size};
            stream.ms = D3D12_SHADER_BYTECODE{msBytecode.data, msBytecode.size};
            stream.ps = D3D12_SHADER_BYTECODE{psBytecode.data, psBytecode.size};
            stream.blend = blendDesc;
            stream.rast = rastDesc;
            stream.ds = dsDesc;
            stream.rtv = rtvFormats;
            stream.dsv = dsvFormat;
            stream.sample = sampleDesc;

            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
            streamDesc.SizeInBytes = sizeof(stream);
            streamDesc.pPipelineStateSubobjectStream = &stream;

            hr = d3dDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso_));
        }
        else
        {
            struct
            {
                SubRS rs;
                SubMS ms;
                SubPS ps;
                SubBlend blend;
                SubRast rast;
                SubDS ds;
                SubRTV rtv;
                SubDSV dsv;
                SubSample sample;
            } stream{};

            stream.rs = d3dRS->GetD3DRootSignature();
            stream.ms = D3D12_SHADER_BYTECODE{msBytecode.data, msBytecode.size};
            stream.ps = D3D12_SHADER_BYTECODE{psBytecode.data, psBytecode.size};
            stream.blend = blendDesc;
            stream.rast = rastDesc;
            stream.ds = dsDesc;
            stream.rtv = rtvFormats;
            stream.dsv = dsvFormat;
            stream.sample = sampleDesc;

            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
            streamDesc.SizeInBytes = sizeof(stream);
            streamDesc.pPipelineStateSubobjectStream = &stream;

            hr = d3dDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso_));
        }

        if (FAILED(hr))
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "[D3D12RHI] CreatePipelineState (MeshShader) failed: 0x%08X",
                     static_cast<unsigned>(hr));
            LOG_ERROR(buf);
            return false;
        }

        if (debugName && pso_)
        {
            int len = MultiByteToWideChar(CP_UTF8, 0, debugName, -1, nullptr, 0);
            std::wstring wideName(len - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, debugName, -1, wideName.data(), len);
            pso_->SetName(wideName.c_str());
        }

        return true;
    }

} // namespace NS::D3D12RHI
