# 04: Error - ErrorCategory・ErrorInfo

## 目的

エラーの性質分類（リトライ可否、回復可能性）と詳細情報構造を実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Error/ErrorCategory.h` | 新規作成 |
| `Source/common/result/Error/ErrorInfo.h` | 新規作成 |

## 設計

### ErrorCategory

```cpp
// Source/common/result/Error/ErrorCategory.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Core/ResultTraits.h"
#include <cstdint>

namespace NS::Result {

/// エラーの時間的性質（2bit: Reserved領域 bit22-23に格納）
enum class ErrorPersistence : std::uint8_t {
    Unknown = 0,    // 不明
    Transient = 1,  // 一時的（リトライで解決可能）
    Permanent = 2,  // 永続的（リトライしても解決しない）
};

/// エラーの重大度（2bit: Reserved領域 bit24-25に格納）
enum class ErrorSeverity : std::uint8_t {
    Unknown = 0,    // 不明
    Recoverable = 1,// 回復可能（処理続行可）
    Fatal = 2,      // 致命的（処理続行不可）
};

/// エラーカテゴリ（組み合わせ）
/// Result値のReserved領域に直接エンコードされる
struct ErrorCategory {
    ErrorPersistence persistence = ErrorPersistence::Unknown;
    ErrorSeverity severity = ErrorSeverity::Unknown;

    [[nodiscard]] constexpr bool IsRetriable() const noexcept {
        return persistence == ErrorPersistence::Transient;
    }

    [[nodiscard]] constexpr bool IsFatal() const noexcept {
        return severity == ErrorSeverity::Fatal;
    }

    [[nodiscard]] constexpr bool IsRecoverable() const noexcept {
        return severity == ErrorSeverity::Recoverable;
    }

    [[nodiscard]] constexpr bool IsKnown() const noexcept {
        return persistence != ErrorPersistence::Unknown ||
               severity != ErrorSeverity::Unknown;
    }
};

/// エラーカテゴリを取得（Result値から直接デコード）
[[nodiscard]] inline constexpr ErrorCategory GetErrorCategory(::NS::Result result) noexcept {
    if (result.IsSuccess()) {
        return ErrorCategory{};
    }
    const auto raw = result.GetRawValue();
    return ErrorCategory{
        .persistence = static_cast<ErrorPersistence>(
            Detail::ResultTraits::GetPersistenceFromValue(raw)),
        .severity = static_cast<ErrorSeverity>(
            Detail::ResultTraits::GetSeverityFromValue(raw))
    };
}

/// リトライ可能か判定
[[nodiscard]] inline constexpr bool IsRetriable(::NS::Result result) noexcept {
    return GetErrorCategory(result).IsRetriable();
}

/// 致命的エラーか判定
[[nodiscard]] inline constexpr bool IsFatal(::NS::Result result) noexcept {
    return GetErrorCategory(result).IsFatal();
}

} // namespace NS::Result
```

### ErrorInfo

```cpp
// Source/common/result/Error/ErrorInfo.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Error/ErrorCategory.h"
#include <string_view>

namespace NS::Result {

/// エラー詳細情報
struct ErrorInfo {
    /// モジュール名（例: "FileSystem"）
    std::string_view moduleName;

    /// エラー名（例: "PathNotFound"）
    std::string_view errorName;

    /// 人間が読めるメッセージ
    std::string_view message;

    /// カテゴリ情報
    ErrorCategory category;

    /// 数値ID
    int module = 0;
    int description = 0;

    /// 有効かどうか
    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return !moduleName.empty();
    }
};

/// ErrorInfo解決関数のシグネチャ
using InfoResolver = ErrorInfo(*)(::NS::Result);

namespace Detail {

/// モジュール別情報解決テーブル
class InfoRegistry {
public:
    static constexpr int kMaxModules = 512;

    static void Register(int module, InfoResolver resolver) noexcept;
    static ErrorInfo Resolve(::NS::Result result) noexcept;

private:
    static InfoResolver s_resolvers[kMaxModules];
};

} // namespace Detail

