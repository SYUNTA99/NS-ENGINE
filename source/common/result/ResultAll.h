/// @file ResultAll.h
/// @brief Result型システム統合ヘッダー
#pragma once

// Core
#include "common/result/Core/InternalAccessor.h"
#include "common/result/Core/Result.h"
#include "common/result/Core/ResultBase.h"
#include "common/result/Core/ResultConfig.h"
#include "common/result/Core/ResultTraits.h"

// Error
#include "common/result/Error/ErrorCategory.h"
#include "common/result/Error/ErrorDefines.h"
#include "common/result/Error/ErrorRange.h"
#include "common/result/Error/ErrorResultBase.h"

// Module - Core
#include "common/result/Module/CommonResult.h"
#include "common/result/Module/ModuleId.h"

// Module - System
#include "common/result/Module/FileSystemResult.h"
#include "common/result/Module/MemoryResult.h"
#include "common/result/Module/NetworkResult.h"
#include "common/result/Module/OsResult.h"

// Module - Engine
#include "common/result/Module/AudioResult.h"
#include "common/result/Module/EcsResult.h"
#include "common/result/Module/GraphicsResult.h"
#include "common/result/Module/InputResult.h"
#include "common/result/Module/PhysicsResult.h"
#include "common/result/Module/ResourceResult.h"
#include "common/result/Module/SceneResult.h"

// Context
#include "common/result/Context/ErrorChain.h"
#include "common/result/Context/ResultContext.h"
#include "common/result/Context/SourceLocation.h"

// Utility
#include "common/result/Utility/ErrorInfo.h"
#include "common/result/Utility/ResultFormatter.h"
#include "common/result/Utility/ResultMacros.h"
#include "common/result/Utility/ResultRegistry.h"

// Diagnostics
#include "common/result/Diagnostics/ResultLogging.h"
#include "common/result/Diagnostics/ResultStatistics.h"
