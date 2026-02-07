/// @file RHIGPUProfiler.h
/// @brief GPUプロファイリングシステム
/// @details GPU実行時間の計測とプロファイリング機能を提供。
/// @see 05-06-gpu-profiler.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHICommandContext;

    //=========================================================================
    // ERHIGPUProfileEventType (05-06)
    //=========================================================================

    /// GPUプロファイルイベントタイプ
    enum class ERHIGPUProfileEventType : uint8
    {
        /// 描画コール
        Draw,

        /// コンピュートディスパッチ
        Dispatch,

        /// コピー操作
        Copy,

        /// レンダーパス
        RenderPass,

        /// カスタムマーカー
        Custom
    };

    //=========================================================================
    // ERHIGPUProfileFlags (05-06)
    //=========================================================================

    /// プロファイルスコープフラグ
    enum class ERHIGPUProfileFlags : uint32
    {
        None = 0,

        /// CPU時間も計測
        IncludeCPUTime = 1 << 0,

        /// メモリ使用量も計測
        IncludeMemory = 1 << 1,

        /// パイプライン統計も計測
        IncludePipelineStats = 1 << 2,

        /// 詳細統計
        Verbose = 1 << 3
    };
    RHI_ENUM_CLASS_FLAGS(ERHIGPUProfileFlags)

    //=========================================================================
    // RHIGPUProfileEvent (05-06)
    //=========================================================================

    /// GPUプロファイルイベント
    struct RHIGPUProfileEvent
    {
        /// イベント名
        const char* name = nullptr;

        /// イベントタイプ
        ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom;

        /// 開始タイムスタンプ（GPU ticks）
        uint64 startTimestamp = 0;

        /// 終了タイムスタンプ（GPU ticks）
        uint64 endTimestamp = 0;

        /// 経過時間（マイクロ秒）
        double elapsedMicroseconds = 0.0;

        /// 親イベントインデックス（-1 = ルート）
        int32 parentIndex = -1;

        /// 深度レベル
        uint32 depth = 0;

        /// フレーム番号
        uint64 frameNumber = 0;

        /// 追加統計（フラグに応じて有効）
        struct
        {
            double cpuElapsedMicroseconds = 0.0;
            uint64 memoryUsedBytes = 0;
            uint64 drawCalls = 0;
            uint64 primitives = 0;
            uint64 vertices = 0;
        } stats;
    };

    //=========================================================================
    // IRHIGPUProfiler (05-06)
    //=========================================================================

    /// GPUプロファイラインターフェース
    class RHI_API IRHIGPUProfiler
    {
    public:
        virtual ~IRHIGPUProfiler() = default;

        //=====================================================================
        // 有効化/無効化
        //=====================================================================

        /// プロファイリング開始
        virtual void BeginProfiling() = 0;

        /// プロファイリング終了
        virtual void EndProfiling() = 0;

        /// プロファイリング中か
        virtual bool IsProfiling() const = 0;

        //=====================================================================
        // スコープ操作
        //=====================================================================

        /// プロファイルスコープ開始
        /// @return スコープID（EndScope用）
        virtual uint32 BeginScope(IRHICommandContext* context,
                                  const char* name,
                                  ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom,
                                  ERHIGPUProfileFlags flags = ERHIGPUProfileFlags::None) = 0;

        /// プロファイルスコープ終了
        virtual void EndScope(IRHICommandContext* context, uint32 scopeId) = 0;

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// フレームの結果が準備完了か
        virtual bool IsFrameReady(uint64 frameNumber) const = 0;

        /// フレームの結果取得
        virtual bool GetFrameResults(uint64 frameNumber, std::vector<RHIGPUProfileEvent>& outEvents) = 0;

        /// 最新の完了フレーム番号取得
        virtual uint64 GetLatestCompletedFrame() const = 0;

        //=====================================================================
        // 統計
        //=====================================================================

        /// タイムスタンプ周波数取得（Hz）
        virtual uint64 GetTimestampFrequency() const = 0;

        /// フレーム全体のGPU時間取得（マイクロ秒）
        virtual double GetFrameGPUTime(uint64 frameNumber) const = 0;
    };

    //=========================================================================
    // RHIGPUProfileScope (05-06)
    //=========================================================================

    /// RAIIプロファイルスコープ
    class RHIGPUProfileScope
    {
    public:
        RHIGPUProfileScope(IRHIGPUProfiler* profiler,
                           IRHICommandContext* context,
                           const char* name,
                           ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom)
            : m_profiler(profiler), m_context(context), m_scopeId(0)
        {
            if (m_profiler && m_profiler->IsProfiling())
            {
                m_scopeId = m_profiler->BeginScope(m_context, name, type);
            }
        }

        ~RHIGPUProfileScope()
        {
            if (m_profiler && m_scopeId != 0)
            {
                m_profiler->EndScope(m_context, m_scopeId);
            }
        }

        NS_DISALLOW_COPY(RHIGPUProfileScope);

    private:
        IRHIGPUProfiler* m_profiler;
        IRHICommandContext* m_context;
        uint32 m_scopeId;
    };

} // namespace NS::RHI

