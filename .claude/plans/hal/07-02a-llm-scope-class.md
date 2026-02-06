# 07-02a: LLMScopeクラス

## 目的

RAIIベースのLLMスコープクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション5「スコープマクロ」

## 依存（commonモジュール）

- `common/utility/macros.h` - `NS_DISALLOW_COPY_AND_MOVE`

## 依存（HAL）

- 07-01a LLMタグ列挙型
- 07-01b LowLevelMemTrackerクラス

## 変更対象

変更:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`（スコープクラス追加）

## TODO

- [ ] `LLMScope` RAIIクラス定義
- [ ] `LLMPauseScope` クラス定義
- [ ] コピー/ムーブ禁止

## 実装内容

```cpp
// LowLevelMemTracker.h に追加

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    /// LLMスコープ（RAII）
    ///
    /// コンストラクタでタグをプッシュし、デストラクタでポップする。
    /// スコープベースのメモリトラッキングを実現。
    class LLMScope
    {
    public:
        /// タグを指定してスコープ開始
        explicit LLMScope(LLMTag tag)
        {
            LowLevelMemTracker::Get().PushTag(tag);
        }

        /// スコープ終了（タグをポップ）
        ~LLMScope()
        {
            LowLevelMemTracker::Get().PopTag();
        }

        NS_DISALLOW_COPY_AND_MOVE(LLMScope);
    };

    /// LLM一時停止スコープ（RAII）
    ///
    /// スコープ内でLLMトラッキングを一時停止する。
    class LLMPauseScope
    {
    public:
        LLMPauseScope()
        {
            LowLevelMemTracker::Get().PushTag(LLMTag::Paused);
        }

        ~LLMPauseScope()
        {
            LowLevelMemTracker::Get().PopTag();
        }

        NS_DISALLOW_COPY_AND_MOVE(LLMPauseScope);
    };

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
}
```

## 検証

- `LLMScope` がRAII動作する
- スコープ終了時にタグが自動ポップされる
- `LLMPauseScope` が `Paused` タグをプッシュ

## 注意事項

- コピー/ムーブ禁止（二重ポップ防止）
- スタックローカル変数として使用

## 次のサブ計画

→ 07-02b: LLMスコープマクロ
