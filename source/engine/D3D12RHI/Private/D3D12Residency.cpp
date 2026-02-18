/// @file D3D12Residency.cpp
/// @brief D3D12 レジデンシー管理実装
#include "D3D12Residency.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    D3D12ResidencyManager::~D3D12ResidencyManager()
    {
        Shutdown();
    }

    bool D3D12ResidencyManager::Init(D3D12Device* device, IDXGIAdapter1* adapter)
    {
        if (!device || !adapter)
            return false;

        device_ = device;

        // IDXGIAdapter3取得（QueryVideoMemoryInfo対応）
        HRESULT hr = adapter->QueryInterface(IID_PPV_ARGS(&adapter3_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12Residency] Failed to query IDXGIAdapter3");
            return false;
        }

        // アダプター情報からメモリ容量取得
        DXGI_ADAPTER_DESC1 adapterDesc{};
        adapter->GetDesc1(&adapterDesc);
        dedicatedVideoMemory_ = adapterDesc.DedicatedVideoMemory;
        sharedSystemMemory_ = adapterDesc.SharedSystemMemory;

        // 初回メモリ予算取得
        UpdateMemoryBudget();

        return true;
    }

    void D3D12ResidencyManager::Shutdown()
    {
        adapter3_.Reset();
        device_ = nullptr;
        pressureCallback_ = nullptr;
    }

    void D3D12ResidencyManager::UpdateMemoryBudget()
    {
        if (!adapter3_)
            return;

        // ローカルメモリ（専用VRAM）
        DXGI_QUERY_VIDEO_MEMORY_INFO localInfo{};
        HRESULT hr = adapter3_->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo);
        if (SUCCEEDED(hr))
        {
            dedicatedBudget_ = localInfo.Budget;
            dedicatedUsage_ = localInfo.CurrentUsage;
        }

        // 非ローカルメモリ（共有メモリ）
        DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo{};
        hr = adapter3_->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo);
        if (SUCCEEDED(hr))
        {
            sharedBudget_ = nonLocalInfo.Budget;
            sharedUsage_ = nonLocalInfo.CurrentUsage;
        }

        // メモリ圧迫チェック
        if (pressureCallback_ && dedicatedBudget_ > 0 && dedicatedUsage_ > dedicatedBudget_)
        {
            uint64 overBudget = dedicatedUsage_ - dedicatedBudget_;
            pressureCallback_(overBudget);
        }
    }

    bool D3D12ResidencyManager::MakeResident(ID3D12Pageable* const* objects, uint32 count)
    {
        if (!device_ || !objects || count == 0)
            return false;

        auto* d3dDevice = device_->GetD3DDevice();
        if (!d3dDevice)
            return false;

        HRESULT hr = d3dDevice->MakeResident(count, objects);
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12Residency] MakeResident failed");
            return false;
        }
        return true;
    }

    bool D3D12ResidencyManager::Evict(ID3D12Pageable* const* objects, uint32 count)
    {
        if (!device_ || !objects || count == 0)
            return false;

        auto* d3dDevice = device_->GetD3DDevice();
        if (!d3dDevice)
            return false;

        HRESULT hr = d3dDevice->Evict(count, objects);
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12Residency] Evict failed");
            return false;
        }
        return true;
    }

} // namespace NS::D3D12RHI