//=============================================================================
// プロファイルスコープマクロ (05-06)
//=============================================================================

#ifndef NS_GPU_PROFILING_ENABLED
#if NS_BUILD_DEBUG
#define NS_GPU_PROFILING_ENABLED 1
#else
#define NS_GPU_PROFILING_ENABLED 0
#endif
#endif

#if NS_GPU_PROFILING_ENABLED
#define RHI_GPU_PROFILE_SCOPE(Profiler, Context, Name)                                                                 \
    ::NS::RHI::RHIGPUProfileScope NS_MACRO_CONCATENATE(_gpuProfileScope, __LINE__)(Profiler, Context, Name)

#define RHI_GPU_PROFILE_SCOPE_TYPE(Profiler, Context, Name, Type)                                                      \
    ::NS::RHI::RHIGPUProfileScope NS_MACRO_CONCATENATE(_gpuProfileScope, __LINE__)(Profiler, Context, Name, Type)
#else
#define RHI_GPU_PROFILE_SCOPE(Profiler, Context, Name)
#define RHI_GPU_PROFILE_SCOPE_TYPE(Profiler, Context, Name, Type)
#endif

namespace NS::RHI
{
    //=========================================================================
    // RHIFrameProfileData (05-06)
    //=========================================================================

    /// フレームプロファイルデータ
    struct RHIFrameProfileData
    {
        /// フレーム番号
        uint64 frameNumber = 0;

        /// フレーム全体のGPU時間（マイクロ秒）
        double totalGPUTime = 0.0;

        /// フレーム全体のCPU時間（マイクロ秒）
        double totalCPUTime = 0.0;

        /// イベント一覧
        std::vector<RHIGPUProfileEvent> events;

        /// カテゴリ別集計
        struct CategoryStats
        {
            double drawTime = 0.0;
            double dispatchTime = 0.0;
            double copyTime = 0.0;
            double renderPassTime = 0.0;
            double otherTime = 0.0;
        } categoryStats;

        /// 最も時間のかかったイベント（上位N件のインデックス）
        std::vector<uint32> topEventIndices;
    };

    //=========================================================================
    // RHIProfileHistory (05-06)
    //=========================================================================

    /// プロファイルヒストリー
    class RHI_API RHIProfileHistory
    {
    public:
        /// フレームデータ追加
        void AddFrame(const RHIFrameProfileData& data);

        /// 平均GPU時間取得（過去Nフレーム）
        double GetAverageGPUTime(uint32 frameCount = 60) const;

        /// 最大GPU時間取得
        double GetMaxGPUTime(uint32 frameCount = 60) const;

        /// フレームデータ取得
        const RHIFrameProfileData* GetFrame(uint64 frameNumber) const;

        /// 履歴クリア
        void Clear();

    private:
        static constexpr uint32 kMaxHistoryFrames = 300;
        std::vector<RHIFrameProfileData> m_history;
    };

} // namespace NS::RHI
