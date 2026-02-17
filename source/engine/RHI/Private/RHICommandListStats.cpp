/// @file RHICommandListStats.cpp
/// @brief コマンドリスト統計・フレーム統計実装
#include "RHICommandListStats.h"

#include <cstdio>

namespace NS::RHI
{
    //=========================================================================
    // RHICommandListStats
    //=========================================================================

    void RHICommandListStats::Accumulate(const RHICommandListStats& other)
    {
        // 描画コール
        draws.drawCalls += other.draws.drawCalls;
        draws.drawIndexedCalls += other.draws.drawIndexedCalls;
        draws.drawInstancedCalls += other.draws.drawInstancedCalls;
        draws.drawIndirectCalls += other.draws.drawIndirectCalls;
        draws.dispatchCalls += other.draws.dispatchCalls;
        draws.dispatchIndirectCalls += other.draws.dispatchIndirectCalls;
        draws.dispatchMeshCalls += other.draws.dispatchMeshCalls;
        draws.dispatchRaysCalls += other.draws.dispatchRaysCalls;

        // 状態変更
        stateChanges.psoChanges += other.stateChanges.psoChanges;
        stateChanges.rootSignatureChanges += other.stateChanges.rootSignatureChanges;
        stateChanges.renderTargetChanges += other.stateChanges.renderTargetChanges;
        stateChanges.viewportChanges += other.stateChanges.viewportChanges;
        stateChanges.scissorChanges += other.stateChanges.scissorChanges;
        stateChanges.blendFactorChanges += other.stateChanges.blendFactorChanges;
        stateChanges.stencilRefChanges += other.stateChanges.stencilRefChanges;
        stateChanges.primitiveTopologyChanges += other.stateChanges.primitiveTopologyChanges;

        // バインディング
        bindings.vertexBufferBinds += other.bindings.vertexBufferBinds;
        bindings.indexBufferBinds += other.bindings.indexBufferBinds;
        bindings.constantBufferBinds += other.bindings.constantBufferBinds;
        bindings.srvBinds += other.bindings.srvBinds;
        bindings.uavBinds += other.bindings.uavBinds;
        bindings.samplerBinds += other.bindings.samplerBinds;
        bindings.descriptorTableBinds += other.bindings.descriptorTableBinds;

        // バリア
        barriers.textureBarriers += other.barriers.textureBarriers;
        barriers.bufferBarriers += other.barriers.bufferBarriers;
        barriers.uavBarriers += other.barriers.uavBarriers;
        barriers.aliasingBarriers += other.barriers.aliasingBarriers;
        barriers.batchedBarriers += other.barriers.batchedBarriers;
        barriers.redundantBarriers += other.barriers.redundantBarriers;

        // メモリ操作
        memoryOps.bufferCopies += other.memoryOps.bufferCopies;
        memoryOps.textureCopies += other.memoryOps.textureCopies;
        memoryOps.bufferUpdates += other.memoryOps.bufferUpdates;
        memoryOps.totalCopyBytes += other.memoryOps.totalCopyBytes;
        memoryOps.totalUpdateBytes += other.memoryOps.totalUpdateBytes;

        commandCount += other.commandCount;
        renderPassCount += other.renderPassCount;
        estimatedGPUCycles += other.estimatedGPUCycles;
    }

    std::string RHICommandListStats::ToSummaryString() const
    {
        char buf[256];
        std::snprintf(buf,
                      sizeof(buf),
                      "Draws: %u  Dispatches: %u  PSO: %u  Barriers: %u  Cmds: %u",
                      draws.GetTotalDrawCalls(),
                      draws.GetTotalDispatchCalls(),
                      stateChanges.psoChanges,
                      barriers.textureBarriers + barriers.bufferBarriers,
                      commandCount);
        return std::string(buf);
    }

