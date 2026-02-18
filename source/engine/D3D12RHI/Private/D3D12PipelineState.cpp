/// @file D3D12PipelineState.cpp
/// @brief D3D12 パイプラインステート実装
#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12Texture.h" // ConvertPixelFormat
#include "RHI/Public/IRHIShader.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    D3D12_BLEND ConvertBlendFactor(NS::RHI::ERHIBlendFactor factor)
    {
        switch (factor)
        {
        case NS::RHI::ERHIBlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case NS::RHI::ERHIBlendFactor::One:
            return D3D12_BLEND_ONE;
        case NS::RHI::ERHIBlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case NS::RHI::ERHIBlendFactor::InvSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case NS::RHI::ERHIBlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case NS::RHI::ERHIBlendFactor::InvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case NS::RHI::ERHIBlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case NS::RHI::ERHIBlendFactor::InvDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case NS::RHI::ERHIBlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case NS::RHI::ERHIBlendFactor::InvDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case NS::RHI::ERHIBlendFactor::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case NS::RHI::ERHIBlendFactor::BlendFactor:
            return D3D12_BLEND_BLEND_FACTOR;
        case NS::RHI::ERHIBlendFactor::InvBlendFactor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case NS::RHI::ERHIBlendFactor::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case NS::RHI::ERHIBlendFactor::InvSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case NS::RHI::ERHIBlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case NS::RHI::ERHIBlendFactor::InvSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ZERO;
        }
    }

    D3D12_BLEND_OP ConvertBlendOp(NS::RHI::ERHIBlendOp op)
    {
        switch (op)
        {
        case NS::RHI::ERHIBlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case NS::RHI::ERHIBlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case NS::RHI::ERHIBlendOp::RevSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case NS::RHI::ERHIBlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case NS::RHI::ERHIBlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
        }
    }

    D3D12_LOGIC_OP ConvertLogicOp(NS::RHI::ERHILogicOp op)
    {
        switch (op)
        {
        case NS::RHI::ERHILogicOp::Clear:
            return D3D12_LOGIC_OP_CLEAR;
        case NS::RHI::ERHILogicOp::Set:
            return D3D12_LOGIC_OP_SET;
        case NS::RHI::ERHILogicOp::Copy:
            return D3D12_LOGIC_OP_COPY;
        case NS::RHI::ERHILogicOp::CopyInverted:
            return D3D12_LOGIC_OP_COPY_INVERTED;
        case NS::RHI::ERHILogicOp::Noop:
            return D3D12_LOGIC_OP_NOOP;
        case NS::RHI::ERHILogicOp::Invert:
            return D3D12_LOGIC_OP_INVERT;
        case NS::RHI::ERHILogicOp::And:
            return D3D12_LOGIC_OP_AND;
        case NS::RHI::ERHILogicOp::Nand:
            return D3D12_LOGIC_OP_NAND;
        case NS::RHI::ERHILogicOp::Or:
            return D3D12_LOGIC_OP_OR;
        case NS::RHI::ERHILogicOp::Nor:
            return D3D12_LOGIC_OP_NOR;
        case NS::RHI::ERHILogicOp::Xor:
            return D3D12_LOGIC_OP_XOR;
        case NS::RHI::ERHILogicOp::Equiv:
            return D3D12_LOGIC_OP_EQUIV;
        case NS::RHI::ERHILogicOp::AndReverse:
            return D3D12_LOGIC_OP_AND_REVERSE;
        case NS::RHI::ERHILogicOp::AndInverted:
            return D3D12_LOGIC_OP_AND_INVERTED;
        case NS::RHI::ERHILogicOp::OrReverse:
            return D3D12_LOGIC_OP_OR_REVERSE;
        case NS::RHI::ERHILogicOp::OrInverted:
            return D3D12_LOGIC_OP_OR_INVERTED;
        default:
            return D3D12_LOGIC_OP_NOOP;
        }
    }

    D3D12_FILL_MODE ConvertFillMode(NS::RHI::ERHIFillMode mode)
    {
        switch (mode)
        {
        case NS::RHI::ERHIFillMode::Wireframe:
            return D3D12_FILL_MODE_WIREFRAME;
        case NS::RHI::ERHIFillMode::Solid:
            return D3D12_FILL_MODE_SOLID;
        default:
            return D3D12_FILL_MODE_SOLID;
        }
    }

    D3D12_CULL_MODE ConvertCullMode(NS::RHI::ERHICullMode mode)
    {
        switch (mode)
        {
        case NS::RHI::ERHICullMode::None:
            return D3D12_CULL_MODE_NONE;
        case NS::RHI::ERHICullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case NS::RHI::ERHICullMode::Back:
            return D3D12_CULL_MODE_BACK;
        default:
            return D3D12_CULL_MODE_NONE;
        }
    }

    D3D12_STENCIL_OP ConvertStencilOp(NS::RHI::ERHIStencilOp op)
    {
        switch (op)
        {
        case NS::RHI::ERHIStencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case NS::RHI::ERHIStencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case NS::RHI::ERHIStencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case NS::RHI::ERHIStencilOp::IncrSat:
            return D3D12_STENCIL_OP_INCR_SAT;
        case NS::RHI::ERHIStencilOp::DecrSat:
            return D3D12_STENCIL_OP_DECR_SAT;
        case NS::RHI::ERHIStencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case NS::RHI::ERHIStencilOp::IncrWrap:
            return D3D12_STENCIL_OP_INCR;
        case NS::RHI::ERHIStencilOp::DecrWrap:
            return D3D12_STENCIL_OP_DECR;
        default:
            return D3D12_STENCIL_OP_KEEP;
        }
    }

    D3D12_COMPARISON_FUNC ConvertCompareFunc(NS::RHI::ERHICompareFunc func)
    {
        switch (func)
        {
        case NS::RHI::ERHICompareFunc::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case NS::RHI::ERHICompareFunc::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case NS::RHI::ERHICompareFunc::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case NS::RHI::ERHICompareFunc::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case NS::RHI::ERHICompareFunc::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case NS::RHI::ERHICompareFunc::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case NS::RHI::ERHICompareFunc::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case NS::RHI::ERHICompareFunc::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_NEVER;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(NS::RHI::ERHIPrimitiveTopologyType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIPrimitiveTopologyType::Point:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case NS::RHI::ERHIPrimitiveTopologyType::Line:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case NS::RHI::ERHIPrimitiveTopologyType::Triangle:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case NS::RHI::ERHIPrimitiveTopologyType::Patch:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    }

    DXGI_FORMAT ConvertVertexFormat(NS::RHI::ERHIVertexFormat format)
    {
        switch (format)
        {
        case NS::RHI::ERHIVertexFormat::Float1:
            return DXGI_FORMAT_R32_FLOAT;
        case NS::RHI::ERHIVertexFormat::Float2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case NS::RHI::ERHIVertexFormat::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case NS::RHI::ERHIVertexFormat::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case NS::RHI::ERHIVertexFormat::Half2:
            return DXGI_FORMAT_R16G16_FLOAT;
        case NS::RHI::ERHIVertexFormat::Half4:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case NS::RHI::ERHIVertexFormat::Int1:
            return DXGI_FORMAT_R32_SINT;
        case NS::RHI::ERHIVertexFormat::Int2:
            return DXGI_FORMAT_R32G32_SINT;
        case NS::RHI::ERHIVertexFormat::Int3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case NS::RHI::ERHIVertexFormat::Int4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case NS::RHI::ERHIVertexFormat::UInt1:
            return DXGI_FORMAT_R32_UINT;
        case NS::RHI::ERHIVertexFormat::UInt2:
            return DXGI_FORMAT_R32G32_UINT;
        case NS::RHI::ERHIVertexFormat::UInt3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case NS::RHI::ERHIVertexFormat::UInt4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case NS::RHI::ERHIVertexFormat::Short2N:
            return DXGI_FORMAT_R16G16_SNORM;
        case NS::RHI::ERHIVertexFormat::Short4N:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case NS::RHI::ERHIVertexFormat::UShort2N:
            return DXGI_FORMAT_R16G16_UNORM;
        case NS::RHI::ERHIVertexFormat::UShort4N:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case NS::RHI::ERHIVertexFormat::Short2:
            return DXGI_FORMAT_R16G16_SINT;
        case NS::RHI::ERHIVertexFormat::Short4:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case NS::RHI::ERHIVertexFormat::UShort2:
            return DXGI_FORMAT_R16G16_UINT;
        case NS::RHI::ERHIVertexFormat::UShort4:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case NS::RHI::ERHIVertexFormat::Byte4N:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case NS::RHI::ERHIVertexFormat::UByte4N:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case NS::RHI::ERHIVertexFormat::UByte4N_BGRA:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case NS::RHI::ERHIVertexFormat::Byte4:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case NS::RHI::ERHIVertexFormat::UByte4:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case NS::RHI::ERHIVertexFormat::UInt1010102N:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //=========================================================================
    // ステート変換ヘルパー（内部）
    //=========================================================================

    static D3D12_RENDER_TARGET_BLEND_DESC ConvertRTBlendDesc(const NS::RHI::RHIRenderTargetBlendDesc& src)
    {
        D3D12_RENDER_TARGET_BLEND_DESC dst{};
        dst.BlendEnable = src.blendEnable ? TRUE : FALSE;
        dst.LogicOpEnable = src.logicOpEnable ? TRUE : FALSE;
        dst.SrcBlend = ConvertBlendFactor(src.srcBlend);
        dst.DestBlend = ConvertBlendFactor(src.dstBlend);
        dst.BlendOp = ConvertBlendOp(src.blendOp);
        dst.SrcBlendAlpha = ConvertBlendFactor(src.srcBlendAlpha);
        dst.DestBlendAlpha = ConvertBlendFactor(src.dstBlendAlpha);
        dst.BlendOpAlpha = ConvertBlendOp(src.blendOpAlpha);
        dst.LogicOp = ConvertLogicOp(src.logicOp);
        dst.RenderTargetWriteMask = static_cast<UINT8>(src.writeMask);
        return dst;
    }

    D3D12_BLEND_DESC ConvertBlendState(const NS::RHI::RHIBlendStateDesc& src)
    {
        D3D12_BLEND_DESC dst{};
        dst.AlphaToCoverageEnable = src.alphaToCoverageEnable ? TRUE : FALSE;
        dst.IndependentBlendEnable = src.independentBlendEnable ? TRUE : FALSE;
        for (uint32 i = 0; i < 8; ++i)
            dst.RenderTarget[i] = ConvertRTBlendDesc(src.renderTarget[i]);
        return dst;
    }

    D3D12_RASTERIZER_DESC ConvertRasterizerState(const NS::RHI::RHIRasterizerStateDesc& src)
    {
        D3D12_RASTERIZER_DESC dst{};
        dst.FillMode = ConvertFillMode(src.fillMode);
        dst.CullMode = ConvertCullMode(src.cullMode);
        dst.FrontCounterClockwise = (src.frontFace == NS::RHI::ERHIFrontFace::CounterClockwise) ? TRUE : FALSE;
        dst.DepthBias = static_cast<INT>(src.depthBias);
        dst.DepthBiasClamp = src.depthBiasClamp;
        dst.SlopeScaledDepthBias = src.slopeScaledDepthBias;
        dst.DepthClipEnable = src.depthClipEnable ? TRUE : FALSE;
        dst.MultisampleEnable = src.multisampleEnable ? TRUE : FALSE;
        dst.AntialiasedLineEnable = src.antialiasedLineEnable ? TRUE : FALSE;
        dst.ForcedSampleCount = src.forcedSampleCount;
        dst.ConservativeRaster = (src.conservativeRaster == NS::RHI::ERHIConservativeRaster::Off)
                                     ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
                                     : D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
        return dst;
    }

    static D3D12_DEPTH_STENCILOP_DESC ConvertStencilFace(const NS::RHI::RHIStencilFaceDesc& src)
    {
        D3D12_DEPTH_STENCILOP_DESC dst{};
        dst.StencilFailOp = ConvertStencilOp(src.stencilFailOp);
        dst.StencilDepthFailOp = ConvertStencilOp(src.stencilDepthFailOp);
        dst.StencilPassOp = ConvertStencilOp(src.stencilPassOp);
        dst.StencilFunc = ConvertCompareFunc(src.stencilFunc);
        return dst;
    }

    D3D12_DEPTH_STENCIL_DESC ConvertDepthStencilState(const NS::RHI::RHIDepthStencilStateDesc& src)
    {
        D3D12_DEPTH_STENCIL_DESC dst{};
        dst.DepthEnable = src.depthTestEnable ? TRUE : FALSE;
        dst.DepthWriteMask = src.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        dst.DepthFunc = ConvertCompareFunc(src.depthFunc);
        dst.StencilEnable = src.stencilTestEnable ? TRUE : FALSE;
        dst.StencilReadMask = src.stencilReadMask;
        dst.StencilWriteMask = src.stencilWriteMask;
        dst.FrontFace = ConvertStencilFace(src.frontFace);
        dst.BackFace = ConvertStencilFace(src.backFace);
        return dst;
    }

    //=========================================================================
    // D3D12GraphicsPipelineState
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12GraphicsPipelineState::GetDevice() const
    {
        return device_;
    }

    NS::RHI::RHIShaderBytecode D3D12GraphicsPipelineState::GetCachedBlob() const
    {
        ComPtr<ID3DBlob> blob;
        if (pso_ && SUCCEEDED(pso_->GetCachedBlob(&blob)))
        {
            return NS::RHI::RHIShaderBytecode::FromData(blob->GetBufferPointer(), blob->GetBufferSize());
        }
        return {};
    }

    bool D3D12GraphicsPipelineState::Init(D3D12Device* device,
                                          const NS::RHI::RHIGraphicsPipelineStateDesc& desc,
                                          const char* debugName)
    {
        if (!device)
            return false;

        device_ = device;
        rootSignature_ = desc.rootSignature;
        vertexShader_ = desc.vertexShader;
        pixelShader_ = desc.pixelShader;
        topologyType_ = desc.primitiveTopologyType;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dDesc{};

        // ルートシグネチャ
        if (desc.rootSignature)
        {
            auto* d3dRS = static_cast<D3D12RootSignature*>(desc.rootSignature);
            d3dDesc.pRootSignature = d3dRS->GetD3DRootSignature();
        }

        // シェーダー
        if (desc.vertexShader)
        {
            auto bc = desc.vertexShader->GetBytecode();
            d3dDesc.VS = {bc.data, bc.size};
        }
        if (desc.pixelShader)
        {
            auto bc = desc.pixelShader->GetBytecode();
            d3dDesc.PS = {bc.data, bc.size};
        }
        if (desc.geometryShader)
        {
            auto bc = desc.geometryShader->GetBytecode();
            d3dDesc.GS = {bc.data, bc.size};
        }
        if (desc.hullShader)
        {
            auto bc = desc.hullShader->GetBytecode();
            d3dDesc.HS = {bc.data, bc.size};
        }
        if (desc.domainShader)
        {
            auto bc = desc.domainShader->GetBytecode();
            d3dDesc.DS = {bc.data, bc.size};
        }

        // 入力レイアウト
        D3D12_INPUT_ELEMENT_DESC inputElements[32]{};
        uint32 numElements = desc.inputLayout.elementCount;
        if (numElements > 32)
            numElements = 32;

        for (uint32 i = 0; i < numElements; ++i)
        {
            const auto& src = desc.inputLayout.elements[i];
            auto& dst = inputElements[i];
            dst.SemanticName = src.semanticName;
            dst.SemanticIndex = src.semanticIndex;
            dst.Format = ConvertVertexFormat(src.format);
            dst.InputSlot = src.inputSlot;
            dst.AlignedByteOffset = src.alignedByteOffset;
            dst.InputSlotClass = (src.inputClass == NS::RHI::ERHIInputClassification::PerInstance)
                                     ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
                                     : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            dst.InstanceDataStepRate = src.instanceDataStepRate;
        }

        d3dDesc.InputLayout.pInputElementDescs = inputElements;
        d3dDesc.InputLayout.NumElements = numElements;

        // ステート
        d3dDesc.BlendState = ConvertBlendState(desc.blendState);
        d3dDesc.SampleMask = desc.sampleMask;
        d3dDesc.RasterizerState = ConvertRasterizerState(desc.rasterizerState);
        d3dDesc.DepthStencilState = ConvertDepthStencilState(desc.depthStencilState);

        // プリミティブトポロジー
        d3dDesc.PrimitiveTopologyType = ConvertPrimitiveTopologyType(desc.primitiveTopologyType);

        // ストリップカット
        switch (desc.stripCutValue)
        {
        case NS::RHI::RHIGraphicsPipelineStateDesc::IndexBufferStripCutValue::MaxUInt16:
            d3dDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
            break;
        case NS::RHI::RHIGraphicsPipelineStateDesc::IndexBufferStripCutValue::MaxUInt32:
            d3dDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
            break;
        default:
            d3dDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
            break;
        }

        // レンダーターゲット
        d3dDesc.NumRenderTargets = desc.renderTargetFormats.count;
        for (uint32 i = 0; i < desc.renderTargetFormats.count; ++i)
        {
            d3dDesc.RTVFormats[i] = D3D12Texture::ConvertPixelFormat(desc.renderTargetFormats.formats[i]);
        }
        d3dDesc.DSVFormat = D3D12Texture::ConvertPixelFormat(desc.renderTargetFormats.depthStencilFormat);
        d3dDesc.SampleDesc.Count = static_cast<UINT>(desc.renderTargetFormats.sampleCount);
        d3dDesc.SampleDesc.Quality = desc.renderTargetFormats.sampleQuality;

        // ノードマスク
        d3dDesc.NodeMask = desc.nodeMask;

        // PSO作成
        HRESULT hr = device->GetD3DDevice()->CreateGraphicsPipelineState(&d3dDesc, IID_PPV_ARGS(&pso_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateGraphicsPipelineState failed");
            return false;
        }

        // デバッグ名
        if (debugName && pso_)
        {
            wchar_t wname[128]{};
            for (int i = 0; i < 127 && debugName[i]; ++i)
                wname[i] = static_cast<wchar_t>(debugName[i]);
            pso_->SetName(wname);
        }

        return true;
    }

    //=========================================================================
    // D3D12ComputePipelineState
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12ComputePipelineState::GetDevice() const
    {
        return device_;
    }

    void D3D12ComputePipelineState::GetThreadGroupSize(uint32& outX, uint32& outY, uint32& outZ) const
    {
        outX = threadGroupX_;
        outY = threadGroupY_;
        outZ = threadGroupZ_;
    }

    NS::RHI::RHIShaderBytecode D3D12ComputePipelineState::GetCachedBlob() const
    {
        ComPtr<ID3DBlob> blob;
        if (pso_ && SUCCEEDED(pso_->GetCachedBlob(&blob)))
        {
            return NS::RHI::RHIShaderBytecode::FromData(blob->GetBufferPointer(), blob->GetBufferSize());
        }
        return {};
    }

    bool D3D12ComputePipelineState::Init(D3D12Device* device,
                                         const NS::RHI::RHIComputePipelineStateDesc& desc,
                                         const char* debugName)
    {
        if (!device || !desc.computeShader)
            return false;

        device_ = device;
        rootSignature_ = desc.rootSignature;
        computeShader_ = desc.computeShader;

        // スレッドグループサイズはデフォルト（リフレクション経由で設定可能）
        threadGroupX_ = 1;
        threadGroupY_ = 1;
        threadGroupZ_ = 1;

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3dDesc{};

        // ルートシグネチャ
        if (desc.rootSignature)
        {
            auto* d3dRS = static_cast<D3D12RootSignature*>(desc.rootSignature);
            d3dDesc.pRootSignature = d3dRS->GetD3DRootSignature();
        }

        // コンピュートシェーダー
        auto bc = desc.computeShader->GetBytecode();
        d3dDesc.CS = {bc.data, bc.size};

        d3dDesc.NodeMask = desc.nodeMask;

        // PSO作成
        HRESULT hr = device->GetD3DDevice()->CreateComputePipelineState(&d3dDesc, IID_PPV_ARGS(&pso_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateComputePipelineState failed");
            return false;
        }

        // デバッグ名
        if (debugName && pso_)
        {
            wchar_t wname[128]{};
            for (int i = 0; i < 127 && debugName[i]; ++i)
                wname[i] = static_cast<wchar_t>(debugName[i]);
            pso_->SetName(wname);
        }

        return true;
    }

    //=========================================================================
    // D3D12InputLayout
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12InputLayout::GetDevice() const
    {
        return device_;
    }

    bool D3D12InputLayout::GetElement(uint32 index, NS::RHI::RHIInputElementDesc& outElement) const
    {
        if (index >= elementCount_)
            return false;
        outElement = elements_[index];
        return true;
    }

    uint32 D3D12InputLayout::GetStride(uint32 slot) const
    {
        uint32 stride = 0;
        for (uint32 i = 0; i < elementCount_; ++i)
        {
            if (elements_[i].inputSlot == slot)
            {
                uint32 elemSize = NS::RHI::GetVertexFormatSize(elements_[i].format);
                uint32 end = elements_[i].alignedByteOffset + elemSize;
                if (end > stride)
                    stride = end;
            }
        }
        return stride;
    }

    bool D3D12InputLayout::Init(D3D12Device* device, const NS::RHI::RHIInputLayoutDesc& desc, const char* debugName)
    {
        (void)debugName;
        if (!device)
            return false;

        device_ = device;
        elementCount_ = desc.elementCount;
        if (elementCount_ > kMaxElements)
            elementCount_ = kMaxElements;

        for (uint32 i = 0; i < elementCount_; ++i)
            elements_[i] = desc.elements[i];

        return true;
    }

} // namespace NS::D3D12RHI
