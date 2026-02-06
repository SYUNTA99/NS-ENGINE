/// @file GenericPlatformTLS.h
/// @brief プラットフォーム非依存のスレッドローカルストレージインターフェース
#pragma once

#include "common/utility/types.h"

namespace NS
{
    /// プラットフォーム非依存のTLS（スレッドローカルストレージ）インターフェース
    ///
    /// ## 使用パターン
    ///
    /// ```cpp
    /// // 起動時（1回だけ）
    /// uint32 slot = PlatformTLS::AllocTlsSlot();
    ///
    /// // 各スレッドで
    /// PlatformTLS::SetTlsValue(slot, myThreadLocalData);
    /// auto* data = PlatformTLS::GetTlsValue(slot);
    ///
    /// // 終了時（1回だけ）
    /// PlatformTLS::FreeTlsSlot(slot);
    /// ```
    ///
    /// ## スレッドセーフティ
    ///
    /// - **AllocTlsSlot/FreeTlsSlot**: 排他制御が必要（通常は起動/終了時のみ）
    /// - **SetTlsValue/GetTlsValue**: スレッドセーフ（各スレッドは自身の値のみアクセス）
    struct GenericPlatformTLS
    {
        /// 無効なTLSスロットを示す定数
        static constexpr uint32 kInvalidTlsSlot = 0xFFFFFFFF;

        /// TLSスロットを割り当て
        ///
        /// @return 割り当てられたスロットインデックス、失敗時kInvalidTlsSlot
        static uint32 AllocTlsSlot();

        /// TLSスロットを解放
        ///
        /// @param slotIndex AllocTlsSlotで取得したスロット
        /// @note 無効なスロットを渡した場合の動作は未定義
        static void FreeTlsSlot(uint32 slotIndex);

        /// TLS値を設定
        ///
        /// @param slotIndex スロットインデックス
        /// @param value 設定する値（nullptr可）
        static void SetTlsValue(uint32 slotIndex, void* value);

        /// TLS値を取得
        ///
        /// @param slotIndex スロットインデックス
        /// @return 設定された値、未設定ならnullptr
        static void* GetTlsValue(uint32 slotIndex);
    };
} // namespace NS
