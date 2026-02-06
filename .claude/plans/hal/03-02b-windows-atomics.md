# 03-02b: Windowsアトミック実装

## 目的

Windows固有のアトミック操作を実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション4「アトミック操作」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 03-02a アトミックインターフェース

## 必要なヘッダ（Windows）

- `<intrin.h>` - Interlocked系組み込み関数

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformAtomics.h`
- `source/engine/hal/Public/HAL/PlatformAtomics.h`

## TODO

- [ ] `WindowsPlatformAtomics` 32bit実装
- [ ] `WindowsPlatformAtomics` 64bit実装
- [ ] `WindowsPlatformAtomics` ポインタ実装
- [ ] typedef `PlatformAtomics` 定義
- [ ] `PlatformAtomics.h` エントリポイント作成

## 実装内容

### WindowsPlatformAtomics.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformAtomics.h"
#include <intrin.h>

namespace NS
{
    struct WindowsPlatformAtomics : public GenericPlatformAtomics
    {
        // 32bit
        static FORCEINLINE int32 InterlockedIncrement(volatile int32* value)
        {
            return (int32)::_InterlockedIncrement((volatile long*)value);
        }

        static FORCEINLINE int32 InterlockedDecrement(volatile int32* value)
        {
            return (int32)::_InterlockedDecrement((volatile long*)value);
        }

        static FORCEINLINE int32 InterlockedAdd(volatile int32* value, int32 amount)
        {
            return (int32)::_InterlockedExchangeAdd((volatile long*)value, (long)amount) + amount;
        }

        static FORCEINLINE int32 InterlockedExchange(volatile int32* value, int32 exchange)
        {
            return (int32)::_InterlockedExchange((volatile long*)value, (long)exchange);
        }

        static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* dest, int32 exchange, int32 comparand)
        {
            return (int32)::_InterlockedCompareExchange((volatile long*)dest, (long)exchange, (long)comparand);
        }

        // 64bit
        static FORCEINLINE int64 InterlockedIncrement(volatile int64* value)
        {
            return (int64)::_InterlockedIncrement64(value);
        }

        static FORCEINLINE int64 InterlockedDecrement(volatile int64* value)
        {
            return (int64)::_InterlockedDecrement64(value);
        }

        static FORCEINLINE int64 InterlockedAdd(volatile int64* value, int64 amount)
        {
            return (int64)::_InterlockedExchangeAdd64(value, amount) + amount;
        }

        static FORCEINLINE int64 InterlockedExchange(volatile int64* value, int64 exchange)
        {
            return (int64)::_InterlockedExchange64(value, exchange);
        }

        static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* dest, int64 exchange, int64 comparand)
        {
            return (int64)::_InterlockedCompareExchange64(dest, exchange, comparand);
        }

        // ポインタ
        static FORCEINLINE void* InterlockedExchangePtr(void** dest, void* exchange)
        {
            return ::_InterlockedExchangePointer(dest, exchange);
        }

        static FORCEINLINE void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand)
        {
            return ::_InterlockedCompareExchangePointer(dest, exchange, comparand);
        }
    };

    typedef WindowsPlatformAtomics PlatformAtomics;
}
```

### PlatformAtomics.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformAtomics.h"
#include COMPILED_PLATFORM_HEADER(PlatformAtomics.h)
```

## 検証

- `PlatformAtomics::InterlockedIncrement` が正しく+1
- `PlatformAtomics::InterlockedCompareExchange` がCAS動作
- マルチスレッドでの競合状態がない

## 注意事項

- Windowsでは `long` は32bit（`int32` と同サイズ）
- すべてヘッダ内インライン実装（cppファイル不要）

## 次のサブ計画

→ 03-03a: 時間インターフェース
