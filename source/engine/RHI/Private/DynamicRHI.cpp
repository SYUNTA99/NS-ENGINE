/// @file DynamicRHI.cpp
/// @brief IDynamicRHI グローバルインスタンス管理とモジュール登録
#include "Common/Logging/Logging.h"
#include "IDynamicRHI.h"
#include "IDynamicRHIModule.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // グローバルRHIインスタンス
    //=========================================================================

    IDynamicRHI* g_dynamicRHI = nullptr;

    //=========================================================================
    // モジュール登録
    //=========================================================================

    static constexpr uint32 kMaxRHIModules = 8;

    struct RHIModuleEntry
    {
        const char* name = nullptr;
        IDynamicRHIModule* module = nullptr;
    };

    static RHIModuleEntry s_registeredModules[kMaxRHIModules] = {};
    static uint32 s_registeredModuleCount = 0;

    RHIModuleRegistrar::RHIModuleRegistrar(const char* name, IDynamicRHIModule* module)
    {
        if (s_registeredModuleCount < kMaxRHIModules)
        {
            s_registeredModules[s_registeredModuleCount].name = name;
            s_registeredModules[s_registeredModuleCount].module = module;
            s_registeredModuleCount++;
        }
    }

    IDynamicRHIModule* const* GetRegisteredRHIModules(uint32& count)
    {
        count = s_registeredModuleCount;
        // Return pointer to first module pointer
        static IDynamicRHIModule* s_modulePointers[kMaxRHIModules] = {};
        for (uint32 i = 0; i < s_registeredModuleCount; ++i)
        {
            s_modulePointers[i] = s_registeredModules[i].module;
        }
        return s_modulePointers;
    }

    IDynamicRHIModule* FindRHIModule(const char* name)
    {
        for (uint32 i = 0; i < s_registeredModuleCount; ++i)
        {
            if (s_registeredModules[i].name && std::strcmp(s_registeredModules[i].name, name) == 0)
            {
                return s_registeredModules[i].module;
            }
        }
        return nullptr;
    }

    bool PlatformCreateDynamicRHI()
    {
        // Preferred backend order (Windows default)
        const char* preferredOrder[] = {"D3D12", "Vulkan"};

        for (const char* backendName : preferredOrder)
        {
            IDynamicRHIModule* module = FindRHIModule(backendName);
            if (module && module->IsSupported())
            {
                IDynamicRHI* rhi = module->CreateRHI();
                if (rhi)
                {
                    if (rhi->Init())
                    {
                        rhi->PostInit();
                        g_dynamicRHI = rhi;
                        char buf[256];
                        snprintf(buf, sizeof(buf), "[RHI] Initialized %s backend", backendName);
                        LOG_INFO(buf);
                        return true;
                    }
                    else
                    {
                        rhi->Shutdown();
                        delete rhi;
                        char buf[256];
                        snprintf(buf, sizeof(buf), "[RHI] Failed to init %s backend, trying next", backendName);
                        LOG_WARN(buf);
                    }
                }
            }
        }

        LOG_ERROR("[RHI] Failed to create any RHI backend");
        return false;
    }

} // namespace NS::RHI
