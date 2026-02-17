/// @file IRHICommandAllocator.cpp
/// @brief コマンドアロケーター実装
#include "IRHICommandAllocator.h"
#include "IRHIFence.h"

namespace NS::RHI
{
    //=========================================================================
    // IRHICommandAllocator
    //=========================================================================

    bool IRHICommandAllocator::IsWaitComplete() const
    {
        IRHIFence* fence = GetWaitFence();
        if (fence == nullptr) {
            return true;
}

        return fence->IsCompleted(GetWaitFenceValue());
    }

} // namespace NS::RHI