/// エラー情報を取得
[[nodiscard]] inline ErrorInfo GetErrorInfo(::NS::Result result) noexcept {
    if (result.IsSuccess()) {
        return ErrorInfo{
            .moduleName = "Success",
            .errorName = "Success",
            .message = "Operation completed successfully",
            .category = {},
            .module = 0,
            .description = 0
        };
    }
    return Detail::InfoRegistry::Resolve(result);
}

} // namespace NS::Result
```

### 実装

ErrorCategoryはResult値のReserved領域に直接エンコードされるため、
別途の実装ファイルは不要。全てヘッダーオンリーでconstexpr対応。

```
Reserved領域 (10bit) のレイアウト:
┌─────────────────────────────────────────┐
│ bit31-26 │ bit25-24    │ bit23-22       │
│ (6bit)   │ (2bit)      │ (2bit)         │
│ 将来拡張 │ Severity    │ Persistence    │
│ 未使用   │ 0=Unknown   │ 0=Unknown      │
│          │ 1=Recover   │ 1=Transient    │
│          │ 2=Fatal     │ 2=Permanent    │
└─────────────────────────────────────────┘
```

```cpp
// Source/common/result/Error/ErrorInfo.cpp
#include "common/result/Error/ErrorInfo.h"

namespace NS::Result::Detail {

InfoResolver InfoRegistry::s_resolvers[kMaxModules] = {};

void InfoRegistry::Register(int module, InfoResolver resolver) noexcept {
    if (module >= 0 && module < kMaxModules) {
        s_resolvers[module] = resolver;
    }
}

ErrorInfo InfoRegistry::Resolve(::NS::Result result) noexcept {
    const int module = result.GetModule();
    if (module >= 0 && module < kMaxModules && s_resolvers[module]) {
        return s_resolvers[module](result);
    }
    return ErrorInfo{
        .moduleName = "Unknown",
        .errorName = "Unknown",
        .message = "Unknown error",
        .category = {},
        .module = module,
        .description = result.GetDescription()
    };
}

} // namespace NS::Result::Detail
```

## 使用例

```cpp
// カテゴリ付きエラー定義（03-error-result.md参照）
NS_DEFINE_ERROR_RESULT_CAT(ResultTimeout, ModuleId::Common, 300,
    NS_PERSISTENCE_TRANSIENT, NS_SEVERITY_RECOVERABLE);

NS_DEFINE_ERROR_RESULT_CAT(ResultDataCorrupted, ModuleId::Common, 500,
    NS_PERSISTENCE_PERMANENT, NS_SEVERITY_FATAL);

// 使用
NS::Result result = LoadFile(path);
if (result.IsFailure()) {
    // カテゴリはResult値から直接取得（レジストリ不要）
    auto category = NS::Result::GetErrorCategory(result);

    if (category.IsRetriable()) {
        // リトライ処理（Transientエラーの場合）
        for (int i = 0; i < 3; ++i) {
            result = LoadFile(path);
            if (result.IsSuccess()) break;
        }
    }

    if (category.IsFatal()) {
        // 致命的エラー処理
        LOG_FATAL("Fatal error: Module={}, Desc={}",
            result.GetModule(), result.GetDescription());
        std::abort();
    }
}

// constexprでカテゴリ判定も可能
static_assert(NS::Result::GetErrorCategory(
    NS::CommonResult::ResultTimeout()).IsRetriable());
```

## TODO

- [ ] `ErrorCategory.h` 作成（ヘッダーオンリー）
- [ ] `ErrorInfo.h` 作成
- [ ] `ErrorInfo.cpp` 作成（InfoRegistryの実装）
- [ ] constexprカテゴリ判定検証
- [ ] ビルド確認

## 設計変更履歴

- ErrorCategoryをResult値のReserved領域に直接格納するよう変更
- CategoryRegistryを廃止（レジストリ不要、constexpr対応）
- ErrorInfoは依然としてレジストリベース（文字列情報のため）
