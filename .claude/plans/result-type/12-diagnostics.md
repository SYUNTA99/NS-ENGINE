# 12: Diagnostics - Statistics・Logging

## 目的

エラー統計収集と診断ログ機能を実装。本番環境での問題分析支援。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Diagnostics/ResultStatistics.h` | 新規作成 |
| `Source/common/result/Diagnostics/ResultStatistics.cpp` | 新規作成 |
| `Source/common/result/Diagnostics/ResultLogging.h` | 新規作成 |
| `Source/common/result/Diagnostics/ResultLogging.cpp` | 新規作成 |

## 設計

### ResultStatistics

```cpp
// Source/common/result/Diagnostics/ResultStatistics.h
#pragma once

#include "common/result/Core/Result.h"
#include <cstdint>
#include <vector>
#include <atomic>
#include <chrono>
#include <string_view>

namespace NS::Result {

/// エラー発生記録
struct ErrorRecord {
    ::NS::Result result;
    std::uint64_t count = 0;
    std::uint64_t firstOccurrence = 0;  // ミリ秒タイムスタンプ
    std::uint64_t lastOccurrence = 0;
};

/// モジュール別統計
struct ModuleStats {
    int moduleId = 0;
    std::uint64_t totalErrors = 0;
    std::uint64_t uniqueErrors = 0;
    std::vector<ErrorRecord> topErrors;
};

/// 統計サマリー
struct StatsSummary {
    std::uint64_t totalErrors = 0;
    std::uint64_t totalSuccess = 0;
    std::uint64_t uniqueErrors = 0;
    double errorRate = 0.0;
    std::chrono::milliseconds uptime{0};
    std::vector<ModuleStats> moduleStats;
};

/// エラー統計収集
class ResultStatistics {
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

    struct ErrorEntry {
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

    // エラーエントリはModule*Description空間でハッシュ
    mutable std::mutex m_mutex;
    std::unordered_map<std::uint32_t, ErrorEntry> m_errors;
};

//=========================================================================
// グローバル関数（便利関数）
//=========================================================================

/// エラーを記録
inline void RecordError(::NS::Result result) noexcept {
    ResultStatistics::Instance().RecordError(result);
}

/// 成功を記録
inline void RecordSuccess() noexcept {
    ResultStatistics::Instance().RecordSuccess();
}

/// Resultを記録（成功/失敗を自動判定）
inline void RecordResult(::NS::Result result) noexcept {
    if (result.IsSuccess()) {
        RecordSuccess();
    } else {
        RecordError(result);
    }
}

} // namespace NS::Result
```

### ResultLogging

```cpp
// Source/common/result/Diagnostics/ResultLogging.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Utility/ResultFormatter.h"
#include "common/logging/logging.h"
#include <string_view>
#include <functional>

namespace NS::Result {

/// ログレベル
enum class ResultLogLevel {
    None,     // ログ出力なし
    Error,    // エラーのみ
    Warning,  // 警告以上
    Info,     // 情報以上
    Debug,    // デバッグ以上
    Verbose,  // 全て
};

/// ログ設定
struct ResultLogConfig {
    ResultLogLevel level = ResultLogLevel::Error;
    bool logSuccess = false;                    // 成功もログ出力
    bool logContext = true;                     // コンテキスト情報を含める
    bool logChain = true;                       // エラーチェーンを含める
    bool recordStatistics = true;               // 統計に記録
    FormatOptions formatOptions{};              // フォーマットオプション
};

/// Resultログ出力
class ResultLogger {
public:
    /// シングルトン取得
    [[nodiscard]] static ResultLogger& Instance() noexcept;

    /// 設定を取得/設定
    [[nodiscard]] const ResultLogConfig& GetConfig() const noexcept { return m_config; }
    void SetConfig(const ResultLogConfig& config) noexcept { m_config = config; }

    /// ログレベル設定
    void SetLogLevel(ResultLogLevel level) noexcept { m_config.level = level; }

    /// Resultをログ出力
    void Log(
        ::NS::Result result,
        std::string_view context = {},
        SourceLocation location = SourceLocation::FromStd()
    ) noexcept;

    /// 条件付きログ（失敗時のみ）
    void LogIfFailed(
        ::NS::Result result,
        std::string_view context = {},
        SourceLocation location = SourceLocation::FromStd()
    ) noexcept;

    /// カスタムログハンドラを設定
    using LogHandler = std::function<void(::NS::Result, std::string_view, const SourceLocation&)>;
    void SetCustomHandler(LogHandler handler) noexcept { m_customHandler = std::move(handler); }

private:
    ResultLogger() = default;

    void LogImpl(
        ::NS::Result result,
        std::string_view context,
        const SourceLocation& location
    ) noexcept;

