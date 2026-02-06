# 07-01b: LowLevelMemTrackerクラス

## 目的

Low Level Memory Trackerのシングルトンクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション4「トラッカークラス」

## 依存（commonモジュール）

- `common/utility/macros.h` - `NS_DISALLOW_COPY_AND_MOVE`

## 依存（HAL）

- 07-01a LLMタグ列挙型

## 変更対象

変更:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`（クラス追加）

## TODO

- [ ] `LowLevelMemTracker` シングルトンクラス定義
- [ ] `PushTag` / `PopTag` メソッド
- [ ] 有効/無効切り替えメソッド
- [ ] タグスタック管理

## 実装内容

```cpp
// LowLevelMemTracker.h に追加

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    /// タグ統計情報
    struct LLMTagStats
    {
        LLMTag tag;             ///< タグID
        int64 amount;           ///< 現在の割り当て量（バイト）
        int64 peak;             ///< ピーク割り当て量（バイト）
    };

    /// Low Level Memory Tracker
    ///
    /// スレッドごとのタグスタックを管理し、メモリ割り当てを
    /// タグごとに分類するためのシングルトンクラス。
    ///
    /// ## 内部アロケーション
    /// LLM自体のメモリ追跡を避けるため、内部データ構造は
    /// MallocAnsiを直接使用するか、シンプルなバンプアロケータを使用。
    class LowLevelMemTracker
    {
    public:
        /// シングルトンインスタンス取得
        static LowLevelMemTracker& Get();

        // ========== タグ操作 ==========

        /// 現在のスレッドにタグをプッシュ
        void PushTag(LLMTag tag);

        /// 現在のスレッドからタグをポップ
        void PopTag();

        /// 現在のタグを取得
        LLMTag GetCurrentTag() const;

        // ========== タグセット操作 ==========

        /// 現在のスレッドにタグセットをプッシュ
        void PushTagSet(LLMTag tag, LLMTagSet tagSet);

        /// 現在のスレッドからタグセットをポップ
        void PopTagSet();

        // ========== メモリ追跡（高レベルAPI） ==========

        /// メモリ割り当てを記録
        /// @param tag タグ
        /// @param size 割り当てサイズ（バイト）
        void TrackAllocation(LLMTag tag, int64 size);

        /// メモリ解放を記録
        /// @param tag タグ
        /// @param size 解放サイズ（バイト）
        void TrackFree(LLMTag tag, int64 size);

        // ========== 低レベル追跡API ==========

        /// メモリ割り当てを通知
        /// @param tracker トラッカー種別
        /// @param ptr 割り当てポインタ
        /// @param size サイズ（バイト）
        /// @param tag タグ
        /// @param allocType 割り当て種別
        void OnLowLevelAlloc(LLMTracker tracker, void* ptr, int64 size,
                             LLMTag tag, LLMAllocType allocType);

        /// メモリ解放を通知
        /// @param tracker トラッカー種別
        /// @param ptr 解放ポインタ
        /// @param allocType 割り当て種別
        void OnLowLevelFree(LLMTracker tracker, void* ptr, LLMAllocType allocType);

        /// ポインタ移動を通知（デフラグ用）
        /// @param tracker トラッカー種別
        /// @param destPtr 移動先ポインタ
        /// @param sourcePtr 移動元ポインタ
        void OnLowLevelAllocMoved(LLMTracker tracker, void* destPtr, void* sourcePtr);

        /// メモリ量変更を通知（ポインタなしの場合）
        /// @param tracker トラッカー種別
        /// @param deltaMemory 変更量（正=増加、負=減少）
        /// @param tag タグ
        void OnLowLevelChangeInMemoryUse(LLMTracker tracker, int64 deltaMemory, LLMTag tag);

        // ========== 統計取得 ==========

        /// タグの累計割り当て量を取得
        /// @param tag タグID
        /// @return 現在の割り当て量（バイト）
        int64 GetTagAmount(LLMTag tag) const;

        /// 全タグの統計を取得
        /// @param outStats 出力先配列
        /// @param maxCount 配列の最大要素数
        /// @return 実際に書き込んだ要素数
        uint32 GetTagStats(LLMTagStats* outStats, uint32 maxCount) const;

        // ========== 制御 ==========

        /// トラッキング有効かどうか
        bool IsEnabled() const { return m_enabled; }

        /// トラッキング有効/無効設定
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        /// 統計情報をダンプ
        void DumpStats();

        /// 全統計をリセット
        void ResetStats();

    private:
        LowLevelMemTracker();
        ~LowLevelMemTracker() = default;

        NS_DISALLOW_COPY_AND_MOVE(LowLevelMemTracker);

        // ========== 内部データ構造 ==========

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

        /// TLSベースのタグスタック最大深度
        static constexpr int32 kMaxTagStackDepth = 64;

        // TLSスタック実装は05-02 PlatformTLSで定義
    };

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
}
```

## 検証

- `LowLevelMemTracker::Get()` がシングルトンを返す
- `PushTag` / `PopTag` が対応している
- `PushTagSet` / `PopTagSet` が対応している
- `SetEnabled(false)` でトラッキングが停止
- `GetTagAmount` が正確な値を返す
- `GetTagStats` が全タグの統計を取得できる
- 内部データ構造がLLM追跡対象外（自己追跡を避ける）

## 注意事項

- スレッドセーフである必要がある（TLS使用）
- スタックオーバーフロー防止（`kMaxTagStackDepth`）
- シングルトンの初期化タイミングに注意
- **自己追跡回避**: LLM内部のメモリ割り当てはLLM追跡対象外にする
  - MallocAnsiを直接使用、またはシンプルなバンプアロケータ
  - `m_tagData` は固定サイズ配列（動的割り当てなし）

## 次のサブ計画

→ 07-02a: LLMScopeクラス
