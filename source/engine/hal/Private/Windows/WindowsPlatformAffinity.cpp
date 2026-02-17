/// @file WindowsPlatformAffinity.cpp
/// @brief Windows固有のスレッドアフィニティ・優先度管理実装

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Windows/WindowsPlatformAffinity.h"

namespace NS
{
    CPUTopology WindowsPlatformAffinity::s_topology = {};
    bool WindowsPlatformAffinity::s_topologyInitialized = false;

    void WindowsPlatformAffinity::InitializeTopology()
    {
        if (s_topologyInitialized)
        {
            return;
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_topology.logicalProcessorCount = sysInfo.dwNumberOfProcessors;

        // 物理コア数取得
        DWORD length = 0;
        GetLogicalProcessorInformation(nullptr, &length);
        if (length > 0)
        {
            auto* buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(HeapAlloc(GetProcessHeap(), 0, length));
            if (buffer && GetLogicalProcessorInformation(buffer, &length))
            {
                DWORD offset = 0;
                uint32 physicalCores = 0;
                while (offset < length)
                {
                    auto* ptr = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(
                        reinterpret_cast<uint8*>(buffer) + offset);
                    if (ptr->Relationship == RelationProcessorCore)
                    {
                        physicalCores++;
                    }
                    offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                }
                s_topology.physicalCoreCount = physicalCores;
            }
            if (buffer)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
            }
        }

        // ハイブリッドCPU検出（GetSystemCpuSetInformation使用）
        s_topology.isHybridCPU = false;
        s_topology.performanceCoreCount = s_topology.physicalCoreCount;
        s_topology.efficiencyCoreCount = 0;
        s_topology.performanceCoreMask =
            (s_topology.logicalProcessorCount >= 64) ? UINT64_MAX : ((1ULL << s_topology.logicalProcessorCount) - 1);
        s_topology.efficiencyCoreMask = 0;

        // GetSystemCpuSetInformation は Windows 10 1607+ で利用可能
        using GetSystemCpuSetInformationFn = BOOL(WINAPI*)(PSYSTEM_CPU_SET_INFORMATION, ULONG, PULONG, HANDLE, ULONG);
        HMODULE hKernel32 = ::GetModuleHandleW(L"kernel32.dll");
        auto pGetSystemCpuSetInfo = hKernel32 ? reinterpret_cast<GetSystemCpuSetInformationFn>(
                                                    ::GetProcAddress(hKernel32, "GetSystemCpuSetInformation"))
                                              : nullptr;

        if (pGetSystemCpuSetInfo)
        {
            ULONG cpuSetInfoLength = 0;
            pGetSystemCpuSetInfo(nullptr, 0, &cpuSetInfoLength, GetCurrentProcess(), 0);
            if (cpuSetInfoLength > 0)
            {
                auto* cpuSetBuffer = static_cast<uint8*>(HeapAlloc(GetProcessHeap(), 0, cpuSetInfoLength));
                if (cpuSetBuffer && pGetSystemCpuSetInfo(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpuSetBuffer),
                                                         cpuSetInfoLength,
                                                         &cpuSetInfoLength,
                                                         GetCurrentProcess(),
                                                         0))
                {
                    uint8 minEfficiency = 0xFF;
                    uint8 maxEfficiency = 0;
                    ULONG offset2 = 0;
                    auto* info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpuSetBuffer);

                    // まずEfficiencyClassの範囲を調査
                    while (offset2 < cpuSetInfoLength)
                    {
                        info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpuSetBuffer + offset2);
                        if (info->Type == CpuSetInformation)
                        {
                            uint8 eff = info->CpuSet.EfficiencyClass;
                            if (eff < minEfficiency)
                                minEfficiency = eff;
                            if (eff > maxEfficiency)
                                maxEfficiency = eff;
                        }
                        offset2 += info->Size;
                    }

                    // 異なるEfficiencyClassが存在 → ハイブリッドCPU
                    if (maxEfficiency > minEfficiency)
                    {
                        s_topology.isHybridCPU = true;
                        s_topology.performanceCoreMask = 0;
                        s_topology.efficiencyCoreMask = 0;
                        uint32 pCores = 0;
                        uint32 eCores = 0;

                        offset2 = 0;
                        while (offset2 < cpuSetInfoLength)
                        {
                            info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpuSetBuffer + offset2);
                            if (info->Type == CpuSetInformation)
                            {
                                uint8 logicalIdx = info->CpuSet.LogicalProcessorIndex;
                                if (logicalIdx < 64)
                                {
                                    if (info->CpuSet.EfficiencyClass == maxEfficiency)
                                    {
                                        s_topology.performanceCoreMask |= (1ULL << logicalIdx);
                                        pCores++;
                                    }
                                    else
                                    {
                                        s_topology.efficiencyCoreMask |= (1ULL << logicalIdx);
                                        eCores++;
                                    }
                                }
                            }
                            offset2 += info->Size;
                        }
                        s_topology.performanceCoreCount = pCores;
                        s_topology.efficiencyCoreCount = eCores;
                    }
                }
                if (cpuSetBuffer)
                {
                    HeapFree(GetProcessHeap(), 0, cpuSetBuffer);
                }
            }
        }

        s_topologyInitialized = true;
    }

    uint64 WindowsPlatformAffinity::GetAffinityMask(ThreadType type)
    {
        InitializeTopology();

        const uint64 allCores = (s_topology.logicalProcessorCount >= 64) ? UINT64_MAX : ((1ULL << s_topology.logicalProcessorCount) - 1);

        // ハイブリッドCPU対応
        if (s_topology.isHybridCPU)
        {
            switch (type)
            {
            case ThreadType::MainGame:
            case ThreadType::Rendering:
            case ThreadType::RHI:
            case ThreadType::Audio:
                return s_topology.performanceCoreMask;

            case ThreadType::Loading:
            case ThreadType::Background:
                return s_topology.efficiencyCoreMask;

            default:
                return allCores;
            }
        }

        // 通常CPU：全コア使用
        return allCores;
    }

    ThreadPriority WindowsPlatformAffinity::GetDefaultPriority(ThreadType type)
    {
        switch (type)
        {
        case ThreadType::MainGame:
            return ThreadPriority::Normal;
        case ThreadType::Rendering:
            return ThreadPriority::AboveNormal;
        case ThreadType::RHI:
            return ThreadPriority::AboveNormal;
        case ThreadType::Audio:
            return ThreadPriority::TimeCritical;
        case ThreadType::TaskGraph:
            return ThreadPriority::Normal;
        case ThreadType::Pool:
            return ThreadPriority::Normal;
        case ThreadType::Loading:
            return ThreadPriority::BelowNormal;
        case ThreadType::Background:
            return ThreadPriority::Lowest;
        default:
            return ThreadPriority::Normal;
        }
    }

    const CPUTopology& WindowsPlatformAffinity::GetCPUTopology()
    {
        InitializeTopology();
        return s_topology;
    }

    bool WindowsPlatformAffinity::SetCurrentThreadAffinity(uint64 mask)
    {
        DWORD_PTR result = ::SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(mask));
        return result != 0;
    }

    bool WindowsPlatformAffinity::SetCurrentThreadPriority(ThreadPriority priority)
    {
        return ::SetThreadPriority(GetCurrentThread(), ToWindowsPriority(priority)) != 0;
    }

    int32 WindowsPlatformAffinity::ToWindowsPriority(ThreadPriority priority)
    {
        switch (priority)
        {
        case ThreadPriority::TimeCritical:
            return THREAD_PRIORITY_TIME_CRITICAL;
        case ThreadPriority::Highest:
            return THREAD_PRIORITY_HIGHEST;
        case ThreadPriority::AboveNormal:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        case ThreadPriority::Normal:
            return THREAD_PRIORITY_NORMAL;
        case ThreadPriority::BelowNormal:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case ThreadPriority::Lowest:
            return THREAD_PRIORITY_LOWEST;
        case ThreadPriority::SlightlyBelowNormal:
            return THREAD_PRIORITY_BELOW_NORMAL;
        default:
            return THREAD_PRIORITY_NORMAL;
        }
    }

    uint32 WindowsPlatformAffinity::GetCurrentProcessorNumber()
    {
        return ::GetCurrentProcessorNumber();
    }

    void WindowsPlatformAffinity::Sleep(uint32 milliseconds)
    {
        ::Sleep(milliseconds);
    }

    void WindowsPlatformAffinity::YieldThread()
    {
        ::SwitchToThread();
    }
} // namespace NS
