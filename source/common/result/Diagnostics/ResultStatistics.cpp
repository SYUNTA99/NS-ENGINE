/// @file ResultStatistics.cpp
/// @brief エラー統計収集の実装
#include "common/result/Diagnostics/ResultStatistics.h"

#include "common/result/Core/InternalAccessor.h"

#include <algorithm>
#include <random>

namespace NS { namespace result {

    namespace
    {

        std::uint32_t MakeKey(::NS::Result result) noexcept
        {
            return (static_cast<std::uint32_t>(result.GetModule()) << 16) |
                   static_cast<std::uint32_t>(result.GetDescription());
        }

        std::uint64_t GetCurrentTimestamp() noexcept
        {
            auto now = std::chrono::steady_clock::now();
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        }

        thread_local std::mt19937 t_rng{std::random_device{}()};

        bool ShouldSample(double rate) noexcept
        {
            if (rate >= 1.0)
                return true;
            if (rate <= 0.0)
                return false;
            std::uniform_real_distribution<> dist(0.0, 1.0);
            return dist(t_rng) < rate;
        }

    } // namespace

    ResultStatistics::ResultStatistics() : m_startTime(std::chrono::steady_clock::now()) {}

    ResultStatistics& ResultStatistics::Instance() noexcept
    {
        static ResultStatistics instance;
        return instance;
    }

    void ResultStatistics::RecordError(::NS::Result result) noexcept
    {
        if (!m_enabled.load(std::memory_order_relaxed))
        {
            return;
        }

        if (!ShouldSample(m_samplingRate.load(std::memory_order_relaxed)))
        {
            return;
        }

        m_totalErrors.fetch_add(1, std::memory_order_relaxed);

        const auto key = MakeKey(result);
        const auto timestamp = GetCurrentTimestamp();

        std::lock_guard lock(m_mutex);

        // エントリ数制限
        if (m_errors.size() >= kMaxTrackedErrors && m_errors.find(key) == m_errors.end())
        {
            return;
        }

        auto& entry = m_errors[key];
        entry.count.fetch_add(1, std::memory_order_relaxed);

        std::uint64_t expected = 0;
        entry.firstOccurrence.compare_exchange_strong(expected, timestamp, std::memory_order_relaxed);
        entry.lastOccurrence.store(timestamp, std::memory_order_relaxed);
    }

    void ResultStatistics::RecordSuccess() noexcept
    {
        if (!m_enabled.load(std::memory_order_relaxed))
        {
            return;
        }
        m_totalSuccess.fetch_add(1, std::memory_order_relaxed);
    }

    void ResultStatistics::RecordBatch(const ::NS::Result* results, std::size_t count) noexcept
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            if (results[i].IsSuccess())
            {
                RecordSuccess();
            }
            else
            {
                RecordError(results[i]);
            }
        }
    }

    std::vector<ErrorRecord> ResultStatistics::GetTopErrors(std::size_t count) const noexcept
    {
        std::lock_guard lock(m_mutex);

        std::vector<ErrorRecord> records;
        records.reserve(m_errors.size());

        for (const auto& [key, entry] : m_errors)
        {
            ::NS::Result result = detail::InternalAccessor::ConstructFromRaw(key);
            records.push_back(ErrorRecord{.result = result,
                                          .count = entry.count.load(std::memory_order_relaxed),
                                          .firstOccurrence = entry.firstOccurrence.load(std::memory_order_relaxed),
                                          .lastOccurrence = entry.lastOccurrence.load(std::memory_order_relaxed)});
        }

        std::partial_sort(records.begin(),
                          records.begin() + (std::min)(count, records.size()),
                          records.end(),
                          [](const auto& a, const auto& b) { return a.count > b.count; });

        if (records.size() > count)
        {
            records.resize(count);
        }

        return records;
    }

    ModuleStats ResultStatistics::GetModuleStats(int moduleId) const noexcept
    {
        std::lock_guard lock(m_mutex);

        ModuleStats stats;
        stats.moduleId = moduleId;

        for (const auto& [key, entry] : m_errors)
        {
            int module = static_cast<int>(key >> 16);
            if (module == moduleId)
            {
                stats.uniqueErrors++;
                stats.totalErrors += entry.count.load(std::memory_order_relaxed);

                ::NS::Result result = detail::InternalAccessor::ConstructFromRaw(key);
                stats.topErrors.push_back(
                    ErrorRecord{.result = result,
                                .count = entry.count.load(std::memory_order_relaxed),
                                .firstOccurrence = entry.firstOccurrence.load(std::memory_order_relaxed),
                                .lastOccurrence = entry.lastOccurrence.load(std::memory_order_relaxed)});
            }
        }

        std::sort(stats.topErrors.begin(), stats.topErrors.end(), [](const auto& a, const auto& b) {
            return a.count > b.count;
        });

        return stats;
    }

    std::uint64_t ResultStatistics::GetErrorCount(::NS::Result result) const noexcept
    {
        std::lock_guard lock(m_mutex);
        const auto key = MakeKey(result);
        auto it = m_errors.find(key);
        if (it != m_errors.end())
        {
            return it->second.count.load(std::memory_order_relaxed);
        }
        return 0;
    }

    StatsSummary ResultStatistics::GetSummary() const noexcept
    {
        StatsSummary summary;
        summary.totalErrors = m_totalErrors.load(std::memory_order_relaxed);
        summary.totalSuccess = m_totalSuccess.load(std::memory_order_relaxed);
        summary.errorRate = GetErrorRate();
        summary.uptime =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_startTime);

        {
            std::lock_guard lock(m_mutex);
            summary.uniqueErrors = m_errors.size();
        }

        return summary;
    }

    std::uint64_t ResultStatistics::GetTotalErrors() const noexcept
    {
        return m_totalErrors.load(std::memory_order_relaxed);
    }

    std::uint64_t ResultStatistics::GetTotalSuccess() const noexcept
    {
        return m_totalSuccess.load(std::memory_order_relaxed);
    }

    double ResultStatistics::GetErrorRate() const noexcept
    {
        const auto total = GetTotalErrors() + GetTotalSuccess();
        if (total == 0)
            return 0.0;
        return static_cast<double>(GetTotalErrors()) / static_cast<double>(total);
    }

    void ResultStatistics::Reset() noexcept
    {
        std::lock_guard lock(m_mutex);
        m_errors.clear();
        m_totalErrors.store(0, std::memory_order_relaxed);
        m_totalSuccess.store(0, std::memory_order_relaxed);
        m_startTime = std::chrono::steady_clock::now();
    }

    void ResultStatistics::SetEnabled(bool enabled) noexcept
    {
        m_enabled.store(enabled, std::memory_order_relaxed);
    }

    bool ResultStatistics::IsEnabled() const noexcept
    {
        return m_enabled.load(std::memory_order_relaxed);
    }

    void ResultStatistics::SetSamplingRate(double rate) noexcept
    {
        m_samplingRate.store(std::clamp(rate, 0.0, 1.0), std::memory_order_relaxed);
    }

}} // namespace NS::result
