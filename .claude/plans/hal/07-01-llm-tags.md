> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [07-01a: LLMタグ列挙](07-01a-llm-tag-enum.md)
> - [07-01b: LLMTrackerクラス](07-01b-llm-tracker-class.md)

# 07-01: LLM タグシステム（旧版）

## 目的

Low Level Memory Tracker のタグシステムを実装する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション3「タグシステム」

## 依存（commonモジュール）

- `common/utility/macros.h` - ビルド構成検出（`NS_BUILD_DEBUG`, `NS_BUILD_RELEASE`）
- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

- 04-01 premakeビルド設定（`NS_DEBUG`, `NS_DEVELOPMENT` マクロはビルド構成で定義）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`

## TODO

- [ ] `LLMTag` 列挙型定義
- [ ] LLM有効化マクロ定義
- [ ] 基本タグ（Untagged, Total等）
- [ ] カテゴリタグ（Textures, Meshes等）

## 実装内容

```cpp
// LowLevelMemTracker.h
#pragma once

// LLM有効化フラグ
#ifndef LLM_ENABLED_IN_CONFIG
    #if defined(NS_DEBUG) || defined(NS_DEVELOPMENT)
        #define LLM_ENABLED_IN_CONFIG 1
    #else
        #define LLM_ENABLED_IN_CONFIG 0
    #endif
#endif

#ifndef PLATFORM_SUPPORTS_LLM
    #define PLATFORM_SUPPORTS_LLM 1
#endif

#define ENABLE_LOW_LEVEL_MEM_TRACKER (LLM_ENABLED_IN_CONFIG && PLATFORM_SUPPORTS_LLM)

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER
    enum class LLMTag : uint32
    {
        Untagged,
        Paused,
        Total,
        Untracked,
        TrackedTotal,

        // エンジンコア
        EngineMisc,
        Malloc,

        // グラフィックス
        Textures,
        RenderTargets,
        Shaders,
        Meshes,

        // オーディオ
        Audio,

        // 物理
        Physics,

        // UI
        UI,

        // カスタム開始点
        CustomTagStart = 1000,

        MaxTagCount = 2000
    };
#endif
}
```

## 検証

- ENABLE_LOW_LEVEL_MEM_TRACKER がビルド設定に応じて切り替わる
- タグ列挙型が正しく定義される
- Releaseビルドで `LLMTag` が定義されない

## 注意事項

- LLMはDebug/Developmentビルドでのみ有効
- `CustomTagStart` 以降はゲーム固有タグに使用可能
- タグは `uint32` サイズ（最大4億種類）

## 次のサブ計画

→ 07-02: LLMスコープマクロ
