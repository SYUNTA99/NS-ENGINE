/// @file RHITimestamp.h
/// @brief GPUタイムスタンプクエリ・タイマー・フレームタイムライン
/// @details タイムスタンプ結果、GPUタイマー、スコープタイマー、
///          フレームタイムラインを提供。
/// @see 14-03-timestamp.md
#pragma once

#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHIQuery.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // RHITimestampResult (14-03)
    //=========================================================================

    /// タイムスタンプ結果
    struct RHI_API RHITimestampResult
    {
        uint64 timestamp = 0;
        bool valid = false;

        /// ナノ秒への変換
        double ToNanoseconds(uint64 frequency) const
        {
            return valid ? (static_cast<double>(timestamp) * 1e9 / frequency) : 0.0;
        }

        /// ミリ秒への変換
        double ToMilliseconds(uint64 frequency) const { return ToNanoseconds(frequency) / 1e6; }

        /// 秒への変換
        double ToSeconds(uint64 frequency) const { return ToNanoseconds(frequency) / 1e9; }
    };

    //=========================================================================
    // RHITimestampInterval (14-03)
    //=========================================================================

    /// タイムスタンプ間隔
    struct RHI_API RHITimestampInterval
    {
        uint64 start = 0;
        uint64 end = 0;
        bool valid = false;

        /// 経過時間（タイムスタンプ単位）
        uint64 GetElapsed() const { return valid ? (end - start) : 0; }

        /// ナノ秒への変換
        double ToNanoseconds(uint64 frequency) const
        {
            return valid ? (static_cast<double>(end - start) * 1e9 / frequency) : 0.0;
        }

        /// ミリ秒への変換
        double ToMilliseconds(uint64 frequency) const { return ToNanoseconds(frequency) / 1e6; }
    };

    //=========================================================================
    // RHIGPUTimer (14-03)
    //=========================================================================

    /// GPUタイマー
    /// 特定の処理のGPU実行時間を計測
    class RHI_API RHIGPUTimer
    {
    public:
        RHIGPUTimer() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 maxMeasurements = 256, uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // 計測
        //=====================================================================

        /// 計測開始
        /// @param context コマンドコンテキスト
        /// @param name 計測名
        /// @return 計測ID（EndTimerに渡す）
        uint32 BeginTimer(IRHICommandContext* context, const char* name);

        /// 計測終了
        void EndTimer(IRHICommandContext* context, uint32 timerId);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了か
        bool AreResultsReady() const;

        /// 計測結果取得（ミリ秒）
        double GetTimerResult(uint32 timerId) const;

        /// 計測結果取得（名前指定）
        double GetTimerResult(const char* name) const;

        /// 全計測結果
        struct TimerResult
        {
            const char* name;
            double milliseconds;
        };

        /// 全計測結果取得
        uint32 GetAllResults(TimerResult* outResults, uint32 maxResults) const;

        //=====================================================================
        // 情報
        //=====================================================================

        /// タイムスタンプ周波数取得
        uint64 GetTimestampFrequency() const { return m_frequency; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        uint64 m_frequency = 0;

        struct Measurement
        {
            const char* name;
            uint32 startQueryIndex;
            uint32 endQueryIndex;
        };
        Measurement* m_measurements = nullptr;
        uint32 m_measurementCount = 0;
        uint32 m_maxMeasurements = 0;
    };

    //=========================================================================
    // RHIScopedGPUTimer (14-03)
    //=========================================================================

    /// スコープGPUタイマー（RAII）
    class RHI_API RHIScopedGPUTimer
    {
    public:
        RHIScopedGPUTimer(RHIGPUTimer* timer, IRHICommandContext* context, const char* name)
            : m_timer(timer), m_context(context)
        {
            if (m_timer && m_context)
            {
                m_timerId = m_timer->BeginTimer(m_context, name);
            }
        }

        ~RHIScopedGPUTimer()
        {
            if (m_timer && m_context && m_timerId != ~0u)
            {
                m_timer->EndTimer(m_context, m_timerId);
            }
        }

        NS_DISALLOW_COPY(RHIScopedGPUTimer);

    private:
        RHIGPUTimer* m_timer = nullptr;
        IRHICommandContext* m_context = nullptr;
        uint32 m_timerId = ~0u;
    };

    //=========================================================================
    // RHIFrameTimelineEntry (14-03)
    //=========================================================================

    /// フレームタイムラインエントリ
    struct RHI_API RHIFrameTimelineEntry
    {
        const char* name = nullptr;
        double startMs = 0.0; ///< フレーム開始からのミリ秒
        double endMs = 0.0;
        uint32 level = 0; ///< 階層レベル
        uint32 color = 0xFFFFFFFF;

        double DurationMs() const { return endMs - startMs; }
    };

    //=========================================================================
    // RHIFrameTimeline (14-03)
    //=========================================================================

    /// フレームタイムライン
    /// 1フレームのGPU処理をタイムライン形式で記録
    class RHI_API RHIFrameTimeline
    {
    public:
        RHIFrameTimeline() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 maxEntries = 1024, uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(IRHICommandContext* context);
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // 記録
        //=====================================================================

        /// 区間開始
        void BeginSection(IRHICommandContext* context, const char* name, uint32 color = 0xFFFFFFFF);

        /// 区間終了
        void EndSection(IRHICommandContext* context);

        /// マーカー（瞬間）
        void InsertMarker(IRHICommandContext* context, const char* name, uint32 color = 0xFFFFFFFF);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了か
        bool AreResultsReady() const;

        /// フレーム全体の時間（ミリ秒）
        double GetFrameTimeMs() const;

        /// エントリ数取得
        uint32 GetEntryCount() const { return m_resultEntryCount; }

        /// エントリ取得
        const RHIFrameTimelineEntry* GetEntries() const { return m_resultEntries; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        uint64 m_frequency = 0;

        RHIFrameTimelineEntry* m_resultEntries = nullptr;
        uint32 m_resultEntryCount = 0;
        uint32 m_maxEntries = 0;
        uint32 m_currentLevel = 0;
    };

} // namespace NS::RHI

/// スコープGPUタイマーマクロ
#define RHI_SCOPED_GPU_TIMER(timer, context, name)                                                                     \
    ::NS::RHI::RHIScopedGPUTimer NS_MACRO_CONCATENATE(_rhiTimer, __LINE__)(timer, context, name)
