# 06: Context - ErrorChain

## 目的

エラー原因の連鎖追跡機能を実装。根本原因の特定を支援。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Context/ErrorChain.h` | 新規作成 |
| `Source/common/result/Context/ErrorChain.cpp` | 新規作成 |

## 設計

### ErrorChain

```cpp
// Source/common/result/Context/ErrorChain.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Context/SourceLocation.h"
#include <string_view>
#include <span>
#include <array>
#include <vector>
#include <deque>
#include <optional>
#include <algorithm>
#include <cstdint>

namespace NS::Result {

/// チェーン内のエラーエントリ
struct ChainEntry {
    ::NS::Result result;
    SourceLocation location;
    std::string_view message;
    std::uint64_t timestamp = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return result.IsFailure();
    }
};

/// エラーチェーン（固定サイズ）
/// チェーンの順序:
///   [0] = 最終エラー（最上位、呼び出し元に返されるエラー）
///   [1] = 直接の原因
///   [2] = その原因
///   ...
///   [N-1] = 根本原因（最も深いエラー）
///
/// 例: Initialize() -> LoadConfig() -> ReadFile() がエラー
///   [0] = InitializationFailed (Initialize で返す)
///   [1] = LoadFailed (LoadConfig で返す)
///   [2] = PathNotFound (ReadFile の結果、根本原因)
class ErrorChain {
public:
    static constexpr std::size_t kMaxDepth = 8;

    ErrorChain() noexcept = default;

    /// 最終エラー（チェーンの先頭、呼び出し元に返される）
    [[nodiscard]] ::NS::Result GetResult() const noexcept {
        return m_count > 0 ? m_entries[0].result : ::NS::Result{};
    }

    /// 根本原因（チェーンの末尾、最も深いエラー）
    [[nodiscard]] ::NS::Result GetRootCause() const noexcept {
        return m_count > 0 ? m_entries[m_count - 1].result : ::NS::Result{};
    }

    /// エントリ数
    [[nodiscard]] std::size_t GetDepth() const noexcept { return m_count; }

    /// 空かどうか
    [[nodiscard]] bool IsEmpty() const noexcept { return m_count == 0; }

    /// エントリ取得（0=最終エラー, N-1=根本原因）
    [[nodiscard]] const ChainEntry& operator[](std::size_t index) const noexcept {
        return m_entries[index];
    }

    /// イテレーション（最終エラー→根本原因の順）
    [[nodiscard]] std::span<const ChainEntry> GetEntries() const noexcept {
        return std::span(m_entries.data(), m_count);
    }

    /// コピーしてエントリを取得（span寿命問題を回避）
    [[nodiscard]] std::vector<ChainEntry> GetEntriesCopy() const noexcept {
        return std::vector<ChainEntry>(m_entries.begin(), m_entries.begin() + m_count);
    }

private:
    friend class ErrorChainBuilder;

    std::array<ChainEntry, kMaxDepth> m_entries{};
    std::size_t m_count = 0;

    /// 末尾に追加（内部用、原因を追加）
    void AppendCause(const ChainEntry& entry) noexcept {
        if (m_count < kMaxDepth) {
            m_entries[m_count++] = entry;
        }
    }

    /// 先頭に設定（内部用、最終エラーを設定）
    void SetResult(const ChainEntry& entry) noexcept {
        if (m_count == 0) {
            m_entries[m_count++] = entry;
        } else {
            m_entries[0] = entry;
        }
    }
};

/// チェーンビルダー
/// 使用例:
///   // ReadFile() が PathNotFound を返した場合
///   return ErrorChainBuilder(ResultLoadFailed(), location, "Config load failed")
///       .CausedBy(readFileResult)  // PathNotFound が根本原因
///       .ToResult();
///
///   // チェーン: [0]=LoadFailed, [1]=PathNotFound
class ErrorChainBuilder {
public:
    /// 最終エラー（呼び出し元に返すエラー）を指定してビルダーを作成
    explicit ErrorChainBuilder(
        ::NS::Result result,
        SourceLocation location = SourceLocation::FromStd(),
        std::string_view message = {}
    ) noexcept;

