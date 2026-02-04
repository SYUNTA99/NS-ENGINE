# 05: Context - SourceLocation・ResultContext

## 目的

エラー発生箇所の自動記録とコンテキスト付きResult型を実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Context/SourceLocation.h` | 新規作成 |
| `Source/common/result/Context/ResultContext.h` | 新規作成 |
| `Source/common/result/Context/ResultContext.cpp` | 新規作成 |

## 設計

### SourceLocation（C++20）

```cpp
// Source/common/result/Context/SourceLocation.h
#pragma once

#include <source_location>
#include <string_view>
#include <cstdint>

namespace NS::Result {

/// エラー発生箇所情報
struct SourceLocation {
    std::string_view file;
    std::string_view function;
    std::uint32_t line = 0;
    std::uint32_t column = 0;

    constexpr SourceLocation() noexcept = default;

    constexpr SourceLocation(
        std::string_view file_,
        std::string_view function_,
        std::uint32_t line_,
        std::uint32_t column_ = 0
    ) noexcept
        : file(file_)
        , function(function_)
        , line(line_)
        , column(column_)
    {}

    /// std::source_locationから構築
    static SourceLocation FromStd(
        const std::source_location& loc = std::source_location::current()
    ) noexcept {
        return SourceLocation{
            loc.file_name(),
            loc.function_name(),
            static_cast<std::uint32_t>(loc.line()),
            static_cast<std::uint32_t>(loc.column())
        };
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return !file.empty() && line > 0;
    }
};

/// 現在位置を取得するマクロ（C++20以前の互換性用）
#define NS_CURRENT_SOURCE_LOCATION() \
    ::NS::Result::SourceLocation::FromStd(std::source_location::current())

} // namespace NS::Result
```

### ResultContext

```cpp
// Source/common/result/Context/ResultContext.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Context/SourceLocation.h"
#include <string_view>
#include <optional>
#include <deque>
#include <vector>

namespace NS::Result {

/// コンテキスト情報
struct ContextInfo {
    SourceLocation location;
    std::string_view message;
    std::uint64_t timestamp = 0;  // 発生時刻（ミリ秒）

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return location.IsValid();
    }
};

/// コンテキスト付きResult
/// Resultを拡張し、発生箇所とメッセージを保持
class ResultContext {
public:
    ResultContext() noexcept = default;

    explicit ResultContext(::NS::Result result) noexcept
        : m_result(result)
    {}

    ResultContext(
        ::NS::Result result,
        SourceLocation location,
        std::string_view message = {}
    ) noexcept;

    /// 暗黙変換
    [[nodiscard]] constexpr operator ::NS::Result() const noexcept {
        return m_result;
    }

    /// Result委譲
    [[nodiscard]] constexpr bool IsSuccess() const noexcept { return m_result.IsSuccess(); }
    [[nodiscard]] constexpr bool IsFailure() const noexcept { return m_result.IsFailure(); }
    [[nodiscard]] constexpr int GetModule() const noexcept { return m_result.GetModule(); }
    [[nodiscard]] constexpr int GetDescription() const noexcept { return m_result.GetDescription(); }

    /// コンテキスト情報
    [[nodiscard]] const ContextInfo& GetContext() const noexcept { return m_context; }
    [[nodiscard]] bool HasContext() const noexcept { return m_context.IsValid(); }

    /// 生のResult取得
    [[nodiscard]] constexpr ::NS::Result GetResult() const noexcept { return m_result; }

private:
    ::NS::Result m_result;
    ContextInfo m_context;
};

/// コンテキストストレージ（スレッドローカル）
/// 設計方針:
/// - LRUベースでサイズ制限（古いエントリから削除）
/// - Result値 + タイムスタンプでユニーク識別
/// - 同じResult値でも複数保持可能（異なるタイミングの発生）
namespace Detail {

class ContextStorage {
public:
    static constexpr std::size_t kMaxContexts = 64;
    static constexpr std::size_t kMaxPerResult = 4;  // 同一Result値の最大保持数

    /// コンテキストを記録
    static void Push(::NS::Result result, const ContextInfo& context) noexcept;

    /// 最新のコンテキストを取得して削除
    static std::optional<ContextInfo> Pop(::NS::Result result) noexcept;

    /// 最新のコンテキストを参照（削除しない）
    static std::optional<ContextInfo> Peek(::NS::Result result) noexcept;

    /// 特定Resultの全コンテキストを取得
    static std::vector<ContextInfo> GetAll(::NS::Result result) noexcept;

    /// クリア
    static void Clear() noexcept;

    /// 統計情報
    static std::size_t GetCount() noexcept;

private:
    struct Entry {
        ::NS::Result result;
        ContextInfo context;
        std::uint64_t accessTime = 0;  // LRU用
    };

    // dequeで順序を保持、古いものから削除
    static thread_local std::deque<Entry> s_storage;
    static thread_local std::uint64_t s_accessCounter;

    static void Evict() noexcept;  // サイズ超過時に古いエントリを削除
};

} // namespace Detail

/// コンテキストを記録
inline void RecordContext(
    ::NS::Result result,
    SourceLocation location,
    std::string_view message = {}
) noexcept {
    if (result.IsFailure()) {
        Detail::ContextStorage::Push(result, ContextInfo{location, message, 0});
    }
}

/// コンテキストを取得
[[nodiscard]] inline std::optional<ContextInfo> GetResultContext(::NS::Result result) noexcept {
    return Detail::ContextStorage::Peek(result);
}

} // namespace NS::Result
```

### マクロ

```cpp
// ResultContext.h の末尾に追加

