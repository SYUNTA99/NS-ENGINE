/// @file WindowsPlatformTLS.h
/// @brief Windows固有のスレッドローカルストレージ実装
#pragma once

// windows.hを先にインクルード
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "GenericPlatform/GenericPlatformTLS.h"
#include "HAL/PlatformMacros.h"

namespace NS
{
    /// Windows固有のTLS実装
    ///
    /// TlsAlloc/TlsFree/TlsSetValue/TlsGetValueを使用。
    struct WindowsPlatformTLS : public GenericPlatformTLS
    {
        static FORCEINLINE uint32 AllocTlsSlot() { return ::TlsAlloc(); }

        static FORCEINLINE void FreeTlsSlot(uint32 slotIndex) { ::TlsFree(slotIndex); }

        static FORCEINLINE void SetTlsValue(uint32 slotIndex, void* value) { ::TlsSetValue(slotIndex, value); }

        static FORCEINLINE void* GetTlsValue(uint32 slotIndex) { return ::TlsGetValue(slotIndex); }
    };

    /// 現在のプラットフォームのTLS
    using PlatformTLS = WindowsPlatformTLS;
} // namespace NS
