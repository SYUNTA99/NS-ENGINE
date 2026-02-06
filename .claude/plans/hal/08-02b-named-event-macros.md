# 08-02b: ScopedNamedEventマクロと実装

## 目的

名前付きイベントのマクロと実装を行う。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション5「名前付きイベント」

## 依存（commonモジュール）

- `common/utility/macros.h` - トークン連結（`NS_MACRO_CONCATENATE`）

## 依存（HAL）

- 01-03 PreprocessorHelpers（`NS_CONCAT`）
- 08-02a ScopedNamedEventクラス

## 必要なヘッダ（Windows）

- `<windows.h>` - PIX連携用（オプション）

## 変更対象

変更:
- `source/engine/hal/Public/HAL/ScopedEvent.h`（マクロ追加）

新規作成:
- `source/engine/hal/Private/HAL/ScopedEvent.cpp`

## TODO

- [ ] `SCOPED_NAMED_EVENT` マクロ定義
- [ ] `SCOPED_NAMED_EVENT_TEXT` マクロ定義
- [ ] 無効時の空マクロ定義
- [ ] `ScopedNamedEvent` 実装（cpp）

## 実装内容

### ScopedEvent.h（マクロ追加）

```cpp
// ScopedEvent.h に追加

#include "HAL/PreprocessorHelpers.h"

namespace NS
{
// イベントマーカー有効化フラグ
#ifndef ENABLE_NAMED_EVENTS
    #if defined(NS_DEBUG) || defined(NS_DEVELOPMENT)
        #define ENABLE_NAMED_EVENTS 1
    #else
        #define ENABLE_NAMED_EVENTS 0
    #endif
#endif

#if ENABLE_NAMED_EVENTS

    /// ANSI文字列リテラルでスコープイベント
    /// 使用例: SCOPED_NAMED_EVENT("RenderScene", NamedEventColor::Blue())
    #define SCOPED_NAMED_EVENT(Name, Color) \
        NS::ScopedNamedEvent NS_CONCAT(scopedEvent_, __LINE__)(Name, Color)

    /// TEXT()マクロ対応スコープイベント
    /// 使用例: SCOPED_NAMED_EVENT_TEXT("RenderScene", NamedEventColor::Blue())
    #define SCOPED_NAMED_EVENT_TEXT(Name, Color) \
        NS::ScopedNamedEvent NS_CONCAT(scopedEvent_, __LINE__)(TEXT(Name), Color)

    /// 色指定なしバージョン
    #define SCOPED_NAMED_EVENT_F(Name) \
        SCOPED_NAMED_EVENT(Name, NS::NamedEventColor::Blue())

#else

    // Releaseビルドでは空マクロ
    #define SCOPED_NAMED_EVENT(Name, Color)
    #define SCOPED_NAMED_EVENT_TEXT(Name, Color)
    #define SCOPED_NAMED_EVENT_F(Name)

#endif // ENABLE_NAMED_EVENTS
}
```

### ScopedEvent.cpp

```cpp
#include "HAL/ScopedEvent.h"
#include "HAL/Platform.h"

#if ENABLE_NAMED_EVENTS

#if PLATFORM_WINDOWS
#include <windows.h>
// PIX連携（USE_PIX定義時のみ）
#if defined(USE_PIX) && USE_PIX
#include <pix3.h>
#endif
#endif

namespace NS
{
    ScopedNamedEvent::ScopedNamedEvent(const ANSICHAR* name, NamedEventColor color)
    {
        BeginEvent(name, color);
    }

    ScopedNamedEvent::ScopedNamedEvent(const TCHAR* name, NamedEventColor color)
    {
        BeginEvent(name, color);
    }

    ScopedNamedEvent::~ScopedNamedEvent()
    {
        EndEvent();
    }

    void ScopedNamedEvent::BeginEvent(const ANSICHAR* name, NamedEventColor color)
    {
#if PLATFORM_WINDOWS
#if defined(USE_PIX) && USE_PIX
        PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#else
        // PIXなしの場合はOutputDebugStringで代用（オプション）
        // OutputDebugStringA(name);
        (void)name;
        (void)color;
#endif
#else
        (void)name;
        (void)color;
#endif
    }

    void ScopedNamedEvent::BeginEvent(const TCHAR* name, NamedEventColor color)
    {
#if PLATFORM_WINDOWS
#if defined(USE_PIX) && USE_PIX
        PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#else
        (void)name;
        (void)color;
#endif
#else
        (void)name;
        (void)color;
#endif
    }

    void ScopedNamedEvent::EndEvent()
    {
#if PLATFORM_WINDOWS
#if defined(USE_PIX) && USE_PIX
        PIXEndEvent();
#endif
#endif
    }
}

#endif // ENABLE_NAMED_EVENTS
```

## 検証

- `SCOPED_NAMED_EVENT("Test", NamedEventColor::Red())` が正しく展開
- Releaseビルドでマクロが空になる
- PIX有効時にイベントが表示される（PIXデバッグ時）

## 注意事項

- PIX SDKが必要（`USE_PIX` 定義時のみ）
- PIXなしの場合は空実装（オーバーヘッドなし）
- `__LINE__` で一意な変数名を生成

## 次のサブ計画

→ 09-01a: CPUInfo構造体