/// 失敗時にコンテキストを記録して早期リターン
#define NS_RETURN_IF_FAILED_CTX(expr) \
    do { \
        if (auto _result = (expr); _result.IsFailure()) { \
            ::NS::Result::RecordContext(_result, NS_CURRENT_SOURCE_LOCATION()); \
            return _result; \
        } \
    } while (false)

/// メッセージ付き
#define NS_RETURN_IF_FAILED_CTX_MSG(expr, msg) \
    do { \
        if (auto _result = (expr); _result.IsFailure()) { \
            ::NS::Result::RecordContext(_result, NS_CURRENT_SOURCE_LOCATION(), msg); \
            return _result; \
        } \
    } while (false)

/// コンテキスト付きResultを返す
#define NS_MAKE_RESULT_CTX(result) \
    ::NS::Result::ResultContext((result), NS_CURRENT_SOURCE_LOCATION())

#define NS_MAKE_RESULT_CTX_MSG(result, msg) \
    ::NS::Result::ResultContext((result), NS_CURRENT_SOURCE_LOCATION(), (msg))
```

### 実装

```cpp
// Source/common/result/Context/ResultContext.cpp
#include "common/result/Context/ResultContext.h"
#include <chrono>
#include <algorithm>

namespace NS::Result {

namespace {

std::uint64_t GetCurrentTimestamp() noexcept {
    auto now = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count()
    );
}

} // namespace

ResultContext::ResultContext(
    ::NS::Result result,
    SourceLocation location,
    std::string_view message
) noexcept
    : m_result(result)
{
    m_context = ContextInfo{
        .location = location,
        .message = message,
        .timestamp = GetCurrentTimestamp()
    };

    if (result.IsFailure()) {
        Detail::ContextStorage::Push(result, m_context);
    }
}

namespace Detail {

thread_local std::deque<ContextStorage::Entry> ContextStorage::s_storage;
thread_local std::uint64_t ContextStorage::s_accessCounter = 0;

void ContextStorage::Push(::NS::Result result, const ContextInfo& context) noexcept {
    // 同一Resultの保持数制限をチェック
    std::size_t sameResultCount = std::count_if(s_storage.begin(), s_storage.end(),
        [&result](const Entry& e) { return e.result == result; });

    if (sameResultCount >= kMaxPerResult) {
        // 同一Resultの最も古いエントリを削除
        auto it = std::find_if(s_storage.begin(), s_storage.end(),
            [&result](const Entry& e) { return e.result == result; });
        if (it != s_storage.end()) {
            s_storage.erase(it);
        }
    }

    // サイズ超過時は古いエントリを削除
    if (s_storage.size() >= kMaxContexts) {
        Evict();
    }

    s_storage.push_back(Entry{
        .result = result,
        .context = context,
        .accessTime = ++s_accessCounter
    });
}

std::optional<ContextInfo> ContextStorage::Pop(::NS::Result result) noexcept {
    // 後ろから検索（最新のものを返す）
    for (auto it = s_storage.rbegin(); it != s_storage.rend(); ++it) {
        if (it->result == result) {
            ContextInfo info = it->context;
            s_storage.erase(std::next(it).base());
            return info;
        }
    }
    return std::nullopt;
}

std::optional<ContextInfo> ContextStorage::Peek(::NS::Result result) noexcept {
    // 後ろから検索（最新のものを返す）
    for (auto it = s_storage.rbegin(); it != s_storage.rend(); ++it) {
        if (it->result == result) {
            it->accessTime = ++s_accessCounter;  // LRU更新
            return it->context;
        }
    }
    return std::nullopt;
}

std::vector<ContextInfo> ContextStorage::GetAll(::NS::Result result) noexcept {
    std::vector<ContextInfo> results;
    for (const auto& entry : s_storage) {
        if (entry.result == result) {
            results.push_back(entry.context);
        }
    }
    return results;
}

void ContextStorage::Clear() noexcept {
    s_storage.clear();
    s_accessCounter = 0;
}

std::size_t ContextStorage::GetCount() noexcept {
    return s_storage.size();
}

void ContextStorage::Evict() noexcept {
    if (s_storage.empty()) return;

    // LRU: accessTimeが最小のエントリを削除
    auto minIt = std::min_element(s_storage.begin(), s_storage.end(),
        [](const Entry& a, const Entry& b) { return a.accessTime < b.accessTime; });

    if (minIt != s_storage.end()) {
        s_storage.erase(minIt);
    }
}

} // namespace Detail

} // namespace NS::Result
```

## 使用例

```cpp
NS::Result LoadTexture(const char* path) {
    NS_RETURN_IF_FAILED_CTX(ValidatePath(path));
    NS_RETURN_IF_FAILED_CTX_MSG(ReadFile(path), "Failed to read texture file");
    NS_RETURN_IF_FAILED_CTX(DecodeImage(data));
    return NS::ResultSuccess();
}

// 呼び出し側
NS::Result result = LoadTexture("missing.png");
if (result.IsFailure()) {
    if (auto ctx = NS::Result::GetResultContext(result)) {
        LOG_ERROR("Error at {}:{} in {}",
            ctx->location.file,
            ctx->location.line,
            ctx->location.function);
        if (!ctx->message.empty()) {
            LOG_ERROR("  Message: {}", ctx->message);
        }
    }
}
```

## TODO

- [ ] `Source/common/result/Context/` ディレクトリ作成
- [ ] `SourceLocation.h` 作成
- [ ] `ResultContext.h` 作成
- [ ] `ResultContext.cpp` 作成
- [ ] スレッドローカルストレージ検証
- [ ] マクロ動作検証
- [ ] ビルド確認
