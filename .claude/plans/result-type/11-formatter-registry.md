# 11: Utility - Formatter・Registry

## 目的

エラーメッセージのフォーマットと型レジストリを実装。デバッグ・診断支援。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Utility/ResultFormatter.h` | 新規作成 |
| `Source/common/result/Utility/ResultFormatter.cpp` | 新規作成 |
| `Source/common/result/Utility/ResultRegistry.h` | 新規作成 |
| `Source/common/result/Utility/ResultRegistry.cpp` | 新規作成 |

## 設計

### ResultFormatter

```cpp
// Source/common/result/Utility/ResultFormatter.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Error/ErrorInfo.h"
#include "common/result/Context/ResultContext.h"
#include "common/result/Context/ErrorChain.h"
#include <string>
#include <string_view>
#include <format>

namespace NS::Result {

/// フォーマットオプション
struct FormatOptions {
    bool includeModuleName = true;     // モジュール名を含める
    bool includeErrorName = true;      // エラー名を含める
    bool includeNumericValues = true;  // 数値ID (Module, Desc) を含める
    bool includeMessage = true;        // メッセージを含める
    bool includeContext = false;       // コンテキスト情報を含める
    bool includeChain = false;         // エラーチェーンを含める
    bool verbose = false;              // 詳細モード
    std::string_view separator = ": "; // 区切り文字
    std::string_view chainIndent = "  -> "; // チェーンのインデント
};

/// Resultをフォーマット
[[nodiscard]] std::string FormatResult(
    ::NS::Result result,
    const FormatOptions& options = {}
) noexcept;

/// コンパクトフォーマット（デフォルト）
/// 例: "FileSystem::PathNotFound (2:1)"
[[nodiscard]] std::string FormatResultCompact(::NS::Result result) noexcept;

/// 詳細フォーマット
/// 例: "FileSystem::PathNotFound (Module=2, Desc=1): 指定されたパスが見つかりません"
[[nodiscard]] std::string FormatResultVerbose(::NS::Result result) noexcept;

/// チェーン付きフォーマット
/// 例:
/// "Common::LoadFailed (1:602): リソースの読み込みに失敗
///   -> FileSystem::PathNotFound (2:1): 指定されたパスが見つかりません"
[[nodiscard]] std::string FormatResultWithChain(::NS::Result result) noexcept;

/// コンテキスト付きフォーマット
/// 例: "FileSystem::PathNotFound at loader.cpp:123 in LoadFile()"
[[nodiscard]] std::string FormatResultWithContext(::NS::Result result) noexcept;

/// デバッグ用完全フォーマット
[[nodiscard]] std::string FormatResultFull(::NS::Result result) noexcept;

/// 16進数でRaw値を取得
[[nodiscard]] std::string FormatResultRaw(::NS::Result result) noexcept;

/// std::formatサポート
class ResultFormatAdapter {
public:
    explicit ResultFormatAdapter(::NS::Result result, FormatOptions options = {}) noexcept
        : m_result(result), m_options(options) {}

    [[nodiscard]] std::string ToString() const noexcept {
        return FormatResult(m_result, m_options);
    }

private:
    ::NS::Result m_result;
    FormatOptions m_options;
};

} // namespace NS::Result

// std::format specialization
template <>
struct std::formatter<::NS::Result> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(::NS::Result result, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", ::NS::Result::FormatResultCompact(result));
    }
};
```

### ResultFormatter実装

```cpp
// Source/common/result/Utility/ResultFormatter.cpp
#include "common/result/Utility/ResultFormatter.h"
#include "common/result/Module/ModuleId.h"
#include <format>

namespace NS::Result {

namespace {

// スレッドローカルバッファで動的メモリ確保を最小化
thread_local char t_formatBuffer[2048];

// 高速フォーマット用ヘルパー
class FastStringBuilder {
public:
    FastStringBuilder() noexcept : m_pos(0) {}

    void Append(std::string_view str) noexcept {
        const std::size_t remaining = sizeof(t_formatBuffer) - m_pos - 1;
        const std::size_t copyLen = std::min(str.size(), remaining);
        std::memcpy(t_formatBuffer + m_pos, str.data(), copyLen);
        m_pos += copyLen;
    }

