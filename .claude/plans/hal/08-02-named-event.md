> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [08-02a: ScopedNamedEventクラス](08-02a-named-event-class.md)
> - [08-02b: 名前付きイベントマクロ](08-02b-named-event-macros.md)

# 08-02: 名前付きイベント（旧版）

## 目的

プロファイリング用の名前付きイベントを実装する。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション5「名前付きイベント」

## 依存（commonモジュール）

- `common/utility/macros.h` - トークン連結（`NS_MACRO_CONCATENATE`）、ビルド構成検出（`NS_BUILD_DEBUG`）
- `common/utility/types.h` - 基本型（`uint8`）

## 依存（HAL）

- 01-04 プラットフォーム型（`ANSICHAR`, `TCHAR`）
- 04-01 premakeビルド設定（`NS_DEBUG`, `NS_DEVELOPMENT`）

## 注意

- `NS_CONCAT` は `common/utility/macros.h` の `NS_MACRO_CONCATENATE` を使用

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/ScopedEvent.h`

## TODO

- [ ] `ScopedNamedEvent` クラス
- [ ] `FScopedEvent` マクロ
- [ ] プラットフォーム固有イベントAPI連携

## 実装内容

```cpp
// ScopedEvent.h
#pragma once

namespace NS
{
    struct NamedEventColor
    {
        uint8 r, g, b;

        static constexpr NamedEventColor Red() { return {255, 0, 0}; }
        static constexpr NamedEventColor Green() { return {0, 255, 0}; }
        static constexpr NamedEventColor Blue() { return {0, 0, 255}; }
        static constexpr NamedEventColor Yellow() { return {255, 255, 0}; }
        static constexpr NamedEventColor Cyan() { return {0, 255, 255}; }
        static constexpr NamedEventColor Magenta() { return {255, 0, 255}; }
    };

    class ScopedNamedEvent
    {
    public:
        ScopedNamedEvent(const ANSICHAR* name, NamedEventColor color = NamedEventColor::Blue());
        ScopedNamedEvent(const TCHAR* name, NamedEventColor color = NamedEventColor::Blue());
        ~ScopedNamedEvent();

    private:
        void BeginEvent(const ANSICHAR* name, NamedEventColor color);
        void EndEvent();
    };

#if defined(NS_DEBUG) || defined(NS_DEVELOPMENT)
    #define SCOPED_NAMED_EVENT(Name, Color) \
        NS::ScopedNamedEvent NS_CONCAT(scopedEvent_, __LINE__)(Name, Color)
    #define SCOPED_NAMED_EVENT_TEXT(Name, Color) \
        NS::ScopedNamedEvent NS_CONCAT(scopedEvent_, __LINE__)(TEXT(Name), Color)
#else
    #define SCOPED_NAMED_EVENT(Name, Color)
    #define SCOPED_NAMED_EVENT_TEXT(Name, Color)
#endif
}
```

## 検証

- プロファイラでイベントが表示される
- リリースビルドでマクロが空になる
- 色が正しく表示される（PIX、Tracy等で確認）

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `PIXBeginEvent`, `PIXEndEvent`（オプション）

## 注意事項

- Release ビルドではマクロが空になる（ゼロオーバーヘッド）
- PIX/Tracy等のプロファイラ連携は別途実装
- 色は可視性向上のため（機能に影響なし）

## 次のサブ計画

→ 09-01: CPU機能検出
