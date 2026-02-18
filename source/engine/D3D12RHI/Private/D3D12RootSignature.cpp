/// @file D3D12RootSignature.cpp
/// @brief D3D12 ルートシグネチャ実装
#include "D3D12RootSignature.h"
#include "D3D12Device.h"
#include "D3D12Sampler.h"          // ConvertCompareFunc, ConvertAddressMode, ConvertFilter
#include "RHI/Public/IRHIShader.h" // RHIShaderBytecode

namespace NS::D3D12RHI
{
    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    D3D12_SHADER_VISIBILITY ConvertShaderVisibility(NS::RHI::EShaderVisibility vis)
    {
        switch (vis)
        {
        case NS::RHI::EShaderVisibility::Vertex:
            return D3D12_SHADER_VISIBILITY_VERTEX;
        case NS::RHI::EShaderVisibility::Hull:
            return D3D12_SHADER_VISIBILITY_HULL;
        case NS::RHI::EShaderVisibility::Domain:
            return D3D12_SHADER_VISIBILITY_DOMAIN;
        case NS::RHI::EShaderVisibility::Geometry:
            return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case NS::RHI::EShaderVisibility::Pixel:
            return D3D12_SHADER_VISIBILITY_PIXEL;
        case NS::RHI::EShaderVisibility::Amplification:
            return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
        case NS::RHI::EShaderVisibility::Mesh:
            return D3D12_SHADER_VISIBILITY_MESH;
        default:
            return D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    D3D12_ROOT_SIGNATURE_FLAGS ConvertRootSignatureFlags(NS::RHI::ERHIRootSignatureFlags flags)
    {
        using namespace NS::RHI;
        D3D12_ROOT_SIGNATURE_FLAGS d3dFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyVertexShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyHullShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyDomainShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyGeometryShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyPixelShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyAmplificationShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::DenyMeshShaderRootAccess))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::AllowInputAssemblerInputLayout))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::AllowStreamOutput))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::LocalRootSignature))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::CBVSRVUAVHeapDirectlyIndexed))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        if (EnumHasAnyFlags(flags, ERHIRootSignatureFlags::SamplerHeapDirectlyIndexed))
            d3dFlags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

        return d3dFlags;
    }

    D3D12_DESCRIPTOR_RANGE_TYPE ConvertDescriptorRangeType(NS::RHI::ERHIDescriptorRangeType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIDescriptorRangeType::SRV:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case NS::RHI::ERHIDescriptorRangeType::UAV:
            return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        case NS::RHI::ERHIDescriptorRangeType::CBV:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case NS::RHI::ERHIDescriptorRangeType::Sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        default:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        }
    }

    //=========================================================================
    // 静的サンプラー変換
    //=========================================================================

    static D3D12_FILTER ConvertStaticSamplerFilter(NS::RHI::ERHIFilterMode mode, bool comparison)
    {
        switch (mode)
        {
        case NS::RHI::ERHIFilterMode::Point:
            return comparison ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;
        case NS::RHI::ERHIFilterMode::Linear:
            return comparison ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case NS::RHI::ERHIFilterMode::Anisotropic:
            return comparison ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
        default:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        }
    }

    static D3D12_TEXTURE_ADDRESS_MODE ConvertStaticAddressMode(NS::RHI::ERHIAddressMode mode)
    {
        switch (mode)
        {
        case NS::RHI::ERHIAddressMode::Wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case NS::RHI::ERHIAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case NS::RHI::ERHIAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case NS::RHI::ERHIAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case NS::RHI::ERHIAddressMode::MirrorOnce:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    static D3D12_STATIC_BORDER_COLOR ConvertStaticBorderColor(NS::RHI::RHIStaticSamplerDesc::BorderColor color)
    {
        switch (color)
        {
        case NS::RHI::RHIStaticSamplerDesc::BorderColor::TransparentBlack:
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        case NS::RHI::RHIStaticSamplerDesc::BorderColor::OpaqueBlack:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        case NS::RHI::RHIStaticSamplerDesc::BorderColor::OpaqueWhite:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        default:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        }
    }

    //=========================================================================
    // D3D12RootSignature
    //=========================================================================

    D3D12RootSignature::~D3D12RootSignature() = default;

    NS::RHI::IRHIDevice* D3D12RootSignature::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    NS::RHI::ERHIRootParameterType D3D12RootSignature::GetParameterType(uint32 index) const
    {
        if (index < paramCount_)
            return paramInfo_[index].type;
        return NS::RHI::ERHIRootParameterType::DescriptorTable;
    }

    NS::RHI::EShaderVisibility D3D12RootSignature::GetParameterVisibility(uint32 index) const
    {
        if (index < paramCount_)
            return paramInfo_[index].visibility;
        return NS::RHI::EShaderVisibility::All;
    }

    uint32 D3D12RootSignature::GetDescriptorTableSize(uint32 paramIndex) const
    {
        if (paramIndex < paramCount_)
            return paramInfo_[paramIndex].descriptorTableSize;
        return 0;
    }

    NS::RHI::RHIShaderBytecode D3D12RootSignature::GetSerializedBlob() const
    {
        NS::RHI::RHIShaderBytecode bytecode{};
        if (serializedBlob_)
        {
            bytecode.data = serializedBlob_->GetBufferPointer();
            bytecode.size = serializedBlob_->GetBufferSize();
        }
        return bytecode;
    }

    //=========================================================================
    // Init: RHIRootSignatureDesc → D3D12 Root Signature
    //=========================================================================

    bool D3D12RootSignature::Init(D3D12Device* device, const NS::RHI::RHIRootSignatureDesc& desc, const char* debugName)
    {
        if (!device)
        {
            return false;
        }

        device_ = device;
        flags_ = desc.flags;

        // ディスクリプタレンジ用一時バッファ（全パラメータのレンジをフラットに格納）
        constexpr uint32 kMaxRanges = 256;
        D3D12_DESCRIPTOR_RANGE1 d3dRanges[kMaxRanges];
        uint32 rangeOffset = 0;

        // ルートパラメータ変換
        constexpr uint32 kMaxParams = 64;
        D3D12_ROOT_PARAMETER1 d3dParams[kMaxParams];

        uint32 numParams = desc.parameterCount < kMaxParams ? desc.parameterCount : kMaxParams;
        if (desc.parameterCount > kMaxParams)
            LOG_WARN("[D3D12RHI] RootSignature parameter count " + std::to_string(desc.parameterCount) +
                     " exceeds max " + std::to_string(kMaxParams) + ", truncated");
        paramCount_ = numParams;
        for (uint32 i = 0; i < numParams; ++i)
        {
            const auto& rhiParam = desc.parameters[i];
            auto& d3dParam = d3dParams[i];

            d3dParam.ShaderVisibility = ConvertShaderVisibility(rhiParam.shaderVisibility);

            // パラメータ情報キャッシュ
            if (i < kMaxCachedParams)
            {
                paramInfo_[i].type = rhiParam.parameterType;
                paramInfo_[i].visibility = rhiParam.shaderVisibility;
                paramInfo_[i].descriptorTableSize = 0;
            }

            switch (rhiParam.parameterType)
            {
            case NS::RHI::ERHIRootParameterType::DescriptorTable:
            {
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                uint32 startRange = rangeOffset;
                uint32 tableSize = 0;
                for (uint32 r = 0; r < rhiParam.descriptorTable.rangeCount && rangeOffset < kMaxRanges; ++r)
                {
                    const auto& rhiRange = rhiParam.descriptorTable.ranges[r];
                    auto& d3dRange = d3dRanges[rangeOffset++];

                    d3dRange.RangeType = ConvertDescriptorRangeType(rhiRange.rangeType);
                    d3dRange.NumDescriptors = rhiRange.numDescriptors;
                    d3dRange.BaseShaderRegister = rhiRange.baseShaderRegister;
                    d3dRange.RegisterSpace = rhiRange.registerSpace;
                    d3dRange.OffsetInDescriptorsFromTableStart =
                        rhiRange.offsetInDescriptorsFromTableStart == NS::RHI::RHIDescriptorRange::kAppendFromTableStart
                            ? D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
                            : rhiRange.offsetInDescriptorsFromTableStart;
                    d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

                    if (rhiRange.numDescriptors != NS::RHI::kUnboundedDescriptorCount)
                    {
                        tableSize += rhiRange.numDescriptors;
                    }
                }
                d3dParam.DescriptorTable.NumDescriptorRanges = rangeOffset - startRange;
                d3dParam.DescriptorTable.pDescriptorRanges = &d3dRanges[startRange];

                if (i < kMaxCachedParams)
                    paramInfo_[i].descriptorTableSize = tableSize;
                break;
            }
            case NS::RHI::ERHIRootParameterType::Constants:
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                d3dParam.Constants.ShaderRegister = rhiParam.constants.shaderRegister;
                d3dParam.Constants.RegisterSpace = rhiParam.constants.registerSpace;
                d3dParam.Constants.Num32BitValues = rhiParam.constants.num32BitValues;
                break;
            case NS::RHI::ERHIRootParameterType::CBV:
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                d3dParam.Descriptor.ShaderRegister = rhiParam.descriptor.shaderRegister;
                d3dParam.Descriptor.RegisterSpace = rhiParam.descriptor.registerSpace;
                d3dParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
                break;
            case NS::RHI::ERHIRootParameterType::SRV:
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
                d3dParam.Descriptor.ShaderRegister = rhiParam.descriptor.shaderRegister;
                d3dParam.Descriptor.RegisterSpace = rhiParam.descriptor.registerSpace;
                d3dParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
                break;
            case NS::RHI::ERHIRootParameterType::UAV:
                d3dParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
                d3dParam.Descriptor.ShaderRegister = rhiParam.descriptor.shaderRegister;
                d3dParam.Descriptor.RegisterSpace = rhiParam.descriptor.registerSpace;
                d3dParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
                break;
            }
        }

        // 静的サンプラー変換
        constexpr uint32 kMaxStaticSamplers = 32;
        D3D12_STATIC_SAMPLER_DESC d3dStaticSamplers[kMaxStaticSamplers];
        uint32 numStaticSamplers =
            desc.staticSamplerCount < kMaxStaticSamplers ? desc.staticSamplerCount : kMaxStaticSamplers;
        if (desc.staticSamplerCount > kMaxStaticSamplers)
        {
            LOG_WARN("[D3D12RHI] RootSignature static sampler count exceeds max, truncated");
        }
        staticSamplerCount_ = numStaticSamplers;

        for (uint32 i = 0; i < numStaticSamplers; ++i)
        {
            const auto& rhiSampler = desc.staticSamplers[i];
            auto& d3dSampler = d3dStaticSamplers[i];

            bool isComparison = (rhiSampler.comparisonFunc != NS::RHI::ERHICompareFunc::Never);
            d3dSampler.Filter = ConvertStaticSamplerFilter(rhiSampler.filter, isComparison);
            d3dSampler.AddressU = ConvertStaticAddressMode(rhiSampler.addressU);
            d3dSampler.AddressV = ConvertStaticAddressMode(rhiSampler.addressV);
            d3dSampler.AddressW = ConvertStaticAddressMode(rhiSampler.addressW);
            d3dSampler.MipLODBias = rhiSampler.mipLODBias;
            d3dSampler.MaxAnisotropy = rhiSampler.maxAnisotropy;
            d3dSampler.ComparisonFunc = ConvertCompareFunc(rhiSampler.comparisonFunc);
            d3dSampler.BorderColor = ConvertStaticBorderColor(rhiSampler.borderColor);
            d3dSampler.MinLOD = rhiSampler.minLOD;
            d3dSampler.MaxLOD = rhiSampler.maxLOD;
            d3dSampler.ShaderRegister = rhiSampler.shaderRegister;
            d3dSampler.RegisterSpace = rhiSampler.registerSpace;
            d3dSampler.ShaderVisibility = ConvertShaderVisibility(rhiSampler.shaderVisibility);
        }

        // Version 1.1 Root Signature
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
        versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        versionedDesc.Desc_1_1.NumParameters = numParams;
        versionedDesc.Desc_1_1.pParameters = numParams > 0 ? d3dParams : nullptr;
        versionedDesc.Desc_1_1.NumStaticSamplers = numStaticSamplers;
        versionedDesc.Desc_1_1.pStaticSamplers = numStaticSamplers > 0 ? d3dStaticSamplers : nullptr;
        versionedDesc.Desc_1_1.Flags = ConvertRootSignatureFlags(desc.flags);

        // シリアライズ
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedDesc, &serializedBlob_, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                LOG_ERROR(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            LOG_HRESULT(hr, "[D3D12RHI] D3D12SerializeVersionedRootSignature failed");
            return false;
        }

        // Root Signature作成
        hr = device_->GetD3DDevice()->CreateRootSignature(
            0, serializedBlob_->GetBufferPointer(), serializedBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSig_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateRootSignature failed");
            return false;
        }

        // デバッグ名
        if (debugName && rootSig_)
        {
            wchar_t wname[128]{};
            for (int i = 0; i < 127 && debugName[i]; ++i)
            {
                wname[i] = static_cast<wchar_t>(debugName[i]);
            }
            rootSig_->SetName(wname);
        }

        return true;
    }

    //=========================================================================
    // InitFromBlob: 事前シリアライズ済みBlob → Root Signature
    //=========================================================================

    bool D3D12RootSignature::InitFromBlob(D3D12Device* device,
                                          const NS::RHI::RHIShaderBytecode& blob,
                                          const char* debugName)
    {
        if (!device || !blob.data || blob.size == 0)
        {
            return false;
        }

        device_ = device;

        HRESULT hr = device_->GetD3DDevice()->CreateRootSignature(0, blob.data, blob.size, IID_PPV_ARGS(&rootSig_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateRootSignature from blob failed");
            return false;
        }

        if (debugName && rootSig_)
        {
            wchar_t wname[128]{};
            for (int i = 0; i < 127 && debugName[i]; ++i)
            {
                wname[i] = static_cast<wchar_t>(debugName[i]);
            }
            rootSig_->SetName(wname);
        }

        return true;
    }

} // namespace NS::D3D12RHI
