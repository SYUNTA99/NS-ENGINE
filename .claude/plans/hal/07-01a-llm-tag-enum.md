# 07-01a: LLMタグ列挙型

## 目的

Low Level Memory Trackerのタグ列挙型と有効化マクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション3「タグシステム」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`）

## 依存（HAL）

（HAL固有の依存なし - ビルドマクロのみ）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`

## TODO

- [ ] `LLM_ENABLED_IN_CONFIG` マクロ定義
- [ ] `PLATFORM_SUPPORTS_LLM` マクロ定義
- [ ] `ENABLE_LOW_LEVEL_MEM_TRACKER` マクロ定義
- [ ] `LLMTag` 列挙型定義

## 実装内容

```cpp
// LowLevelMemTracker.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

// ========== ビルド設定マクロ ==========

// LLM有効化フラグ（ビルド設定から上書き可能）
// premake: defines { "LLM_ENABLED_IN_CONFIG=1" }
#ifndef LLM_ENABLED_IN_CONFIG
    #if defined(NS_DEBUG) || defined(NS_DEVELOPMENT)
        #define LLM_ENABLED_IN_CONFIG 1
    #else
        #define LLM_ENABLED_IN_CONFIG 0
    #endif
#endif

// プラットフォームサポートフラグ
// 一部プラットフォームでは無効化（メモリ制約など）
#ifndef PLATFORM_SUPPORTS_LLM
    #define PLATFORM_SUPPORTS_LLM 1
#endif

// 最終的な有効化フラグ
#define ENABLE_LOW_LEVEL_MEM_TRACKER (LLM_ENABLED_IN_CONFIG && PLATFORM_SUPPORTS_LLM)

// ========== LLMスコープマクロ（条件コンパイル） ==========

#if ENABLE_LOW_LEVEL_MEM_TRACKER
    #define LLM_SCOPE(tag) NS::LLMScope NS_CONCAT(_llmScope_, __LINE__)(NS::LLMTag::tag)
    #define LLM_SCOPE_CUSTOM(tag) NS::LLMScope NS_CONCAT(_llmScope_, __LINE__)(tag)
    #define LLM_TAGSET_SCOPE(tag, tagSet) \
        NS::LLMTagSetScope NS_CONCAT(_llmTagSetScope_, __LINE__)( \
            NS::LLMTag::tag, NS::LLMTagSet::tagSet)
#else
    #define LLM_SCOPE(tag) do {} while(0)
    #define LLM_SCOPE_CUSTOM(tag) do {} while(0)
    #define LLM_TAGSET_SCOPE(tag, tagSet) do {} while(0)
#endif

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    // ========== 定数 ==========

    /// 汎用タグ終端（0〜110: エンジン汎用タグ）
    constexpr uint32 kLLMGenericTagEnd = 110;

    /// プラットフォームタグ開始（111〜149: プラットフォーム固有タグ）
    constexpr uint32 kLLMPlatformTagStart = 111;

    /// プラットフォームタグ終端
    constexpr uint32 kLLMPlatformTagEnd = 149;

    /// プロジェクトタグ開始（150〜255: プロジェクト固有タグ）
    constexpr uint32 kLLMProjectTagStart = 150;

    /// プロジェクトタグ終端
    constexpr uint32 kLLMProjectTagEnd = 255;

    /// 総タグ最大数（uint8互換: 0〜255）
    constexpr uint32 kLLMMaxTagCount = 256;

    // ========== タグ列挙型 ==========

    /// Low Level Memory Tracker タグ
    ///
    /// メモリ割り当てを分類するためのタグ。
    /// プロファイリングやメモリリーク検出に使用。
    ///
    /// ## タグID範囲（uint8: 0〜255）
    /// - 0〜110: エンジン汎用タグ
    /// - 111〜149: プラットフォーム固有タグ
    /// - 150〜255: プロジェクト/ゲーム固有タグ
    enum class LLMTag : uint8
    {
        // ========== システムタグ (0-9) ==========
        Untagged = 0,       ///< 未分類（デフォルト）
        Paused,             ///< 一時停止中（追跡停止）
        Total,              ///< 全メモリ合計
        Untracked,          ///< 追跡対象外
        TrackedTotal,       ///< 追跡対象合計

        // ========== エンジンコア (10-19) ==========
        EngineMisc = 10,    ///< エンジンその他
        Malloc,             ///< アロケータオーバーヘッド
        Containers,         ///< TArray, TMap等

        // ========== グラフィックス (20-39) ==========
        Textures = 20,      ///< テクスチャ
        RenderTargets,      ///< レンダーターゲット
        Shaders,            ///< シェーダー
        Meshes,             ///< メッシュ/頂点バッファ
        Particles,          ///< パーティクル
        RHIMisc,            ///< RHIその他

        // ========== オーディオ (40-49) ==========
        Audio = 40,         ///< オーディオ全般

        // ========== 物理 (50-59) ==========
        Physics = 50,       ///< 物理シミュレーション

        // ========== UI (60-69) ==========
        UI = 60,            ///< ユーザーインターフェース

        // ========== ネットワーク (70-79) ==========
        Networking = 70,    ///< ネットワーク

        // ========== ゲームプレイ (80-99) ==========
        Animation = 80,     ///< アニメーション
        AI,                 ///< 人工知能
        Scripting,          ///< スクリプト
        World,              ///< ワールド/レベル
        Actors,             ///< アクター/エンティティ

        // ========== 境界マーカー ==========
        GenericTagEnd = kLLMGenericTagEnd,          ///< 汎用タグ終端
        PlatformTagStart = kLLMPlatformTagStart,    ///< プラットフォームタグ開始
        PlatformTagEnd = kLLMPlatformTagEnd,        ///< プラットフォームタグ終端
        ProjectTagStart = kLLMProjectTagStart,      ///< プロジェクトタグ開始
        ProjectTagEnd = kLLMProjectTagEnd,          ///< プロジェクトタグ終端
        MaxTagCount = kLLMMaxTagCount - 1           ///< 最大タグID（255）
    };

    // ========== タグセット ==========

    /// タグセット（追加の分類軸）
    ///
    /// アセット別追跡など、タグとは別の分類軸を提供。
    /// タグと組み合わせて詳細なメモリプロファイリングを実現。
    enum class LLMTagSet : uint8
    {
        None = 0,           ///< 標準タグセット
        Assets,             ///< アセット別追跡
        AssetClasses,       ///< アセットクラス別追跡
        Max
    };

    // ========== トラッカー種別 ==========

    /// LLMトラッカー種別
    ///
    /// LLMは2系統のトラッカーを使用:
    /// - Default: GMalloc経由の通常エンジン割り当て
    /// - Platform: OS直接/低レベル割り当て（スレッドスタック等）
    enum class LLMTracker : uint8
    {
        Platform = 0,   ///< OS/低レベル割り当て用
        Default,        ///< 通常エンジン割り当て用（Malloc経由）
        Max
    };

    // ========== 割り当て種別 ==========

    /// LLM割り当て種別
    ///
    /// OnLowLevelAlloc/Free で割り当て元を識別。
    /// Malloc合計と未使用の計算に使用。
    enum class LLMAllocType : uint8
    {
        None = 0,       ///< 未指定
        Malloc,         ///< GMalloc経由の通常割り当て
        System,         ///< OS直接割り当て
        RHI,            ///< RHI割り当て
        Count
    };

    // ========== カスタムタグ管理 ==========

    /// カスタムタグ情報
    struct LLMCustomTagInfo
    {
        const TCHAR* name;          ///< 表示名（静的文字列）
        const TCHAR* statGroup;     ///< 統計グループ（nullptr = デフォルト）
        LLMTag parentTag;           ///< 親タグ（階層表示用、Untagged = ルート）
    };

    /// プロジェクトタグ最大数
    constexpr uint32 kLLMMaxProjectTags = kLLMProjectTagEnd - kLLMProjectTagStart + 1;

    /// カスタムタグ登録
    ///
    /// @param name タグ名（静的文字列、コピーされない）
    /// @param statGroup 統計グループ名（nullptr可）
    /// @param parentTag 親タグ（階層化用）
    /// @return 登録されたタグID、失敗時はLLMTag::Untagged
    ///
    /// @warning 登録フェーズでのみ呼び出し可能（FinalizeTagRegistration後はアサート）
    /// @note 最大kLLMMaxCustomTags個まで登録可能。
    ///
    /// ## 使用例
    /// ```cpp
    /// // 起動時に登録
    /// static LLMTag s_gameplayTag = RegisterLLMCustomTag(
    ///     TEXT("Gameplay"),
    ///     TEXT("Game"),
    ///     LLMTag::EngineMisc
    /// );
    ///
    /// // 使用時
    /// LLM_SCOPE_CUSTOM(s_gameplayTag);
    /// ```
    LLMTag RegisterLLMCustomTag(
        const TCHAR* name,
        const TCHAR* statGroup = nullptr,
        LLMTag parentTag = LLMTag::Untagged);

    // ========== 登録フェーズ管理 ==========

    /// タグ登録を終了
    ///
    /// 起動処理完了後に呼び出す。
    /// 以降のRegisterLLMCustomTag呼び出しはアサート失敗。
    ///
    /// @note メインスレッドから一度だけ呼び出す
    void FinalizeTagRegistration();

    /// 登録フェーズかどうか
    ///
    /// @return 登録可能ならtrue
    bool IsInRegistrationPhase();

    // ========== タグ情報取得 ==========

    /// タグ名取得
    ///
    /// @param tag タグID
    /// @return タグ名（静的文字列）、未登録タグは"Unknown"
    /// @note スレッドセーフ（登録フェーズ終了後）
    const TCHAR* GetLLMTagName(LLMTag tag);

    /// タグが有効かどうか
    ///
    /// @param tag タグID
    /// @return 有効なタグならtrue
    /// @note スレッドセーフ
    bool IsValidLLMTag(LLMTag tag);

    /// 登録済みカスタムタグ数取得
    /// @note スレッドセーフ
    uint32 GetLLMCustomTagCount();

    // ========== 前方宣言 ==========

    class LLMScope;         // 07-02aで定義
    class LLMTagSetScope;   // 07-02aで定義

    // ========== 内部実装 ==========

    namespace Private
    {
        /// 登録フェーズフラグ
        extern bool g_llmRegistrationPhase;

        /// 登録済みプロジェクトタグ数
        extern uint32 g_llmProjectTagCount;

        /// プロジェクトタグ情報配列
        extern LLMCustomTagInfo g_llmProjectTags[kLLMMaxProjectTags];
    }

