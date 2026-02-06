/// @file WindowsPlatformMemory.cpp
/// @brief Windows固有のメモリ管理実装

// Windows APIはWIN32_LEAN_AND_MEANで最小化（先にインクルード）
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

#include "Windows/WindowsPlatformMemory.h"

#pragma comment(lib, "psapi.lib")

namespace NS
{
    // 静的メンバの実体
    bool GenericPlatformMemory::s_initialized = false;
    PlatformMemoryConstants GenericPlatformMemory::s_constants = {};

    void WindowsPlatformMemory::Init()
    {
        if (s_initialized)
        {
            return;
        }

        // システム情報取得
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        s_constants.pageSize = static_cast<SIZE_T>(sysInfo.dwPageSize);
        s_constants.allocationGranularity = static_cast<SIZE_T>(sysInfo.dwAllocationGranularity);
        s_constants.numberOfCores = static_cast<uint32>(sysInfo.dwNumberOfProcessors);
        s_constants.numberOfThreads = static_cast<uint32>(sysInfo.dwNumberOfProcessors);

        // メモリ情報取得
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            s_constants.totalPhysical = memStatus.ullTotalPhys;
            s_constants.totalVirtual = memStatus.ullTotalVirtual;
        }

        // キャッシュラインサイズ取得
        // GetLogicalProcessorInformationで取得可能だが、一般的に64バイト
        s_constants.cacheLineSize = 64;

        // 物理コア数の正確な取得（GetLogicalProcessorInformation使用）
        DWORD length = 0;
        GetLogicalProcessorInformation(nullptr, &length);
        if (length > 0)
        {
            auto* buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(HeapAlloc(GetProcessHeap(), 0, length));
            if (buffer && GetLogicalProcessorInformation(buffer, &length))
            {
                uint32 physicalCores = 0;
                uint32 logicalProcessors = 0;
                DWORD offset = 0;
                while (offset < length)
                {
                    auto* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(
                        reinterpret_cast<uint8*>(buffer) + offset);

                    if (info->Relationship == RelationProcessorCore)
                    {
                        physicalCores++;
                        // 各コアの論理プロセッサ数をカウント
                        DWORD_PTR mask = info->ProcessorMask;
                        while (mask)
                        {
                            logicalProcessors += (mask & 1);
                            mask >>= 1;
                        }
                    }
                    else if (info->Relationship == RelationCache && info->Cache.Level == 1)
                    {
                        s_constants.cacheLineSize = static_cast<SIZE_T>(info->Cache.LineSize);
                    }
                    offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                }
                s_constants.numberOfCores = physicalCores;
                s_constants.numberOfThreads = logicalProcessors;
            }
            if (buffer)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
            }
        }

        s_initialized = true;
    }

    bool WindowsPlatformMemory::IsInitialized()
    {
        return s_initialized;
    }

    PlatformMemoryStats WindowsPlatformMemory::GetStats()
    {
        PlatformMemoryStats stats;

        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            stats.availablePhysical = memStatus.ullAvailPhys;
            stats.availableVirtual = memStatus.ullAvailVirtual;
            stats.usedPhysical = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
            stats.usedVirtual = memStatus.ullTotalVirtual - memStatus.ullAvailVirtual;
        }

        // プロセスのメモリ情報取得
        PROCESS_MEMORY_COUNTERS_EX pmc;
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
        {
            stats.peakUsedPhysical = pmc.PeakWorkingSetSize;
            stats.peakUsedVirtual = pmc.PeakPagefileUsage;
        }

        return stats;
    }

    const PlatformMemoryConstants& WindowsPlatformMemory::GetConstants()
    {
        return s_constants;
    }

    void* WindowsPlatformMemory::BinnedAllocFromOS(SIZE_T size)
    {
        return ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

    void WindowsPlatformMemory::BinnedFreeToOS(void* ptr, SIZE_T /*size*/)
    {
        if (ptr)
        {
            ::VirtualFree(ptr, 0, MEM_RELEASE);
        }
    }

    void* WindowsPlatformMemory::VirtualReserve(SIZE_T size)
    {
        return ::VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    }

    bool WindowsPlatformMemory::VirtualCommit(void* ptr, SIZE_T size)
    {
        return ::VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
    }

    bool WindowsPlatformMemory::VirtualDecommit(void* ptr, SIZE_T size)
    {
        return ::VirtualFree(ptr, size, MEM_DECOMMIT) != 0;
    }

    void WindowsPlatformMemory::VirtualFree(void* ptr, SIZE_T /*size*/)
    {
        if (ptr)
        {
            ::VirtualFree(ptr, 0, MEM_RELEASE);
        }
    }
} // namespace NS
