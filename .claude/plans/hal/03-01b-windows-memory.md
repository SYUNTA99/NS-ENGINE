# 03-01b: Windowsメモリ実装

## 目的

Windows固有のメモリ管理実装を行う。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション1「メモリ管理」

## 依存（commonモジュール）

- `common/utility/macros.h` - インライン制御（`NS_FORCEINLINE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 03-01a メモリ型定義（`PlatformMemoryStats`, `PlatformMemoryConstants`）

## 必要なヘッダ（Windows）

- `<windows.h>` - `VirtualAlloc`, `VirtualFree`, `GlobalMemoryStatusEx`
- `<sysinfoapi.h>` - `GetSystemInfo`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformMemory.h`
- `source/engine/hal/Public/HAL/PlatformMemory.h`
- `source/engine/hal/Private/Windows/WindowsPlatformMemory.cpp`

## TODO

- [ ] `WindowsPlatformMemory` 構造体（継承）
- [ ] `BinnedAllocFromOS` 実装（VirtualAlloc）
- [ ] `BinnedFreeToOS` 実装（VirtualFree）
- [ ] `GetStats` 実装（GlobalMemoryStatusEx）
- [ ] `GetConstants` 実装（GetSystemInfo）
- [ ] `PlatformMemory.h` エントリポイント作成
- [ ] typedef `PlatformMemory` 定義

## 実装内容

### WindowsPlatformMemory.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformMemory.h"

namespace NS
{
    struct WindowsPlatformMemory : public GenericPlatformMemory
    {
        static void Init();
        static PlatformMemoryStats GetStats();
        static const PlatformMemoryConstants& GetConstants();
        static void* BinnedAllocFromOS(SIZE_T size);
        static void BinnedFreeToOS(void* ptr, SIZE_T size);
    };

    typedef WindowsPlatformMemory PlatformMemory;
}
```

### WindowsPlatformMemory.cpp

```cpp
#include "Windows/WindowsPlatformMemory.h"
#include <windows.h>

namespace NS
{
    static PlatformMemoryConstants s_memoryConstants;
    static bool s_initialized = false;

    void WindowsPlatformMemory::Init()
    {
        if (s_initialized) return;

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);

        s_memoryConstants.pageSize = sysInfo.dwPageSize;
        s_memoryConstants.allocationGranularity = sysInfo.dwAllocationGranularity;
        s_memoryConstants.totalPhysical = statex.ullTotalPhys;
        s_memoryConstants.totalVirtual = statex.ullTotalVirtual;

        s_initialized = true;
    }

    void* WindowsPlatformMemory::BinnedAllocFromOS(SIZE_T size)
    {
        return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

    void WindowsPlatformMemory::BinnedFreeToOS(void* ptr, SIZE_T size)
    {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
}
```

### PlatformMemory.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformMemory.h"
#include COMPILED_PLATFORM_HEADER(PlatformMemory.h)
```

## 検証

- `PlatformMemory::Init()` が正常終了
- `PlatformMemory::GetStats()` が有効な値を返す
- `PlatformMemory::BinnedAllocFromOS(4096)` が非nullを返す
- `PlatformMemory::BinnedFreeToOS()` がクラッシュしない

## 次のサブ計画

→ 03-02a: アトミック型定義