    /// 原因エラーを追加（チェーンの末尾に追加される）
    /// 複数回呼び出すと、呼び出し順にチェーンが深くなる
    ErrorChainBuilder& CausedBy(
        ::NS::Result cause,
        SourceLocation location = {},
        std::string_view message = {}
    ) noexcept;

    /// 別のチェーンを原因として追加（チェーン全体を末尾に追加）
    ErrorChainBuilder& CausedBy(const ErrorChain& chain) noexcept;

    /// チェーンを構築（ストレージには保存しない）
    [[nodiscard]] ErrorChain Build() const noexcept;

    /// Resultとして取得し、チェーンをストレージに保存
    [[nodiscard]] ::NS::Result ToResult() noexcept;

private:
    ErrorChain m_chain;
};

/// チェーンストレージ（スレッドローカル）
/// 設計方針:
/// - LRUベースでサイズ制限
/// - ErrorChainをコピー保持（span寿命問題を回避）
namespace Detail {

class ChainStorage {
public:
    static constexpr std::size_t kMaxChains = 32;

    /// チェーンを保存
    static void Store(::NS::Result result, const ErrorChain& chain) noexcept;

    /// チェーンを検索（コピーを返す、寿命問題を回避）
    static std::optional<ErrorChain> Find(::NS::Result result) noexcept;

    /// チェーンを削除
    static void Remove(::NS::Result result) noexcept;

    /// クリア
    static void Clear() noexcept;

    /// 統計
    static std::size_t GetCount() noexcept;

private:
    struct Entry {
        ::NS::Result result;
        ErrorChain chain;
        std::uint64_t accessTime = 0;
    };

    static thread_local std::deque<Entry> s_storage;
    static thread_local std::uint64_t s_accessCounter;

    static void Evict() noexcept;
};

} // namespace Detail

/// チェーン付きResultを作成
[[nodiscard]] inline ::NS::Result MakeChainedResult(
    ::NS::Result result,
    ::NS::Result cause,
    std::string_view message = {},
    SourceLocation location = SourceLocation::FromStd()
) noexcept {
    ErrorChainBuilder builder(result, location, message);
    builder.CausedBy(cause);
    return builder.ToResult();
}

/// エラーチェーンを取得（コピーを返す）
[[nodiscard]] inline std::optional<ErrorChain> GetErrorChain(::NS::Result result) noexcept {
    return Detail::ChainStorage::Find(result);
}

/// エラーチェーンがあるか
[[nodiscard]] inline bool HasErrorChain(::NS::Result result) noexcept {
    return GetErrorChain(result).has_value();
}

} // namespace NS::Result
```

### 実装

```cpp
// Source/common/result/Context/ErrorChain.cpp
#include "common/result/Context/ErrorChain.h"
#include <chrono>

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

ErrorChainBuilder::ErrorChainBuilder(
    ::NS::Result result,
    SourceLocation location,
    std::string_view message
) noexcept {
    // 最終エラーをチェーンの先頭に設定
    m_chain.SetResult(ChainEntry{
        .result = result,
        .location = location,
        .message = message,
        .timestamp = GetCurrentTimestamp()
    });
}

ErrorChainBuilder& ErrorChainBuilder::CausedBy(
    ::NS::Result cause,
    SourceLocation location,
    std::string_view message
) noexcept {
    // 原因をチェーンの末尾に追加
    m_chain.AppendCause(ChainEntry{
        .result = cause,
        .location = location,
        .message = message,
        .timestamp = GetCurrentTimestamp()
    });
    return *this;
}

ErrorChainBuilder& ErrorChainBuilder::CausedBy(const ErrorChain& chain) noexcept {
    // 既存チェーンの全エントリを原因として追加
    // chain[0]（相手の最終エラー）から順に追加
    for (const auto& entry : chain.GetEntries()) {
        m_chain.AppendCause(entry);
    }
    return *this;
}

ErrorChain ErrorChainBuilder::Build() const noexcept {
    return m_chain;
}

::NS::Result ErrorChainBuilder::ToResult() noexcept {
    if (m_chain.GetDepth() > 0) {
        ::NS::Result result = m_chain.GetResult();
        Detail::ChainStorage::Store(result, m_chain);
        return result;
    }
    return ::NS::Result{};
}