    void Append(char c) noexcept {
        if (m_pos < sizeof(t_formatBuffer) - 1) {
            t_formatBuffer[m_pos++] = c;
        }
    }

    void AppendNumber(int value) noexcept {
        char buf[16];
        auto result = std::to_chars(buf, buf + sizeof(buf), value);
        Append(std::string_view(buf, result.ptr - buf));
    }

    [[nodiscard]] std::string ToString() const noexcept {
        return std::string(t_formatBuffer, m_pos);
    }

private:
    std::size_t m_pos;
};

} // namespace

std::string FormatResult(::NS::Result result, const FormatOptions& options) noexcept {
    if (result.IsSuccess()) {
        return "Success";
    }

    FastStringBuilder builder;
    ErrorInfo info = GetErrorInfo(result);

    // モジュール名::エラー名
    if (options.includeModuleName && options.includeErrorName) {
        builder.Append(info.moduleName);
        builder.Append("::");
        builder.Append(info.errorName);
    } else if (options.includeModuleName) {
        builder.Append(info.moduleName);
    } else if (options.includeErrorName) {
        builder.Append(info.errorName);
    }

    // 数値ID
    if (options.includeNumericValues) {
        if (options.verbose) {
            builder.Append(" (Module=");
            builder.AppendNumber(info.module);
            builder.Append(", Desc=");
            builder.AppendNumber(info.description);
            builder.Append(')');
        } else {
            builder.Append(" (");
            builder.AppendNumber(info.module);
            builder.Append(':');
            builder.AppendNumber(info.description);
            builder.Append(')');
        }
    }

    // メッセージ
    if (options.includeMessage && !info.message.empty()) {
        builder.Append(options.separator);
        builder.Append(info.message);
    }

    // コンテキスト
    if (options.includeContext) {
        if (auto ctx = GetResultContext(result)) {
            builder.Append("\n  at ");
            builder.Append(ctx->location.file);
            builder.Append(':');
            builder.AppendNumber(static_cast<int>(ctx->location.line));
            if (!ctx->location.function.empty()) {
                builder.Append(" in ");
                builder.Append(ctx->location.function);
            }
            if (!ctx->message.empty()) {
                builder.Append("\n  Message: ");
                builder.Append(ctx->message);
            }
        }
    }

    // チェーン
    if (options.includeChain) {
        if (const auto* chain = GetErrorChain(result)) {
            for (std::size_t i = 1; i < chain->GetDepth(); ++i) {
                const auto& entry = (*chain)[i];
                ErrorInfo entryInfo = GetErrorInfo(entry.result);
                builder.Append('\n');
                builder.Append(options.chainIndent);
                builder.Append(entryInfo.moduleName);
                builder.Append("::");
                builder.Append(entryInfo.errorName);
                if (options.includeNumericValues) {
                    builder.Append(" (");
                    builder.AppendNumber(entryInfo.module);
                    builder.Append(':');
                    builder.AppendNumber(entryInfo.description);
                    builder.Append(')');
                }
                if (!entryInfo.message.empty()) {
                    builder.Append(options.separator);
                    builder.Append(entryInfo.message);
                }
            }
        }
    }

    return builder.ToString();
}

std::string FormatResultCompact(::NS::Result result) noexcept {
    return FormatResult(result, FormatOptions{
        .includeMessage = false
    });
}

std::string FormatResultVerbose(::NS::Result result) noexcept {
    return FormatResult(result, FormatOptions{
        .verbose = true
    });
}

std::string FormatResultWithChain(::NS::Result result) noexcept {
    return FormatResult(result, FormatOptions{
        .includeChain = true
    });
}

std::string FormatResultWithContext(::NS::Result result) noexcept {
    return FormatResult(result, FormatOptions{
        .includeContext = true
    });
}

std::string FormatResultFull(::NS::Result result) noexcept {
    return FormatResult(result, FormatOptions{
        .includeContext = true,
        .includeChain = true,
        .verbose = true
    });
}

std::string FormatResultRaw(::NS::Result result) noexcept {
    return std::format("0x{:08X}", result.GetRawValue());
}

} // namespace NS::Result
```

### ResultRegistry

```cpp
// Source/common/result/Utility/ResultRegistry.h
#pragma once

