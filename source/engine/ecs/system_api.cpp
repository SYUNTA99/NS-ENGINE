//----------------------------------------------------------------------------
//! @file   system_api.cpp
//! @brief  SystemAPI - 静的メンバ変数の定義
//----------------------------------------------------------------------------

#include "system_api.h"

namespace ECS {

// スレッドローカルな現在のSystemState
thread_local SystemState* SystemAPI::currentState_ = nullptr;

} // namespace ECS
