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
        std::vector<NS::MonitorInfo>* outMonitors = nullptr;
    };

    /// モニター DPI 取得 (Per-Monitor V2 対応)
    int32_t GetMonitorDPI(HMONITOR hMonitor)
    {
        UINT dpiX = 96;
        UINT dpiY = 96;
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
        if (::GetMonitorInfoW(hMonitor, &monInfo) == 0)
        {
            return TRUE;
        }

        NS::MonitorInfo info;
        info.name = monInfo.szDevice;
        info.bIsPrimary = (monInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
        info.dpi = GetMonitorDPI(hMonitor);

        // ディスプレイ矩形
        info.displayRect.left = monInfo.rcMonitor.left;
        info.displayRect.top = monInfo.rcMonitor.top;
        info.displayRect.right = monInfo.rcMonitor.right;
        info.displayRect.bottom = monInfo.rcMonitor.bottom;

        // ワークエリア
        info.workArea.left = monInfo.rcWork.left;
        info.workArea.top = monInfo.rcWork.top;
        info.workArea.right = monInfo.rcWork.right;
        info.workArea.bottom = monInfo.rcWork.bottom;

        // ネイティブ解像度
        info.nativeWidth = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
        info.nativeHeight = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;

        // デバイス情報からID取得
        DISPLAY_DEVICEW dd = {};
        dd.cb = sizeof(dd);
        if (::EnumDisplayDevicesW(monInfo.szDevice, 0, &dd, EDD_GET_DEVICE_INTERFACE_NAME) != 0)
        {
            info.id = dd.DeviceID;
        }

        // 最大解像度を列挙
        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);
        int32_t maxW = 0;
        int32_t maxH = 0;
        for (DWORD i = 0; ::EnumDisplaySettingsW(monInfo.szDevice, i, &dm) != 0; ++i)
        {
            maxW = std::max(maxW, static_cast<int32_t>(dm.dmPelsWidth));
            maxH = std::max(maxH, static_cast<int32_t>(dm.dmPelsHeight));
        }
        info.maxResolutionWidth = maxW > 0 ? maxW : info.nativeWidth;
        info.maxResolutionHeight = maxH > 0 ? maxH : info.nativeHeight;

        data->outMonitors->push_back(std::move(info));
        return TRUE;
    }
} // anonymous namespace

namespace NS
{
    // =========================================================================
    // DisplayMetrics 実装 (Phase 9 Windows版で上書き)
    // =========================================================================

    void DisplayMetrics::RebuildDisplayMetrics(DisplayMetrics& outMetrics)
    {
        outMetrics.monitorInfoArray.clear();

        // モニター列挙
        MonitorEnumData data;
        data.outMonitors = &outMetrics.monitorInfoArray;
        ::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

        // プライマリディスプレイ情報
        outMetrics.primaryDisplayWidth = ::GetSystemMetrics(SM_CXSCREEN);
        outMetrics.primaryDisplayHeight = ::GetSystemMetrics(SM_CYSCREEN);

        // プライマリワークエリア
        RECT workArea;
        if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0) != 0)
        {
            outMetrics.primaryDisplayWorkAreaRect.left = workArea.left;
            outMetrics.primaryDisplayWorkAreaRect.top = workArea.top;
            outMetrics.primaryDisplayWorkAreaRect.right = workArea.right;
            outMetrics.primaryDisplayWorkAreaRect.bottom = workArea.bottom;
        }

        // 仮想デスクトップ
        outMetrics.virtualDisplayRect.left = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        outMetrics.virtualDisplayRect.top = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        outMetrics.virtualDisplayRect.right =
            outMetrics.virtualDisplayRect.left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        outMetrics.virtualDisplayRect.bottom =
            outMetrics.virtualDisplayRect.top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // セーフゾーン適用
        ApplyDefaultSafeZones(outMetrics);
    }

    void DisplayMetrics::ApplyDefaultSafeZones(DisplayMetrics& outMetrics)
    {
        // タイトルセーフゾーン (画面端から 10% 内側がデフォルト)
        float const titleRatio = s_debugTitleSafeZoneRatio;
        if (titleRatio > 0.0F)
        {
            float const padX = static_cast<float>(outMetrics.primaryDisplayWidth) * titleRatio * 0.5F;
            float const padY = static_cast<float>(outMetrics.primaryDisplayHeight) * titleRatio * 0.5F;
            outMetrics.titleSafePaddingSize = {.x=padX, .y=padY, .z=padX, .w=padY};
        }
        else
        {
            outMetrics.titleSafePaddingSize = {.x=0.0F, .y=0.0F, .z=0.0F, .w=0.0F};
        }

        // アクションセーフゾーン (画面端から 5% 内側がデフォルト)
        float const actionRatio = s_debugActionSafeZoneRatio;
        if (actionRatio > 0.0F)
        {
            float const padX = static_cast<float>(outMetrics.primaryDisplayWidth) * actionRatio * 0.5F;
            float const padY = static_cast<float>(outMetrics.primaryDisplayHeight) * actionRatio * 0.5F;
            outMetrics.actionSafePaddingSize = {.x=padX, .y=padY, .z=padX, .w=padY};
        }
        else
        {
            outMetrics.actionSafePaddingSize = {.x=0.0F, .y=0.0F, .z=0.0F, .w=0.0F};
        }
    }

    PlatformRect DisplayMetrics::GetMonitorWorkAreaFromPoint(int32_t x, int32_t y)
    {
        POINT const pt = {x, y};
        HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        MONITORINFO monInfo = {};
        monInfo.cbSize = sizeof(monInfo);
        if (::GetMonitorInfoW(hMon, &monInfo) != 0)
        {
            return {.left=monInfo.rcWork.left, .top=monInfo.rcWork.top, .right=monInfo.rcWork.right, .bottom=monInfo.rcWork.bottom};
        }
        return {.left=0, .top=0, .right=1920, .bottom=1080};
    }

} // namespace NS
