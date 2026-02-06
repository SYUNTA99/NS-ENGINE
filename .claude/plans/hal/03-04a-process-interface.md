# 03-04a: プロセスインターフェース

## 目的

プロセス管理のインターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション2「プロセス管理」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`, `uint64`, `int32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformProcess.h`

## TODO

- [ ] `GenericPlatformProcess` インターフェース宣言
- [ ] スリープ関数宣言
- [ ] DLL管理関数宣言
- [ ] プロセス情報関数宣言
- [ ] スレッド制御関数宣言

## 実装内容

```cpp
// GenericPlatformProcess.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// プラットフォーム非依存のプロセス管理インターフェース
    struct GenericPlatformProcess
    {
        // スリープ
        static void Sleep(float seconds);
        static void SleepNoStats(float seconds);
        static void SleepInfinite();
        static void YieldThread();

        // DLL管理
        static void* GetDllHandle(const TCHAR* filename);
        static void FreeDllHandle(void* dllHandle);
        static void* GetDllExport(void* dllHandle, const TCHAR* procName);

        // プロセス情報
        static uint32 GetCurrentProcessId();
        static uint32 GetCurrentCoreNumber();

        // スレッド制御
        static void SetThreadAffinityMask(uint64 mask);
        static void SetThreadPriority(int32 priority);
    };
}
```

## 検証

- ヘッダがコンパイル可能

## 次のサブ計画

→ 03-04b: Windowsプロセス実装