    ResultLogConfig m_config;
    LogHandler m_customHandler;
};

//=========================================================================
// グローバル関数
//=========================================================================

/// Resultをログ出力
inline void LogResult(
    ::NS::Result result,
    std::string_view context = {},
    SourceLocation location = SourceLocation::FromStd()
) noexcept {
    ResultLogger::Instance().Log(result, context, location);
}

/// 失敗時のみログ出力
inline void LogResultIfFailed(
    ::NS::Result result,
    std::string_view context = {},
    SourceLocation location = SourceLocation::FromStd()
) noexcept {
    ResultLogger::Instance().LogIfFailed(result, context, location);
}

//=========================================================================
// マクロ
//=========================================================================

/// 失敗時にログ出力して早期リターン
#define NS_LOG_AND_RETURN_IF_FAILED(expr, ...) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            ::NS::Result::LogResult(_ns_result_, __VA_ARGS__); \
            return _ns_result_; \
        } \
    } while (false)

/// 診断情報を含めてログ出力
#define NS_LOG_RESULT_DIAGNOSTIC(result) \
    ::NS::Result::LogResult(result, {}, NS_CURRENT_SOURCE_LOCATION())

} // namespace NS::Result
```

### ResultLogging実装

```cpp
// Source/common/result/Diagnostics/ResultLogging.cpp
#include "common/result/Diagnostics/ResultLogging.h"
#include "common/result/Diagnostics/ResultStatistics.h"
#include "common/result/Error/ErrorInfo.h"
#include "common/result/Context/ResultContext.h"
#include "common/result/Context/ErrorChain.h"

namespace NS::Result {

ResultLogger& ResultLogger::Instance() noexcept {
    static ResultLogger instance;
    return instance;
}

void ResultLogger::Log(
    ::NS::Result result,
    std::string_view context,
    SourceLocation location
) noexcept {
    // 成功で、成功ログが無効なら何もしない
    if (result.IsSuccess() && !m_config.logSuccess) {
        return;
    }

    // 統計に記録
    if (m_config.recordStatistics) {
        RecordResult(result);
    }

    LogImpl(result, context, location);
}

void ResultLogger::LogIfFailed(
    ::NS::Result result,
    std::string_view context,
    SourceLocation location
) noexcept {
    if (result.IsFailure()) {
        Log(result, context, location);
    }
}

void ResultLogger::LogImpl(
    ::NS::Result result,
    std::string_view context,
    const SourceLocation& location
) noexcept {
    if (m_config.level == ResultLogLevel::None) {
        return;
    }

    // カスタムハンドラがあれば使用
    if (m_customHandler) {
        m_customHandler(result, context, location);
        return;
    }

    // フォーマットオプションを設定
    FormatOptions options = m_config.formatOptions;
    options.includeContext = m_config.logContext;
    options.includeChain = m_config.logChain;

    std::string message = FormatResult(result, options);

    // コンテキスト情報を追加
    if (!context.empty()) {
        message = std::string(context) + ": " + message;
    }

    // ログレベルに応じて出力
    if (result.IsSuccess()) {
        LOG_INFO("[Result] {}", message);
    } else {
        ErrorInfo info = GetErrorInfo(result);
        if (info.category.IsFatal()) {
            LOG_FATAL("[Result] {}", message);
        } else {
            LOG_ERROR("[Result] {}", message);
        }
    }
}

} // namespace NS::Result
```

### ResultStatistics実装

```cpp
// Source/common/result/Diagnostics/ResultStatistics.cpp
#include "common/result/Diagnostics/ResultStatistics.h"
#include <algorithm>
#include <random>

namespace NS::Result {

namespace {

std::uint32_t MakeKey(::NS::Result result) noexcept {
    return (static_cast<std::uint32_t>(result.GetModule()) << 16) |
           static_cast<std::uint32_t>(result.GetDescription());
}

std::uint64_t GetCurrentTimestamp() noexcept {
    auto now = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count()
    );
}

thread_local std::mt19937 t_rng{std::random_device{}()};

bool ShouldSample(double rate) noexcept {
    if (rate >= 1.0) return true;
    if (rate <= 0.0) return false;
    std::uniform_real_distribution<> dist(0.0, 1.0);
    return dist(t_rng) < rate;
}

} // namespace

ResultStatistics::ResultStatistics()
    : m_startTime(std::chrono::steady_clock::now())
{}

ResultStatistics& ResultStatistics::Instance() noexcept {
    static ResultStatistics instance;
    return instance;
}

void ResultStatistics::RecordError(::NS::Result result) noexcept {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    if (!ShouldSample(m_samplingRate.load(std::memory_order_relaxed))) {
        return;
    }

    m_totalErrors.fetch_add(1, std::memory_order_relaxed);

    const auto key = MakeKey(result);
    const auto timestamp = GetCurrentTimestamp();

    std::lock_guard lock(m_mutex);
    auto& entry = m_errors[key];
    entry.count.fetch_add(1, std::memory_order_relaxed);

    std::uint64_t expected = 0;
    entry.firstOccurrence.compare_exchange_strong(expected, timestamp,
        std::memory_order_relaxed);
    entry.lastOccurrence.store(timestamp, std::memory_order_relaxed);
}

