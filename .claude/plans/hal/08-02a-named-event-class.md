# 08-02a: ScopedNamedEventクラス

## 目的

プロファイリング用の名前付きイベントクラスを定義する。

## 参考

[Platform_Abstraction_Layer_Part8.md](docs/Platform_Abstraction_Layer_Part8.md) - セクション5「名前付きイベント」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint8`）
- `common/utility/macros.h` - `NS_DISALLOW_COPY_AND_MOVE`

## 依存（HAL）

- 01-04 プラットフォーム型（`ANSICHAR`, `TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/HAL/ScopedEvent.h`

## TODO

- [ ] `NamedEventColor` 構造体定義
- [ ] `ScopedNamedEvent` RAIIクラス定義
- [ ] プリセット色定義

## 実装内容

```cpp
// ScopedEvent.h
#pragma once

#include "common/utility/types.h"
#include "common/utility/macros.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// 名前付きイベントの色
    struct NamedEventColor
    {
        uint8 r;
        uint8 g;
        uint8 b;

        constexpr NamedEventColor(uint8 red, uint8 green, uint8 blue)
            : r(red), g(green), b(blue) {}

        // プリセット色
        static constexpr NamedEventColor Red() { return {255, 0, 0}; }
        static constexpr NamedEventColor Green() { return {0, 255, 0}; }
        static constexpr NamedEventColor Blue() { return {0, 0, 255}; }
        static constexpr NamedEventColor Yellow() { return {255, 255, 0}; }
        static constexpr NamedEventColor Cyan() { return {0, 255, 255}; }
        static constexpr NamedEventColor Magenta() { return {255, 0, 255}; }
        static constexpr NamedEventColor Orange() { return {255, 165, 0}; }
        static constexpr NamedEventColor Purple() { return {128, 0, 128}; }
        static constexpr NamedEventColor White() { return {255, 255, 255}; }
        static constexpr NamedEventColor Gray() { return {128, 128, 128}; }
    };

    /// スコープベースの名前付きイベント（RAII）
    ///
    /// プロファイラ（PIX、Tracy等）で可視化されるイベントマーカー。
    /// コンストラクタでイベント開始、デストラクタで終了。
    class ScopedNamedEvent
    {
    public:
        /// ANSI文字列でイベント開始
        explicit ScopedNamedEvent(const ANSICHAR* name, NamedEventColor color = NamedEventColor::Blue());

        /// ワイド文字列でイベント開始
        explicit ScopedNamedEvent(const TCHAR* name, NamedEventColor color = NamedEventColor::Blue());

        /// イベント終了
        ~ScopedNamedEvent();

        NS_DISALLOW_COPY_AND_MOVE(ScopedNamedEvent);

    private:
        void BeginEvent(const ANSICHAR* name, NamedEventColor color);
        void BeginEvent(const TCHAR* name, NamedEventColor color);
        void EndEvent();
    };
}
```

## 検証

- `ScopedNamedEvent` がコンパイル可能
- プリセット色が正しい値
- RAIIパターンが機能

## 次のサブ計画

→ 08-02b: ScopedNamedEventマクロ
