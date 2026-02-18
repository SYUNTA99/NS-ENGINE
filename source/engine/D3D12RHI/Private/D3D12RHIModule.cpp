/// @file D3D12RHIModule.cpp
/// @brief D3D12RHIモジュールエントリ — IDynamicRHIModule実装
#include "D3D12DynamicRHI.h"
#include "D3D12RHIPrivate.h"
#include "RHI/Public/IDynamicRHIModule.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12RHIModule — IDynamicRHIModule実装
    //=========================================================================

    class D3D12RHIModule final : public NS::RHI::IDynamicRHIModule
    {
    public:
        const char* GetModuleName() const override { return "D3D12"; }

        NS::RHI::ERHIInterfaceType GetInterfaceType() const override
        {
            return NS::RHI::ERHIInterfaceType::D3D12;
        }

        bool IsSupported() const override
        {
            // DXGIファクトリの作成を試みてD3D12利用可能性を確認
            ComPtr<IDXGIFactory4> factory;
            HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
            return SUCCEEDED(hr);
        }

        NS::RHI::IDynamicRHI* CreateRHI() override
        {
            return new D3D12DynamicRHI();
        }
    };

    //=========================================================================
    // モジュール登録（静的初期化）
    //=========================================================================

    static D3D12RHIModule s_d3d12Module;
    static NS::RHI::RHIModuleRegistrar s_d3d12Registrar("D3D12", &s_d3d12Module);

} // namespace NS::D3D12RHI
