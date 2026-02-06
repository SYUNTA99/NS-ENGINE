# 09-01a: CPUInfo構造体

## 目的

CPU情報と機能フラグの構造体を定義する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション1「CPU機能検出」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`）
- `common/utility/macros.h` - アーキテクチャ検出（`NS_ARCH_X64`, `NS_ARCH_X86`）

## 依存（HAL）

- 01-02 プラットフォームマクロ（`PLATFORM_CPU_X86_FAMILY`）
- 01-04 プラットフォーム型（`ANSICHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/PlatformMisc.h`

## TODO

- [ ] `CPUFeatureBits` 列挙型定義（x86）
- [ ] `CPUInfo` 構造体定義
- [ ] `GenericPlatformMisc` インターフェース宣言

## 実装内容

```cpp
// PlatformMisc.h
#pragma once

#include "common/utility/types.h"
#include "HAL/Platform.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
#if PLATFORM_CPU_X86_FAMILY

    /// CPU機能ビットフラグ（x86/x64）
    ///
    /// ビット演算で組み合わせ可能な非スコープ列挙型。
    enum CPUFeatureBits : uint32
    {
        CPUFeatureNone = 0,
        CPUFeatureSSE2 = 1U << 0,
        CPUFeatureSSE3 = 1U << 1,
        CPUFeatureSSSE3 = 1U << 2,
        CPUFeatureSSE41 = 1U << 3,
        CPUFeatureSSE42 = 1U << 4,
        CPUFeatureAVX = 1U << 5,
        CPUFeatureFMA3 = 1U << 6,
        CPUFeatureAVX2 = 1U << 7,
        CPUFeatureAVX512 = 1U << 8,
        CPUFeatureAESNI = 1U << 9,
        CPUFeaturePOPCNT = 1U << 10,
        CPUFeatureLZCNT = 1U << 11,
        CPUFeatureBMI1 = 1U << 12,
        CPUFeatureBMI2 = 1U << 13,
    };

#endif // PLATFORM_CPU_X86_FAMILY

    /// CPU詳細情報
    struct CPUInfo
    {
        uint32 numCores;              ///< 物理コア数
        uint32 numLogicalProcessors;  ///< 論理プロセッサ数
        uint32 cacheLineSize;         ///< キャッシュラインサイズ（バイト）
        ANSICHAR vendor[16];          ///< ベンダー名（"GenuineIntel", "AuthenticAMD"）
        ANSICHAR brand[64];           ///< CPU名（"Intel Core i7-..."）
    };

    /// プラットフォーム固有機能の汎用インターフェース
    struct GenericPlatformMisc
    {
        /// プラットフォーム初期化
        static void PlatformInit();

        /// CPU情報取得
        static uint32 GetCPUInfo();

        /// CPU詳細情報取得
        static CPUInfo GetCPUDetails();

        /// キャッシュラインサイズ取得
        static uint32 GetCacheLineSize();

#if PLATFORM_CPU_X86_FAMILY
        /// CPU機能ビット取得（x86/x64）
        static uint32 GetFeatureBits_X86();

        /// 特定のCPU機能があるかチェック（x86/x64）
        static bool CheckFeatureBit_X86(uint32 featureBit);

        /// AVX2命令サポートの有無
        static bool HasAVX2InstructionSupport();

        /// AVX512命令サポートの有無
        static bool HasAVX512InstructionSupport();
#endif

        /// プラットフォーム名取得
        static const TCHAR* GetPlatformName();

        /// OSバージョン取得
        static const TCHAR* GetOSVersion();
    };
}
```

## 検証

- `CPUInfo` 構造体が正しいサイズ
- `CPUFeatureBits` がビット演算可能

## 次のサブ計画

→ 09-01b: WindowsPlatformMisc実装