#include "common/result/Core/Result.h"
#include <string_view>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace NS::Result {

/// エラー型情報
struct ResultTypeInfo {
    int module;
    int description;
    std::string_view typeName;        // "ResultPathNotFound"
    std::string_view fullName;        // "FileSystemResult::ResultPathNotFound"
    std::string_view message;         // 説明メッセージ
    bool isRange = false;             // 範囲型かどうか
    int rangeBegin = 0;
    int rangeEnd = 0;
};

/// エラー型レジストリ
/// スレッドセーフティ:
/// - 起動時（main前）の登録はシングルスレッドで行われると仮定
/// - 実行時は読み取りのみ（書き込みロック不要）
/// - FreezeAfterInit()呼び出し後は完全に読み取り専用
class ResultRegistry {
public:
    /// シングルトン取得
    [[nodiscard]] static ResultRegistry& Instance() noexcept;

    /// 型を登録（起動時のみ呼び出し可能）
    /// FreezeAfterInit()呼び出し後は無視される
    void Register(const ResultTypeInfo& info) noexcept;

    /// 初期化完了を宣言（以降の登録を禁止）
    /// main()開始時に呼び出すことを推奨
    void FreezeAfterInit() noexcept;

    /// 初期化完了済みか
    [[nodiscard]] bool IsFrozen() const noexcept;

    /// Resultから型情報を取得
    [[nodiscard]] const ResultTypeInfo* Find(::NS::Result result) const noexcept;

    /// モジュールの全型を取得
    [[nodiscard]] std::vector<ResultTypeInfo> GetModuleTypes(int module) const noexcept;

    /// 全型を取得
    [[nodiscard]] std::vector<ResultTypeInfo> GetAllTypes() const noexcept;

    /// 型数を取得
    [[nodiscard]] std::size_t GetTypeCount() const noexcept;

    /// 登録をクリア（テスト用、Freeze解除も行う）
    void Clear() noexcept;

private:
    ResultRegistry() = default;

    std::atomic<bool> m_frozen{false};
    mutable std::shared_mutex m_mutex;  // 読み取り時は共有ロック
    std::unordered_map<std::uint32_t, ResultTypeInfo> m_types;
    std::unordered_multimap<int, ResultTypeInfo> m_moduleTypes;
};

/// 自動登録用ヘルパー
template <typename ResultType>
class ResultAutoRegister {
public:
    ResultAutoRegister(
        std::string_view typeName,
        std::string_view fullName,
        std::string_view message
    ) noexcept {
        ResultTypeInfo info{
            .module = ResultType::kModule,
            .description = ResultType::kDescription,
            .typeName = typeName,
            .fullName = fullName,
            .message = message
        };
        ResultRegistry::Instance().Register(info);
    }
};

/// 範囲型の自動登録用
template <typename ResultType>
class ResultRangeAutoRegister {
public:
    ResultRangeAutoRegister(
        std::string_view typeName,
        std::string_view fullName,
        std::string_view message
    ) noexcept {
        ResultTypeInfo info{
            .module = ResultType::kModule,
            .description = ResultType::kDescriptionBegin,
            .typeName = typeName,
            .fullName = fullName,
            .message = message,
            .isRange = true,
            .rangeBegin = ResultType::kDescriptionBegin,
            .rangeEnd = ResultType::kDescriptionEnd
        };
        ResultRegistry::Instance().Register(info);
    }
};

} // namespace NS::Result

/// 自動登録マクロ
#define NS_REGISTER_RESULT(ResultType, message) \
    static ::NS::Result::ResultAutoRegister<ResultType> \
        _ns_result_register_##ResultType(#ResultType, #ResultType, message)

#define NS_REGISTER_RESULT_RANGE(ResultType, message) \
    static ::NS::Result::ResultRangeAutoRegister<ResultType> \
        _ns_result_register_##ResultType(#ResultType, #ResultType, message)
```

### ResultRegistry実装

```cpp
// Source/common/result/Utility/ResultRegistry.cpp
#include "common/result/Utility/ResultRegistry.h"
#include <algorithm>
#include <shared_mutex>