void ResultStatistics::RecordSuccess() noexcept {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    m_totalSuccess.fetch_add(1, std::memory_order_relaxed);
}

void ResultStatistics::RecordBatch(const ::NS::Result* results, std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        if (results[i].IsSuccess()) {
            RecordSuccess();
        } else {
            RecordError(results[i]);
        }
    }
}

std::vector<ErrorRecord> ResultStatistics::GetTopErrors(std::size_t count) const noexcept {
    std::lock_guard lock(m_mutex);

    std::vector<ErrorRecord> records;
    records.reserve(m_errors.size());

    for (const auto& [key, entry] : m_errors) {
        ::NS::Result result = Detail::ConstructResult(key);
        records.push_back(ErrorRecord{
            .result = result,
            .count = entry.count.load(std::memory_order_relaxed),
            .firstOccurrence = entry.firstOccurrence.load(std::memory_order_relaxed),
            .lastOccurrence = entry.lastOccurrence.load(std::memory_order_relaxed)
        });
    }

    std::partial_sort(records.begin(),
        records.begin() + std::min(count, records.size()),
        records.end(),
        [](const auto& a, const auto& b) { return a.count > b.count; });

    if (records.size() > count) {
        records.resize(count);
    }

    return records;
}

StatsSummary ResultStatistics::GetSummary() const noexcept {
    StatsSummary summary;
    summary.totalErrors = m_totalErrors.load(std::memory_order_relaxed);
    summary.totalSuccess = m_totalSuccess.load(std::memory_order_relaxed);
    summary.errorRate = GetErrorRate();
    summary.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_startTime);

    {
        std::lock_guard lock(m_mutex);
        summary.uniqueErrors = m_errors.size();
    }

    return summary;
}

std::uint64_t ResultStatistics::GetTotalErrors() const noexcept {
    return m_totalErrors.load(std::memory_order_relaxed);
}

std::uint64_t ResultStatistics::GetTotalSuccess() const noexcept {
    return m_totalSuccess.load(std::memory_order_relaxed);
}

double ResultStatistics::GetErrorRate() const noexcept {
    const auto total = GetTotalErrors() + GetTotalSuccess();
    if (total == 0) return 0.0;
    return static_cast<double>(GetTotalErrors()) / static_cast<double>(total);
}

void ResultStatistics::Reset() noexcept {
    std::lock_guard lock(m_mutex);
    m_errors.clear();
    m_totalErrors.store(0, std::memory_order_relaxed);
    m_totalSuccess.store(0, std::memory_order_relaxed);
    m_startTime = std::chrono::steady_clock::now();
}

void ResultStatistics::SetEnabled(bool enabled) noexcept {
    m_enabled.store(enabled, std::memory_order_relaxed);
}

bool ResultStatistics::IsEnabled() const noexcept {
    return m_enabled.load(std::memory_order_relaxed);
}

void ResultStatistics::SetSamplingRate(double rate) noexcept {
    m_samplingRate.store(std::clamp(rate, 0.0, 1.0), std::memory_order_relaxed);
}

} // namespace NS::Result
```

## 使用例

```cpp
// 統計収集
void ProcessRequests(std::span<Request> requests) {
    for (const auto& req : requests) {
        NS::Result result = Process(req);
        NS::Result::RecordResult(result);
    }
}

// 定期レポート
void PrintDiagnosticReport() {
    auto summary = ResultStatistics::Instance().GetSummary();

    LOG_INFO("=== Result Statistics ===");
    LOG_INFO("Uptime: {}ms", summary.uptime.count());
    LOG_INFO("Total: {} success, {} errors ({:.2f}% error rate)",
        summary.totalSuccess, summary.totalErrors, summary.errorRate * 100);
    LOG_INFO("Unique errors: {}", summary.uniqueErrors);

    LOG_INFO("Top 5 errors:");
    for (const auto& err : ResultStatistics::Instance().GetTopErrors(5)) {
        LOG_INFO("  {}: {} occurrences", FormatResultCompact(err.result), err.count);
    }
}

// ログ設定
void ConfigureResultLogging() {
    ResultLogConfig config;
    config.level = ResultLogLevel::Error;
    config.logContext = true;
    config.logChain = true;
    config.recordStatistics = true;
    ResultLogger::Instance().SetConfig(config);
}

// カスタムハンドラ
ResultLogger::Instance().SetCustomHandler([](auto result, auto ctx, const auto& loc) {
    // カスタムログ処理（外部サービスへ送信など）
    ExternalLoggingService::Send(FormatResultFull(result));
});
```

## TODO

- [ ] `Source/common/result/Diagnostics/` ディレクトリ作成
- [ ] `ResultStatistics.h` 作成
- [ ] `ResultStatistics.cpp` 作成
- [ ] `ResultLogging.h` 作成
- [ ] `ResultLogging.cpp` 作成
- [ ] スレッドセーフティ検証
- [ ] パフォーマンス測定
- [ ] ビルド確認
