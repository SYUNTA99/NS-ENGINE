/// @file ScopedEvent.cpp
/// @brief プロファイリング用名前付きイベント実装

// windows.hを最初にインクルードしてTEXTマクロ衝突を回避
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "HAL/ScopedEvent.h"
#include "HAL/Platform.h"

namespace NS
{
    // =========================================================================
    // ScopedNamedEvent
    // =========================================================================

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
        NS_UNUSED(name);
        NS_UNUSED(color);

#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIX用のイベント開始
        // PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#endif
    }

    void ScopedNamedEvent::BeginEvent(const TCHAR* name, NamedEventColor color)
    {
        NS_UNUSED(name);
        NS_UNUSED(color);

#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIX用のイベント開始
        // PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#endif
    }

    void ScopedNamedEvent::EndEvent()
    {
#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIXEndEvent();
#endif
    }

    // =========================================================================
    // NamedEvent
    // =========================================================================

    void NamedEvent::BeginEvent(const ANSICHAR* name, NamedEventColor color)
    {
        NS_UNUSED(name);
        NS_UNUSED(color);

#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#endif
    }

    void NamedEvent::BeginEvent(const TCHAR* name, NamedEventColor color)
    {
        NS_UNUSED(name);
        NS_UNUSED(color);

#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIXBeginEvent(PIX_COLOR(color.r, color.g, color.b), name);
#endif
    }

    void NamedEvent::EndEvent()
    {
#if PLATFORM_WINDOWS && defined(USE_PIX)
        // PIXEndEvent();
#endif
    }
} // namespace NS
