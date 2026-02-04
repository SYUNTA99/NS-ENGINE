# 01: Core - ResultTraits

## 目的

Result型の32bitレイアウトとビット操作を定義。全Result型システムの基盤。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Core/ResultTraits.h` | 新規作成 |

## 設計

### ビットレイアウト

```
┌─────────────────────────────────────────────────────────────┐
│                    32bit Result Value                        │
├─────────────────────────────────────────────────────────────┤
│  31  30  29  28  27  26  25  24  23  22 │ 21 ... 9 │ 8 ... 0│
│  ├──────────────────────────────────────┤├─────────┤├───────┤│
│  │           Reserved (10bit)           ││Desc(13) ││Mod(9) ││
│  │           将来拡張用                  ││詳細     ││モジュ ││
│  │           (ErrorCategory等)          ││エラー   ││ールID ││
│  └──────────────────────────────────────┘└─────────┘└───────┘│
│                                                              │
│  値 = 0: 成功                                                │
│  値 ≠ 0: 失敗                                                │
└─────────────────────────────────────────────────────────────┘
```

### ResultTraits クラス

```cpp
// Source/common/result/Core/ResultTraits.h
#pragma once

#include <cstdint>
#include <climits>
#include <type_traits>

namespace NS::Result::Detail {

/// Result型の内部表現とビット操作
class ResultTraits {
public:
    //=========================================================================
    // 型定義
    //=========================================================================
    using InnerType = std::uint32_t;

    //=========================================================================
    // レイアウト定数
    //=========================================================================
    static constexpr InnerType kInnerSuccessValue = 0;

    // Module: 9bit (0-8)
    static constexpr int kModuleBitsOffset = 0;
    static constexpr int kModuleBitsCount = 9;
    static constexpr int kModuleBegin = 1;
    static constexpr int kModuleEnd = 1 << kModuleBitsCount;  // 512
    static constexpr InnerType kModuleMask =
        ((InnerType(1) << kModuleBitsCount) - 1) << kModuleBitsOffset;

    // Description: 13bit (9-21)
    static constexpr int kDescriptionBitsOffset = kModuleBitsOffset + kModuleBitsCount;
    static constexpr int kDescriptionBitsCount = 13;
    static constexpr int kDescriptionBegin = 0;
    static constexpr int kDescriptionEnd = 1 << kDescriptionBitsCount;  // 8192
    static constexpr InnerType kDescriptionMask =
        ((InnerType(1) << kDescriptionBitsCount) - 1) << kDescriptionBitsOffset;

    // Reserved: 10bit (22-31)
    // 内部レイアウト:
    //   bit 22-23 (2bit): ErrorPersistence (0=Unknown, 1=Transient, 2=Permanent)
    //   bit 24-25 (2bit): ErrorSeverity (0=Unknown, 1=Recoverable, 2=Fatal)
    //   bit 26-31 (6bit): 将来拡張用（未使用）
    static constexpr int kReservedBitsOffset = kDescriptionBitsOffset + kDescriptionBitsCount;
    static constexpr int kReservedBitsCount = 10;
    static constexpr InnerType kReservedMask =
        ((InnerType(1) << kReservedBitsCount) - 1) << kReservedBitsOffset;

    // Reserved内部レイアウト
    static constexpr int kPersistenceBitsOffset = kReservedBitsOffset;
    static constexpr int kPersistenceBitsCount = 2;
    static constexpr InnerType kPersistenceMask =
        ((InnerType(1) << kPersistenceBitsCount) - 1) << kPersistenceBitsOffset;

    static constexpr int kSeverityBitsOffset = kPersistenceBitsOffset + kPersistenceBitsCount;
    static constexpr int kSeverityBitsCount = 2;
    static constexpr InnerType kSeverityMask =
        ((InnerType(1) << kSeverityBitsCount) - 1) << kSeverityBitsOffset;

    static constexpr int kFutureBitsOffset = kSeverityBitsOffset + kSeverityBitsCount;
    static constexpr int kFutureBitsCount = 6;  // 将来拡張用

    //=========================================================================
    // コンパイル時値生成
    //=========================================================================
    template <int Module, int Description>
    struct MakeInnerValueStatic : std::integral_constant<InnerType,
        (static_cast<InnerType>(Module) << kModuleBitsOffset) |
        (static_cast<InnerType>(Description) << kDescriptionBitsOffset)
    > {
        static_assert(kModuleBegin <= Module && Module < kModuleEnd, "Module out of range");
        static_assert(kDescriptionBegin <= Description && Description < kDescriptionEnd, "Description out of range");
    };

