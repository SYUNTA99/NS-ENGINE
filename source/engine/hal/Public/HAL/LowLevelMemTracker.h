/// @file LowLevelMemTracker.h
/// @brief Low Level Memory Tracker
#pragma once

#include "HAL/PlatformTypes.h"
#include "HAL/PreprocessorHelpers.h"
#include "common/utility/macros.h"
#include "common/utility/types.h"

// =============================================================================
// ビルド設定マクロ
// =============================================================================

/// LLM有効化フラグ（ビルド設定から上書き可能）
#ifndef LLM_ENABLED_IN_CONFIG
#if NS_BUILD_DEBUG
#define LLM_ENABLED_IN_CONFIG 1
#else
#define LLM_ENABLED_IN_CONFIG 0
#endif
#endif

/// プラットフォームサポートフラグ
#ifndef PLATFORM_SUPPORTS_LLM
#define PLATFORM_SUPPORTS_LLM 1
#endif

/// 最終的な有効化フラグ
#define ENABLE_LOW_LEVEL_MEM_TRACKER (LLM_ENABLED_IN_CONFIG && PLATFORM_SUPPORTS_LLM)

// =============================================================================
// LLMスコープマクロ
// =============================================================================

#if ENABLE_LOW_LEVEL_MEM_TRACKER

/// タグ名でスコープを開始
/// @code
/// LLM_SCOPE(Textures);
/// // このスコープ内のメモリ割り当てはTexturesタグで追跡
/// @endcode
#define LLM_SCOPE(Tag) NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(NS::LLMTag::Tag)

/// タグ変数でスコープを開始
/// @code
/// LLMTag myTag = ...;
/// LLM_SCOPE_BYTAG(myTag);
/// @endcode
#define LLM_SCOPE_BYTAG(Tag) NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(Tag)

/// カスタムタグでスコープを開始（07-01aとの互換性）
#define LLM_SCOPE_CUSTOM(Tag) LLM_SCOPE_BYTAG(Tag)

/// トラッキング一時停止スコープ
#define LLM_PAUSE_SCOPE() NS::LLMPauseScope NS_CONCAT(llmPauseScope_, __LINE__)

/// タグセット付きスコープ
#define LLM_TAGSET_SCOPE(Tag, TagSet)                                                                                  \
    NS::LLMTagSetScope NS_CONCAT(llmTagSetScope_, __LINE__)(NS::LLMTag::Tag, NS::LLMTagSet::TagSet)

#else

#define LLM_SCOPE(Tag)                                                                                                 \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define LLM_SCOPE_BYTAG(Tag)                                                                                           \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define LLM_SCOPE_CUSTOM(Tag)                                                                                          \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define LLM_PAUSE_SCOPE()                                                                                              \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define LLM_TAGSET_SCOPE(Tag, TagSet)                                                                                  \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)

