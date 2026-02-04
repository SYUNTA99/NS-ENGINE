# 03: Error - ErrorResultBase・ErrorRange

## 目的

エラー型定義のためのテンプレート基盤と範囲マッチング機能を実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Error/ErrorResultBase.h` | 新規作成 |
| `Source/common/result/Error/ErrorRange.h` | 新規作成 |
| `Source/common/result/Error/ErrorDefines.h` | 新規作成 |

## 設計

### ErrorResultBase

```cpp
// Source/common/result/Error/ErrorResultBase.h
#pragma once

#include "common/result/Core/ResultBase.h"
#include "common/result/Core/InternalAccessor.h"

namespace NS::Result::Detail {

/// 特定エラーを表すコンパイル時型
/// Persistence_/Severity_: ErrorCategory値（Reserved領域に格納）
template <int Module_, int Description_, int Persistence_ = 0, int Severity_ = 0>
class ErrorResultBase : public ResultBase<ErrorResultBase<Module_, Description_, Persistence_, Severity_>> {
public:
    using InnerType = ResultTraits::InnerType;

    static constexpr int kModule = Module_;
    static constexpr int kDescription = Description_;
    static constexpr int kPersistence = Persistence_;
    static constexpr int kSeverity = Severity_;

    // カテゴリ情報を含む完全な内部値
    static constexpr InnerType kInnerValueWithCategory =
        ResultTraits::SetCategory(
            ResultTraits::MakeInnerValueStatic<Module_, Description_>::value,
            Persistence_, Severity_);

    // 比較用（カテゴリを除外した値）
    static constexpr InnerType kInnerValue =
        ResultTraits::MakeInnerValueStatic<Module_, Description_>::value;

    constexpr ErrorResultBase() noexcept = default;

    [[nodiscard]] constexpr operator ::NS::Result() const noexcept {
        return ConstructResult(kInnerValueWithCategory);
    }

    [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept {
        return kInnerValue;  // カテゴリ除外（比較用）
    }

    [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept {
        // カテゴリを除外して比較
        return result.GetInnerValueForDebug() == kInnerValue;
    }

    // Result型との比較（カテゴリは無視）
    [[nodiscard]] friend constexpr bool operator==(::NS::Result lhs, ErrorResultBase) noexcept {
        return CanAccept(lhs);
    }
    [[nodiscard]] friend constexpr bool operator==(ErrorResultBase, ::NS::Result rhs) noexcept {
        return CanAccept(rhs);
    }
    [[nodiscard]] friend constexpr bool operator!=(::NS::Result lhs, ErrorResultBase rhs) noexcept {
        return !(lhs == rhs);
    }
    [[nodiscard]] friend constexpr bool operator!=(ErrorResultBase lhs, ::NS::Result rhs) noexcept {
        return !(lhs == rhs);
    }
};

} // namespace NS::Result::Detail
```

### ErrorRange

```cpp
// Source/common/result/Error/ErrorRange.h
#pragma once

#include "common/result/Core/Result.h"

namespace NS::Result::Detail {

/// エラー範囲を表すテンプレート（階層的マッチング用）
template <int Module_, int DescriptionBegin_, int DescriptionEnd_>
class ErrorRange {
public:
    static constexpr int kModule = Module_;
    static constexpr int kDescriptionBegin = DescriptionBegin_;
    static constexpr int kDescriptionEnd = DescriptionEnd_;

    static_assert(DescriptionBegin_ < DescriptionEnd_, "Invalid range: Begin must be < End");
    static_assert(ResultTraits::IsValidDescription(DescriptionBegin_), "DescriptionBegin out of range");
    static_assert(ResultTraits::IsValidDescription(DescriptionEnd_ - 1), "DescriptionEnd out of range");

    /// この範囲内かどうかを判定
    [[nodiscard]] static constexpr bool Includes(::NS::Result result) noexcept {
        if (result.GetModule() != kModule) {
            return false;
        }
        const int desc = result.GetDescription();
        return kDescriptionBegin <= desc && desc < kDescriptionEnd;
    }

