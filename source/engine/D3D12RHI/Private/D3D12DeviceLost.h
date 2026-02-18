/// @file D3D12DeviceLost.h
/// @brief D3D12 デバイスロスト検出・DRED診断
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIDeviceLost.h"
#include "RHI/Public/RHIGPUEvent.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // HRESULT → ERHIDeviceLostReason 変換
    //=========================================================================

    inline NS::RHI::ERHIDeviceLostReason ConvertDeviceRemovedReason(HRESULT reason)
    {
        switch (reason)
        {
        case DXGI_ERROR_DEVICE_HUNG:
            return NS::RHI::ERHIDeviceLostReason::Hung;
        case DXGI_ERROR_DEVICE_RESET:
            return NS::RHI::ERHIDeviceLostReason::Reset;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            return NS::RHI::ERHIDeviceLostReason::DriverInternalError;
        case DXGI_ERROR_INVALID_CALL:
            return NS::RHI::ERHIDeviceLostReason::InvalidGPUCommand;
        case E_OUTOFMEMORY:
            return NS::RHI::ERHIDeviceLostReason::OutOfMemory;
        default:
            return NS::RHI::ERHIDeviceLostReason::Unknown;
        }
    }

    //=========================================================================
    // D3D12DeviceLostHelper — DRED診断ヘルパー
    //=========================================================================

    class D3D12DeviceLostHelper
    {
    public:
        /// DRED設定（デバイス作成前に呼ぶ）
        static void EnableDRED();

        /// デバイスロスト検出 + 情報収集
        static bool CheckDeviceLost(ID3D12Device* device, NS::RHI::RHIDeviceLostInfo& outInfo);

        /// GPU クラッシュ情報収集（DRED）
        static bool GetCrashInfo(ID3D12Device* device, NS::RHI::RHIGPUCrashInfo& outInfo);

    private:
        /// DRED拡張データ読み取り
        static bool ReadDREDData(ID3D12Device* device, NS::RHI::RHIDeviceLostInfo& outInfo);
    };

} // namespace NS::D3D12RHI
