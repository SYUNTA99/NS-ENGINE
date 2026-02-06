> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [10-03a: GetTypeHashテンプレート](10-03a-gettypehash.md)
> - [10-03b: CRC32実装](10-03b-crc32.md)

# 10-03: ハッシュ関数（旧版）

## 目的

汎用ハッシュ関数を実装する。

## 参考

[Platform_Abstraction_Layer_Part10.md](docs/Platform_Abstraction_Layer_Part10.md) - セクション3「ハッシュ関数」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）
- `common/utility/types.h` - 基本型（`uint32`, `uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`SIZE_T`, `UPTRINT`, `TCHAR`, `ANSICHAR`, `WIDECHAR`）
- 02-03 関数呼び出し規約（`FORCEINLINE` ← `NS_FORCEINLINE`のエイリアス）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/Hash.h`

## TODO

- [ ] CRC32 ハッシュ
- [ ] 文字列ハッシュ（CityHash風）
- [ ] `GetTypeHash` テンプレート
- [ ] `HashCombine` 関数

## 実装内容

```cpp
// Hash.h
#pragma once

namespace NS
{
    FORCEINLINE uint32 GetTypeHash(uint8 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int8 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(uint16 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int16 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(uint32 value) { return value; }
    FORCEINLINE uint32 GetTypeHash(int32 value) { return value; }

    FORCEINLINE uint32 GetTypeHash(uint64 value)
    {
        return (uint32)value ^ (uint32)(value >> 32);
    }

    FORCEINLINE uint32 GetTypeHash(int64 value)
    {
        return GetTypeHash((uint64)value);
    }

    FORCEINLINE uint32 GetTypeHash(const void* ptr)
    {
        return GetTypeHash((UPTRINT)ptr);
    }

    FORCEINLINE uint32 GetTypeHash(float value)
    {
        return *(uint32*)&value;
    }

    FORCEINLINE uint32 GetTypeHash(double value)
    {
        return GetTypeHash(*(uint64*)&value);
    }

    FORCEINLINE uint32 HashCombine(uint32 a, uint32 b)
    {
        return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

    uint32 Crc32(const void* data, SIZE_T length, uint32 crc = 0);

    uint32 StrCrc32(const ANSICHAR* str);
    uint32 StrCrc32(const WIDECHAR* str);

    inline uint32 StrCrc32(const TCHAR* str)
    {
        if constexpr (sizeof(TCHAR) == sizeof(WIDECHAR))
        {
            return StrCrc32((const WIDECHAR*)str);
        }
        else
        {
            return StrCrc32((const ANSICHAR*)str);
        }
    }
}
```

## 検証

- 同一入力で同一ハッシュ値
- HashCombine が良好な分布を持つ
- `GetTypeHash(123)` が決定論的な値を返す

## 注意事項

- `HashCombine` はBoost由来の黄金比アルゴリズム
- CRC32はハードウェア命令使用可（SSE4.2）
- 文字列ハッシュはヌル終端前提

## 備考

これがHAL実装計画の最後のサブ計画です。
