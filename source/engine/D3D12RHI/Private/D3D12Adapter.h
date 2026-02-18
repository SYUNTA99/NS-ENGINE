/// @file D3D12Adapter.h
/// @brief D3D12 GPUアダプター
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIAdapter.h"

namespace NS::D3D12RHI
{
    /// DXGI Factory管理
    /// Factory作成とAdapter列挙を担う
    class D3D12Factory
    {
    public:
        bool Create(bool enableDebug);
        IDXGIFactory6* Get() const { return factory_.Get(); }

    private:
        ComPtr<IDXGIFactory6> factory_;
    };

    /// D3D12 GPUアダプター — IRHIAdapter実装
    class D3D12Adapter final : public NS::RHI::IRHIAdapter
    {
    public:
        /// DXGI Adapterから初期化
        bool Init(IDXGIAdapter1* dxgiAdapter, uint32 adapterIndex);

        /// ネイティブDXGI Adapter取得
        IDXGIAdapter1* GetDXGIAdapter() const { return dxgiAdapter_.Get(); }

        /// Adapter←→Device接続
        void SetDevice(NS::RHI::IRHIDevice* device) { device_ = device; }

        // --- IRHIAdapter ---
        const NS::RHI::RHIAdapterDesc& GetDesc() const override { return desc_; }
        uint32 GetDeviceCount() const override { return device_ ? 1u : 0u; }
        NS::RHI::IRHIDevice* GetDevice(uint32) const override { return device_; }

        NS::RHI::ERHIFeatureSupport SupportsFeature(NS::RHI::ERHIFeature feature) const override;

        uint32 GetMaxTextureSize() const override { return 16384; }
        uint32 GetMaxTextureArrayLayers() const override { return 2048; }
        uint32 GetMaxTexture3DSize() const override { return 2048; }
        uint64 GetMaxBufferSize() const override
        {
            return D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ULL * 1024ULL;
        }
        uint32 GetMaxConstantBufferSize() const override { return D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16; }
        uint32 GetConstantBufferAlignment() const override { return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT; }
        uint32 GetStructuredBufferAlignment() const override { return D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT; }
        NS::RHI::ERHISampleCount GetMaxSampleCount(NS::RHI::ERHIPixelFormat) const override;

        NS::RHI::IRHIPipelineStateCache* GetPipelineStateCache() override { return nullptr; }
        NS::RHI::IRHIRootSignatureManager* GetRootSignatureManager() override { return nullptr; }

        uint32 GetOutputCount() const override { return 0; }
        bool OutputSupportsHDR(uint32) const override { return false; }
        bool GetOutputInfo(uint32, NS::RHI::RHIOutputInfo&) const override { return false; }
        uint32 EnumerateDisplayModes(uint32, NS::RHI::ERHIPixelFormat, NS::RHI::RHIDisplayMode*, uint32) const override
        {
            return 0;
        }
        bool FindClosestDisplayMode(uint32, const NS::RHI::RHIDisplayMode&, NS::RHI::RHIDisplayMode&) const override
        {
            return false;
        }
        bool GetHDROutputCapabilities(uint32, NS::RHI::RHIHDROutputCapabilities&) const override { return false; }

    private:
        ComPtr<IDXGIAdapter1> dxgiAdapter_;
        NS::RHI::RHIAdapterDesc desc_{};
        D3D_FEATURE_LEVEL maxFeatureLevel_ = D3D_FEATURE_LEVEL_12_0;
        NS::RHI::IRHIDevice* device_ = nullptr;
    };

    /// Adapter列挙
    /// @param factory     DXGI Factory
    /// @param outAdapters 出力先
    /// @param maxAdapters 最大数
    /// @return 見つかったAdapter数
    uint32 EnumerateAdapters(IDXGIFactory6* factory, D3D12Adapter* outAdapters, uint32 maxAdapters);

} // namespace NS::D3D12RHI
