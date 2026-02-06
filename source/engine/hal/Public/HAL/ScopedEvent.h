/// @file ScopedEvent.h
/// @brief プロファイリング用名前付きイベント
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/macros.h"
#include "common/utility/types.h"

namespace NS
{
    /// 名前付きイベントの色
    struct NamedEventColor
    {
        uint8 r;
        uint8 g;
        uint8 b;

        constexpr NamedEventColor(uint8 red, uint8 green, uint8 blue) : r(red), g(green), b(blue) {}

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
    ///
    /// @code
    /// void RenderFrame()
    /// {
    ///     SCOPED_NAMED_EVENT(TEXT("RenderFrame"), NamedEventColor::Blue());
    ///     // ... レンダリング処理
    /// }
    /// @endcode
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

    /// 名前付きイベント操作の静的関数
    struct NamedEvent
    {
        /// イベント開始（ANSI）
        static void BeginEvent(const ANSICHAR* name, NamedEventColor color = NamedEventColor::Blue());

        /// イベント開始（ワイド）
        static void BeginEvent(const TCHAR* name, NamedEventColor color = NamedEventColor::Blue());

        /// イベント終了
        static void EndEvent();
    };
} // namespace NS

// =============================================================================
// マクロ定義
// =============================================================================

/// プロファイリングイベントが有効かどうか
#ifndef NS_PROFILER_EVENTS_ENABLED
#if NS_BUILD_DEBUG
#define NS_PROFILER_EVENTS_ENABLED 1
#else
#define NS_PROFILER_EVENTS_ENABLED 0
#endif
#endif

#if NS_PROFILER_EVENTS_ENABLED

/// スコープ付き名前付きイベント
#define SCOPED_NAMED_EVENT(Name, Color) NS::ScopedNamedEvent NS_CONCAT(scopedEvent_, __LINE__)(Name, Color)

/// スコープ付き名前付きイベント（デフォルト色）
#define SCOPED_NAMED_EVENT_DEFAULT(Name) SCOPED_NAMED_EVENT(Name, NS::NamedEventColor::Blue())

/// 名前付きイベント開始
#define NAMED_EVENT_BEGIN(Name, Color) NS::NamedEvent::BeginEvent(Name, Color)

/// 名前付きイベント終了
#define NAMED_EVENT_END() NS::NamedEvent::EndEvent()

#else

#define SCOPED_NAMED_EVENT(Name, Color)                                                                                \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define SCOPED_NAMED_EVENT_DEFAULT(Name)                                                                               \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define NAMED_EVENT_BEGIN(Name, Color)                                                                                 \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)
#define NAMED_EVENT_END()                                                                                              \
    do                                                                                                                 \
    {}                                                                                                                 \
    while (0)

#endif
