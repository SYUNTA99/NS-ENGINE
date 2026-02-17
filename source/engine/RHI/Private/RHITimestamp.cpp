/// @file RHITimestamp.cpp
/// @brief GPUタイマー・フレームタイムライン実装
#include "RHITimestamp.h"
#include "IRHIBuffer.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIGPUTimer
    //=========================================================================

    bool RHIGPUTimer::Initialize(IRHIDevice* device, uint32 maxMeasurements, uint32 numBufferedFrames)
    {
        m_device = device;
        m_maxMeasurements = maxMeasurements;

        // 各計測にはstart + end の2クエリが必要
        if (!m_queryAllocator.Initialize(device, ERHIQueryType::Timestamp, maxMeasurements * 2, numBufferedFrames))
        {
            return false;
        }

        m_measurements = new Measurement[maxMeasurements];
        m_measurementCount = 0;

        // タイムスタンプ周波数取得
        m_frequency = device->GetTimestampFrequency();

        return true;
    }

    void RHIGPUTimer::Shutdown()
    {
        m_queryAllocator.Shutdown();
        delete[] m_measurements;
        m_measurements = nullptr;
        m_measurementCount = 0;
        m_device = nullptr;
    }

    void RHIGPUTimer::BeginFrame()
    {
        m_queryAllocator.BeginFrame(0);
        m_measurementCount = 0;
    }

    void RHIGPUTimer::EndFrame()
    {
        m_queryAllocator.EndFrame();
    }

    uint32 RHIGPUTimer::BeginTimer(IRHICommandContext* context, const char* name)
    {
        if (m_measurementCount >= m_maxMeasurements)
        {
            return ~0U;
        }

        auto alloc = m_queryAllocator.Allocate(2);
        if (!alloc.IsValid())
        {
            return ~0U;
        }

        uint32 const id = m_measurementCount++;
        m_measurements[id].name = name;
        m_measurements[id].startQueryIndex = alloc.startIndex;
        m_measurements[id].endQueryIndex = alloc.startIndex + 1;

        // タイムスタンプ書き込みコマンド
        if (context != nullptr)
        {
            context->WriteTimestamp(alloc.heap, alloc.startIndex);
        }

        return id;
    }

    void RHIGPUTimer::EndTimer(IRHICommandContext* context, uint32 timerId)
    {
        if (timerId >= m_measurementCount)
        {
            return;
        }

        auto& m = m_measurements[timerId];
        if (context != nullptr)
        {
            context->WriteTimestamp(m_queryAllocator.Allocate(1).heap, m.endQueryIndex);
        }
    }

    bool RHIGPUTimer::AreResultsReady() const
    {
        return m_queryAllocator.AreResultsReady(0);
    }

    double RHIGPUTimer::GetTimerResult(uint32 timerId) const
    {
        if (timerId >= m_measurementCount || m_frequency == 0)
        {
            return 0.0;
        }

        IRHIBuffer* resultBuffer = m_queryAllocator.GetResultBuffer(0);
        if (resultBuffer == nullptr)
        {
            return 0.0;
        }

        RHIMapResult mapResult = resultBuffer->Map(ERHIMapMode::Read);
        if (!mapResult.IsValid())
        {
            return 0.0;
        }

        const auto* timestamps = static_cast<const uint64*>(mapResult.data);
        const auto& m = m_measurements[timerId];

        uint64 const start = timestamps[m.startQueryIndex];
        uint64 const end = timestamps[m.endQueryIndex];
        double const ms = static_cast<double>(end - start) * 1000.0 / m_frequency;

        resultBuffer->Unmap();
        return ms;
    }

    double RHIGPUTimer::GetTimerResult(const char* name) const
    {
        if (name == nullptr)
        {
            return 0.0;
        }

        for (uint32 i = 0; i < m_measurementCount; ++i)
        {
            if ((m_measurements[i].name != nullptr) && std::strcmp(m_measurements[i].name, name) == 0)
            {
                return GetTimerResult(i);
            }
        }
        return 0.0;
    }

    uint32 RHIGPUTimer::GetAllResults(TimerResult* outResults, uint32 maxResults) const
    {
        uint32 const count = (m_measurementCount < maxResults) ? m_measurementCount : maxResults;

        for (uint32 i = 0; i < count; ++i)
        {
            outResults[i].name = m_measurements[i].name;
            outResults[i].milliseconds = GetTimerResult(i);
        }
        return count;
    }

    //=========================================================================
    // RHIFrameTimeline
    //=========================================================================

    bool RHIFrameTimeline::Initialize(IRHIDevice* device, uint32 maxEntries, uint32 numBufferedFrames)
    {
        m_device = device;
        m_maxEntries = maxEntries;

        // 各エントリにはstart + end の2クエリ + フレーム全体の2クエリ
        if (!m_queryAllocator.Initialize(device, ERHIQueryType::Timestamp, (maxEntries * 2) + 2, numBufferedFrames))
        {
            return false;
        }

        m_resultEntries = new RHIFrameTimelineEntry[maxEntries];
        m_resultEntryCount = 0;
        m_currentLevel = 0;
        m_frequency = device->GetTimestampFrequency();

        return true;
    }

    void RHIFrameTimeline::Shutdown()
    {
        m_queryAllocator.Shutdown();
        delete[] m_resultEntries;
        m_resultEntries = nullptr;
        m_resultEntryCount = 0;
        m_device = nullptr;
    }

    void RHIFrameTimeline::BeginFrame(IRHICommandContext* context)
    {
        m_queryAllocator.BeginFrame(0);
        m_resultEntryCount = 0;
        m_currentLevel = 0;

        // フレーム開始タイムスタンプ
        if (context != nullptr)
        {
            auto alloc = m_queryAllocator.Allocate(1);
            if (alloc.IsValid())
            {
                context->WriteTimestamp(alloc.heap, alloc.startIndex);
            }
        }
    }

    void RHIFrameTimeline::EndFrame(IRHICommandContext* context)
    {
        // フレーム終了タイムスタンプ
        if (context != nullptr)
        {
            auto alloc = m_queryAllocator.Allocate(1);
            if (alloc.IsValid())
            {
                context->WriteTimestamp(alloc.heap, alloc.startIndex);
            }
        }

        m_queryAllocator.EndFrame();
    }

    void RHIFrameTimeline::BeginSection(IRHICommandContext* context, const char* name, uint32 color)
    {
        if (m_resultEntryCount >= m_maxEntries)
        {
            return;
        }

        auto& entry = m_resultEntries[m_resultEntryCount++];
        entry.name = name;
        entry.level = m_currentLevel++;
        entry.color = color;
        entry.startMs = 0.0; // 後でタイムスタンプから計算
        entry.endMs = 0.0;

        if (context != nullptr)
        {
            auto alloc = m_queryAllocator.Allocate(1);
            if (alloc.IsValid())
            {
                context->WriteTimestamp(alloc.heap, alloc.startIndex);
            }
        }
    }

    void RHIFrameTimeline::EndSection(IRHICommandContext* context)
    {
        if (m_currentLevel > 0)
        {
            --m_currentLevel;
        }

        if (context != nullptr)
        {
            auto alloc = m_queryAllocator.Allocate(1);
            if (alloc.IsValid())
            {
                context->WriteTimestamp(alloc.heap, alloc.startIndex);
            }
        }
    }

    void RHIFrameTimeline::InsertMarker(IRHICommandContext* context, const char* name, uint32 color)
    {
        if (m_resultEntryCount >= m_maxEntries)
        {
            return;
        }

        auto& entry = m_resultEntries[m_resultEntryCount++];
        entry.name = name;
        entry.level = m_currentLevel;
        entry.color = color;
        entry.startMs = 0.0;
        entry.endMs = 0.0;

        if (context != nullptr)
        {
            auto alloc = m_queryAllocator.Allocate(1);
            if (alloc.IsValid())
            {
                context->WriteTimestamp(alloc.heap, alloc.startIndex);
            }
        }
    }

    bool RHIFrameTimeline::AreResultsReady() const
    {
        return m_queryAllocator.AreResultsReady(0);
    }

    double RHIFrameTimeline::GetFrameTimeMs() const
    {
        // フレーム全体の時間はresultバッファの最初と最後のタイムスタンプから算出
        // バックエンド依存の結果読み取り
        return 0.0;
    }

} // namespace NS::RHI
