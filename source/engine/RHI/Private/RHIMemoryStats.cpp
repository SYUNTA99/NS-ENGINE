/// @file RHIMemoryStats.cpp
/// @brief GPUメモリ統計・監視実装
#include "RHIMemoryStats.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIMemoryStats
    //=========================================================================

    const char* RHIMemoryStats::GetCategoryName(ERHIResourceCategory category)
    {
        switch (category)
        {
        case ERHIResourceCategory::Buffer:
            return "Buffer";
        case ERHIResourceCategory::Texture:
            return "Texture";
        case ERHIResourceCategory::RenderTarget:
            return "RenderTarget";
        case ERHIResourceCategory::DepthStencil:
            return "DepthStencil";
        case ERHIResourceCategory::Shader:
            return "Shader";
        case ERHIResourceCategory::PipelineState:
            return "PipelineState";
        case ERHIResourceCategory::QueryHeap:
            return "QueryHeap";
        case ERHIResourceCategory::AccelerationStructure:
            return "AccelerationStructure";
        case ERHIResourceCategory::Descriptor:
            return "Descriptor";
        case ERHIResourceCategory::Staging:
            return "Staging";
        case ERHIResourceCategory::Other:
            return "Other";
        default:
            return "Unknown";
        }
    }

    //=========================================================================
    // RHIMemoryMonitor
    //=========================================================================

    RHIMemoryMonitor::RHIMemoryMonitor(IRHIMemoryTracker* tracker) : m_tracker(tracker) {}

    void RHIMemoryMonitor::SetWarningCallback(RHIMemoryWarningCallback callback)
    {
        m_callback = std::move(callback);
    }

    void RHIMemoryMonitor::SetWarningThresholds(float low, float medium, float high)
    {
        m_lowThreshold = low;
        m_mediumThreshold = medium;
        m_highThreshold = high;
    }

    void RHIMemoryMonitor::Update()
    {
        if (m_tracker == nullptr)
        {
            return;
        }

        RHIMemoryStats const stats = m_tracker->GetStats();
        if (stats.budgetBytes == 0)
        {
            return;
        }

        float const usage = static_cast<float>(stats.totalAllocatedBytes) / static_cast<float>(stats.budgetBytes);

        ERHIMemoryWarningLevel newLevel = ERHIMemoryWarningLevel::None;
        if (usage >= 1.0F)
        {
            newLevel = ERHIMemoryWarningLevel::Critical;
        }
        else if (usage >= m_highThreshold)
        {
            newLevel = ERHIMemoryWarningLevel::High;
        }
        else if (usage >= m_mediumThreshold)
        {
            newLevel = ERHIMemoryWarningLevel::Medium;
        }
        else if (usage >= m_lowThreshold)
        {
            newLevel = ERHIMemoryWarningLevel::Low;
        }

        if (newLevel != m_currentLevel)
        {
            m_currentLevel = newLevel;
            if (m_callback && newLevel != ERHIMemoryWarningLevel::None)
            {
                m_callback(newLevel, stats);
            }
        }
    }

    //=========================================================================
    // デバッグ出力関数
    //=========================================================================

    void RHIPrintMemoryStats(const RHIMemoryStats& stats)
    {
        (void)stats;
        // ログ出力はプラットフォーム依存
    }

    void RHIDrawMemoryStatsImGui(IRHIMemoryTracker* tracker)
    {
        (void)tracker;
        // ImGui依存
    }

    void RHIDrawMemoryGraph(const RHIMemoryStats& stats)
    {
        (void)stats;
        // ImGui依存
    }

} // namespace NS::RHI
