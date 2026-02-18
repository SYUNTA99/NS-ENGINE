/// @file GenericPlatformTime.h
/// @brief プラットフォーム非依存の時間管理インターフェース
#pragma once

#include "HAL/PlatformMacros.h"
#include "common/utility/types.h"

namespace NS
{
    /// 日時構造体
    struct DateTime
    {
        int32 year{0};        ///< 年（例: 2024）
        int32 month{0};       ///< 月（1-12）
        int32 day{0};         ///< 日（1-31）
        int32 dayOfWeek{0};   ///< 曜日（0=日曜, 6=土曜）
        int32 hour{0};        ///< 時（0-23）
        int32 minute{0};      ///< 分（0-59）
        int32 second{0};      ///< 秒（0-59）
        int32 millisecond{0}; ///< ミリ秒（0-999）

        /// デフォルト初期化
        DateTime() = default;
    };

    /// プラットフォーム非依存の時間管理インターフェース
    ///
    /// ## 関数の関係
    ///
    /// ```
    /// InitTiming()
    ///     ↓ 初期化時に一度呼び出し
    ///     ↓ 内部でSecondsPerCycleをキャッシュ
    ///     ↓
    /// ┌───────────────────────────────────────────┐
    /// │            高精度タイマー                   │
    /// │                                           │
    /// │  Cycles64() ─────→ 生のCPUサイクル値      │
    /// │      │              （単調増加）           │
    /// │      │                                    │
    /// │      ├──× GetSecondsPerCycle64()         │
    /// │      │      （サイクル→秒の変換係数）     │
    /// │      ↓                                    │
    /// │  Seconds() ─────→ 秒単位の経過時間       │
    /// │                    （InitTimingからの）   │
    /// └───────────────────────────────────────────┘
    /// ```
    ///
    /// ## 使用パターン
    ///
    /// ```cpp
    /// // フレーム時間計測
    /// double frameStart = PlatformTime::Seconds();
    /// // ... 処理 ...
    /// double frameTime = PlatformTime::Seconds() - frameStart;
    ///
    /// // 高精度計測（オーバーヘッド最小）
    /// uint64 cycleStart = PlatformTime::Cycles64();
    /// // ... 処理 ...
    /// uint64 elapsedCycles = PlatformTime::Cycles64() - cycleStart;
    /// double elapsedSeconds = elapsedCycles * PlatformTime::GetSecondsPerCycle64();
    /// ```
    ///
    /// ## スレッドセーフティ
    ///
    /// - **InitTiming()**: メインスレッドから起動時に一度だけ呼び出す
    /// - **その他全関数**: スレッドセーフ（読み取り専用）
    ///
    /// ## 精度
    ///
    /// - Windows: QueryPerformanceCounter (100ns〜1μs精度)
    /// - 精度はハードウェア/OS依存
    struct GenericPlatformTime
    {
        // =====================================================================
        // 初期化
        // =====================================================================

        /// タイミングシステム初期化
        ///
        /// @return サイクルあたりの秒数（GetSecondsPerCycle64と同値）
        /// @note 起動時にメインスレッドから一度呼び出す
        /// @note 複数回呼び出しても安全（初回のみ初期化）
        static double InitTiming();

        /// 初期化済みかどうか
        static bool IsInitialized();

        // =====================================================================
        // 高精度タイマー
        // =====================================================================

        /// 経過秒数を取得（高精度）
        ///
        /// @return InitTiming()からの経過秒数
        /// @note 内部でCycles64() × SecondsPerCycleを計算
        /// @note 子クラスでインライン実装
        static double Seconds();

        /// 高精度サイクルカウンタを取得
        ///
        /// @return CPUサイクル値（単調増加、ラップアラウンドの可能性）
        /// @note 最も低オーバーヘッド、ナノ秒レベルの計測に使用
        /// @note 64bitなので約584年でラップアラウンド（実用上問題なし）
        /// @note 子クラスでインライン実装
        static uint64 Cycles64();

        /// サイクルあたりの秒数を取得
        ///
        /// @return 変換係数（Cycles64に乗じて秒に変換）
        /// @note InitTiming()で計算されキャッシュされる
        /// @note 子クラスでインライン実装
        static double GetSecondsPerCycle64();

        /// サイクルを秒に変換
        ///
        /// @param cycles サイクル数
        /// @return 秒数
        static FORCEINLINE double CyclesToSeconds(uint64 cycles)
        {
            return static_cast<double>(cycles) * GetSecondsPerCycle64();
        }

        /// 秒をサイクルに変換
        ///
        /// @param seconds 秒数
        /// @return サイクル数
        static FORCEINLINE uint64 SecondsToCycles(double seconds)
        {
            return static_cast<uint64>(seconds / GetSecondsPerCycle64());
        }

        /// 2つのサイクル値の差を秒で取得
        ///
        /// @param startCycles 開始サイクル
        /// @param endCycles 終了サイクル
        /// @return 経過秒数
        static FORCEINLINE double CycleDifferenceToSeconds(uint64 startCycles, uint64 endCycles)
        {
            return CyclesToSeconds(endCycles - startCycles);
        }

        // =====================================================================
        // システム時刻
        // =====================================================================

        /// ローカルシステム時刻を取得
        ///
        /// @param outDateTime 出力先
        /// @note タイムゾーン考慮済み
        static void GetLocalTime(DateTime& outDateTime);

        /// UTC時刻を取得
        ///
        /// @param outDateTime 出力先
        static void GetUtcTime(DateTime& outDateTime);

        /// ローカルシステム時刻を取得（レガシーAPI）
        static void SystemTime(
            int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec);

        /// UTC時刻を取得（レガシーAPI）
        static void UtcTime(
            int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& min, int32& sec, int32& msec);

        // =====================================================================
        // ユーティリティ
        // =====================================================================

        /// 現在のUnixタイムスタンプ（秒）
        ///
        /// @return 1970-01-01 00:00:00 UTCからの秒数
        static int64 GetUnixTimestamp();

        /// 現在のUnixタイムスタンプ（ミリ秒）
        static int64 GetUnixTimestampMillis();

    protected:
        static double s_secondsPerCycle;
        static bool s_initialized;
    };
} // namespace NS