    /// Result型との比較（範囲マッチング）
    [[nodiscard]] friend constexpr bool operator==(::NS::Result lhs, ErrorRange) noexcept {
        return Includes(lhs);
    }
    [[nodiscard]] friend constexpr bool operator==(ErrorRange, ::NS::Result rhs) noexcept {
        return Includes(rhs);
    }
    [[nodiscard]] friend constexpr bool operator!=(::NS::Result lhs, ErrorRange rhs) noexcept {
        return !(lhs == rhs);
    }
    [[nodiscard]] friend constexpr bool operator!=(ErrorRange lhs, ::NS::Result rhs) noexcept {
        return !(lhs == rhs);
    }
};

} // namespace NS::Result::Detail
```

### ErrorDefines（マクロ）

```cpp
// Source/common/result/Error/ErrorDefines.h
#pragma once

#include "common/result/Error/ErrorResultBase.h"
#include "common/result/Error/ErrorRange.h"
#include "common/result/Error/ErrorCategory.h"

/// 単一エラー定義（範囲なし、カテゴリなし）
#define NS_DEFINE_ERROR_RESULT(name, module, description) \
    class name : public ::NS::Result::Detail::ErrorResultBase<module, description, 0, 0> {}

/// 単一エラー定義（カテゴリ付き）
/// persistence: 0=Unknown, 1=Transient, 2=Permanent
/// severity: 0=Unknown, 1=Recoverable, 2=Fatal
#define NS_DEFINE_ERROR_RESULT_CAT(name, module, description, persistence, severity) \
    class name : public ::NS::Result::Detail::ErrorResultBase<module, description, persistence, severity> {}

/// 範囲付きエラー定義
/// - ErrorResultBaseからの継承: Result変換、CanAccept(単一)
/// - ErrorRangeからの継承: Includes(範囲)
#define NS_DEFINE_ERROR_RANGE_RESULT(name, module, descriptionBegin, descriptionEnd) \
    class name \
        : public ::NS::Result::Detail::ErrorResultBase<module, descriptionBegin, 0, 0> \
        , public ::NS::Result::Detail::ErrorRange<module, descriptionBegin, descriptionEnd> \
    { \
    public: \
        using RangeType = ::NS::Result::Detail::ErrorRange<module, descriptionBegin, descriptionEnd>; \
        using RangeType::Includes; \
        [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept { \
            return RangeType::Includes(result); \
        } \
    }

/// 範囲付きエラー定義（カテゴリ付き）
#define NS_DEFINE_ERROR_RANGE_RESULT_CAT(name, module, descriptionBegin, descriptionEnd, persistence, severity) \
    class name \
        : public ::NS::Result::Detail::ErrorResultBase<module, descriptionBegin, persistence, severity> \
        , public ::NS::Result::Detail::ErrorRange<module, descriptionBegin, descriptionEnd> \
    { \
    public: \
        using RangeType = ::NS::Result::Detail::ErrorRange<module, descriptionBegin, descriptionEnd>; \
        using RangeType::Includes; \
        [[nodiscard]] static constexpr bool CanAccept(::NS::Result result) noexcept { \
            return RangeType::Includes(result); \
        } \
    }

/// 抽象エラー範囲（具体的な値を持たない、範囲マッチングのみ）
#define NS_DEFINE_ABSTRACT_ERROR_RANGE(name, module, descriptionBegin, descriptionEnd) \
    class name : public ::NS::Result::Detail::ErrorRange<module, descriptionBegin, descriptionEnd> {}

// カテゴリ定数（マクロ引数用）
#define NS_PERSISTENCE_UNKNOWN    0
#define NS_PERSISTENCE_TRANSIENT  1
#define NS_PERSISTENCE_PERMANENT  2

#define NS_SEVERITY_UNKNOWN       0
#define NS_SEVERITY_RECOVERABLE   1
#define NS_SEVERITY_FATAL         2
```

## 使用例

```cpp
// モジュールID
namespace ModuleId {
    constexpr int FileSystem = 2;
}

// エラー定義
namespace NS::FileSystem {
    // 単一エラー
    NS_DEFINE_ERROR_RESULT(ResultPathNotFound, ModuleId::FileSystem, 1);
    NS_DEFINE_ERROR_RESULT(ResultPathAlreadyExists, ModuleId::FileSystem, 2);

    // 範囲付きエラー（サブエラー用）
    NS_DEFINE_ERROR_RANGE_RESULT(ResultDataCorrupted, ModuleId::FileSystem, 4000, 5000);

    // 抽象範囲（カテゴリ分け用）
    NS_DEFINE_ABSTRACT_ERROR_RANGE(ResultAccessError, ModuleId::FileSystem, 100, 200);
}

// 使用
NS::Result result = SomeOperation();

// 単一エラーとの比較
if (result == NS::FileSystem::ResultPathNotFound()) {
    // 完全一致
}

// 範囲マッチング
if (NS::FileSystem::ResultDataCorrupted::Includes(result)) {
    // 4000-4999の範囲内
}

// 抽象範囲
if (NS::FileSystem::ResultAccessError::Includes(result)) {
    // アクセス系エラー全般
}
```

## TODO

- [ ] `Source/common/result/Error/` ディレクトリ作成
- [ ] `ErrorResultBase.h` 作成
- [ ] `ErrorRange.h` 作成
- [ ] `ErrorDefines.h` 作成
- [ ] 単一エラー検証
- [ ] 範囲マッチング検証
- [ ] ビルド確認
