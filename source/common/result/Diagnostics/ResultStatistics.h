/// @file ResultStatistics.h
/// @brief エラー統計収集
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Core/ResultConfig.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace NS::result
{

    /// エラー発生記録
    struct ErrorRecord
    {
        ::NS::Result result;
        std::uint64_t count = 0;
        std::uint64_t firstOccurrence = 0;
        std::uint64_t lastOccurrence = 0;
    };

    /// モジュール別統計
    struct ModuleStats
    {
        int moduleId = 0;
        std::uint64_t totalErrors = 0;
        std::uint64_t uniqueErrors = 0;
        std::vector<ErrorRecord> topErrors;
    };

    /// 統計サマリー
    struct StatsSummary
    {
        std::uint64_t totalErrors = 0;
        std::uint64_t totalSuccess = 0;
        std::uint64_t uniqueErrors = 0;
        double errorRate = 0.0;
        std::chrono::milliseconds uptime{0};
        std::vector<ModuleStats> moduleStats;
    };

    /// エラー統計収集
    class ResultStatistics
    {
    public:
        /// シングルトン取得
        [[nodiscard]] static ResultStatistics& Instance() noexcept;

        //=========================================================================
        // 記録
        //=========================================================================

        /// エラーを記録
        void RecordError(::NS::Result result) noexcept;

        /// 成功を記録
        void RecordSuccess() noexcept;

        /// バッチ記録（パフォーマンス最適化）
        void RecordBatch(const ::NS::Result* results, std::size_t count) noexcept;

        //=========================================================================
        // 照会
        //=========================================================================

        /// 統計サマリーを取得
        [[nodiscard]] StatsSummary GetSummary() const noexcept;

        /// 頻出エラーを取得
        [[nodiscard]] std::vector<ErrorRecord> GetTopErrors(std::size_t count = 10) const noexcept;

        /// モジュール別統計を取得
        [[nodiscard]] ModuleStats GetModuleStats(int moduleId) const noexcept;

        /// 特定エラーの発生回数を取得
        [[nodiscard]] std::uint64_t GetErrorCount(::NS::Result result) const noexcept;

        /// 総エラー数
        [[nodiscard]] std::uint64_t GetTotalErrors() const noexcept;

        /// 総成功数
        [[nodiscard]] std::uint64_t GetTotalSuccess() const noexcept;

        /// エラー率を取得
        [[nodiscard]] double GetErrorRate() const noexcept;

        //=========================================================================
        // 管理
        //=========================================================================

        /// 統計をリセット
        void Reset() noexcept;

        /// 統計の有効/無効切り替え
        void SetEnabled(bool enabled) noexcept;
        [[nodiscard]] bool IsEnabled() const noexcept;

        /// サンプリングレートを設定（0.0-1.0）
        void SetSamplingRate(double rate) noexcept;

    private:
        ResultStatistics();

        struct ErrorEntry
        {
            std::atomic<std::uint64_t> count{0};
            std::atomic<std::uint64_t> firstOccurrence{0};
            std::atomic<std::uint64_t> lastOccurrence{0};
        };

        static constexpr std::size_t kMaxTrackedErrors = 4096;

        std::atomic<bool> m_enabled{true};
        std::atomic<double> m_samplingRate{1.0};
        std::atomic<std::uint64_t> m_totalErrors{0};
        std::atomic<std::uint64_t> m_totalSuccess{0};
        std::chrono::steady_clock::time_point m_startTime;

        mutable std::mutex m_mutex;
        std::unordered_map<std::uint32_t, ErrorEntry> m_errors;
    };

    //=========================================================================
    // グローバル関数
    //=========================================================================

#if NS_RESULT_ENABLE_STATISTICS

    /// エラーを記録
    inline void RecordError(::NS::Result result) noexcept
    {
        ResultStatistics::Instance().RecordError(result);
    }

    /// 成功を記録
    inline void RecordSuccess() noexcept
    {
        ResultStatistics::Instance().RecordSuccess();
    }

    /// Resultを記録（成功/失敗を自動判定）
    inline void RecordResult(::NS::Result result) noexcept
    {
        if (result.IsSuccess())
        {
            RecordSuccess();
        }
        else
        {
            RecordError(result);
        }
    }

#else

    // リリースビルド: 何もしない
    inline void RecordError(::NS::Result /*result*/) noexcept {}
    inline void RecordSuccess() noexcept {}
    inline void RecordResult(::NS::Result /*result*/) noexcept {}

#endif

} // namespace NS::result
