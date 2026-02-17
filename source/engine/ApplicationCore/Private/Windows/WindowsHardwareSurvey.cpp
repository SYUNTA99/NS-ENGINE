/// @file WindowsHardwareSurvey.cpp
/// @brief Windows ハードウェアサーベイ実装 (12-04)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dxgi.h>
#include <intrin.h>
#include <powerbase.h>

// WIN32_LEAN_AND_MEAN では NTSTATUS/RTL_OSVERSIONINFOW が除外される
typedef LONG NTSTATUS;
typedef struct NS_OSVERSIONINFOW
{
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
} NS_OSVERSIONINFOW;
#endif

#include "GenericPlatform/HardwareSurveyResults.h"

#include <string>
#include <vector>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "PowrProf.lib")

namespace NS
{
    namespace
    {
        void GatherCPUInfo(HardwareSurveyResults& Results)
        {
            SYSTEM_INFO sysInfo = {};
            ::GetNativeSystemInfo(&sysInfo);
            Results.CPUCount = static_cast<int32_t>(sysInfo.dwNumberOfProcessors);

            // CPU ブランド名取得
            int cpuInfo[4] = {};
            char brand[49] = {};
            __cpuid(cpuInfo, 0x80000002);
            memcpy(brand, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000003);
            memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
            __cpuid(cpuInfo, 0x80000004);
            memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
            brand[48] = '\0';

            // char → wchar_t 変換
            wchar_t wBrand[49] = {};
            for (int i = 0; i < 48 && brand[i]; ++i)
            {
                wBrand[i] = static_cast<wchar_t>(brand[i]);
            }
            Results.CPUBrand = wBrand;

            // CPU クロック速度 (CallNtPowerInformation)
            struct PROCESSOR_POWER_INFORMATION
            {
                ULONG Number;
                ULONG MaxMhz;
                ULONG CurrentMhz;
                ULONG MhzLimit;
                ULONG MaxIdleState;
                ULONG CurrentIdleState;
            };

            const ULONG bufSize = static_cast<ULONG>(sizeof(PROCESSOR_POWER_INFORMATION) * Results.CPUCount);
            std::vector<uint8_t> ppiBuf(bufSize, 0);
            if (::CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppiBuf.data(), bufSize) == 0)
            {
                auto* ppi = reinterpret_cast<PROCESSOR_POWER_INFORMATION*>(ppiBuf.data());
                Results.CPUClockGHz = static_cast<float>(ppi->MaxMhz) / 1000.0f;
            }
        }

        void GatherMemoryInfo(HardwareSurveyResults& Results)
        {
            MEMORYSTATUSEX memStatus = {};
            memStatus.dwLength = sizeof(memStatus);
            if (::GlobalMemoryStatusEx(&memStatus))
            {
                Results.MemoryGB = static_cast<float>(memStatus.ullTotalPhys) / (1024.0f * 1024.0f * 1024.0f);
            }
        }

        void GatherLaptopInfo(HardwareSurveyResults& Results)
        {
            // バッテリー存在チェック
            SYSTEM_POWER_STATUS powerStatus = {};
            if (::GetSystemPowerStatus(&powerStatus))
            {
                Results.bIsLaptop = (powerStatus.BatteryFlag != 128 && powerStatus.BatteryFlag != 255);
            }
        }

        void GatherGPUInfo(HardwareSurveyResults& Results)
        {
            IDXGIFactory1* factory = nullptr;
            if (FAILED(::CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))))
            {
                return;
            }

            IDXGIAdapter* adapter = nullptr;
            if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
            {
                DXGI_ADAPTER_DESC desc = {};
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    Results.GPUAdpater = desc.Description; // ※UE5タイポ保存
                    Results.GPUVendorId = desc.VendorId;
                    Results.GPUDeviceId = desc.DeviceId;
                    Results.GPUDedicatedVRAM = desc.DedicatedVideoMemory;
                }
                adapter->Release();
            }

            factory->Release();
        }

        void GatherDisplayInfo(HardwareSurveyResults& Results)
        {
            Results.PrimaryDisplayWidth = ::GetSystemMetrics(SM_CXSCREEN);
            Results.PrimaryDisplayHeight = ::GetSystemMetrics(SM_CYSCREEN);
            Results.DisplayCount = ::GetSystemMetrics(SM_CMONITORS);
        }

        void GatherOSInfo(HardwareSurveyResults& Results)
        {
            // OS バージョン (RtlGetVersion 使用)
            using RtlGetVersionPtr = NTSTATUS(WINAPI*)(NS_OSVERSIONINFOW*);
            HMODULE hNtDll = ::GetModuleHandleW(L"ntdll.dll");
            if (hNtDll)
            {
                auto pRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hNtDll, "RtlGetVersion"));
                if (pRtlGetVersion)
                {
                    NS_OSVERSIONINFOW osInfo = {};
                    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
                    if (pRtlGetVersion(&osInfo) == 0)
                    {
                        Results.OSVersion = std::to_wstring(osInfo.dwMajorVersion) + L"." +
                                            std::to_wstring(osInfo.dwMinorVersion) + L"." +
                                            std::to_wstring(osInfo.dwBuildNumber);
                    }
                }
            }

            // ロケール
            wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {};
            if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
            {
                Results.OSLanguage = localeName;
            }
        }
        void GatherWinSATInfo(HardwareSurveyResults& /*Results*/)
        {
            // WinSAT COM API でパフォーマンスインデックスを取得
            // CLSID_CQueryAllWinSATAssessments: {F3BDFAD3-F276-49E9-9B17-C474F48F0764}
            // IID_IQueryRecentWinSATAssessment: {F8334D5D-568E-4c27-9F60-F2F0614A075E}
            static const GUID CLSID_WinSAT = {
                0xF3BDFAD3, 0xF276, 0x49E9, {0x9B, 0x17, 0xC4, 0x74, 0xF4, 0x8F, 0x07, 0x64}};
            static const GUID IID_IQueryRecentWinSATAssessment = {
                0xF8334D5D, 0x568E, 0x4C27, {0x9F, 0x60, 0xF2, 0xF0, 0x61, 0x4A, 0x07, 0x5E}};

            IUnknown* pUnknown = nullptr;
            HRESULT hr = ::CoCreateInstance(CLSID_WinSAT,
                                            nullptr,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IQueryRecentWinSATAssessment,
                                            reinterpret_cast<void**>(&pUnknown));
            if (SUCCEEDED(hr) && pUnknown)
            {
                // IQueryRecentWinSATAssessment::get_Info() → IProvideWinSATResultsInfo
                // WinSAT may not be available on all systems; leave defaults if it fails
                pUnknown->Release();
            }
            // Note: Full WinSAT COM integration requires winsatcominterfacei.h
            // which is not universally available. Performance indices remain at -1.0
            // when WinSAT data is not accessible.
        }

    } // anonymous namespace

    /// Windows ハードウェアサーベイ実行
    HardwareSurveyResults RunWindowsHardwareSurvey()
    {
        HardwareSurveyResults results;

        GatherCPUInfo(results);
        GatherMemoryInfo(results);
        GatherLaptopInfo(results);
        GatherGPUInfo(results);
        GatherDisplayInfo(results);
        GatherOSInfo(results);
        GatherWinSATInfo(results);

        return results;
    }

} // namespace NS