#else // !ENABLE_LOW_LEVEL_MEM_TRACKER

    // Releaseビルド用スタブ（型一致: 全てuint8）
    enum class LLMTag : uint8 { Untagged = 0 };
    enum class LLMTagSet : uint8 { None = 0 };
    enum class LLMTracker : uint8 { Platform = 0, Default, Max };
    enum class LLMAllocType : uint8 { None = 0 };

    inline LLMTag RegisterLLMCustomTag(const TCHAR*, const TCHAR* = nullptr, LLMTag = LLMTag::Untagged)
    {
        return LLMTag::Untagged;
    }
    inline void FinalizeTagRegistration() {}
    inline bool IsInRegistrationPhase() { return false; }
    inline const TCHAR* GetLLMTagName(LLMTag) { return TEXT("Disabled"); }
    inline bool IsValidLLMTag(LLMTag) { return false; }
    inline uint32 GetLLMCustomTagCount() { return 0; }

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
}
```

## 検証

- Debug/Developmentビルドで `ENABLE_LOW_LEVEL_MEM_TRACKER` が 1
- Releaseビルドで `ENABLE_LOW_LEVEL_MEM_TRACKER` が 0
- `LLMTag` がすべてのビルドで定義される（Releaseはスタブ）
- `LLMTag` が `uint8` ベースで256タグに収まる
- `RegisterLLMCustomTag` が150〜255の範囲でIDを返す
- 最大登録数を超えると `LLMTag::Untagged` を返す
- `LLMTagSet` でアセット別追跡が可能

## 注意事項

- **カスタムタグ登録は起動時のみ**（スレッドセーフではない）
- タグ名は静的文字列を渡すこと（コピーされない）
- Releaseビルドでもコンパイルエラーにならないようスタブ提供
- 親タグを指定すると統計表示で階層化される

## カスタムタグ登録例

```cpp
// GameMemoryTags.cpp
#include "HAL/LowLevelMemTracker.h"

namespace Game
{
    // グローバル変数として起動時に登録
    LLMTag g_tagInventory = NS::RegisterLLMCustomTag(
        TEXT("Inventory"), TEXT("Game"), NS::LLMTag::EngineMisc);

    LLMTag g_tagQuest = NS::RegisterLLMCustomTag(
        TEXT("Quest"), TEXT("Game"), NS::LLMTag::EngineMisc);

    LLMTag g_tagDialog = NS::RegisterLLMCustomTag(
        TEXT("Dialog"), TEXT("Game"), NS::LLMTag::UI);
}

// 使用時
void LoadInventory()
{
    LLM_SCOPE_CUSTOM(Game::g_tagInventory);
    // メモリ割り当てがInventoryタグで追跡される
}
```

## 次のサブ計画

→ 07-01b: LowLevelMemTrackerクラス

## 次のサブ計画

→ 07-01b: LowLevelMemTrackerクラス
