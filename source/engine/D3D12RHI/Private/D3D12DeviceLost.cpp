/// @file D3D12DeviceLost.cpp
/// @brief D3D12 デバイスロスト検出・DRED診断実装
#include "D3D12DeviceLost.h"
#include "D3D12Device.h"

#include <cstdio>
#include <cstring>

namespace NS::D3D12RHI
{
    void D3D12DeviceLostHelper::EnableDRED()
    {
        // DRED設定（デバイス作成前に呼ぶ）
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
        HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
        if (SUCCEEDED(hr))
        {
            dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            LOG_INFO("[D3D12RHI] DRED enabled (AutoBreadcrumbs + PageFault)");
        }
    }

    bool D3D12DeviceLostHelper::CheckDeviceLost(ID3D12Device* device, NS::RHI::RHIDeviceLostInfo& outInfo)
    {
        if (!device)
            return false;

        HRESULT reason = device->GetDeviceRemovedReason();
        if (reason == S_OK)
            return false; // デバイスは正常

        // デバイスロスト検出
        memset(&outInfo, 0, sizeof(outInfo));
        outInfo.reason = ConvertDeviceRemovedReason(reason);
        outInfo.nativeErrorCode = static_cast<int32>(reason);

        // エラーメッセージ構築
        snprintf(
            outInfo.message, sizeof(outInfo.message), "Device removed: HRESULT 0x%08X", static_cast<unsigned>(reason));

        // DRED情報読み取り
        ReadDREDData(device, outInfo);

        return true;
    }

    bool D3D12DeviceLostHelper::GetCrashInfo(ID3D12Device* device, NS::RHI::RHIGPUCrashInfo& outInfo)
    {
        if (!device)
            return false;

        HRESULT reason = device->GetDeviceRemovedReason();
        if (reason == S_OK)
            return false;

        memset(&outInfo, 0, sizeof(outInfo));

        // HRESULT → ERHIGPUCrashReason
        switch (reason)
        {
        case DXGI_ERROR_DEVICE_HUNG:
            outInfo.reason = NS::RHI::ERHIGPUCrashReason::HangTimeout;
            outInfo.message = "Device hung (GPU timeout)";
            break;
        case DXGI_ERROR_DEVICE_RESET:
            outInfo.reason = NS::RHI::ERHIGPUCrashReason::TDRRecovery;
            outInfo.message = "Device reset (TDR recovery)";
            break;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            outInfo.reason = NS::RHI::ERHIGPUCrashReason::DriverError;
            outInfo.message = "Driver internal error";
            break;
        case E_OUTOFMEMORY:
            outInfo.reason = NS::RHI::ERHIGPUCrashReason::OutOfMemory;
            outInfo.message = "Out of GPU memory";
            break;
        default:
            outInfo.reason = NS::RHI::ERHIGPUCrashReason::Unknown;
            outInfo.message = "Unknown device removed reason";
            break;
        }

        // DRED ページフォルト情報
        ComPtr<ID3D12DeviceRemovedExtendedData> dred;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dred))))
        {
            D3D12_DRED_PAGE_FAULT_OUTPUT pageFault{};
            if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&pageFault)))
            {
                outInfo.faultAddress = pageFault.PageFaultVA;
                if (pageFault.PageFaultVA != 0)
                    outInfo.reason = NS::RHI::ERHIGPUCrashReason::PageFault;
            }
        }

        return true;
    }

    bool D3D12DeviceLostHelper::ReadDREDData(ID3D12Device* device, NS::RHI::RHIDeviceLostInfo& outInfo)
    {
        // DRED拡張データ読み取り
        ComPtr<ID3D12DeviceRemovedExtendedData> dred;
        HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dred));
        if (FAILED(hr))
            return false;

        // AutoBreadcrumbs
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs{};
        if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput(&breadcrumbs)))
        {
            const D3D12_AUTO_BREADCRUMB_NODE* node = breadcrumbs.pHeadAutoBreadcrumbNode;
            if (node)
            {
                // 最後のブレッドクラム情報取得
                while (node->pNext)
                    node = node->pNext;

                if (node->pLastBreadcrumbValue && node->BreadcrumbCount > 0)
                {
                    outInfo.lastBreadcrumbId = *node->pLastBreadcrumbValue;
                }

                if (node->pCommandListDebugNameW)
                {
                    // ワイド文字→UTF-8変換
                    WideCharToMultiByte(CP_UTF8,
                                        0,
                                        node->pCommandListDebugNameW,
                                        -1,
                                        outInfo.lastBreadcrumbMessage,
                                        sizeof(outInfo.lastBreadcrumbMessage),
                                        nullptr,
                                        nullptr);
                }
            }
        }

        // PageFault
        D3D12_DRED_PAGE_FAULT_OUTPUT pageFault{};
        if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&pageFault)))
        {
            outInfo.faultAddress = pageFault.PageFaultVA;
            if (pageFault.PageFaultVA != 0)
                outInfo.reason = NS::RHI::ERHIDeviceLostReason::PageFault;
        }

        return true;
    }

} // namespace NS::D3D12RHI
