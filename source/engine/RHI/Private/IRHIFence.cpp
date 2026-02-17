/// @file IRHIFence.cpp
/// @brief フェンス値トラッカー実装
#include "IRHIFence.h"
#include "IRHIQueue.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIFenceValueTracker
    //=========================================================================

    uint64 RHIFenceValueTracker::Signal(IRHIQueue* queue)
    {
        uint64 const value = m_nextValue++;
        queue->Signal(m_fence, value);
        return value;
    }

} // namespace NS::RHI