    //=========================================================================
    // 実行時値操作
    //=========================================================================
    [[nodiscard]] static constexpr InnerType MakeInnerValue(int module, int description) noexcept {
        return (static_cast<InnerType>(module) << kModuleBitsOffset) |
               (static_cast<InnerType>(description) << kDescriptionBitsOffset);
    }

    [[nodiscard]] static constexpr InnerType MakeInnerValueWithReserved(
        int module, int description, int reserved) noexcept {
        return MakeInnerValue(module, description) |
               (static_cast<InnerType>(reserved) << kReservedBitsOffset);
    }

    [[nodiscard]] static constexpr int GetModuleFromValue(InnerType value) noexcept {
        return static_cast<int>((value & kModuleMask) >> kModuleBitsOffset);
    }

    [[nodiscard]] static constexpr int GetDescriptionFromValue(InnerType value) noexcept {
        return static_cast<int>((value & kDescriptionMask) >> kDescriptionBitsOffset);
    }

    [[nodiscard]] static constexpr int GetReservedFromValue(InnerType value) noexcept {
        return static_cast<int>((value & kReservedMask) >> kReservedBitsOffset);
    }

    [[nodiscard]] static constexpr InnerType MaskReservedFromValue(InnerType value) noexcept {
        return value & ~kReservedMask;
    }

    [[nodiscard]] static constexpr InnerType SetReserved(InnerType value, int reserved) noexcept {
        return (value & ~kReservedMask) |
               (static_cast<InnerType>(reserved) << kReservedBitsOffset);
    }

    // ErrorCategory用アクセサ
    [[nodiscard]] static constexpr int GetPersistenceFromValue(InnerType value) noexcept {
        return static_cast<int>((value & kPersistenceMask) >> kPersistenceBitsOffset);
    }

    [[nodiscard]] static constexpr int GetSeverityFromValue(InnerType value) noexcept {
        return static_cast<int>((value & kSeverityMask) >> kSeverityBitsOffset);
    }

    [[nodiscard]] static constexpr InnerType SetPersistence(InnerType value, int persistence) noexcept {
        return (value & ~kPersistenceMask) |
               (static_cast<InnerType>(persistence) << kPersistenceBitsOffset);
    }

    [[nodiscard]] static constexpr InnerType SetSeverity(InnerType value, int severity) noexcept {
        return (value & ~kSeverityMask) |
               (static_cast<InnerType>(severity) << kSeverityBitsOffset);
    }

    [[nodiscard]] static constexpr InnerType SetCategory(InnerType value, int persistence, int severity) noexcept {
        return SetSeverity(SetPersistence(value, persistence), severity);
    }

    //=========================================================================
    // 検証
    //=========================================================================
    [[nodiscard]] static constexpr bool IsValidModule(int module) noexcept {
        return kModuleBegin <= module && module < kModuleEnd;
    }

    [[nodiscard]] static constexpr bool IsValidDescription(int description) noexcept {
        return kDescriptionBegin <= description && description < kDescriptionEnd;
    }

private:
    static_assert(kModuleBitsCount + kDescriptionBitsCount + kReservedBitsCount ==
                  sizeof(InnerType) * CHAR_BIT, "Total bits must equal 32");
};

} // namespace NS::Result::Detail
```

## TODO

- [ ] `Source/common/result/Core/` ディレクトリ作成
- [ ] `ResultTraits.h` 作成
- [ ] static_assert検証
- [ ] ビルド確認

## 検証コード

```cpp
using Traits = NS::Result::Detail::ResultTraits;

// レイアウト検証
static_assert(Traits::kModuleBitsCount == 9);
static_assert(Traits::kDescriptionBitsCount == 13);
static_assert(Traits::kReservedBitsCount == 10);

// 値生成・抽出
constexpr auto v = Traits::MakeInnerValue(2, 100);
static_assert(Traits::GetModuleFromValue(v) == 2);
static_assert(Traits::GetDescriptionFromValue(v) == 100);

// Reserved操作
constexpr auto vr = Traits::SetReserved(v, 5);
static_assert(Traits::GetReservedFromValue(vr) == 5);
static_assert(Traits::MaskReservedFromValue(vr) == v);
```
