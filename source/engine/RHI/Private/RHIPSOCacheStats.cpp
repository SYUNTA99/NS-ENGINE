/// @file RHIPSOCacheStats.cpp
/// @brief PSOキャッシュ統計・ウォームアップ実装
#include "RHIPSOCacheStats.h"
#include "IRHIDevice.h"

#include <thread>

namespace NS::RHI
{
    //=========================================================================
    // RHIPSOWarmupManager
    //=========================================================================

    RHIPSOWarmupManager::RHIPSOWarmupManager(IRHIDevice* device) : m_device(device) {}

    void RHIPSOWarmupManager::AddPSOForWarmup(const RHIGraphicsPipelineStateDesc& desc)
    {
        (void)desc;
        ++m_totalCount;
    }

    void RHIPSOWarmupManager::AddPSOForWarmup(const RHIComputePipelineStateDesc& desc)
    {
        (void)desc;
        ++m_totalCount;
    }

    void RHIPSOWarmupManager::AddPSOForWarmup(const RHIMeshPipelineStateDesc& desc)
    {
        (void)desc;
        ++m_totalCount;
    }

    void RHIPSOWarmupManager::StartWarmup(RHIPSOWarmupCallback progressCallback)
    {
        m_callback = std::move(progressCallback);
        m_compiledCount.store(0);
        m_cancelled.store(false);

        // PSO コンパイルはバックエンド依存
        // 各PSOを順次コンパイルし、progressCallback に通知
        // 実際にはワーカースレッドで非同期実行する
        m_compiledCount.store(m_totalCount);

        if (m_callback)
        {
            m_callback(GetProgress());
        }
    }

    void RHIPSOWarmupManager::WaitForCompletion()
    {
        // 非同期コンパイル待ち（バックエンド依存）
        while (m_compiledCount.load() < m_totalCount && !m_cancelled.load())
        {
            std::this_thread::yield();
        }
    }

    RHIPSOWarmupProgress RHIPSOWarmupManager::GetProgress() const
    {
        RHIPSOWarmupProgress progress;
        progress.totalPSOs = m_totalCount;
        progress.compiledPSOs = m_compiledCount.load();
        progress.isComplete = (progress.compiledPSOs >= progress.totalPSOs);
        progress.elapsedTimeUs = 0;        // タイマー依存
        progress.estimatedRemainingUs = 0; // 推定値
        return progress;
    }

    void RHIPSOWarmupManager::Cancel()
    {
        m_cancelled.store(true);
    }

    //=========================================================================
    // デバッグ出力関数
    //=========================================================================

    void RHIPrintPSOCacheStats(const RHIPSOCacheStats& stats)
    {
        (void)stats;
        // ログ出力はプラットフォーム依存
    }

    void RHIDrawPSOCacheImGui(IRHIPSOCacheTracker* tracker)
    {
        (void)tracker;
        // ImGui依存
    }

    void RHIDrawPSOCompilationGraph(IRHIPSOCacheTracker* tracker)
    {
        (void)tracker;
        // ImGui依存
    }

} // namespace NS::RHI