    std::string RHICommandListStats::ToDetailedString() const
    {
        char buf[1024];
        std::snprintf(buf,
                      sizeof(buf),
                      "=== Command List Stats ===\n"
                      "Draw: %u  DrawIndexed: %u  DrawInstanced: %u  DrawIndirect: %u\n"
                      "Dispatch: %u  DispatchIndirect: %u  DispatchMesh: %u  DispatchRays: %u\n"
                      "PSO changes: %u  RootSig changes: %u  RT changes: %u\n"
                      "VB binds: %u  IB binds: %u  CB binds: %u  SRV: %u  UAV: %u\n"
                      "Tex barriers: %u  Buf barriers: %u  UAV barriers: %u\n"
                      "Commands: %u  RenderPasses: %u\n",
                      draws.drawCalls,
                      draws.drawIndexedCalls,
                      draws.drawInstancedCalls,
                      draws.drawIndirectCalls,
                      draws.dispatchCalls,
                      draws.dispatchIndirectCalls,
                      draws.dispatchMeshCalls,
                      draws.dispatchRaysCalls,
                      stateChanges.psoChanges,
                      stateChanges.rootSignatureChanges,
                      stateChanges.renderTargetChanges,
                      bindings.vertexBufferBinds,
                      bindings.indexBufferBinds,
                      bindings.constantBufferBinds,
                      bindings.srvBinds,
                      bindings.uavBinds,
                      barriers.textureBarriers,
                      barriers.bufferBarriers,
                      barriers.uavBarriers,
                      commandCount,
                      renderPassCount);
        return std::string(buf);
    }

    //=========================================================================
    // RHIFrameStatsTracker
    //=========================================================================

    void RHIFrameStatsTracker::BeginFrame()
    {
        m_currentFrame = RHIFrameStats{};
        // CPU時間計測はプラットフォーム依存
        m_frameStartTime = 0;
    }

    void RHIFrameStatsTracker::AddCommandListStats(const RHICommandListStats& stats)
    {
        m_currentFrame.accumulated.Accumulate(stats);
        m_currentFrame.commandListCount++;
    }

    void RHIFrameStatsTracker::EndFrame()
    {
        // CPU時間記録
        m_currentFrame.cpuRecordTimeUs = 0; // プラットフォーム依存

        // ピーク更新
        uint32 const totalDraws = m_currentFrame.accumulated.draws.GetTotalDrawCalls();
        uint32 const peakDraws = m_peakFrame.accumulated.draws.GetTotalDrawCalls();
        if (totalDraws > peakDraws) {
            m_peakFrame = m_currentFrame;
}

        // 履歴に記録
        m_history[m_historyIndex] = m_currentFrame;
        m_historyIndex = (m_historyIndex + 1) % kHistorySize;
    }

    RHIFrameStats RHIFrameStatsTracker::GetAverageStats(uint32 frameCount) const
    {
        RHIFrameStats avg{};
        if (frameCount == 0) {
            return avg;
}

        uint32 const count = (frameCount < kHistorySize) ? frameCount : kHistorySize;
        for (uint32 i = 0; i < count; ++i)
        {
            uint32 const idx = (m_historyIndex + kHistorySize - 1 - i) % kHistorySize;
            avg.accumulated.Accumulate(m_history[idx].accumulated);
            avg.commandListCount += m_history[idx].commandListCount;
            avg.cpuRecordTimeUs += m_history[idx].cpuRecordTimeUs;
            avg.gpuExecuteTimeUs += m_history[idx].gpuExecuteTimeUs;
        }

        // 平均化
        avg.accumulated.commandCount /= count;
        avg.commandListCount /= count;
        avg.cpuRecordTimeUs /= count;
        avg.gpuExecuteTimeUs /= count;

        return avg;
    }

    //=========================================================================
    // デバッグ出力関数
    //=========================================================================

    void RHIPrintFrameStats(const RHIFrameStats& stats)
    {
        // ログ出力はプラットフォーム依存
        (void)stats;
    }

    void RHIExportStatsToCSV(const RHIFrameStats* stats, uint32 frameCount, const char* filename)
    {
        (void)stats;
        (void)frameCount;
        (void)filename;
        // ファイル出力はプラットフォーム依存
    }

    void RHIDrawStatsImGui(const RHIFrameStatsTracker& tracker)
    {
        (void)tracker;
        // ImGui描画はImGui依存
    }

} // namespace NS::RHI
