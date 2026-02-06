/// @file Hash.h
/// @brief ハッシュ関数
#pragma once

#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    // =========================================================================
    // GetTypeHash - 整数型
    // =========================================================================

    FORCEINLINE uint32 GetTypeHash(uint8 value)
    {
        return value;
    }
    FORCEINLINE uint32 GetTypeHash(int8 value)
    {
        return static_cast<uint32>(value);
    }

    FORCEINLINE uint32 GetTypeHash(uint16 value)
    {
        return value;
    }
    FORCEINLINE uint32 GetTypeHash(int16 value)
    {
        return static_cast<uint32>(value);
    }

    FORCEINLINE uint32 GetTypeHash(uint32 value)
    {
        return value;
    }
    FORCEINLINE uint32 GetTypeHash(int32 value)
    {
        return static_cast<uint32>(value);
    }

    FORCEINLINE uint32 GetTypeHash(uint64 value)
    {
        // 上位32bitと下位32bitをXOR
        return static_cast<uint32>(value) ^ static_cast<uint32>(value >> 32);
    }

    FORCEINLINE uint32 GetTypeHash(int64 value)
    {
        return GetTypeHash(static_cast<uint64>(value));
    }

    // =========================================================================
    // GetTypeHash - ポインタ
    // =========================================================================

    FORCEINLINE uint32 GetTypeHash(const void* ptr)
    {
        return GetTypeHash(reinterpret_cast<UPTRINT>(ptr));
    }

    // =========================================================================
    // GetTypeHash - 浮動小数点
    // =========================================================================

    FORCEINLINE uint32 GetTypeHash(float value)
    {
        // IEEE 754ビットパターンをそのまま使用
        union
        {
            float f;
            uint32 u;
        } converter;
        converter.f = value;
        return converter.u;
    }

    FORCEINLINE uint32 GetTypeHash(double value)
    {
        union
        {
            double d;
            uint64 u;
        } converter;
        converter.d = value;
        return GetTypeHash(converter.u);
    }

    // =========================================================================
    // ハッシュ結合
    // =========================================================================

    /// ハッシュ値を結合（Boost風）
    ///
    /// 黄金比を使用した結合アルゴリズム。
    /// 良好な分布を維持しながら2つのハッシュを結合。
    FORCEINLINE uint32 HashCombine(uint32 a, uint32 b)
    {
        // Boost hash_combine アルゴリズム
        // 0x9e3779b9 は 2^32 / phi (黄金比)
        return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }

    /// 高速ハッシュ結合（品質より速度優先）
    FORCEINLINE uint32 HashCombineFast(uint32 a, uint32 b)
    {
        return a ^ (b * 0x9e3779b9);
    }

    /// 複数のハッシュを結合
    template <typename... Args> FORCEINLINE uint32 HashCombineMultiple(uint32 first, Args... rest)
    {
        if constexpr (sizeof...(rest) == 0)
        {
            return first;
        }
        else
        {
            return HashCombine(first, HashCombineMultiple(rest...));
        }
    }

    // =========================================================================
    // CRC32
    // =========================================================================

    /// CRC32ハッシュ計算
    /// @param data データポインタ
    /// @param length データ長（バイト）
    /// @param crc 初期CRC値（連続計算用）
    /// @return CRC32値
    uint32 Crc32(const void* data, SIZE_T length, uint32 crc = 0);

    /// ANSI文字列のCRC32ハッシュ
    uint32 StrCrc32(const ANSICHAR* str);

    /// ワイド文字列のCRC32ハッシュ
    uint32 StrCrc32(const WIDECHAR* str);

    /// 大文字小文字無視のANSI文字列ハッシュ
    uint32 StrCrc32NoCase(const ANSICHAR* str);

    /// 大文字小文字無視のワイド文字列ハッシュ
    uint32 StrCrc32NoCase(const WIDECHAR* str);
} // namespace NS
