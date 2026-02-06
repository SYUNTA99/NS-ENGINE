> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [07-02a: LLMScopeクラス](07-02a-llm-scope-class.md)
> - [07-02b: LLMScopeマクロ](07-02b-llm-scope-macros.md)

# 07-02: LLM スコープマクロ（旧版）

## 目的

LLMスコープマクロとトラッカークラスを実装する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション5「スコープマクロ」

## 依存（commonモジュール）

- `common/utility/macros.h` - トークン連結（`NS_MACRO_CONCATENATE`）

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 07-01 LLMタグシステム（`LLMTag`, `ENABLE_LOW_LEVEL_MEM_TRACKER`）

## 注意

- `NS_CONCAT` は `common/utility/macros.h` の `NS_MACRO_CONCATENATE` を使用

## 変更対象

変更:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`

新規作成:
- `source/engine/hal/Private/HAL/LowLevelMemTracker.cpp`

## TODO

- [ ] `LLMScope` クラス
- [ ] `LLMPauseScope` クラス
- [ ] `LLM_SCOPE` マクロ
- [ ] `LLM_SCOPE_BYTAG` マクロ

## 実装内容

```cpp
// LowLevelMemTracker.h に追加

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    class LowLevelMemTracker
    {
    public:
        static LowLevelMemTracker& Get();

        void PushTag(LLMTag tag);
        void PopTag();

        bool IsEnabled() const { return m_enabled; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }

    private:
        LowLevelMemTracker() = default;
        bool m_enabled = true;
    };

    class LLMScope
    {
    public:
        LLMScope(LLMTag tag)
        {
            LowLevelMemTracker::Get().PushTag(tag);
        }

        ~LLMScope()
        {
            LowLevelMemTracker::Get().PopTag();
        }
    };

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
    };

    #define LLM_SCOPE(Tag) NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(NS::LLMTag::Tag)
    #define LLM_SCOPE_BYTAG(Tag) NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(Tag)
    #define LLM_PAUSE_SCOPE() NS::LLMPauseScope NS_CONCAT(llmPauseScope_, __LINE__)

#else
    #define LLM_SCOPE(Tag)
    #define LLM_SCOPE_BYTAG(Tag)
    #define LLM_PAUSE_SCOPE()
#endif
}
```

## 検証

- LLM_SCOPE でタグがプッシュ/ポップされる
- リリースビルドでマクロが空になる
- スコープ終了時に自動的にPopされる（RAII）

## 注意事項

- `LLM_SCOPE(Tag)` は `LLMTag::Tag` を使用
- `LLM_SCOPE_BYTAG(tag)` は変数（動的タグ）を使用
- スタックベースなのでネスト可能

## 次のサブ計画

→ 08-01: OutputDevice