#endif

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    // =========================================================================
    // 定数
    // =========================================================================

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

    /// プロジェクトタグ最大数
    constexpr uint32 kLLMMaxProjectTags = kLLMProjectTagEnd - kLLMProjectTagStart + 1;

    // =========================================================================
    // 列挙型
    // =========================================================================

    /// Low Level Memory Tracker タグ
    ///
    /// メモリ割り当てを分類するためのタグ。
    /// プロファイリングやメモリリーク検出に使用。
    enum class LLMTag : uint8
    {
        // システムタグ (0-9)
        Untagged = 0, ///< 未分類（デフォルト）
        Paused,       ///< 一時停止中（追跡停止）
        Total,        ///< 全メモリ合計
        Untracked,    ///< 追跡対象外
        TrackedTotal, ///< 追跡対象合計

        // エンジンコア (10-19)
        EngineMisc = 10, ///< エンジンその他
        Malloc,          ///< アロケータオーバーヘッド
        Containers,      ///< TArray, TMap等

        // グラフィックス (20-39)
        Textures = 20, ///< テクスチャ
        RenderTargets, ///< レンダーターゲット
        Shaders,       ///< シェーダー
        Meshes,        ///< メッシュ/頂点バッファ
        Particles,     ///< パーティクル
        RHIMisc,       ///< RHIその他

        // オーディオ (40-49)
        Audio = 40, ///< オーディオ全般

        // 物理 (50-59)
        Physics = 50, ///< 物理シミュレーション

        // UI (60-69)
        UI = 60, ///< ユーザーインターフェース

        // ネットワーク (70-79)
        Networking = 70, ///< ネットワーク

        // ゲームプレイ (80-99)
        Animation = 80, ///< アニメーション
        AI,             ///< 人工知能
        Scripting,      ///< スクリプト
        World,          ///< ワールド/レベル
        Actors,         ///< アクター/エンティティ

        // 境界マーカー
        GenericTagEnd = kLLMGenericTagEnd,       ///< 汎用タグ終端
        PlatformTagStart = kLLMPlatformTagStart, ///< プラットフォームタグ開始
        PlatformTagEnd = kLLMPlatformTagEnd,     ///< プラットフォームタグ終端
        ProjectTagStart = kLLMProjectTagStart,   ///< プロジェクトタグ開始
        ProjectTagEnd = kLLMProjectTagEnd,       ///< プロジェクトタグ終端
        MaxTagCount = kLLMMaxTagCount - 1        ///< 最大タグID（255）
    };

    /// タグセット（追加の分類軸）
    enum class LLMTagSet : uint8
    {
        None = 0,    ///< 標準タグセット
        Assets,      ///< アセット別追跡
        AssetClasses, ///< アセットクラス別追跡

        Max ///< 終端マーカー（イテレーション用）
    };

    /// LLMトラッカー種別
    enum class LLMTracker : uint8
    {
        Platform = 0, ///< OS/低レベル割り当て用
        Default,      ///< 通常エンジン割り当て用（Malloc経由）

        Max ///< 終端マーカー（イテレーション用）
    };

    /// LLM割り当て種別
    enum class LLMAllocType : uint8
    {
        None = 0, ///< 未指定
        Malloc,   ///< GMalloc経由の通常割り当て
        System,   ///< OS直接割り当て
        RHI,      ///< RHI割り当て

        Max ///< 終端マーカー（イテレーション用）
    };

    // =========================================================================
    // カスタムタグ
    // =========================================================================

    /// カスタムタグ情報
    struct LLMCustomTagInfo
    {
        const TCHAR* name;      ///< 表示名（静的文字列）
        const TCHAR* statGroup; ///< 統計グループ（nullptr = デフォルト）
        LLMTag parentTag;       ///< 親タグ（階層表示用、Untagged = ルート）
    };

    /// カスタムタグ登録
    ///
    /// @param name タグ名（静的文字列、コピーされない）
    /// @param statGroup 統計グループ名（nullptr可）
    /// @param parentTag 親タグ（階層化用）
    /// @return 登録されたタグID、失敗時はLLMTag::Untagged
    LLMTag RegisterLLMCustomTag(const TCHAR* name,
                                const TCHAR* statGroup = nullptr,
                                LLMTag parentTag = LLMTag::Untagged);

    /// タグ登録を終了
    void FinalizeTagRegistration();

    /// 登録フェーズかどうか
    bool IsInRegistrationPhase();

    /// タグ名取得
    const TCHAR* GetLLMTagName(LLMTag tag);

    /// タグが有効かどうか
    bool IsValidLLMTag(LLMTag tag);

    /// 登録済みカスタムタグ数取得
    uint32 GetLLMCustomTagCount();

    // =========================================================================
    // タグ統計
    // =========================================================================

    /// タグ統計情報
    struct LLMTagStats
    {
        LLMTag tag;   ///< タグID
        int64 amount; ///< 現在の割り当て量（バイト）
        int64 peak;   ///< ピーク割り当て量（バイト）
    };

    // =========================================================================
    // LowLevelMemTracker
    // =========================================================================

    /// Low Level Memory Tracker
    ///
    /// スレッドごとのタグスタックを管理し、メモリ割り当てを
    /// タグごとに分類するためのシングルトンクラス。
    class LowLevelMemTracker
    {
    public:
        /// シングルトンインスタンス取得
        static LowLevelMemTracker& Get();

        // =====================================================================
        // タグ操作
        // =====================================================================

        /// 現在のスレッドにタグをプッシュ
        void PushTag(LLMTag tag);

        /// 現在のスレッドからタグをポップ
        void PopTag();

        /// 現在のタグを取得
        [[nodiscard]] LLMTag GetCurrentTag() const;

        // =====================================================================
        // タグセット操作
        // =====================================================================

        /// 現在のスレッドにタグセットをプッシュ
        void PushTagSet(LLMTag tag, LLMTagSet tagSet);

        /// 現在のスレッドからタグセットをポップ
        void PopTagSet();

        // =====================================================================
        // メモリ追跡
        // =====================================================================

        /// メモリ割り当てを記録
        void TrackAllocation(LLMTag tag, int64 size);

        /// メモリ解放を記録
        void TrackFree(LLMTag tag, int64 size);

        // =====================================================================
        // 低レベル追跡API
        // =====================================================================

        /// メモリ割り当てを通知
        void OnLowLevelAlloc(LLMTracker tracker, void* ptr, int64 size, LLMTag tag, LLMAllocType allocType);

        /// メモリ解放を通知
        void OnLowLevelFree(LLMTracker tracker, void* ptr, LLMAllocType allocType);

        /// ポインタ移動を通知（デフラグ用）
        void OnLowLevelAllocMoved(LLMTracker tracker, void* destPtr, void* sourcePtr);

        /// メモリ量変更を通知（ポインタなしの場合）
        void OnLowLevelChangeInMemoryUse(LLMTracker tracker, int64 deltaMemory, LLMTag tag);

        // =====================================================================
        // 統計取得
        // =====================================================================

        /// タグの累計割り当て量を取得
        [[nodiscard]] int64 GetTagAmount(LLMTag tag) const;

        /// 全タグの統計を取得
        uint32 GetTagStats(LLMTagStats* outStats, uint32 maxCount) const;

        // =====================================================================
        // 制御
        // =====================================================================

        /// トラッキング有効かどうか
        [[nodiscard]] bool IsEnabled() const { return m_enabled; }

        /// トラッキング有効/無効設定
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        /// 統計情報をダンプ
        void DumpStats();

        /// 全統計をリセット
        void ResetStats();

        /// TLSベースのタグスタック最大深度
        static constexpr int32 kMaxTagStackDepth = 64;

    private:
        LowLevelMemTracker();
        ~LowLevelMemTracker() = default;

        NS_DISALLOW_COPY_AND_MOVE(LowLevelMemTracker);

        /// タグごとの追跡データ
        struct TagData
        {
            int64 amount;           ///< 現在の割り当て量
            int64 peak;             ///< ピーク割り当て量
            int64 totalAllocations; ///< 総割り当て回数
        };

        /// タグ追跡データ配列（256タグ分）
        TagData m_tagData[kLLMMaxTagCount];

        /// 有効フラグ
        bool m_enabled = true;
    };

    // =========================================================================
    // スコープクラス
    // =========================================================================

    /// LLMスコープ（RAII）
    ///
    /// コンストラクタでタグをプッシュし、デストラクタでポップする。
    class LLMScope
    {
    public:
        explicit LLMScope(LLMTag tag) { LowLevelMemTracker::Get().PushTag(tag); }

        ~LLMScope() { LowLevelMemTracker::Get().PopTag(); }

        NS_DISALLOW_COPY_AND_MOVE(LLMScope);
    };

    /// LLM一時停止スコープ（RAII）
    class LLMPauseScope
    {
    public:
        LLMPauseScope() { LowLevelMemTracker::Get().PushTag(LLMTag::Paused); }

        ~LLMPauseScope() { LowLevelMemTracker::Get().PopTag(); }

        NS_DISALLOW_COPY_AND_MOVE(LLMPauseScope);
    };

    /// タグセットスコープ（RAII）
    class LLMTagSetScope
    {
    public:
        LLMTagSetScope(LLMTag tag, LLMTagSet tagSet) { LowLevelMemTracker::Get().PushTagSet(tag, tagSet); }

        ~LLMTagSetScope() { LowLevelMemTracker::Get().PopTagSet(); }

        NS_DISALLOW_COPY_AND_MOVE(LLMTagSetScope);
    };

    // =========================================================================
    // 内部実装
    // =========================================================================

    namespace Private
    {
        /// 登録フェーズフラグ
        extern bool g_llmRegistrationPhase;

        /// 登録済みプロジェクトタグ数
        extern uint32 g_llmProjectTagCount;

        /// プロジェクトタグ情報配列
        extern LLMCustomTagInfo g_llmProjectTags[kLLMMaxProjectTags];
    } // namespace Private

#else // !ENABLE_LOW_LEVEL_MEM_TRACKER

    // Releaseビルド用スタブ
    enum class LLMTag : uint8
    {
        Untagged = 0,
        Max
    };
    enum class LLMTagSet : uint8
    {
        None = 0,
        Max
    };
    enum class LLMTracker : uint8
    {
        Platform = 0,
        Default,
        Max
    };
    enum class LLMAllocType : uint8
    {
        None = 0,
        Max
    };

    inline LLMTag RegisterLLMCustomTag(const TCHAR* /*unused*/, const TCHAR*  /*unused*/= nullptr, LLMTag  /*unused*/= LLMTag::Untagged)
    {
        return LLMTag::Untagged;
    }
    inline void FinalizeTagRegistration() {}
    inline bool IsInRegistrationPhase()
    {
        return false;
    }
    inline const TCHAR* GetLLMTagName(LLMTag /*unused*/)
    {
        return TEXT("Disabled");
    }
    inline bool IsValidLLMTag(LLMTag /*unused*/)
    {
        return false;
    }
    inline uint32 GetLLMCustomTagCount()
    {
        return 0;
    }

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
} // namespace NS
