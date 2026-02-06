/// @file WindowsPlatformMemory.h
/// @brief Windows固有のメモリ管理実装
#pragma once

#include "GenericPlatform/GenericPlatformMemory.h"

namespace NS
{
    /// Windows固有のメモリ管理実装
    ///
    /// VirtualAlloc/VirtualFree、GlobalMemoryStatusExを使用。
    struct WindowsPlatformMemory : public GenericPlatformMemory
    {
        /// 初期化（GetSystemInfo, GlobalMemoryStatusExを使用）
        static void Init();

        /// 初期化済みかどうか
        static bool IsInitialized();

        /// メモリ統計取得（GlobalMemoryStatusEx使用）
        static PlatformMemoryStats GetStats();

        /// メモリ定数取得
        static const PlatformMemoryConstants& GetConstants();

        /// OS直接割り当て（VirtualAlloc使用）
        static void* BinnedAllocFromOS(SIZE_T size);

        /// OS直接解放（VirtualFree使用）
        static void BinnedFreeToOS(void* ptr, SIZE_T size);

        /// 仮想メモリ予約
        static void* VirtualReserve(SIZE_T size);

        /// 仮想メモリコミット
        static bool VirtualCommit(void* ptr, SIZE_T size);

        /// 仮想メモリデコミット
        static bool VirtualDecommit(void* ptr, SIZE_T size);

        /// 仮想メモリ解放
        static void VirtualFree(void* ptr, SIZE_T size);
    };

    /// 現在のプラットフォームのメモリ管理
    using PlatformMemory = WindowsPlatformMemory;
} // namespace NS
