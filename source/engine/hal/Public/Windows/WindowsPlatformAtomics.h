/// @file WindowsPlatformAtomics.h
/// @brief Windows固有のアトミック操作実装
#pragma once

#include "GenericPlatform/GenericPlatformAtomics.h"
#include <intrin.h>

namespace NS
{
    /// Windows固有のアトミック操作実装
    ///
    /// _Interlocked系組み込み関数を使用。
    struct WindowsPlatformAtomics : public GenericPlatformAtomics
    {
        // =====================================================================
        // メモリバリア
        // =====================================================================

        static FORCEINLINE void ReadBarrier() { _ReadBarrier(); }

        static FORCEINLINE void WriteBarrier() { _WriteBarrier(); }

        static FORCEINLINE void MemoryBarrier()
        {
            ::_ReadWriteBarrier();
            // x86/x64ではコンパイラバリアで十分だが、明示的なハードウェアフェンスも発行
            __faststorefence();
        }

        // =====================================================================
        // 32bit 操作
        // =====================================================================

        static FORCEINLINE int32 InterlockedIncrement(volatile int32* value)
        {
            return static_cast<int32>(::_InterlockedIncrement(reinterpret_cast<volatile long*>(value)));
        }

        static FORCEINLINE int32 InterlockedDecrement(volatile int32* value)
        {
            return static_cast<int32>(::_InterlockedDecrement(reinterpret_cast<volatile long*>(value)));
        }

        static FORCEINLINE int32 InterlockedAdd(volatile int32* value, int32 amount)
        {
            return static_cast<int32>(
                ::_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(value), static_cast<long>(amount)));
        }

        static FORCEINLINE int32 InterlockedExchange(volatile int32* value, int32 exchange)
        {
            return static_cast<int32>(
                ::_InterlockedExchange(reinterpret_cast<volatile long*>(value), static_cast<long>(exchange)));
        }

        static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* dest, int32 exchange, int32 comparand)
        {
            return static_cast<int32>(::_InterlockedCompareExchange(
                reinterpret_cast<volatile long*>(dest), static_cast<long>(exchange), static_cast<long>(comparand)));
        }

        static FORCEINLINE int32 InterlockedAnd(volatile int32* value, int32 andValue)
        {
            return static_cast<int32>(
                ::_InterlockedAnd(reinterpret_cast<volatile long*>(value), static_cast<long>(andValue)));
        }

        static FORCEINLINE int32 InterlockedOr(volatile int32* value, int32 orValue)
        {
            return static_cast<int32>(
                ::_InterlockedOr(reinterpret_cast<volatile long*>(value), static_cast<long>(orValue)));
        }

        // =====================================================================
        // 64bit 操作
        // =====================================================================

        static FORCEINLINE int64 InterlockedIncrement(volatile int64* value)
        {
            return static_cast<int64>(::_InterlockedIncrement64(value));
        }

        static FORCEINLINE int64 InterlockedDecrement(volatile int64* value)
        {
            return static_cast<int64>(::_InterlockedDecrement64(value));
        }

        static FORCEINLINE int64 InterlockedAdd(volatile int64* value, int64 amount)
        {
            return static_cast<int64>(::_InterlockedExchangeAdd64(value, amount));
        }

        static FORCEINLINE int64 InterlockedExchange(volatile int64* value, int64 exchange)
        {
            return static_cast<int64>(::_InterlockedExchange64(value, exchange));
        }

        static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* dest, int64 exchange, int64 comparand)
        {
            return static_cast<int64>(::_InterlockedCompareExchange64(dest, exchange, comparand));
        }

        static FORCEINLINE int64 InterlockedAnd(volatile int64* value, int64 andValue)
        {
            return static_cast<int64>(::_InterlockedAnd64(value, andValue));
        }

        static FORCEINLINE int64 InterlockedOr(volatile int64* value, int64 orValue)
        {
            return static_cast<int64>(::_InterlockedOr64(value, orValue));
        }

        // =====================================================================
        // ポインタ操作
        // =====================================================================

        static FORCEINLINE void* InterlockedExchangePtr(void** dest, void* exchange)
        {
            return ::_InterlockedExchangePointer(dest, exchange);
        }

        static FORCEINLINE void* InterlockedCompareExchangePtr(void** dest, void* exchange, void* comparand)
        {
            return ::_InterlockedCompareExchangePointer(dest, exchange, comparand);
        }

        // =====================================================================
        // ユーティリティ
        // =====================================================================

        template <typename T> static FORCEINLINE T AtomicRead(const volatile T* src)
        {
            T result = *src;
            _ReadBarrier();
            return result;
        }

        template <typename T> static FORCEINLINE void AtomicStore(volatile T* dest, T value)
        {
            _WriteBarrier();
            *dest = value;
        }
    };

    /// 現在のプラットフォームのアトミック操作
    using PlatformAtomics = WindowsPlatformAtomics;
} // namespace NS
