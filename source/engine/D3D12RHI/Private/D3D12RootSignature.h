/// @file D3D12RootSignature.h
/// @brief D3D12 ルートシグネチャ実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIRootSignature.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // 変換ヘルパー
    //=========================================================================

    D3D12_SHADER_VISIBILITY ConvertShaderVisibility(NS::RHI::EShaderVisibility vis);
    D3D12_ROOT_SIGNATURE_FLAGS ConvertRootSignatureFlags(NS::RHI::ERHIRootSignatureFlags flags);
    D3D12_DESCRIPTOR_RANGE_TYPE ConvertDescriptorRangeType(NS::RHI::ERHIDescriptorRangeType type);

    //=========================================================================
    // D3D12RootSignature
    //=========================================================================

    class D3D12RootSignature final : public NS::RHI::IRHIRootSignature
    {
    public:
        ~D3D12RootSignature() override;

        bool Init(D3D12Device* device, const NS::RHI::RHIRootSignatureDesc& desc, const char* debugName);
        bool InitFromBlob(D3D12Device* device, const NS::RHI::RHIShaderBytecode& blob, const char* debugName);

        ID3D12RootSignature* GetD3DRootSignature() const { return rootSig_.Get(); }

        // --- IRHIRootSignature ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        uint32 GetParameterCount() const override { return paramCount_; }
        uint32 GetStaticSamplerCount() const override { return staticSamplerCount_; }
        NS::RHI::ERHIRootSignatureFlags GetFlags() const override { return flags_; }
        NS::RHI::ERHIRootParameterType GetParameterType(uint32 index) const override;
        NS::RHI::EShaderVisibility GetParameterVisibility(uint32 index) const override;
        uint32 GetDescriptorTableSize(uint32 paramIndex) const override;
        NS::RHI::RHIShaderBytecode GetSerializedBlob() const override;

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12RootSignature> rootSig_;
        ComPtr<ID3DBlob> serializedBlob_;

        NS::RHI::ERHIRootSignatureFlags flags_{};
        uint32 paramCount_ = 0;
        uint32 staticSamplerCount_ = 0;

        // パラメータ情報キャッシュ（最大64）
        static constexpr uint32 kMaxCachedParams = 64;
        struct ParamInfo
        {
            NS::RHI::ERHIRootParameterType type;
            NS::RHI::EShaderVisibility visibility;
            uint32 descriptorTableSize; // DescriptorTableの場合のみ有効
        };
        ParamInfo paramInfo_[kMaxCachedParams]{};
    };

} // namespace NS::D3D12RHI