namespace NS::Result {

namespace {

std::uint32_t MakeKey(int module, int description) noexcept {
    return (static_cast<std::uint32_t>(module) << 16) |
           static_cast<std::uint32_t>(description);
}

} // namespace

ResultRegistry& ResultRegistry::Instance() noexcept {
    static ResultRegistry instance;
    return instance;
}

void ResultRegistry::Register(const ResultTypeInfo& info) noexcept {
    // Freeze済みなら登録を無視
    if (m_frozen.load(std::memory_order_acquire)) {
        return;
    }

    std::unique_lock lock(m_mutex);
    const auto key = MakeKey(info.module, info.description);
    m_types[key] = info;
    m_moduleTypes.emplace(info.module, info);
}

void ResultRegistry::FreezeAfterInit() noexcept {
    m_frozen.store(true, std::memory_order_release);
}

bool ResultRegistry::IsFrozen() const noexcept {
    return m_frozen.load(std::memory_order_acquire);
}

const ResultTypeInfo* ResultRegistry::Find(::NS::Result result) const noexcept {
    std::shared_lock lock(m_mutex);

    // 完全一致を検索
    const auto key = MakeKey(result.GetModule(), result.GetDescription());
    if (auto it = m_types.find(key); it != m_types.end()) {
        return &it->second;
    }

    // 範囲型を検索
    const int module = result.GetModule();
    const int desc = result.GetDescription();
    auto range = m_moduleTypes.equal_range(module);
    for (auto it = range.first; it != range.second; ++it) {
        const auto& info = it->second;
        if (info.isRange && info.rangeBegin <= desc && desc < info.rangeEnd) {
            return &info;
        }
    }

    return nullptr;
}

std::vector<ResultTypeInfo> ResultRegistry::GetModuleTypes(int module) const noexcept {
    std::shared_lock lock(m_mutex);
    std::vector<ResultTypeInfo> result;

    auto range = m_moduleTypes.equal_range(module);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.description < b.description; });

    return result;
}

std::vector<ResultTypeInfo> ResultRegistry::GetAllTypes() const noexcept {
    std::shared_lock lock(m_mutex);
    std::vector<ResultTypeInfo> result;
    result.reserve(m_types.size());

    for (const auto& [key, info] : m_types) {
        result.push_back(info);
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            if (a.module != b.module) return a.module < b.module;
            return a.description < b.description;
        });

    return result;
}

std::size_t ResultRegistry::GetTypeCount() const noexcept {
    std::shared_lock lock(m_mutex);
    return m_types.size();
}

void ResultRegistry::Clear() noexcept {
    std::unique_lock lock(m_mutex);
    m_types.clear();
    m_moduleTypes.clear();
    m_frozen.store(false, std::memory_order_release);
}

} // namespace NS::Result
```

## 使用例

```cpp
// 自動登録
NS_REGISTER_RESULT(FileSystemResult::ResultPathNotFound, "指定されたパスが見つかりません");
NS_REGISTER_RESULT_RANGE(FileSystemResult::ResultAccessError, "アクセスエラー");

// フォーマット
NS::Result result = FileSystemResult::ResultPathNotFound();
LOG_ERROR("{}", result);                              // FileSystem::PathNotFound (2:1)
LOG_ERROR("{}", FormatResultVerbose(result));        // 詳細表示
LOG_ERROR("{}", FormatResultWithChain(result));      // チェーン付き

// レジストリ照会
if (auto* info = ResultRegistry::Instance().Find(result)) {
    LOG_INFO("Error: {} - {}", info->fullName, info->message);
}

// 全エラー型の一覧
for (const auto& info : ResultRegistry::Instance().GetAllTypes()) {
    std::cout << info.fullName << " (" << info.module << ":" << info.description << ")\n";
}
```

## TODO

- [ ] `ResultFormatter.h` 作成
- [ ] `ResultFormatter.cpp` 作成
- [ ] `ResultRegistry.h` 作成
- [ ] `ResultRegistry.cpp` 作成
- [ ] std::format対応検証
- [ ] スレッドセーフティ検証
- [ ] ビルド確認