namespace Detail {

thread_local std::deque<ChainStorage::Entry> ChainStorage::s_storage;
thread_local std::uint64_t ChainStorage::s_accessCounter = 0;

void ChainStorage::Store(::NS::Result result, const ErrorChain& chain) noexcept {
    // 既存エントリを更新
    for (auto& entry : s_storage) {
        if (entry.result == result) {
            entry.chain = chain;
            entry.accessTime = ++s_accessCounter;
            return;
        }
    }

    // サイズ超過時は古いエントリを削除
    if (s_storage.size() >= kMaxChains) {
        Evict();
    }

    // 新規追加
    s_storage.push_back(Entry{
        .result = result,
        .chain = chain,
        .accessTime = ++s_accessCounter
    });
}

std::optional<ErrorChain> ChainStorage::Find(::NS::Result result) noexcept {
    for (auto& entry : s_storage) {
        if (entry.result == result) {
            entry.accessTime = ++s_accessCounter;  // LRU更新
            return entry.chain;  // コピーを返す
        }
    }
    return std::nullopt;
}

void ChainStorage::Remove(::NS::Result result) noexcept {
    auto it = std::find_if(s_storage.begin(), s_storage.end(),
        [&result](const Entry& e) { return e.result == result; });
    if (it != s_storage.end()) {
        s_storage.erase(it);
    }
}

void ChainStorage::Clear() noexcept {
    s_storage.clear();
    s_accessCounter = 0;
}

std::size_t ChainStorage::GetCount() noexcept {
    return s_storage.size();
}

void ChainStorage::Evict() noexcept {
    if (s_storage.empty()) return;

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
// 低レベル関数
NS::Result ReadFile(const char* path) {
    // ファイル読み込み失敗
    return FileSystemResult::ResultPathNotFound();
}

// 中レベル関数
NS::Result LoadConfig(const char* path) {
    NS::Result readResult = ReadFile(path);
    if (readResult.IsFailure()) {
        return NS::Result::MakeChainedResult(
            CommonResult::ResultOperationFailed(),
            readResult,
            "Config file read failed"
        );
    }
    return NS::ResultSuccess();
}

// 高レベル関数
NS::Result Initialize() {
    NS::Result configResult = LoadConfig("settings.cfg");
    if (configResult.IsFailure()) {
        return NS::Result::MakeChainedResult(
            CommonResult::ResultInitializationFailed(),
            configResult,
            "Failed to initialize from config"
        );
    }
    return NS::ResultSuccess();
}

// エラー処理
NS::Result result = Initialize();
if (result.IsFailure()) {
    LOG_ERROR("Error: {}", FormatResult(result));

    if (auto chain = NS::Result::GetErrorChain(result)) {
        LOG_ERROR("Error chain (depth={}):", chain->GetDepth());
        for (const auto& entry : chain->GetEntriesCopy()) {
            LOG_ERROR("  -> {} at {}:{}",
                FormatResult(entry.result),
                entry.location.file,
                entry.location.line);
            if (!entry.message.empty()) {
                LOG_ERROR("     Message: {}", entry.message);
            }
        }
        LOG_ERROR("Root cause: {}", FormatResult(chain->GetRootCause()));
    }
}
```

## 出力例

```
Error: Common::OperationFailed (Module=1, Desc=100)
Error chain (depth=3):
  -> Common::InitializationFailed at main.cpp:45
     Message: Failed to initialize from config
  -> Common::OperationFailed at config.cpp:23
     Message: Config file read failed
  -> FileSystem::PathNotFound at file.cpp:67
Root cause: FileSystem::PathNotFound (Module=2, Desc=1)
```

## TODO

- [ ] `ErrorChain.h` 作成
- [ ] `ErrorChain.cpp` 作成
- [ ] チェーン構築検証
- [ ] ストレージ検証
- [ ] メモリ使用量確認
- [ ] ビルド確認

## 設計変更履歴

- ErrorChainの順序を明確化（[0]=最終エラー, [N-1]=根本原因）
- Push/Append を SetResult/AppendCause に変更（セマンティクス明確化）
- GetEntries() に加えて GetEntriesCopy() を追加（span寿命問題対策）
- ChainStorage を deque + LRU に変更（固定配列から改善）
- GetErrorChain() が optional<ErrorChain> を返すよう変更（ポインタ寿命問題を回避）
