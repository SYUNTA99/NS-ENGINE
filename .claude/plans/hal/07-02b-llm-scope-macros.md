# 07-02b: LLMスコープマクロ

## 目的

LLMスコープ用の便利マクロを定義する。

## 参考

[Platform_Abstraction_Layer_Part7.md](docs/Platform_Abstraction_Layer_Part7.md) - セクション5「スコープマクロ」

## 依存（commonモジュール）

- `common/utility/macros.h` - トークン連結（`NS_MACRO_CONCATENATE`）

## 依存（HAL）

- 01-03 PreprocessorHelpers（`NS_CONCAT`）
- 07-02a LLMScopeクラス

## 変更対象

変更:
- `source/engine/hal/Public/HAL/LowLevelMemTracker.h`（マクロ追加）

新規作成:
- `source/engine/hal/Private/HAL/LowLevelMemTracker.cpp`

## TODO

- [ ] `LLM_SCOPE` マクロ定義
- [ ] `LLM_SCOPE_BYTAG` マクロ定義
- [ ] `LLM_PAUSE_SCOPE` マクロ定義
- [ ] 無効時の空マクロ定義
- [ ] `LowLevelMemTracker` 実装（cpp）

## 実装内容

### LowLevelMemTracker.h（マクロ追加）

```cpp
// LowLevelMemTracker.h に追加

#include "HAL/PreprocessorHelpers.h"

namespace NS
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER

    // スコープマクロ

    /// タグ名でスコープを開始
    /// 使用例: LLM_SCOPE(Textures)
    #define LLM_SCOPE(Tag) \
        NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(NS::LLMTag::Tag)

    /// タグ変数でスコープを開始
    /// 使用例: LLM_SCOPE_BYTAG(myTag)
    #define LLM_SCOPE_BYTAG(Tag) \
        NS::LLMScope NS_CONCAT(llmScope_, __LINE__)(Tag)

    /// トラッキング一時停止スコープ
    #define LLM_PAUSE_SCOPE() \
        NS::LLMPauseScope NS_CONCAT(llmPauseScope_, __LINE__)

#else

    // 無効時は空マクロ
    #define LLM_SCOPE(Tag)
    #define LLM_SCOPE_BYTAG(Tag)
    #define LLM_PAUSE_SCOPE()

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
}
```

### LowLevelMemTracker.cpp

```cpp
#include "HAL/LowLevelMemTracker.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER

namespace NS
{
    // TLSスロット（簡易実装）
    static thread_local LLMTag s_tagStack[LowLevelMemTracker::kMaxTagStackDepth];
    static thread_local int32 s_tagStackTop = 0;

    LowLevelMemTracker& LowLevelMemTracker::Get()
    {
        static LowLevelMemTracker instance;
        return instance;
    }

    LowLevelMemTracker::LowLevelMemTracker()
        : m_enabled(true)
    {
    }

    void LowLevelMemTracker::PushTag(LLMTag tag)
    {
        if (!m_enabled)
        {
            return;
        }

        if (s_tagStackTop < kMaxTagStackDepth)
        {
            s_tagStack[s_tagStackTop++] = tag;
        }
    }

    void LowLevelMemTracker::PopTag()
    {
        if (!m_enabled)
        {
            return;
        }

        if (s_tagStackTop > 0)
        {
            --s_tagStackTop;
        }
    }

    LLMTag LowLevelMemTracker::GetCurrentTag() const
    {
        if (s_tagStackTop > 0)
        {
            return s_tagStack[s_tagStackTop - 1];
        }
        return LLMTag::Untagged;
    }

    void LowLevelMemTracker::DumpStats()
    {
        // TODO: 統計情報出力
    }

    void LowLevelMemTracker::ResetStats()
    {
        // TODO: 統計リセット
    }
}

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
```

## 検証

- `LLM_SCOPE(Textures)` が正しく展開される
- スコープ終了時にタグがポップされる
- Releaseビルドでマクロが空になる
- `thread_local` が各スレッドで独立

## 注意事項

- `__LINE__` を使用して一意な変数名を生成
- `NS_CONCAT` はプリプロセッサ展開が2段階必要
- `thread_local` はC++11以降で使用可能

## 次のサブ計画

→ 08-01a: OutputDeviceインターフェース
