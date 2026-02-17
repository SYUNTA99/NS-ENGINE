/// @file WindowsDisplayInfo.cpp
/// @brief Windows ディスプレイ列挙 + EDID パース (09-03)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <setupapi.h>
#include <shellscalingapi.h>
#endif

#include "GenericPlatform/GenericApplication.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "setupapi.lib")

namespace
{
    /// EnumDisplayMonitors コールバックデータ
    struct MonitorEnumData
    {
        std::vector<NS::MonitorInfo>* OutMonitors = nullptr;
    };

    /// モニター DPI 取得 (Per-Monitor V2 対応)
    int32_t GetMonitorDPI(HMONITOR hMonitor)
    {
        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(::GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
        {
            return static_cast<int32_t>(dpiX);
        }
        return 96;
    }

    /// EnumDisplayMonitors コールバック
    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lpRect*/, LPARAM lParam)
    {
        auto* data = reinterpret_cast<MonitorEnumData*>(lParam);

        MONITORINFOEXW monInfo = {};
        monInfo.cbSize = sizeof(monInfo);
        if (!::GetMonitorInfoW(hMonitor, &monInfo))
        {
            return TRUE;
        }

        NS::MonitorInfo info;
        info.Name = monInfo.szDevice;
        info.bIsPrimary = (monInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
        info.DPI = GetMonitorDPI(hMonitor);

        // ディスプレイ矩形
        info.DisplayRect.Left = monInfo.rcMonitor.left;
        info.DisplayRect.Top = monInfo.rcMonitor.top;
        info.DisplayRect.Right = monInfo.rcMonitor.right;
        info.DisplayRect.Bottom = monInfo.rcMonitor.bottom;

        // ワークエリア
        info.WorkArea.Left = monInfo.rcWork.left;
        info.WorkArea.Top = monInfo.rcWork.top;
        info.WorkArea.Right = monInfo.rcWork.right;
        info.WorkArea.Bottom = monInfo.rcWork.bottom;

        // ネイティブ解像度
        info.NativeWidth = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
        info.NativeHeight = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;

        // デバイス情報からID取得
        DISPLAY_DEVICEW dd = {};
        dd.cb = sizeof(dd);
        if (::EnumDisplayDevicesW(monInfo.szDevice, 0, &dd, EDD_GET_DEVICE_INTERFACE_NAME))
        {
            info.ID = dd.DeviceID;
        }

        // 最大解像度を列挙
        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);
        int32_t maxW = 0, maxH = 0;
        for (DWORD i = 0; ::EnumDisplaySettingsW(monInfo.szDevice, i, &dm); ++i)
        {
            maxW = std::max(maxW, static_cast<int32_t>(dm.dmPelsWidth));
            maxH = std::max(maxH, static_cast<int32_t>(dm.dmPelsHeight));
        }
        info.MaxResolutionWidth = maxW > 0 ? maxW : info.NativeWidth;
        info.MaxResolutionHeight = maxH > 0 ? maxH : info.NativeHeight;

        data->OutMonitors->push_back(std::move(info));
        return TRUE;
    }
} // anonymous namespace

namespace NS
{
    // =========================================================================
    // DisplayMetrics 実装 (Phase 9 Windows版で上書き)
    // =========================================================================

    void DisplayMetrics::RebuildDisplayMetrics(DisplayMetrics& OutMetrics)
    {
        OutMetrics.MonitorInfoArray.clear();

        // モニター列挙
        MonitorEnumData data;
        data.OutMonitors = &OutMetrics.MonitorInfoArray;
        ::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

        // プライマリディスプレイ情報
        OutMetrics.PrimaryDisplayWidth = ::GetSystemMetrics(SM_CXSCREEN);
        OutMetrics.PrimaryDisplayHeight = ::GetSystemMetrics(SM_CYSCREEN);

        // プライマリワークエリア
        RECT workArea;
        if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0))
        {
            OutMetrics.PrimaryDisplayWorkAreaRect.Left = workArea.left;
            OutMetrics.PrimaryDisplayWorkAreaRect.Top = workArea.top;
            OutMetrics.PrimaryDisplayWorkAreaRect.Right = workArea.right;
            OutMetrics.PrimaryDisplayWorkAreaRect.Bottom = workArea.bottom;
        }

        // 仮想デスクトップ
        OutMetrics.VirtualDisplayRect.Left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        OutMetrics.VirtualDisplayRect.Top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        OutMetrics.VirtualDisplayRect.Right =
            OutMetrics.VirtualDisplayRect.Left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        OutMetrics.VirtualDisplayRect.Bottom =
            OutMetrics.VirtualDisplayRect.Top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // セーフゾーン適用
        ApplyDefaultSafeZones(OutMetrics);
    }

    void DisplayMetrics::ApplyDefaultSafeZones(DisplayMetrics& OutMetrics)
    {
        // タイトルセーフゾーン (画面端から 10% 内側がデフォルト)
        float titleRatio = s_debugTitleSafeZoneRatio;
        if (titleRatio > 0.0f)
        {
            float padX = static_cast<float>(OutMetrics.PrimaryDisplayWidth) * titleRatio * 0.5f;
            float padY = static_cast<float>(OutMetrics.PrimaryDisplayHeight) * titleRatio * 0.5f;
            OutMetrics.TitleSafePaddingSize = {padX, padY, padX, padY};
        }
        else
        {
            OutMetrics.TitleSafePaddingSize = {0.0f, 0.0f, 0.0f, 0.0f};
        }

        // アクションセーフゾーン (画面端から 5% 内側がデフォルト)
        float actionRatio = s_debugActionSafeZoneRatio;
        if (actionRatio > 0.0f)
        {
            float padX = static_cast<float>(OutMetrics.PrimaryDisplayWidth) * actionRatio * 0.5f;
            float padY = static_cast<float>(OutMetrics.PrimaryDisplayHeight) * actionRatio * 0.5f;
            OutMetrics.ActionSafePaddingSize = {padX, padY, padX, padY};
        }
        else
        {
            OutMetrics.ActionSafePaddingSize = {0.0f, 0.0f, 0.0f, 0.0f};
        }
    }

    PlatformRect DisplayMetrics::GetMonitorWorkAreaFromPoint(int32_t X, int32_t Y)
    {
        POINT pt = {X, Y};
        HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        MONITORINFO monInfo = {};
        monInfo.cbSize = sizeof(monInfo);
        if (::GetMonitorInfoW(hMon, &monInfo))
        {
            return {monInfo.rcWork.left, monInfo.rcWork.top, monInfo.rcWork.right, monInfo.rcWork.bottom};
        }
        return {0, 0, 1920, 1080};
    }

} // namespace NS
