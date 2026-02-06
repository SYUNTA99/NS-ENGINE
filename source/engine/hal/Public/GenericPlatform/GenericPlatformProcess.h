/// @file GenericPlatformProcess.h
/// @brief プラットフォーム非依存のプロセス管理インターフェース
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// プラットフォーム非依存のプロセス管理インターフェース
    ///
    /// ## スレッドセーフティ
    ///
    /// 全関数はスレッドセーフ。
    struct GenericPlatformProcess
    {
        // =====================================================================
        // スリープ
        // =====================================================================

        /// 指定秒数スリープ
        ///
        /// @param seconds スリープ時間（秒）
        /// @note プロファイリング統計にカウントされる
        static void Sleep(float seconds);

        /// 指定秒数スリープ（統計なし）
        ///
        /// @param seconds スリープ時間（秒）
        /// @note プロファイリング統計にカウントされない
        static void SleepNoStats(float seconds);

        /// 無限スリープ
        ///
        /// @note 他スレッドからの割り込みでのみ復帰
        static void SleepInfinite();

        /// 他スレッドに実行権を譲る
        ///
        /// @note 即座にリターンする可能性あり（他スレッドがなければ）
        static void YieldThread();

        // =====================================================================
        // DLL管理
        // =====================================================================

        /// DLLをロード
        ///
        /// @param filename DLLファイルパス
        /// @return DLLハンドル、失敗時nullptr
        /// @note FreeDllHandleで解放すること
        static void* GetDllHandle(const TCHAR* filename);

        /// DLLを解放
        ///
        /// @param dllHandle GetDllHandleで取得したハンドル
        /// @note nullptr安全
        static void FreeDllHandle(void* dllHandle);

        /// DLLから関数ポインタを取得
        ///
        /// @param dllHandle GetDllHandleで取得したハンドル
        /// @param procName 関数名（Windows: ANSIに変換される）
        /// @return 関数ポインタ、失敗時nullptr
        static void* GetDllExport(void* dllHandle, const TCHAR* procName);

        // =====================================================================
        // プロセス情報
        // =====================================================================

        /// 現在のプロセスIDを取得
        ///
        /// @return プロセスID
        static uint32 GetCurrentProcessId();

        /// 現在の論理コア番号を取得
        ///
        /// @return 論理コア番号（0始まり）
        static uint32 GetCurrentCoreNumber();

        // =====================================================================
        // スレッド制御
        // =====================================================================

        /// 現在のスレッドのアフィニティマスクを設定
        ///
        /// @param mask コアのビットマスク（bit0=コア0, bit1=コア1, ...）
        /// @note 0を設定するとデフォルトに戻る
        static void SetThreadAffinityMask(uint64 mask);

        /// 現在のスレッドの優先度を設定
        ///
        /// @param priority プラットフォーム固有の優先度値
        /// @note Windowsでは THREAD_PRIORITY_* 定数を使用
        static void SetThreadPriority(int32 priority);
    };
} // namespace NS
