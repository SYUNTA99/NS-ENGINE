> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [09-01a: CPUInfo構造体](09-01a-cpu-info-struct.md)
> - [09-01b: WindowsPlatformMisc実装](09-01b-windows-platform-misc.md)

# 09-01: CPU機能検出（旧版）

## 目的

CPU機能フラグの検出機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション1「CPU機能検出」

## 依存（commonモジュール）

- `common/utility/macros.h` - アーキテクチャ検出（`NS_ARCH_X64`, `NS_ARCH_X86`）
- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_CPU_X86_FAMILY` ← common由来）
- 01-04 プラットフォーム型（`ANSICHAR`, `TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/PlatformMisc.h`
- `source/engine/hal/Public/Windows/WindowsPlatformMisc.h`
- `source/engine/hal/Private/Windows/WindowsPlatformMisc.cpp`

## TODO

- [ ] `CPUFeatureBits` 列挙型（x86）
- [ ] CPU機能検出関数
- [ ] CPUInfo構造体
- [ ] CPUID呼び出し実装（Windows）

## 実装内容

```cpp
// PlatformMisc.h
#pragma once

namespace NS
{
#if PLATFORM_CPU_X86_FAMILY
    // ビットフラグ用非スコープ列挙型
    enum CPUFeatureBits : uint32
    {
        CPUFeatureSSE2 = 1U << 2,
        CPUFeatureSSSE3 = 1U << 3,
        CPUFeatureSSE42 = 1U << 4,
        CPUFeatureAVX = 1U << 5,
        CPUFeatureAVX2 = 1U << 8,
        CPUFeatureAVX512 = 1U << 10
    };
#endif

    struct CPUInfo
    {
        uint32 numCores;
        uint32 numLogicalProcessors;
        uint32 cacheLineSize;
        ANSICHAR vendor[16];
        ANSICHAR brand[64];
    };

    struct GenericPlatformMisc
    {
        static void PlatformInit();

        static uint32 GetCPUInfo();
        static CPUInfo GetCPUDetails();
        static uint32 GetCacheLineSize();

#if PLATFORM_CPU_X86_FAMILY
        static uint32 GetFeatureBits_X86();
        static bool CheckFeatureBit_X86(uint32 featureBit);
        static bool HasAVX2InstructionSupport();
#endif

        static const TCHAR* GetPlatformName();
        static const TCHAR* GetOSVersion();
    };
}

// WindowsPlatformMisc.h
namespace NS
{
    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        static uint32 GetFeatureBits_X86();
        static CPUInfo GetCPUDetails();
    };

    typedef WindowsPlatformMisc PlatformMisc;
}
```

## 検証

- AVX2対応CPUで HasAVX2InstructionSupport() が true を返す
- CPU情報が正しく取得される
- `GetCPUDetails()` がベンダー名（Intel/AMD）を返す

## 必要なヘッダ（Windows実装）

- `<intrin.h>` - `__cpuid`, `__cpuidex`

## 注意事項

- CPUID命令はx86/x64でのみ使用可能
- ARM64では別のCPU機能検出が必要
- AVX2検出はOS対応も確認が必要（XSAVE）

## 次のサブ計画

→ 09-02: Windows固有機能
