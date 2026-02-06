# 10-03a: GetTypeHashテンプレート

## 目的

型ごとのハッシュ関数とハッシュ結合関数を定義する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション3「ハッシュ関数」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`uint32`, `uint64`, `int8`〜`int64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`UPTRINT`）
- 02-03 関数呼び出し規約（`FORCEINLINE`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/Hash.h`

## TODO

- [ ] 整数型の `GetTypeHash` 実装
- [ ] 浮動小数点型の `GetTypeHash` 実装
- [ ] ポインタの `GetTypeHash` 実装
- [ ] `HashCombine` 関数
- [ ] `HashCombineFast` 関数

## 実装内容

```cpp
// Hash.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PlatformMacros.h"

namespace NS
{
    // 8bit整数
    FORCEINLINE uint32 GetTypeHash(uint8 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int8 value) { return static_cast<uint32>(value); }

    // 16bit整数
    FORCEINLINE uint32 GetTypeHash(uint16 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int16 value) { return static_cast<uint32>(value); }

    // 32bit整数
    FORCEINLINE uint32 GetTypeHash(uint32 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int32 value) { return static_cast<uint32>(value); }

    // 64bit整数
    FORCEINLINE uint32 GetTypeHash(uint64 value)
    {
        // 上位32bitと下位32bitをXOR
        return static_cast<uint32>(value) ^ static_cast<uint32>(value >> 32);
    }

    FORCEINLINE uint32 GetTypeHash(int64 value)
    {
        return GetTypeHash(static_cast<uint64>(value));
    }

    // ポインタ
    FORCEINLINE uint32 GetTypeHash(const void* ptr)
    {
        return GetTypeHash(reinterpret_cast<UPTRINT>(ptr));
    }

    // 浮動小数点（ビットパターンをハッシュ）
    FORCEINLINE uint32 GetTypeHash(float value)
    {
        // IEEE 754ビットパターンをそのまま使用
        union { float f; uint32 u; } converter;
        converter.f = value;
        return converter.u;
    }

    FORCEINLINE uint32 GetTypeHash(double value)
    {
        union { double d; uint64 u; } converter;
        converter.d = value;
        return GetTypeHash(converter.u);
    }

    /// ハッシュ値を結合（Boost風）
    ///
    /// 黄金比を使用した結合アルゴリズム。
    /// 良好な分布を維持しながら2つのハッシュを結合。
    FORCEINLINE uint32 HashCombine(uint32 a, uint32 b)
    {
        // Boost hash_combine アルゴリズム
        // 0x9e3779b9 は 2^32 / phi (黄金比)
        return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

    /// 高速ハッシュ結合（品質より速度優先）
    FORCEINLINE uint32 HashCombineFast(uint32 a, uint32 b)
    {
        return a ^ (b * 0x9e3779b9);
    }

    /// 複数のハッシュを結合
    template<typename... Args>
    FORCEINLINE uint32 HashCombineMultiple(uint32 first, Args... rest)
    {
        if constexpr (sizeof...(rest) == 0)
        {
            return first;
        }
        else
        {
            return HashCombine(first, HashCombineMultiple(rest...));
        }
    }
}
```

## 検証

- `GetTypeHash(123)` が決定論的な値
- `GetTypeHash(0.0f)` と `GetTypeHash(-0.0f)` が異なる
- `HashCombine(a, b)` が良好な分布

## 注意事項

- 浮動小数点のハッシュはビットパターンベース（`-0.0` と `0.0` は異なる）
- `HashCombine` はBoost由来の標準的なアルゴリズム
- ポインタのハッシュはアドレスベース

## 次のサブ計画

→ 10-03b: CRC32実装
