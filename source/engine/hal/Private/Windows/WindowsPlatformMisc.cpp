/// @file WindowsPlatformMisc.cpp
/// @brief Windows固有のプラットフォーム機能実装

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Windows/WindowsPlatformMisc.h"

#include <cstring>
#include <intrin.h>
#include <objbase.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace NS
{
    uint32 WindowsPlatformMisc::s_cpuFeatures = 0;
    CPUInfo WindowsPlatformMisc::s_cpuInfo = {};
    bool WindowsPlatformMisc::s_initialized = false;

    void WindowsPlatformMisc::PlatformInit()
    {
        if (s_initialized)
        {
            return;
        }

        // CPUIDでベンダー文字列取得
        int cpuInfo[4] = {};
        __cpuid(cpuInfo, 0);

        // ベンダー文字列（EBX, EDX, ECX の順）
        *reinterpret_cast<int*>(s_cpuInfo.vendor) = cpuInfo[1];
        *reinterpret_cast<int*>(s_cpuInfo.vendor + 4) = cpuInfo[3];
        *reinterpret_cast<int*>(s_cpuInfo.vendor + 8) = cpuInfo[2];
        s_cpuInfo.vendor[12] = '\0';

        // ブランド文字列
        char brand[49] = {};
        __cpuid(cpuInfo, 0x80000000);
        if (static_cast<uint32>(cpuInfo[0]) >= 0x80000004)
        {
            __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
            __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
            __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
        }
        strncpy_s(s_cpuInfo.brand, sizeof(s_cpuInfo.brand), brand, _TRUNCATE);

        // 機能フラグ検出
        s_cpuFeatures = GetFeatureBits_X86();

        // コア数
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_cpuInfo.numLogicalProcessors = sysInfo.dwNumberOfProcessors;

        // 物理コア数の取得
        DWORD length = 0;
        GetLogicalProcessorInformation(nullptr, &length);
        if (length > 0)
        {
            auto buffer =
                reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(HeapAlloc(GetProcessHeap(), 0, length));
            if (buffer)
            {
                if (GetLogicalProcessorInformation(buffer, &length))
                {
                    uint32 coreCount = 0;
                    DWORD offset = 0;
                    auto current = buffer;
                    while (offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length)
                    {
                        if (current->Relationship == RelationProcessorCore)
                        {
                            ++coreCount;
                        }
                        ++current;
                        offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                    }
                    s_cpuInfo.numCores = coreCount > 0 ? coreCount : s_cpuInfo.numLogicalProcessors;
                }
                HeapFree(GetProcessHeap(), 0, buffer);
            }
        }

        if (s_cpuInfo.numCores == 0)
        {
            s_cpuInfo.numCores = s_cpuInfo.numLogicalProcessors;
        }

        // キャッシュラインサイズ
        s_cpuInfo.cacheLineSize = 64;

        s_initialized = true;
    }

    uint32 WindowsPlatformMisc::GetCPUInfo()
    {
        if (!s_initialized)
        {
            PlatformInit();
        }
        return s_cpuFeatures;
    }

    CPUInfo WindowsPlatformMisc::GetCPUDetails()
    {
        if (!s_initialized)
        {
            PlatformInit();
        }
        return s_cpuInfo;
    }

    uint32 WindowsPlatformMisc::GetCacheLineSize()
    {
        if (!s_initialized)
        {
            PlatformInit();
        }
        return s_cpuInfo.cacheLineSize;
    }

#if PLATFORM_CPU_X86_FAMILY

    uint32 WindowsPlatformMisc::GetFeatureBits_X86()
    {
        uint32 features = 0;
        int cpuInfo[4] = {};

        __cpuid(cpuInfo, 1);

        // EDXのビット
        if (cpuInfo[3] & (1 << 26))
            features |= CPUFeatureSSE2;

        // ECXのビット
        if (cpuInfo[2] & (1 << 0))
            features |= CPUFeatureSSE3;
        if (cpuInfo[2] & (1 << 9))
            features |= CPUFeatureSSSE3;
        if (cpuInfo[2] & (1 << 19))
            features |= CPUFeatureSSE41;
        if (cpuInfo[2] & (1 << 20))
            features |= CPUFeatureSSE42;
        if (cpuInfo[2] & (1 << 23))
            features |= CPUFeaturePOPCNT;
        if (cpuInfo[2] & (1 << 25))
            features |= CPUFeatureAESNI;

        // AVX（OSサポート確認も必要）
        bool osxsaveSupported = (cpuInfo[2] & (1 << 27)) != 0;
        bool avxSupported = (cpuInfo[2] & (1 << 28)) != 0;

        if (avxSupported && osxsaveSupported)
        {
            // XSAVEが有効でOSがAVX状態を保存しているか確認
            unsigned long long xcr0 = _xgetbv(0);
            if ((xcr0 & 0x6) == 0x6)
            {
                features |= CPUFeatureAVX;

                // FMA3
                if (cpuInfo[2] & (1 << 12))
                    features |= CPUFeatureFMA3;

                // 拡張機能チェック
                __cpuidex(cpuInfo, 7, 0);

                // AVX2
                if (cpuInfo[1] & (1 << 5))
                    features |= CPUFeatureAVX2;
                // BMI1
                if (cpuInfo[1] & (1 << 3))
                    features |= CPUFeatureBMI1;
                // BMI2
                if (cpuInfo[1] & (1 << 8))
                    features |= CPUFeatureBMI2;
                // LZCNT
                __cpuid(cpuInfo, 0x80000001);
                if (cpuInfo[2] & (1 << 5))
                    features |= CPUFeatureLZCNT;

                // AVX512チェック（XSAVE状態も確認）
                __cpuidex(cpuInfo, 7, 0);
                if ((cpuInfo[1] & (1 << 16)) && ((xcr0 & 0xE6) == 0xE6))
                {
                    features |= CPUFeatureAVX512;
                }
            }
        }

        return features;
    }

    bool WindowsPlatformMisc::CheckFeatureBit_X86(uint32 featureBit)
    {
        if (!s_initialized)
        {
            PlatformInit();
        }
        return (s_cpuFeatures & featureBit) != 0;
    }

    bool WindowsPlatformMisc::HasAVX2InstructionSupport()
    {
        return CheckFeatureBit_X86(CPUFeatureAVX2);
    }

    bool WindowsPlatformMisc::HasAVX512InstructionSupport()
    {
        return CheckFeatureBit_X86(CPUFeatureAVX512);
    }

#endif

    const TCHAR* WindowsPlatformMisc::GetPlatformName()
    {
        return TEXT("Windows");
    }

    const TCHAR* WindowsPlatformMisc::GetOSVersion()
    {
        // 簡易実装（詳細なバージョン取得は必要に応じて拡張）
        return TEXT("Windows 10+");
    }

    // =========================================================================
    // COM管理
    // =========================================================================

    static thread_local int32 s_comInitCount = 0;

    void WindowsPlatformMisc::CoInitialize(COMModel model)
    {
        if (s_comInitCount == 0)
        {
            DWORD flags = (model == COMModel::Multithreaded) ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED;

            HRESULT hr = ::CoInitializeEx(nullptr, flags);

            // S_OK または S_FALSE（既に初期化済み）は成功
            if (SUCCEEDED(hr))
            {
                ++s_comInitCount;
            }
        }
        else
        {
            ++s_comInitCount;
        }
    }

    void WindowsPlatformMisc::CoUninitialize()
    {
        if (s_comInitCount > 0)
        {
            --s_comInitCount;
            if (s_comInitCount == 0)
            {
                ::CoUninitialize();
            }
        }
    }

    bool WindowsPlatformMisc::IsCOMInitialized()
    {
        return s_comInitCount > 0;
    }

    // =========================================================================
    // レジストリ・バージョン
    // =========================================================================

    bool WindowsPlatformMisc::QueryRegKey(
        void* key, const TCHAR* subKey, const TCHAR* valueName, TCHAR* outData, SIZE_T outDataSize)
    {
        if (!subKey || !outData || outDataSize == 0)
        {
            return false;
        }

        HKEY hKey = nullptr;
        if (RegOpenKeyExW(static_cast<HKEY>(key), subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return false;
        }

        DWORD type = 0;
        DWORD size = static_cast<DWORD>(outDataSize * sizeof(TCHAR));
        bool success = (RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(outData), &size) ==
                        ERROR_SUCCESS);

        RegCloseKey(hKey);
        return success && (type == REG_SZ || type == REG_EXPAND_SZ);
    }

    bool WindowsPlatformMisc::VerifyWindowsVersion(uint32 majorVersion, uint32 minorVersion, uint32 buildNumber)
    {
        OSVERSIONINFOEXW osvi = {};
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMajorVersion = majorVersion;
        osvi.dwMinorVersion = minorVersion;
        osvi.dwBuildNumber = buildNumber;

        DWORDLONG conditionMask = 0;
        VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

        DWORD typeMask = VER_MAJORVERSION | VER_MINORVERSION;
        if (buildNumber > 0)
        {
            VER_SET_CONDITION(conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
            typeMask |= VER_BUILDNUMBER;
        }

        return VerifyVersionInfoW(&osvi, typeMask, conditionMask) != 0;
    }

    // =========================================================================
    // システム状態
    // =========================================================================

    StorageDeviceType WindowsPlatformMisc::GetStorageDeviceType(const TCHAR* path)
    {
        if (!path)
        {
            return StorageDeviceType::Unknown;
        }

        // ドライブ文字を抽出
        TCHAR rootPath[4] = {};
        rootPath[0] = path[0];
        rootPath[1] = TEXT(':');
        rootPath[2] = TEXT('\\');
        rootPath[3] = TEXT('\0');

        UINT driveType = GetDriveTypeW(rootPath);
        if (driveType != DRIVE_FIXED)
        {
            return StorageDeviceType::Unknown;
        }

        // 簡易実装（詳細な検出にはDeviceIoControlが必要）
        return StorageDeviceType::HDD;
    }

    bool WindowsPlatformMisc::IsRemoteSession()
    {
        return GetSystemMetrics(SM_REMOTESESSION) != 0;
    }

    void WindowsPlatformMisc::PreventScreenSaver()
    {
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
    }
} // namespace NS
