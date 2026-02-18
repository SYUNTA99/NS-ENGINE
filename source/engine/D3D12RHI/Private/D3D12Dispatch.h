/// @file D3D12Dispatch.h
/// @brief D3D12 DispatchTable関数群
#pragma once

#include "RHI/Public/RHIDispatchTable.h"

namespace NS::D3D12RHI
{
    /// DispatchTableにD3D12実装を登録
    void RegisterD3D12DispatchTable(NS::RHI::RHIDispatchTable& table);

} // namespace NS::D3D12RHI
