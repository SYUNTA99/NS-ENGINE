/// @file ModuleId.cpp
/// @brief モジュール名取得の実装
#include "common/result/Module/ModuleId.h"

namespace NS { namespace result {

    const char* GetModuleName(int moduleId) noexcept
    {
        switch (moduleId)
        {
        // Core
        case ModuleId::Common:
            return "Common";

        // System
        case ModuleId::FileSystem:
            return "FileSystem";
        case ModuleId::Os:
            return "Os";
        case ModuleId::Memory:
            return "Memory";
        case ModuleId::Network:
            return "Network";
        case ModuleId::Thread:
            return "Thread";

        // Engine
        case ModuleId::Graphics:
            return "Graphics";
        case ModuleId::Ecs:
            return "Ecs";
        case ModuleId::Audio:
            return "Audio";
        case ModuleId::Input:
            return "Input";
        case ModuleId::Resource:
            return "Resource";
        case ModuleId::Scene:
            return "Scene";
        case ModuleId::Physics:
            return "Physics";

        // Application
        case ModuleId::Application:
            return "Application";
        case ModuleId::Ui:
            return "Ui";
        case ModuleId::Script:
            return "Script";

        default:
            if (moduleId >= ModuleId::UserBegin && moduleId < ModuleId::UserEnd)
            {
                return "User";
            }
            return "Unknown";
        }
    }

}} // namespace NS::result
