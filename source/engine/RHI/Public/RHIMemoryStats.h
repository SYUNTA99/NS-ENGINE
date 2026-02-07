/// @file RHIMemoryStats.h
/// @brief GPUメモリ統計・トラッカー・監視システム
/// @details リソースタイプ別、ヒープ別のGPUメモリ使用量を追跡・可視化。
/// @see 25-02-memory-stats.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

#include <array>
#include <functional>
#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHIResource;
    class IRHIMemoryTracker;

    //=========================================================================
    // ERHIResourceCategory (25-02)
    //=========================================================================

    /// リソースカテゴリ
    enum class ERHIResourceCategory : uint8
    {
        Buffer,
        Texture,
        RenderTarget,
        DepthStencil,
        Shader,
        PipelineState,
        QueryHeap,
        AccelerationStructure,
        Descriptor,
        Staging,
        Other,
        Count
    };

    //=========================================================================
    // RHICategoryMemoryStats (25-02)
    //=========================================================================

    /// カテゴリ別メモリ統計
    struct RHICategoryMemoryStats
    {
        uint64 allocatedBytes = 0;  ///< 割り当て済みバイト数
        uint64 usedBytes = 0;       ///< 実使用バイト数（パディング除く）
        uint32 resourceCount = 0;   ///< リソース数
        uint32 allocationCount = 0; ///< アロケーション数

        float GetUtilization() const
        {
            return allocatedBytes > 0 ? static_cast<float>(usedBytes) / allocatedBytes : 0.0f;
        }
    };

    //=========================================================================
    // RHIHeapMemoryStats (25-02)
    //=========================================================================

    /// ヒープ別メモリ統計
    struct RHIHeapMemoryStats
    {
        uint64 totalSize = 0;          ///< ヒープ総サイズ
        uint64 usedSize = 0;           ///< 使用サイズ
        uint64 peakUsedSize = 0;       ///< ピーク使用サイズ
        uint32 allocationCount = 0;    ///< アロケーション数
        uint32 fragmentationCount = 0; ///< フラグメンテーション数

        float GetUsagePercent() const
        {
            return totalSize > 0 ? static_cast<float>(usedSize) / totalSize * 100.0f : 0.0f;
        }
    };

    //=========================================================================
    // RHIMemoryStats (25-02)
    //=========================================================================

    /// GPUメモリ統計（統合）
    struct RHI_API RHIMemoryStats
    {
        // カテゴリ別
        std::array<RHICategoryMemoryStats, static_cast<size_t>(ERHIResourceCategory::Count)> categoryStats;

        // ヒープ別
        RHIHeapMemoryStats defaultHeap;  ///< GPU専用メモリ
        RHIHeapMemoryStats uploadHeap;   ///< アップロードヒープ
        RHIHeapMemoryStats readbackHeap; ///< リードバックヒープ

        // 総計
        uint64 totalAllocatedBytes = 0;
        uint64 totalUsedBytes = 0;
        uint64 budgetBytes = 0;    ///< OSが許可するメモリ量
        uint64 availableBytes = 0; ///< 利用可能なメモリ量

        // デバイス情報
        uint64 dedicatedVideoMemory = 0; ///< 専用ビデオメモリ
        uint64 sharedSystemMemory = 0;   ///< 共有システムメモリ

        /// 予算超過しているか
        bool IsOverBudget() const { return totalAllocatedBytes > budgetBytes; }

        /// カテゴリ名取得
        static const char* GetCategoryName(ERHIResourceCategory category);
    };

    //=========================================================================
    // RHIResourceMemoryInfo (25-02)
    //=========================================================================

    /// リソースメモリ情報
    struct RHIResourceMemoryInfo
    {
        IRHIResource* resource = nullptr;
        ERHIResourceCategory category = ERHIResourceCategory::Other;
        uint64 allocatedSize = 0;
        uint64 usedSize = 0;
        const char* debugName = nullptr;
        uint64 allocationTime = 0; ///< 割り当て時刻（マイクロ秒）
    };

    //=========================================================================
    // IRHIMemoryTracker (25-02)
    //=========================================================================

    /// メモリトラッカーインターフェース
    class IRHIMemoryTracker
    {
    public:
        virtual ~IRHIMemoryTracker() = default;

        /// リソース割り当てを記録
        virtual void OnResourceAllocated(IRHIResource* resource,
                                         ERHIResourceCategory category,
                                         uint64 allocatedSize,
                                         uint64 usedSize,
                                         const char* debugName) = 0;

        /// リソース解放を記録
        virtual void OnResourceFreed(IRHIResource* resource) = 0;

        /// 現在の統計を取得
        virtual RHIMemoryStats GetStats() const = 0;

        /// 指定カテゴリのリソース一覧
        virtual void GetResourcesByCategory(ERHIResourceCategory category,
                                            std::vector<RHIResourceMemoryInfo>& outResources) const = 0;

        /// メモリ使用量上位Nリソース
        virtual void GetTopResources(uint32 count, std::vector<RHIResourceMemoryInfo>& outResources) const = 0;

        /// メモリリークチェック
        virtual void CheckForLeaks() const = 0;

        /// 統計をリセット（ピーク値等）
        virtual void ResetPeakStats() = 0;
    };

    //=========================================================================
    // ERHIMemoryWarningLevel (25-02)
    //=========================================================================

    /// メモリ警告レベル
    enum class ERHIMemoryWarningLevel : uint8
    {
        None,
        Low,     ///< 80%使用
        Medium,  ///< 90%使用
        High,    ///< 95%使用
        Critical ///< 100%超過
    };

    /// メモリ警告コールバック
    using RHIMemoryWarningCallback = std::function<void(ERHIMemoryWarningLevel, const RHIMemoryStats&)>;

    //=========================================================================
    // RHIMemoryMonitor (25-02)
    //=========================================================================

    /// メモリ監視
    class RHI_API RHIMemoryMonitor
    {
    public:
        explicit RHIMemoryMonitor(IRHIMemoryTracker* tracker);

        /// 警告コールバック設定
        void SetWarningCallback(RHIMemoryWarningCallback callback);

        /// 警告閾値設定（パーセント）
        void SetWarningThresholds(float low, float medium, float high);

        /// 更新（毎フレーム呼び出し）
        void Update();

        /// 現在の警告レベル
        ERHIMemoryWarningLevel GetCurrentWarningLevel() const { return m_currentLevel; }

    private:
        IRHIMemoryTracker* m_tracker;
        RHIMemoryWarningCallback m_callback;
        ERHIMemoryWarningLevel m_currentLevel = ERHIMemoryWarningLevel::None;
        float m_lowThreshold = 0.8f;
        float m_mediumThreshold = 0.9f;
        float m_highThreshold = 0.95f;
    };

    //=========================================================================
    // デバッグ出力関数 (25-02)
    //=========================================================================

    /// メモリ統計をログ出力
    RHI_API void RHIPrintMemoryStats(const RHIMemoryStats& stats);

    /// ImGuiメモリウィンドウ
    RHI_API void RHIDrawMemoryStatsImGui(IRHIMemoryTracker* tracker);

    /// メモリ使用量グラフ描画
    RHI_API void RHIDrawMemoryGraph(const RHIMemoryStats& stats);

} // namespace NS::RHI
