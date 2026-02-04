# Unreal Engine 5.7 プラットフォーム抽象化レイヤー (HAL) 完全解説

## 第7部：Low Level Memory Tracker (LLM) 完全ガイド

---

## 目次

1. [LLM概要](#1-llm概要)
2. [有効化と構成](#2-有効化と構成)
3. [タグシステム](#3-タグシステム)
4. [タグセット](#4-タグセット)
5. [スコープマクロ](#5-スコープマクロ)
6. [カスタムタグ定義](#6-カスタムタグ定義)
7. [ランタイム制御](#7-ランタイム制御)
8. [統計システム統合](#8-統計システム統合)
9. [パフォーマンス考慮事項](#9-パフォーマンス考慮事項)
10. [LLM実装詳細](#10-llm-実装詳細)

---

## 1. LLM概要

### Low Level Memory Tracker (LLM) とは

LLMは、Unreal Engineのメモリ割り当てを詳細に追跡するための低レベルメモリトラッキングシステムです。すべてのメモリ割り当てをタグ付けして分類し、メモリ使用量の分析とデバッグを可能にします。

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    LLM アーキテクチャ概要                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【アプリケーション層】                                                  │
│  ┌───────────────────────────────────────────────────────────────────┐ │
│  │  LLM_SCOPE(Tag) / LLM_SCOPE_BYTAG(Tag)                            │ │
│  │  LLM_PLATFORM_SCOPE(Tag)                                          │ │
│  │  LLM_TAGSET_SCOPE(Tag, TagSet)                                    │ │
│  └───────────────────────────────────────────────────────────────────┘ │
│                              │                                          │
│                              ▼                                          │
│  【トラッカー層】                                                        │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  ┌─────────────────────┐  ┌─────────────────────┐              │   │
│  │  │   Default Tracker   │  │  Platform Tracker   │              │   │
│  │  │   (Malloc経由)     │  │  (OS直接割り当て)   │              │   │
│  │  └─────────────────────┘  └─────────────────────┘              │   │
│  │           │                        │                            │   │
│  │           └────────────┬───────────┘                            │   │
│  │                        ▼                                        │   │
│  │  ┌─────────────────────────────────────────────────────────┐   │   │
│  │  │            LowLevelMemTracker (シングルトン)            │   │   │
│  │  │   ├─ TagDatas[] - タグデータ配列                         │   │   │
│  │  │   ├─ Trackers[] - トラッカー配列                         │   │   │
│  │  │   └─ Allocator  - LLM内部アロケータ                      │   │   │
│  │  └─────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                              │                                          │
│                              ▼                                          │
│  【出力層】                                                              │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  LLMCsvWriter      - CSV出力                                   │   │
│  │  LLMTraceWriter    - Unreal Insights トレース                  │   │
│  │  Stats System       - 統計ビューアー表示                        │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 主要クラス

| クラス | 説明 |
|--------|------|
| `LowLevelMemTracker` | LLMのシングルトンメインクラス |
| `LLMScope` | スタック上のスコープタグ管理 |
| `LLMPauseScope` | トラッキング一時停止スコープ |
| `LLMTagDeclaration` | カスタムタグの宣言 |
| `LLMAllocator` | LLM内部用メモリアロケータ |

---

## 2. 有効化と構成

### コンパイル時フラグ

```cpp
// LowLevelMemTrackerDefines.h

// LLM有効化の主要フラグ
#ifndef LLM_ENABLED_IN_CONFIG
    #define LLM_ENABLED_IN_CONFIG ( \
        !UE_BUILD_SHIPPING && \
        (!UE_BUILD_TEST || ALLOW_LOW_LEVEL_MEM_TRACKER_IN_TEST) && \
        WITH_ENGINE \
    )
#endif

// プラットフォームサポート
#ifndef PLATFORM_SUPPORTS_LLM
    #define PLATFORM_SUPPORTS_LLM 1
#endif

// 最終的な有効化フラグ
#define ENABLE_LOW_LEVEL_MEM_TRACKER (LLM_ENABLED_IN_CONFIG && PLATFORM_SUPPORTS_LLM)
```

### ビルド構成別の動作

| ビルド構成 | LLM状態 | 説明 |
|-----------|---------|------|
| Development | 有効 | フル機能利用可能 |
| Debug | 有効 | フル機能利用可能 |
| Test | 条件付き | `ALLOW_LOW_LEVEL_MEM_TRACKER_IN_TEST`による |
| Shipping | 無効 | 常に無効化 |

### 機能フラグ

```cpp
// アセットタグ追跡（高コスト）
#ifndef LLM_ALLOW_ASSETS_TAGS
    #define LLM_ALLOW_ASSETS_TAGS 0
#endif

// UObjectクラスタグ（アセットタグと排他）
#ifndef LLM_ALLOW_UOBJECTCLASSES_TAGS
    #define LLM_ALLOW_UOBJECTCLASSES_TAGS !LLM_ALLOW_ASSETS_TAGS
#endif

// 統計タグ
#ifndef LLM_ALLOW_STATS
    #define LLM_ALLOW_STATS 0
#endif

// 統計タグの有効化条件
#define LLM_ENABLED_STAT_TAGS (LLM_ALLOW_STATS || LLM_ALLOW_ASSETS_TAGS)
```

---

## 3. タグシステム

### ELLMTag 列挙型

```cpp
enum class ELLMTag : LLM_TAG_TYPE
{
    // 基本タグ
    Untagged,           // 未タグ付け割り当て
    Paused,             // 一時停止中
    Total,              // 合計
    Untracked,          // 追跡外
    TrackedTotal,       // 追跡合計

    // エンジンコアタグ
    EngineMisc,         // エンジン雑多
    TaskGraphTasksMisc, // タスクグラフ雑多
    Name,              // Name文字列

    // メモリシステムタグ
    Malloc,            // Malloc割り当て
    MallocUnused,      // Malloc未使用
    ThreadStack,        // スレッドスタック
    ProgramSize,        // プログラムサイズ
    BackupOOMMemoryPool,// OOMバックアッププール

    // グラフィックスタグ
    Textures,           // テクスチャ
    RenderTargets,      // レンダーターゲット
    SceneRender,        // シーンレンダリング
    Shaders,            // シェーダー
    PSO,                // パイプラインステートオブジェクト
    RHIMisc,            // RHI雑多

    // メッシュタグ
    Meshes,             // メッシュ
    StaticMesh,         // 静的メッシュ
    SkeletalMesh,       // スケルタルメッシュ
    InstancedMesh,      // インスタンスメッシュ
    Landscape,          // ランドスケープ

    // オーディオタグ
    Audio,              // オーディオ
    AudioMisc,          // オーディオ雑多
    AudioSoundWaves,    // サウンドウェーブ
    AudioMixer,         // オーディオミキサー

    // 物理タグ
    Physics,            // 物理
    PhysX,              // PhysX
    Chaos,              // Chaos Physics

    // アニメーションタグ
    Animation,          // アニメーション

    // UIタグ
    UI,                 // UI

    // ネットワークタグ
    Networking,         // ネットワーキング
    Replays,            // リプレイ

    // その他
    UObject,            // UObject
    Materials,          // マテリアル
    Particles,          // パーティクル
    Niagara,            // Niagara
    AsyncLoading,       // 非同期ロード

    GenericTagCount,    // 汎用タグ数

    // プラットフォームタグ範囲
    PlatformTagStart = 111,
    PlatformTagEnd = 149,

    // プロジェクトタグ範囲
    ProjectTagStart = 150,
    ProjectTagEnd = 255,
};
```

### タグ範囲

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      LLM タグ範囲 (0-255)                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  0────────────────110  111─────────149  150─────────255                 │
│  │ 汎用エンジンタグ │  │プラットフォーム│  │プロジェクトタグ │           │
│  │ (GenericTags)   │  │   タグ        │  │               │           │
│  └─────────────────┘  └───────────────┘  └───────────────┘           │
│                                                                         │
│  constexpr uint32 LLM_TAG_COUNT = 256;                                 │
│  constexpr uint32 LLM_CUSTOM_TAG_START = 111;                          │
│  constexpr uint32 LLM_CUSTOM_TAG_END = 255;                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. タグセット

### ELLMTagSet 列挙型

```cpp
enum class ELLMTagSet : uint8
{
    None,           // 標準タグセット
    Assets,         // アセット別追跡
    AssetClasses,   // アセットクラス別追跡
    UObjectClasses, // UObjectクラス別追跡
    Max,
};
```

### タグセット詳細

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    タグセット機能比較                                    │
├────────────────┬────────────┬───────────────────────────────────────────┤
│ タグセット     │ 有効化条件  │ 用途                                     │
├────────────────┼────────────┼───────────────────────────────────────────┤
│ None           │ 常に有効    │ 標準のカテゴリ別メモリ追跡              │
│ Assets         │ -llmtagsets │ 個々のアセット別メモリ追跡              │
│                │ =assets     │ （高オーバーヘッド）                     │
│ AssetClasses   │ 自動        │ アセットクラス別追跡                    │
│ UObjectClasses │ 自動        │ UObjectサブクラス別追跡                 │
└────────────────┴────────────┴───────────────────────────────────────────┘
```

### ランタイム有効化

```bash
# コマンドライン引数でタグセット有効化
-llmtagsets=assets           # アセット追跡有効化
-llmtagsets=assets,stats     # 複数タグセット有効化
```

---

## 5. スコープマクロ

### 基本スコープマクロ

```cpp
// 基本的なタグスコープ
LLM_SCOPE(ELLMTag::Audio)

// 名前によるタグスコープ
LLM_SCOPE_BYNAME(TEXT("MyCategory"))

// タグ宣言によるスコープ
LLM_SCOPE_BYTAG(MyCustomTag)

// タグセット付きスコープ
LLM_TAGSET_SCOPE(ELLMTag::Audio, ELLMTagSet::Assets)

// プラットフォームトラッカー用スコープ
LLM_PLATFORM_SCOPE(ELLMTag::ThreadStack)
LLM_PLATFORM_SCOPE_BYNAME(TEXT("PlatformCategory"))
LLM_PLATFORM_SCOPE_BYTAG(MyPlatformTag)

// スコープクリア
LLM_SCOPE_CLEAR()
LLM_TAGSET_SCOPE_CLEAR(ELLMTagSet::Assets)
```

### 動的スコープマクロ

```cpp
// 動的タグスコープ（ランタイムで名前決定）
LLM_SCOPE_DYNAMIC(UniqueName, Tracker, TagSet, Constructor)

// アセットパスによる動的スコープ
LLM_SCOPE_DYNAMIC_STAT_OBJECTPATH(Object, ELLMTagSet::Assets)
LLM_SCOPE_DYNAMIC_STAT_OBJECTPATH_FNAME(ObjectPath, ELLMTagSet::Assets)

// レンダーリソース用スコープ
LLM_SCOPE_RENDER_RESOURCE(TEXT("MyTexture"))
```

### 一時停止スコープマクロ

```cpp
// トラッキング一時停止
LLM_SCOPED_PAUSE_TRACKING(ELLMAllocType::Malloc)

// 特定トラッカーの一時停止
LLM_SCOPED_PAUSE_TRACKING_FOR_TRACKER(ELLMTracker::Default, ELLMAllocType::Malloc)

// タグと量を指定した一時停止
LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::Untagged, Amount, Tracker, AllocType)
```

### Realloc用スコープマクロ

```cpp
// 既存ポインタからタグを継承
LLM_REALLOC_SCOPE(Ptr)
LLM_REALLOC_PLATFORM_SCOPE(Ptr)
```

### 統計連携スコープマクロ

```cpp
// 統計タグ付きスコープ
LLM_SCOPED_TAG_WITH_STAT(Stat, ELLMTracker::Default)
LLM_SCOPED_TAG_WITH_STAT_IN_SET(Stat, ELLMTagSet::Assets, ELLMTracker::Default)
LLM_SCOPED_TAG_WITH_STAT_NAME(StatName, ELLMTracker::Default)

// 単一統計タグ（宣言と使用を同時に）
LLM_SCOPED_SINGLE_STAT_TAG(MyStat)
LLM_SCOPED_SINGLE_PLATFORM_STAT_TAG(MyPlatformStat)
```

---

## 6. カスタムタグ定義

### タグ宣言と定義

```cpp
// ヘッダーファイル (.h)
// タグの宣言
LLM_DECLARE_TAG(MyGame_Combat)
LLM_DECLARE_TAG_API(MyGame_Combat, MYGAME_API)  // モジュールAPI付き

// ソースファイル (.cpp)
// タグの定義
LLM_DEFINE_TAG(MyGame_Combat,
    TEXT("Combat"),           // 表示名
    NAME_None,                // 親タグ名（なしの場合）
    GET_STATFNAME(STAT_CombatLLM), // 統計名
    GET_STATFNAME(STAT_GameSummaryLLM) // サマリー統計名
)

// 静的タグ定義
LLM_DEFINE_STATIC_TAG(MyGame_AI,
    TEXT("AI"),
    NAME_None,
    NAME_None,
    NAME_None
)
```

### ブートストラップタグ

グローバルコンストラクタ内で使用可能なタグです。

```cpp
// ヘッダーファイル
LLM_DECLARE_BOOTSTRAP_TAG(EarlyInit)
LLM_DECLARE_BOOTSTRAP_TAG_API(EarlyInit, CORE_API)

// ソースファイル
LLM_DEFINE_BOOTSTRAP_TAG(EarlyInit,
    TEXT("EarlyInit"),
    NAME_None,
    NAME_None,
    NAME_None
)

// 使用
LLM_SCOPE_BY_BOOTSTRAP_TAG(EarlyInit)
```

### プラットフォーム/プロジェクトタグ登録

```cpp
// カスタム列挙型定義
enum class EMyProjectTag : int32
{
    Combat = (int32)ELLMTag::ProjectTagStart,
    AI,
    Inventory,
    // ...
};

// 登録（初期化時）
void RegisterProjectTags()
{
    LowLevelMemTracker::Get().RegisterProjectTag(
        (int32)EMyProjectTag::Combat,
        TEXT("Combat"),
        GET_STATFNAME(STAT_CombatLLM),
        GET_STATFNAME(STAT_GameSummaryLLM),
        -1  // 親タグなし
    );
}
```

---

## 7. ランタイム制御

### コマンドライン引数

```bash
# LLM有効化
-llm

# タグセット有効化
-llmtagsets=assets
-llmtagsets=assets,stats

# CSV出力
-llmcsv

# トレース出力
-llmtrace
```

### プログラムによる制御

```cpp
// LLM有効確認
if (LowLevelMemTracker::IsEnabled())
{
    // LLM操作
}

// 条件付き実行
LLM_IF_ENABLED(
    // LLMが有効な場合のみ実行されるコード
    DoSomethingExpensive();
)

// 有効状態チェック
bool bEnabled = LLM_IS_ENABLED();
```

### データ取得API

```cpp
LowLevelMemTracker& LLM = LowLevelMemTracker::Get();

// 合計追跡メモリ取得
uint64 TotalMemory = LLM.GetTotalTrackedMemory(ELLMTracker::Default);

// タグ別メモリ量取得
int64 TextureMemory = LLM.GetTagAmountForTracker(
    ELLMTracker::Default,
    ELLMTag::Textures
);

// 名前によるタグ検索
uint64 TagValue;
if (LLM.FindTagByName(TEXT("Textures"), TagValue))
{
    // タグが見つかった
}

// 追跡中のタグ一覧取得
TArray<const UE::LLMPrivate::TagData*> Tags = LLM.GetTrackedTags();
```

### メモリ変更通知

```cpp
// 割り当て通知
LLM.OnLowLevelAlloc(
    ELLMTracker::Default,
    Ptr,
    Size,
    ELLMTag::Textures,
    ELLMAllocType::Malloc
);

// 解放通知
LLM.OnLowLevelFree(
    ELLMTracker::Default,
    Ptr,
    ELLMAllocType::Malloc
);

// ポインタ移動通知（デフラグ用）
LLM.OnLowLevelAllocMoved(
    ELLMTracker::Default,
    DestPtr,
    SourcePtr
);

// 量の直接変更（ポインタなしの場合）
LLM.OnLowLevelChangeInMemoryUse(
    ELLMTracker::Default,
    DeltaMemory,
    ELLMTag::Textures
);
```

---

## 8. 統計システム統合

### 統計グループ

```cpp
// LLM関連の統計グループ
DECLARE_STATS_GROUP(TEXT("LLM FULL"), STATGROUP_LLMFULL, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLM Platform"), STATGROUP_LLMPlatform, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLM Summary"), STATGROUP_LLM, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLM Overhead"), STATGROUP_LLMOverhead, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLM Assets"), STATGROUP_LLMAssets, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("LLM UObjects"), STATGROUP_LLMUObjects, STATCAT_Advanced);
```

### サマリー統計

```cpp
// サマリー統計の定義例
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Engine"), STAT_EngineSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Textures"), STAT_TexturesSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Meshes"), STAT_MeshesSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Audio"), STAT_AudioSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Physics"), STAT_PhysicsSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("Animation"), STAT_AnimationSummaryLLM, STATGROUP_LLM, CORE_API);
DECLARE_LLM_MEMORY_STAT_EXTERN(TEXT("UI"), STAT_UISummaryLLM, STATGROUP_LLM, CORE_API);
```

### コンソールコマンド

```
// ゲーム内で使用可能なコマンド
stat llm              // LLMサマリー表示
stat llmfull          // LLM詳細表示
stat llmplatform      // プラットフォームLLM表示
stat llmassets        // アセット別LLM表示（-llmtagsets=assets必要）
stat llmuobjects      // UObject別LLM表示
```

---

## 9. パフォーマンス考慮事項

### オーバーヘッド

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    LLM オーバーヘッド                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【メモリオーバーヘッド】                                                │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  LLM_MEMORY_OVERHEAD = 600MB (推定、400万割り当て時)             │   │
│  │                                                                   │   │
│  │  内訳:                                                            │   │
│  │  ├─ 割り当てあたり: ~16-32バイト (タグ情報、サイズ等)            │   │
│  │  ├─ タグデータ: タグ数 × ~64バイト                               │   │
│  │  └─ スレッドローカル状態: スレッド数 × ~1KB                      │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【CPUオーバーヘッド】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  基本トラッキング: 割り当て/解放あたり ~100-500ns                 │   │
│  │  Assets TagSet:   割り当て/解放あたり ~1-10μs (高コスト)         │   │
│  │  統計公開:        フレームあたり ~1-5ms                           │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### ベストプラクティス

```cpp
// 良い例: 重要なシステムにのみタグを付ける
void LoadTexture(const String& Path)
{
    LLM_SCOPE(ELLMTag::Textures);  // 適切な粒度
    // テクスチャロード処理
}

// 悪い例: 細かすぎるタグ付け（オーバーヘッド増大）
void ProcessPixel(int32 X, int32 Y)
{
    LLM_SCOPE_BYNAME(String::Printf(TEXT("Pixel_%d_%d"), X, Y));  // NG!
    // ピクセル処理
}

// 良い例: 条件付き高コスト操作
#if LLM_ALLOW_ASSETS_TAGS
    if (LowLevelMemTracker::Get().IsTagSetActive(ELLMTagSet::Assets))
    {
        LLM_SCOPE_DYNAMIC_STAT_OBJECTPATH(Object, ELLMTagSet::Assets);
    }
#endif
```

### タグセット使用時の注意

```cpp
// Assets TagSetの有効化による影響
//
// 有効化前:
//   - 標準タグのみ追跡
//   - 低オーバーヘッド
//
// 有効化後 (-llmtagsets=assets):
//   - 全アセットに個別タグ作成
//   - 大量のStatID生成
//   - メモリ使用量大幅増加
//   - CPU負荷増大
//
// 推奨: 開発/プロファイリング時のみ使用
```

### トラッカーの選択

| トラッカー | 用途 | 例 |
|-----------|------|-----|
| `ELLMTracker::Default` | 通常のエンジン/ゲーム割り当て | テクスチャ、メッシュ、UObject |
| `ELLMTracker::Platform` | OS直接/低レベル割り当て | スレッドスタック、MMIO |

---

## 付録：LLMタグ完全リスト

### エンジンコアタグ

| タグ | 説明 | サマリーグループ |
|------|------|-----------------|
| `Untagged` | 未タグ付け | - |
| `Total` | 合計 | TrackedTotal |
| `TrackedTotal` | 追跡合計 | TrackedTotal |
| `Untracked` | 追跡外 | TrackedTotal |
| `EngineMisc` | エンジン雑多 | Engine |
| `Name` | Name | Engine |
| `Stats` | 統計 | Engine |
| `TaskGraphTasksMisc` | タスクグラフ | Engine |

### メモリシステムタグ

| タグ | 説明 | サマリーグループ |
|------|------|-----------------|
| `Malloc` | Malloc | - |
| `MallocUnused` | Malloc未使用 | Engine |
| `RHIUnused` | RHI未使用 | Engine |
| `ThreadStack` | スレッドスタック | Engine |
| `ProgramSize` | プログラムサイズ | Engine |
| `BackupOOMMemoryPool` | OOMバックアップ | Engine |
| `LinearAllocator` | 線形アロケータ | - |

### グラフィックスタグ

| タグ | 説明 | サマリーグループ |
|------|------|-----------------|
| `Textures` | テクスチャ | Textures |
| `TextureMetaData` | テクスチャメタデータ | Textures |
| `VirtualTextureSystem` | 仮想テクスチャ | Textures |
| `RenderTargets` | レンダーターゲット | Engine |
| `SceneRender` | シーンレンダー | Engine |
| `Shaders` | シェーダー | Engine |
| `PSO` | PSO | Engine |
| `RHIMisc` | RHI雑多 | Engine |

### メッシュ/ジオメトリタグ

| タグ | 説明 | サマリーグループ |
|------|------|-----------------|
| `Meshes` | メッシュ | Meshes |
| `StaticMesh` | 静的メッシュ | StaticMesh |
| `SkeletalMesh` | スケルタルメッシュ | Engine |
| `InstancedMesh` | インスタンスメッシュ | Engine |
| `Landscape` | ランドスケープ | Engine |

### オーディオタグ

| タグ | 説明 | 親タグ |
|------|------|--------|
| `Audio` | オーディオ | - |
| `AudioMisc` | オーディオ雑多 | Audio |
| `AudioSoundWaves` | サウンドウェーブ | Audio |
| `AudioSoundWaveProxies` | SWプロキシ | Audio |
| `AudioMixer` | ミキサー | Audio |
| `AudioMixerPlugins` | ミキサープラグイン | Audio |
| `AudioPrecache` | プリキャッシュ | Audio |
| `AudioDecompress` | デコンプレス | Audio |
| `AudioStreamCache` | ストリームキャッシュ | Audio |
| `AudioSynthesis` | シンセシス | Audio |

### 物理タグ

| タグ | 説明 | 親タグ |
|------|------|--------|
| `Physics` | 物理 | - |
| `PhysX` | PhysX | Physics |
| `PhysXGeometry` | PhysXジオメトリ | Physics |
| `PhysXTrimesh` | PhysXトライメッシュ | Physics |
| `Chaos` | Chaos | Physics |
| `ChaosGeometry` | Chaosジオメトリ | Physics |
| `ChaosParticles` | Chaosパーティクル | Physics |

---

## 10. LLM 実装詳細

### タグ定数

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         タグシステム定数                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  #define LLM_TAG_TYPE uint8                                            │
│   → タグの基底型（最大256タグ）                                         │
│                                                                         │
│  constexpr uint32 LLM_TAG_COUNT = 256;                                 │
│   → タグの最大数                                                        │
│                                                                         │
│  constexpr uint32 LLM_CUSTOM_TAG_START = 111;                          │
│   → PlatformTagStart                                                   │
│                                                                         │
│  constexpr uint32 LLM_CUSTOM_TAG_END = 255;                            │
│   → ProjectTagEnd                                                      │
│                                                                         │
│  constexpr uint32 LLM_CUSTOM_TAG_COUNT = 145;                          │
│   → カスタムタグ数 (255 - 111 + 1)                                     │
│                                                                         │
│  ELLMTag::PlatformTagStart = 111                                       │
│  ELLMTag::PlatformTagEnd = 149                                         │
│  ELLMTag::ProjectTagStart = 150                                        │
│  ELLMTag::ProjectTagEnd = 255                                          │
│                                                                         │
│  #define LLM_MEMORY_OVERHEAD (600LL*1024*1024)                         │
│   → LLMメモリオーバーヘッド推定 (400万割り当て時)                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### ELLMTracker enum

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          トラッカー種別                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  enum class ELLMTracker : uint8                                        │
│  {                                                                      │
│      Platform,  // OS/低レベル割り当て用                                │
│                 // スレッドスタック、MMIO等                             │
│                                                                         │
│      Default,   // 通常エンジン割り当て用                               │
│                 // Malloc経由の全割り当て                              │
│                                                                         │
│      Max,       // トラッカー数                                         │
│  };                                                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### ELLMTagSet enum

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          タグセット種別                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  enum class ELLMTagSet : uint8                                         │
│  {                                                                      │
│      None,           // 標準タグセット（常に有効）                      │
│                                                                         │
│      Assets,         // アセット別追跡                                  │
│                      // -llmtagsets=assets で有効化                     │
│                      // 高コスト: 全アセットに個別タグ                  │
│                                                                         │
│      AssetClasses,   // アセットクラス別追跡                            │
│                                                                         │
│      UObjectClasses, // UObjectクラス別追跡                             │
│                      // LLM_ALLOW_UOBJECTCLASSES_TAGS で制御            │
│                      // Assets と排他（メモリ節約）                     │
│                                                                         │
│      Max,            // タグセット数                                    │
│  };                                                                     │
│                                                                         │
│  注: ShouldReduceThreads() と IsAssetTagForAssets() 参照               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### ELLMAllocType enum

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          割り当て種別                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  enum class ELLMAllocType                                              │
│  {                                                                      │
│      None = 0,   // 未指定                                              │
│      Malloc,    // GMalloc経由の通常割り当て                          │
│      System,     // OS直接割り当て                                      │
│      RHI,        // RHI割り当て                                         │
│      Count                                                             │
│  };                                                                     │
│                                                                         │
│  用途: OnLowLevelAlloc/Free での割り当て種別指定                       │
│        Malloc 合計と未使用の計算に使用                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### UE::LLM::ESizeParams enum

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       サイズ取得パラメータ                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  namespace UE::LLM                                                     │
│  {                                                                      │
│      enum class ESizeParams : uint8                                    │
│      {                                                                  │
│          Default = 0,          // デフォルト（現在値）                  │
│          ReportCurrent = 0,    // 現在値をレポート                      │
│          ReportPeak = 1,       // ピーク値をレポート                    │
│          RelativeToSnapshot = 2// スナップショットからの相対値         │
│      };                                                                 │
│                                                                         │
│      ENUM_CLASS_FLAGS(ESizeParams);  // フラグとして組み合わせ可能     │
│  }                                                                      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### LLMAllocator クラス

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      LLM内部アロケータ                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  class LLMAllocator (UE::LLMPrivate)                                  │
│  {                                                                      │
│      // プラットフォーム関数で OS から直接メモリ取得                    │
│      LLMAllocFunction PlatformAlloc;                                   │
│      LLMFreeFunction PlatformFree;                                     │
│                                                                         │
│      // ビンアロケーション                                              │
│      AllocatorPrivate::Bin* Bins;                                     │
│      int64 Total;           // 合計割り当てサイズ                       │
│      int32 PageSize;        // ページサイズ                             │
│      int32 NumBins;         // ビン数                                   │
│                                                                         │
│      // API                                                             │
│      void Initialise(LLMAllocFunction, LLMFreeFunction, int32 PageSize);│
│      void* Alloc(size_t Size);                                         │
│      void* Malloc(size_t Size);                                        │
│      void Free(void* Ptr, size_t Size);                                │
│      void* Realloc(void* Ptr, size_t OldSize, size_t NewSize);         │
│      int64 GetTotal() const;                                           │
│                                                                         │
│      // テンプレートヘルパー                                            │
│      template<T> T* New(Args&&...);                                    │
│      template<T> void Delete(T* Ptr);                                  │
│  };                                                                     │
│                                                                         │
│  typedef void*(*LLMAllocFunction)(size_t);                             │
│  typedef void(*LLMFreeFunction)(void*, size_t);                        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### LLMTagInfo 構造体

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        タグ情報構造体                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  struct LLMTagInfo                                                    │
│  {                                                                      │
│      const TCHAR* Name;       // タグ名                                │
│      Name StatName;          // LLMFULL 統計グループに表示            │
│      Name SummaryStatName;   // LLM サマリーグループに追加            │
│      int32 ParentTag = -1;    // 親タグ（-1 = なし）                   │
│  };                                                                     │
│                                                                         │
│  用途: RegisterProjectTag() に渡す情報を整理                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### LLMScope クラス詳細

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       スコープクラス詳細                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【LLMScope - 基本スコープ】                                            │
│  class LLMScope                                                       │
│  {                                                                      │
│      // コンストラクタ（3種）                                           │
│      explicit LLMScope(Name TagName, bool bIsStatTag,                │
│          ELLMTagSet InTagSet, ELLMTracker InTracker, bool bOverride);  │
│      explicit LLMScope(ELLMTag TagEnum, bool bIsStatTag,              │
│          ELLMTagSet InTagSet, ELLMTracker InTracker, bool bOverride);  │
│      explicit LLMScope(const TagData* TagData, bool bIsStatTag,      │
│          ELLMTagSet InTagSet, ELLMTracker InTracker, bool bOverride);  │
│                                                                         │
│      // メンバ                                                          │
│      ELLMTracker Tracker;                                              │
│      bool bEnabled = false;                                            │
│      ELLMTagSet TagSet;                                                │
│  };                                                                     │
│                                                                         │
│  【LLMPauseScope - 一時停止スコープ】                                   │
│   ・トラッキングを一時停止                                              │
│   ・特定の AllocType のみ一時停止可能                                   │
│                                                                         │
│  【LLMScopeDynamic - 動的タグスコープ】                                 │
│   ・実行時にタグを動的作成                                              │
│   ・ILLMDynamicTagConstructor で統計ID作成                             │
│   ・TryFindTag() / TryAddTagAndActivate() / Activate()                 │
│                                                                         │
│  【LLMScopeFromPtr - ポインタ継承スコープ】                             │
│   ・既存ポインタのタグを継承                                            │
│   ・Realloc時に使用                                                     │
│                                                                         │
│  【LLMClearScope - クリアスコープ】                                     │
│   ・現在のタグをクリア                                                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### LLM チェック/Ensure マクロ

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     LLM 内部チェックマクロ                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  #if DO_CHECK                                                          │
│                                                                         │
│  LLMCheck(expr)                                                        │
│   → 失敗時に例外発生 (PlatformMisc::RaiseException(1))                │
│                                                                         │
│  LLMCheckf(expr, format, ...)                                          │
│   → フォーマット付きチェック                                           │
│                                                                         │
│  LLMEnsure(expr)                                                       │
│   → 最初の失敗時のみログ出力（継続）                                   │
│   → LLMTrueOnFirstCallOnly で1回限り                                   │
│                                                                         │
│  #else                                                                 │
│   → 全て空マクロ                                                        │
│                                                                         │
│  #endif                                                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### ETagReferenceSource enum

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       タグ参照元種別                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  enum class ETagReferenceSource (UE::LLMPrivate)                       │
│  {                                                                      │
│      Scope,          // LLM_SCOPE マクロから                           │
│      Declare,        // LLM_DECLARE_TAG から                           │
│      EnumTag,        // ELLMTag 列挙値から                             │
│      CustomEnumTag,  // カスタム列挙タグから                           │
│      FunctionAPI,    // API関数呼び出しから                            │
│      ImplicitParent, // 暗黙の親タグとして                             │
│  };                                                                     │
│                                                                         │
│  用途: 重複タグ名検出時のエラーレポート                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### LowLevelMemTracker 追加API

```
┌─────────────────────────────────────────────────────────────────────────┐
│                   LowLevelMemTracker 追加API                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【初期化状態】                                                          │
│   bool IsInitialized() const;    // 完全初期化済みか                   │
│   bool IsConfigured() const;     // 設定完了か                         │
│   void BootstrapInitialise();    // ブートストラップ初期化             │
│   void FinishInitialise();       // Name利用可能後の完了初期化        │
│                                                                         │
│  【フォーク対応】                                                        │
│   void OnPreFork();              // fork()前のコールバック             │
│                                                                         │
│  【ダンプ】                                                              │
│   enum class EDumpFormat { PlainText, CSV };                           │
│   void DumpToLog(EDumpFormat, OutputDevice*, ESizeParams, ELLMTagSet);│
│   void PublishDataSingleFrame(); // 単一フレームデータ公開             │
│                                                                         │
│  【タグ情報取得】                                                        │
│   Name GetTagDisplayName(const TagData*) const;                      │
│   String GetTagDisplayPathName(const TagData*) const;                │
│   Name GetTagUniqueName(const TagData*) const;                       │
│   const TagData* GetTagParent(const TagData*) const;                 │
│   bool GetTagIsEnumTag(const TagData*) const;                         │
│   ELLMTag GetTagClosestEnumTag(const TagData*) const;                 │
│                                                                         │
│  【内部状態変数】                                                        │
│   static EEnabled EnabledState;  // NotYetKnown/Disabled/Enabled       │
│   uint64 ProgramSize;                                                  │
│   int64 MemoryUsageCurrentOverhead;                                    │
│   int64 MemoryUsagePlatformTotalUntracked;                             │
│   bool ActiveSets[(int32)ELLMTagSet::Max];                             │
│   bool bCsvWriterEnabled;                                              │
│   bool bTraceWriterEnabled;                                            │
│   bool bAutoPublish;                                                   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### サマリー統計完全リスト

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    LLMサマリー統計 (STATGROUP_LLM)                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  STAT_EngineSummaryLLM          "Engine"                               │
│  STAT_ProjectSummaryLLM         "Project"                              │
│  STAT_NetworkingSummaryLLM      "Networking"                           │
│  STAT_AudioSummaryLLM           "Audio"                                │
│  STAT_TrackedTotalSummaryLLM    "Total"                                │
│  STAT_MeshesSummaryLLM          "Meshes"                               │
│  STAT_PhysicsSummaryLLM         "Physics"                              │
│  STAT_PhysXSummaryLLM           "PhysX"                                │
│  STAT_ChaosSummaryLLM           "Chaos"                                │
│  STAT_UObjectSummaryLLM         "UObject"                              │
│  STAT_AnimationSummaryLLM       "Animation"                            │
│  STAT_StaticMeshSummaryLLM      "StaticMesh"                           │
│  STAT_MaterialsSummaryLLM       "Materials"                            │
│  STAT_ParticlesSummaryLLM       "Particles"                            │
│  STAT_NiagaraSummaryLLM         "Niagara"                              │
│  STAT_UISummaryLLM              "UI"                                   │
│  STAT_NavigationSummaryLLM      "Navigation"                           │
│  STAT_TexturesSummaryLLM        "Textures"                             │
│  STAT_MediaStreamingSummaryLLM  "MediaStreaming"                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 関連ドキュメント

- [第3部：メモリ管理](Platform_Abstraction_Layer_Part3.md)
- [第6部：メモリアロケータ詳細](Platform_Abstraction_Layer_Part6.md)
- [第8部：デバッグ・診断ツール](Platform_Abstraction_Layer_Part8.md)
