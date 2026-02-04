# 02: Core - ResultBase・Result

## 目的

CRTPベースクラス、基本Result型、ResultSuccess型、InternalAccessorを実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Core/ResultBase.h` | 新規作成 |
| `Source/common/result/Core/Result.h` | 新規作成 |
| `Source/common/result/Core/Result.cpp` | 新規作成 |
| `Source/common/result/Core/InternalAccessor.h` | 新規作成 |

## 設計

### ResultBase (CRTP)

```cpp
// Source/common/result/Core/ResultBase.h
#pragma once

#include "common/result/Core/ResultTraits.h"

namespace NS::Result::Detail {

/// CRTP基底クラス - GetModule/GetDescriptionを共有
template <typename Self>
class ResultBase {
public:
    using InnerType = ResultTraits::InnerType;
    static constexpr InnerType kInnerSuccessValue = ResultTraits::kInnerSuccessValue;

    [[nodiscard]] constexpr int GetModule() const noexcept {
        return ResultTraits::GetModuleFromValue(
            static_cast<const Self&>(*this).GetInnerValueForDebug());
    }

    [[nodiscard]] constexpr int GetDescription() const noexcept {
        return ResultTraits::GetDescriptionFromValue(
            static_cast<const Self&>(*this).GetInnerValueForDebug());
    }

    [[nodiscard]] constexpr int GetReserved() const noexcept {
        return ResultTraits::GetReservedFromValue(
            static_cast<const Self&>(*this).GetInnerValueForDebug());
    }

protected:
    ResultBase() noexcept = default;
    ~ResultBase() noexcept = default;
    ResultBase(const ResultBase&) noexcept = default;
    ResultBase& operator=(const ResultBase&) noexcept = default;
};

} // namespace NS::Result::Detail
```

### Result クラス

```cpp
// Source/common/result/Core/Result.h
#pragma once

#include <type_traits>
#include "common/result/Core/ResultBase.h"

namespace NS {

class ResultSuccess;

namespace Result::Detail {
class InternalAccessor;
[[noreturn]] void OnUnhandledResult(::NS::Result result) noexcept;
}

/// 処理結果を表す型
class Result : public Result::Detail::ResultBase<Result> {
    friend Result::Detail::InternalAccessor;
public:
    Result() noexcept = default;

    [[nodiscard]] constexpr bool IsSuccess() const noexcept { return m_value == kInnerSuccessValue; }
    [[nodiscard]] constexpr bool IsFailure() const noexcept { return !IsSuccess(); }

    [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept {
        return Result::Detail::ResultTraits::MaskReservedFromValue(m_value);
    }

    [[nodiscard]] constexpr InnerType GetRawValue() const noexcept { return m_value; }

    using ResultBase::GetModule;
    using ResultBase::GetDescription;
    using ResultBase::GetReserved;

    operator ResultSuccess() const noexcept;

    [[nodiscard]] static constexpr bool CanAccept(Result) noexcept { return true; }

    // 比較演算子
    [[nodiscard]] constexpr bool operator==(Result other) const noexcept {
        return GetInnerValueForDebug() == other.GetInnerValueForDebug();
    }
    [[nodiscard]] constexpr bool operator!=(Result other) const noexcept {
        return !(*this == other);
    }

private:
    InnerType m_value = kInnerSuccessValue;
    explicit constexpr Result(InnerType value) noexcept : m_value(value) {}
};

static_assert(std::is_trivially_copyable_v<Result>);
static_assert(sizeof(Result) == sizeof(Result::InnerType));

/// 成功を表す型
class ResultSuccess : public Result::Detail::ResultBase<ResultSuccess> {
public:
    [[nodiscard]] constexpr operator Result() const noexcept;
    [[nodiscard]] constexpr bool IsSuccess() const noexcept { return true; }
    [[nodiscard]] constexpr InnerType GetInnerValueForDebug() const noexcept { return kInnerSuccessValue; }
    [[nodiscard]] static constexpr bool CanAccept(Result result) noexcept { return result.IsSuccess(); }
};

static_assert(std::is_trivially_copyable_v<ResultSuccess>);

} // namespace NS
```

### InternalAccessor

```cpp
// Source/common/result/Core/InternalAccessor.h
#pragma once

#include "common/result/Core/Result.h"

namespace NS::Result::Detail {

/// 内部アクセス用クラス
class InternalAccessor {
public:
    [[nodiscard]] static constexpr ::NS::Result ConstructResult(ResultTraits::InnerType value) noexcept {
        return ::NS::Result(value);
    }

    [[nodiscard]] static constexpr ResultTraits::InnerType GetInnerValue(::NS::Result result) noexcept {
        return result.m_value;
    }

    [[nodiscard]] static constexpr ::NS::Result SetReserved(::NS::Result result, int reserved) noexcept {
        return ::NS::Result(ResultTraits::SetReserved(result.m_value, reserved));
    }
};

[[nodiscard]] inline constexpr ::NS::Result ConstructResult(ResultTraits::InnerType value) noexcept {
    return InternalAccessor::ConstructResult(value);
}

} // namespace NS::Result::Detail

// ResultSuccess::operator Result の実装
inline constexpr NS::Result NS::ResultSuccess::operator Result() const noexcept {
    return Result::Detail::ConstructResult(kInnerSuccessValue);
}

// Result::operator ResultSuccess の実装
inline NS::Result::operator ResultSuccess() const noexcept {
    if (!ResultSuccess::CanAccept(*this)) {
        Result::Detail::OnUnhandledResult(*this);
    }
    return ResultSuccess();
}
```

### OnUnhandledResult 実装

```cpp
// Source/common/result/Core/Result.cpp
#include "common/result/Core/Result.h"
#include "common/result/Core/InternalAccessor.h"
#include "common/logging/logging.h"
#include "common/assert/assert.h"
#include <cstdlib>

namespace NS::Result::Detail {

[[noreturn]] void OnUnhandledResult(::NS::Result result) noexcept {
    LOG_ERROR("Unhandled Result: Module={}, Description={}, Raw=0x{:08X}",
        result.GetModule(), result.GetDescription(), result.GetRawValue());

    NS_ASSERT(false, "Unhandled Result - conversion to ResultSuccess failed");
    std::abort();
}

} // namespace NS::Result::Detail
```

## TODO

- [ ] `ResultBase.h` 作成
- [ ] `Result.h` 作成
- [ ] `InternalAccessor.h` 作成
- [ ] `Result.cpp` 作成
- [ ] trivial性検証
- [ ] ビルド確認
