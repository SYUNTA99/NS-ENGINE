# 09-01b: WindowsPlatformMisc実装

## 目的

Windows固有のCPU機能検出を実装する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション1「CPU機能検出」

## 依存（commonモジュール）

- `common/utility/macros.h` - アーキテクチャ検出

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 09-01a CPUInfo構造体

## 必要なヘッダ（Windows）

- `<windows.h>` - `GetLogicalProcessorInformation`
- `<intrin.h>` - `__cpuid`, `__cpuidex`, `_xgetbv`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformMisc.h`
- `source/engine/hal/Private/Windows/WindowsPlatformMisc.cpp`

## TODO

- [ ] `WindowsPlatformMisc` クラス定義
- [ ] CPUID呼び出し実装
- [ ] CPU機能検出実装
- [ ] AVX/AVX2/AVX512検出実装（XSAVE対応確認含む）
- [ ] コア数検出実装

## 実装内容

### WindowsPlatformMisc.h

```cpp
#pragma once

#include "HAL/PlatformMisc.h"

namespace NS
{
    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        static void PlatformInit();
        static uint32 GetCPUInfo();
        static CPUInfo GetCPUDetails();
        static uint32 GetCacheLineSize();

#if PLATFORM_CPU_X86_FAMILY
        static uint32 GetFeatureBits_X86();
        static bool CheckFeatureBit_X86(uint32 featureBit);
        static bool HasAVX2InstructionSupport();
        static bool HasAVX512InstructionSupport();
#endif

        static const TCHAR* GetPlatformName();
        static const TCHAR* GetOSVersion();

    private:
        static uint32 s_cpuFeatures;
        static CPUInfo s_cpuInfo;
        static bool s_initialized;
    };

    typedef WindowsPlatformMisc PlatformMisc;
}
```

### WindowsPlatformMisc.cpp

```cpp
#include "Windows/WindowsPlatformMisc.h"
#include <windows.h>
#include <intrin.h>
#include <cstring>

namespace NS
{
    uint32 WindowsPlatformMisc::s_cpuFeatures = 0;
    CPUInfo WindowsPlatformMisc::s_cpuInfo = {};
    bool WindowsPlatformMisc::s_initialized = false;

    void WindowsPlatformMisc::PlatformInit()
    {
        if (s_initialized) return;

        // CPUIDでベンダー文字列取得
        int cpuInfo[4] = {};
        __cpuid(cpuInfo, 0);

        // ベンダー文字列
        *reinterpret_cast<int*>(s_cpuInfo.vendor) = cpuInfo[1];
        *reinterpret_cast<int*>(s_cpuInfo.vendor + 4) = cpuInfo[3];
        *reinterpret_cast<int*>(s_cpuInfo.vendor + 8) = cpuInfo[2];
        s_cpuInfo.vendor[12] = '\0';

        // ブランド文字列
        char brand[49] = {};
        __cpuid(cpuInfo, 0x80000000);
        if (static_cast<uint32>(cpuInfo[0]) >= 0x80000004)
        {
            __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
            __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
            __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
        }
        std::strncpy(s_cpuInfo.brand, brand, 63);
        s_cpuInfo.brand[63] = '\0';

        // 機能フラグ検出
        s_cpuFeatures = GetFeatureBits_X86();

        // コア数
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_cpuInfo.numLogicalProcessors = sysInfo.dwNumberOfProcessors;
        s_cpuInfo.numCores = sysInfo.dwNumberOfProcessors; // 物理コア数は別途計算が必要

        // キャッシュラインサイズ
        s_cpuInfo.cacheLineSize = sysInfo.dwPageSize > 0 ? 64 : 64; // デフォルト64

        s_initialized = true;
    }

    uint32 WindowsPlatformMisc::GetFeatureBits_X86()
    {
        uint32 features = 0;
        int cpuInfo[4] = {};

        __cpuid(cpuInfo, 1);

        // SSE2 (EDX bit 26)
        if (cpuInfo[3] & (1 << 26)) features |= CPUFeatureSSE2;
        // SSE3 (ECX bit 0)
        if (cpuInfo[2] & (1 << 0)) features |= CPUFeatureSSE3;
        // SSSE3 (ECX bit 9)
        if (cpuInfo[2] & (1 << 9)) features |= CPUFeatureSSSE3;
        // SSE4.1 (ECX bit 19)
        if (cpuInfo[2] & (1 << 19)) features |= CPUFeatureSSE41;
        // SSE4.2 (ECX bit 20)
        if (cpuInfo[2] & (1 << 20)) features |= CPUFeatureSSE42;
        // POPCNT (ECX bit 23)
        if (cpuInfo[2] & (1 << 23)) features |= CPUFeaturePOPCNT;
        // AES-NI (ECX bit 25)
        if (cpuInfo[2] & (1 << 25)) features |= CPUFeatureAESNI;
        // AVX (ECX bit 28) - OSサポート確認も必要
        if (cpuInfo[2] & (1 << 28))
        {
            // XSAVEが有効でOSがAVX状態を保存しているか確認
            if ((cpuInfo[2] & (1 << 27)) && ((_xgetbv(0) & 0x6) == 0x6))
            {
                features |= CPUFeatureAVX;

                // FMA3 (ECX bit 12)
                if (cpuInfo[2] & (1 << 12)) features |= CPUFeatureFMA3;

                // AVX2チェック
                __cpuidex(cpuInfo, 7, 0);
                if (cpuInfo[1] & (1 << 5)) features |= CPUFeatureAVX2;
                if (cpuInfo[1] & (1 << 3)) features |= CPUFeatureBMI1;
                if (cpuInfo[1] & (1 << 8)) features |= CPUFeatureBMI2;

                // AVX512チェック（XSAVE状態も確認）
                if ((cpuInfo[1] & (1 << 16)) && ((_xgetbv(0) & 0xE6) == 0xE6))
                {
                    features |= CPUFeatureAVX512;
                }
            }
        }

        return features;
    }

    bool WindowsPlatformMisc::HasAVX2InstructionSupport()
    {
        return CheckFeatureBit_X86(CPUFeatureAVX2);
    }

    bool WindowsPlatformMisc::HasAVX512InstructionSupport()
    {
        return CheckFeatureBit_X86(CPUFeatureAVX512);
    }

    bool WindowsPlatformMisc::CheckFeatureBit_X86(uint32 featureBit)
    {
        if (!s_initialized)
        {
            PlatformInit();
        }
        return (s_cpuFeatures & featureBit) != 0;
    }

    // ... 他のメソッド
}
```

## 検証

- AVX2対応CPUで `HasAVX2InstructionSupport()` が true
- `GetCPUDetails().vendor` が "GenuineIntel" または "AuthenticAMD"
- `GetCPUDetails().numLogicalProcessors` が正しい値

## 注意事項

- AVX/AVX2はOSサポート（XSAVE）も確認が必要
- `_xgetbv` はAVX対応CPU以外では使用不可
- 初期化は一度だけ行う（静的変数でキャッシュ）

## 次のサブ計画

→ 09-02a: Windows COM初期化
