/// @file RHIGPUProfiler.cpp
/// @brief プロファイルヒストリー実装
#include "RHIGPUProfiler.h"

#include <algorithm>

namespace NS::RHI
{
    //=========================================================================
    // RHIProfileHistory
    //=========================================================================

    void RHIProfileHistory::AddFrame(const RHIFrameProfileData& data)
    {
        if (m_history.size() >= kMaxHistoryFrames) {
            m_history.erase(m_history.begin());
}
        m_history.push_back(data);
    }

    double RHIProfileHistory::GetAverageGPUTime(uint32 frameCount) const
    {
        if (m_history.empty()) {
            return 0.0;
}

        uint32 const count =
            (frameCount < static_cast<uint32>(m_history.size())) ? frameCount : static_cast<uint32>(m_history.size());

        double total = 0.0;
        for (uint32 i = static_cast<uint32>(m_history.size()) - count; i < m_history.size(); ++i) {
            total += m_history[i].totalGPUTime;
}

        return total / count;
    }

    double RHIProfileHistory::GetMaxGPUTime(uint32 frameCount) const
    {
        if (m_history.empty()) {
            return 0.0;
}

        uint32 const count =
            (frameCount < static_cast<uint32>(m_history.size())) ? frameCount : static_cast<uint32>(m_history.size());

        double maxTime = 0.0;
        for (uint32 i = static_cast<uint32>(m_history.size()) - count; i < m_history.size(); ++i)
        {
            maxTime = std::max(m_history[i].totalGPUTime, maxTime);
        }
        return maxTime;
    }

    const RHIFrameProfileData* RHIProfileHistory::GetFrame(uint64 frameNumber) const
    {
        for (const auto& frame : m_history)
        {
            if (frame.frameNumber == frameNumber) {
                return &frame;
}
        }
        return nullptr;
    }

    void RHIProfileHistory::Clear()
    {
        m_history.clear();
    }

} // namespace NS::RHI
