/// @file D3D12Adapter.cpp
/// @brief D3D12 GPUアダプター列挙・初期化
#include "D3D12Adapter.h"

#include <cstdio>
#include <cwchar>

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12Factory
    //=========================================================================

    bool D3D12Factory::Create(bool enableDebug)
    {
        UINT flags = 0;
        if (enableDebug)
        {
            flags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        ComPtr<IDXGIFactory2> factory2;
        HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory2));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateDXGIFactory2 failed");
            return false;
        }

        hr = factory2.As(&factory_);
        if (FAILED(hr))
        {
            LOG_WARN("[D3D12RHI] IDXGIFactory6 not available");
            // Factory6 がない環境ではFactory4にフォールバック
            ComPtr<IDXGIFactory4> factory4;
            hr = factory2.As(&factory4);
            if (FAILED(hr))
            {
                LOG_ERROR("[D3D12RHI] IDXGIFactory4 not available");
                return false;
            }
            // Factory6 として扱えないが、Factory4 の機能で列挙は可能
            // ここでは Factory6 を必須とする（Windows 10 1803+）
            LOG_ERROR("[D3D12RHI] IDXGIFactory6 required (Windows 10 1803+)");
            return false;
        }

        return true;
    }

    //=========================================================================
    // D3D12Adapter
    //=========================================================================

    bool D3D12Adapter::Init(IDXGIAdapter1* dxgiAdapter, uint32 adapterIndex)
    {
        dxgiAdapter_ = dxgiAdapter;

        // DXGI_ADAPTER_DESC1 取得
        DXGI_ADAPTER_DESC1 desc1{};
        HRESULT hr = dxgiAdapter->GetDesc1(&desc1);
        if (FAILED(hr))
        {
            char msg[128];
            std::snprintf(msg, sizeof(msg), "[D3D12RHI] GetDesc1 failed for adapter %u", adapterIndex);
            LOG_HRESULT(hr, msg);
            return false;
        }

        // ソフトウェアアダプタ判定
        bool isSoftware = (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

        // D3D_FEATURE_LEVEL チェック（12_0以上が必須）
        static const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
        };

        maxFeatureLevel_ = D3D_FEATURE_LEVEL_12_0;
        for (D3D_FEATURE_LEVEL fl : featureLevels)
        {
            hr = D3D12CreateDevice(dxgiAdapter, fl, __uuidof(ID3D12Device), nullptr);
            if (SUCCEEDED(hr))
            {
                maxFeatureLevel_ = fl;
                break;
            }
        }

        // RHIAdapterDesc に変換
        desc_.adapterIndex = adapterIndex;

        // WCHAR → char 変換（ASCII範囲のみ）
        char nameBuf[128]{};
        for (int i = 0; i < 127 && desc1.Description[i] != 0; ++i)
        {
            nameBuf[i] = static_cast<char>(desc1.Description[i] > 127 ? '?' : desc1.Description[i]);
        }
        desc_.deviceName = nameBuf;

        desc_.vendorId = desc1.VendorId;
        desc_.deviceId = desc1.DeviceId;
        desc_.subsystemId = desc1.SubSysId;
        desc_.revision = desc1.Revision;
        desc_.dedicatedVideoMemory = desc1.DedicatedVideoMemory;
        desc_.dedicatedSystemMemory = desc1.DedicatedSystemMemory;
        desc_.sharedSystemMemory = desc1.SharedSystemMemory;
        desc_.isSoftwareAdapter = isSoftware;
        desc_.isDiscreteGPU = !isSoftware && desc1.DedicatedVideoMemory > 0;
        desc_.numDeviceNodes = 1;

        // Feature Level → ERHIFeatureLevel マッピング
        switch (maxFeatureLevel_)
        {
        case D3D_FEATURE_LEVEL_12_2:
            desc_.maxFeatureLevel = NS::RHI::ERHIFeatureLevel::SM6_6;
            break;
        case D3D_FEATURE_LEVEL_12_1:
            desc_.maxFeatureLevel = NS::RHI::ERHIFeatureLevel::SM6;
            break;
        default:
            desc_.maxFeatureLevel = NS::RHI::ERHIFeatureLevel::SM6;
            break;
        }

        {
            char logBuf[256];
            std::snprintf(logBuf,
                          sizeof(logBuf),
                          "[D3D12RHI] Adapter[%u]: %s (Vendor: %s, VRAM: %llu MB, FL: 0x%X)",
                          adapterIndex,
                          nameBuf,
                          NS::RHI::GetVendorName(desc_.vendorId),
                          static_cast<unsigned long long>(desc_.dedicatedVideoMemory / (1024ULL * 1024ULL)),
                          static_cast<unsigned>(maxFeatureLevel_));
            LOG_INFO(logBuf);
        }

        return true;
    }

    NS::RHI::ERHIFeatureSupport D3D12Adapter::SupportsFeature(NS::RHI::ERHIFeature feature) const
    {
        using NS::RHI::ERHIFeature;
        using NS::RHI::ERHIFeatureSupport;

        switch (feature)
        {
        case ERHIFeature::TextureCompressionBC:
        case ERHIFeature::StructuredBuffer:
        case ERHIFeature::ByteAddressBuffer:
        case ERHIFeature::TypedBuffer:
        case ERHIFeature::MultiDrawIndirect:
        case ERHIFeature::DrawIndirectCount:
        case ERHIFeature::DepthBoundsTest:
        case ERHIFeature::ExecuteIndirect:
            return ERHIFeatureSupport::RuntimeGuaranteed;

        case ERHIFeature::Bindless:
            return (maxFeatureLevel_ >= D3D_FEATURE_LEVEL_12_0) ? ERHIFeatureSupport::RuntimeGuaranteed
                                                                : ERHIFeatureSupport::Unsupported;

        // Feature検出はDevice作成後（サブ計画05）で正確に判定
        case ERHIFeature::RayTracing:
        case ERHIFeature::MeshShaders:
        case ERHIFeature::VariableRateShading:
        case ERHIFeature::WaveOperations:
        case ERHIFeature::WorkGraphs:
        case ERHIFeature::EnhancedBarriers:
        case ERHIFeature::GPUUploadHeaps:
        case ERHIFeature::AtomicInt64:
        case ERHIFeature::SamplerFeedback:
            return ERHIFeatureSupport::RuntimeDependent;

        default:
            return ERHIFeatureSupport::Unsupported;
        }
    }

    NS::RHI::ERHISampleCount D3D12Adapter::GetMaxSampleCount(NS::RHI::ERHIPixelFormat) const
    {
        return NS::RHI::ERHISampleCount::Count8;
    }

    //=========================================================================
    // Adapter列挙
    //=========================================================================

    uint32 EnumerateAdapters(IDXGIFactory6* factory, D3D12Adapter* outAdapters, uint32 maxAdapters)
    {
        uint32 count = 0;

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; count < maxAdapters && SUCCEEDED(factory->EnumAdapterByGpuPreference(
                                                    i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
             ++i)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            // ソフトウェアアダプタはスキップ
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter.Reset();
                continue;
            }

            // D3D_FEATURE_LEVEL_12_0 が使えるか確認
            HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr);
            if (FAILED(hr))
            {
                adapter.Reset();
                continue;
            }

            if (outAdapters[count].Init(adapter.Get(), count))
            {
                ++count;
            }

            adapter.Reset();
        }

        {
            char logBuf[128];
            std::snprintf(logBuf, sizeof(logBuf), "[D3D12RHI] Found %u D3D12-capable adapter(s)", count);
            LOG_INFO(logBuf);
        }
        return count;
    }

} // namespace NS::D3D12RHI
