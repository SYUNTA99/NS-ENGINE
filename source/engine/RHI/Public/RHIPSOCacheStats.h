/// @file RHIPSOCacheStats.h
/// @brief PSOキャッシュ統計・ウォームアップマネージャー
/// @details PSO使用状況の追跡、キャッシュヒット率・コンパイル時間・メモリ使用量の可視化。
/// @see 25-03-pso-cache-stats.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

#include <atomic>
#include <functional>
#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHIDevice;
    struct RHIGraphicsPipelineStateDesc;
    struct RHIComputePipelineStateDesc;
    struct RHIMeshPipelineStateDesc;
    class IRHIPSOCacheTracker;

    //=========================================================================
    // ERHIPSOType (25-03)
    //=========================================================================

    /// PSO種別
    enum class ERHIPSOType : uint8
    {
        Graphics,
        Compute,
        MeshShader,
        RayTracing,
    };

    //=========================================================================
    // RHIPSOInstanceStats (25-03)
    //=========================================================================

    /// 個別PSOの統計
    struct RHIPSOInstanceStats
    {
        ERHIPSOType type = ERHIPSOType::Graphics;
        uint64 hash = 0;                 ///< PSOハッシュ
        const char* debugName = nullptr; ///< デバッグ名
        uint64 compilationTimeUs = 0;    ///< コンパイル時間（マイクロ秒）
        uint64 memorySize = 0;           ///< メモリサイズ
        uint64 lastUsedFrame = 0;        ///< 最後に使用されたフレーム
        uint32 useCount = 0;             ///< 使用回数
        bool isFromCache = false;        ///< ディスクキャッシュから読み込み
    };

    //=========================================================================
    // RHIPSOCacheStats (25-03)
    //=========================================================================

    /// PSOキャッシュ全体の統計
    struct RHI_API RHIPSOCacheStats
    {
        // カウント
        uint32 totalPSOCount = 0;      ///< 総PSO数
        uint32 graphicsPSOCount = 0;   ///< Graphics PSO数
        uint32 computePSOCount = 0;    ///< Compute PSO数
        uint32 meshShaderPSOCount = 0; ///< メッシュシェーダーPSO数
        uint32 rayTracingPSOCount = 0; ///< レイトレーシングPSO数

        // キャッシュヒット
        uint32 cacheHits = 0;     ///< キャッシュヒット数
        uint32 cacheMisses = 0;   ///< キャッシュミス数
        uint32 diskCacheHits = 0; ///< ディスクキャッシュヒット数

        // 時間
        uint64 totalCompilationTimeUs = 0;   ///< 総コンパイル時間
        uint64 peakCompilationTimeUs = 0;    ///< 最大コンパイル時間
        uint64 averageCompilationTimeUs = 0; ///< 平均コンパイル時間

        // メモリ
        uint64 totalMemoryBytes = 0;  ///< 総メモリ使用量
        uint64 graphicsPSOMemory = 0; ///< Graphics PSOメモリ
        uint64 computePSOMemory = 0;  ///< Compute PSOメモリ

        // 効率
        float cacheHitRate = 0.0f;     ///< キャッシュヒット率
        float diskCacheHitRate = 0.0f; ///< ディスクキャッシュヒット率

        /// ヒット率を計算
        void CalculateRates()
        {
            uint32 total = cacheHits + cacheMisses;
            cacheHitRate = total > 0 ? static_cast<float>(cacheHits) / total : 0.0f;
            diskCacheHitRate = cacheMisses > 0 ? static_cast<float>(diskCacheHits) / cacheMisses : 0.0f;
        }
    };

    //=========================================================================
    // IRHIPSOCacheTracker (25-03)
    //=========================================================================

    /// PSOキャッシュトラッカー
    class IRHIPSOCacheTracker
    {
    public:
        virtual ~IRHIPSOCacheTracker() = default;

        /// PSO作成を記録
        virtual void OnPSOCreated(uint64 hash,
                                  ERHIPSOType type,
                                  uint64 compilationTimeUs,
                                  uint64 memorySize,
                                  bool fromDiskCache,
                                  const char* debugName) = 0;

        /// PSO使用を記録
        virtual void OnPSOUsed(uint64 hash) = 0;

        /// PSO破棄を記録
        virtual void OnPSODestroyed(uint64 hash) = 0;

        /// 統計取得
        virtual RHIPSOCacheStats GetStats() const = 0;

        /// 個別PSO統計取得
        virtual void GetPSOInstanceStats(std::vector<RHIPSOInstanceStats>& outStats) const = 0;

        /// 使用頻度上位N件
        virtual void GetMostUsedPSOs(uint32 count, std::vector<RHIPSOInstanceStats>& outStats) const = 0;

        /// 未使用PSO（指定フレーム数以上）
        virtual void GetUnusedPSOs(uint64 frameThreshold, std::vector<RHIPSOInstanceStats>& outStats) const = 0;

        /// 統計リセット
        virtual void ResetStats() = 0;
    };

    //=========================================================================
    // RHIPSOWarmupProgress (25-03)
    //=========================================================================

    /// PSOウォームアップ進捗
    struct RHIPSOWarmupProgress
    {
        uint32 totalPSOs = 0;            ///< 総PSO数
        uint32 compiledPSOs = 0;         ///< コンパイル済みPSO数
        uint64 elapsedTimeUs = 0;        ///< 経過時間
        uint64 estimatedRemainingUs = 0; ///< 推定残り時間
        bool isComplete = false;

        float GetProgress() const { return totalPSOs > 0 ? static_cast<float>(compiledPSOs) / totalPSOs : 1.0f; }
    };

    /// PSOウォームアップコールバック
    using RHIPSOWarmupCallback = std::function<void(const RHIPSOWarmupProgress&)>;

    //=========================================================================
    // RHIPSOWarmupManager (25-03)
    //=========================================================================

    /// PSOウォームアップマネージャー
    class RHI_API RHIPSOWarmupManager
    {
    public:
        explicit RHIPSOWarmupManager(IRHIDevice* device);

        /// ウォームアップ対象PSO追加
        void AddPSOForWarmup(const RHIGraphicsPipelineStateDesc& desc);
        void AddPSOForWarmup(const RHIComputePipelineStateDesc& desc);
        void AddPSOForWarmup(const RHIMeshPipelineStateDesc& desc);

        /// ウォームアップ開始（非同期）
        void StartWarmup(RHIPSOWarmupCallback progressCallback = nullptr);

        /// ウォームアップ待ち
        void WaitForCompletion();

        /// 進捗取得
        RHIPSOWarmupProgress GetProgress() const;

        /// キャンセル
        void Cancel();

    private:
        IRHIDevice* m_device;
        std::atomic<uint32> m_compiledCount{0};
        std::atomic<bool> m_cancelled{false};
        RHIPSOWarmupCallback m_callback;
        uint32 m_totalCount = 0;
    };

    //=========================================================================
    // デバッグ出力関数 (25-03)
    //=========================================================================

    /// PSOキャッシュ統計をログ出力
    RHI_API void RHIPrintPSOCacheStats(const RHIPSOCacheStats& stats);

    /// ImGuiでPSOキャッシュ情報表示
    RHI_API void RHIDrawPSOCacheImGui(IRHIPSOCacheTracker* tracker);

    /// PSOコンパイル時間グラフ
    RHI_API void RHIDrawPSOCompilationGraph(IRHIPSOCacheTracker* tracker);

} // namespace NS::RHI
