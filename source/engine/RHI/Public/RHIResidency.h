/// @file RHIResidency.h
/// @brief GPUメモリ常駐管理
/// @details 常駐状態、優先度、自動退避管理、テクスチャストリーミングを提供。
/// @see 11-05-residency.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    class IRHIDevice;
    class IRHIFence;
    class IRHIQueue;

    //=========================================================================
    // ERHIResidencyStatus (11-05)
    //=========================================================================

    /// 常駐状態
    enum class ERHIResidencyStatus : uint8
    {
        Resident,            ///< 常駐中
        Evicted,             ///< 退避中（VRAMにない）
        PendingMakeResident, ///< 常駐待機中
        PendingEvict,        ///< 退避待機中
    };

    //=========================================================================
    // ERHIResidencyPriority (11-05)
    //=========================================================================

    /// 常駐優先度
    enum class ERHIResidencyPriority : uint8
    {
        Minimum, ///< 最低（退避候補）
        Low,     ///< 低
        Normal,  ///< 通常
        High,    ///< 高（できるだけ常駐維持）
        Maximum, ///< 最高（常に常駐）
    };

    //=========================================================================
    // IRHIResidentResource (11-05)
    //=========================================================================

    /// 常駐可能リソースインターフェース
    class RHI_API IRHIResidentResource
    {
    public:
        virtual ~IRHIResidentResource() = default;

        virtual ERHIResidencyStatus GetResidencyStatus() const = 0;
        virtual ERHIResidencyPriority GetResidencyPriority() const = 0;
        virtual void SetResidencyPriority(ERHIResidencyPriority priority) = 0;
        virtual uint64 GetSize() const = 0;
        virtual uint64 GetLastUsedFrame() const = 0;
        virtual void SetLastUsed(uint64 frame, uint64 fenceValue) = 0;
        virtual uint64 GetLastUsedFenceValue() const = 0;
    };

    //=========================================================================
    // RHIResidencyManager (11-05)
    //=========================================================================

    /// 常駐マネージャー設定
    struct RHI_API RHIResidencyManagerConfig
    {
        uint64 maxVideoMemoryUsage = 0;         ///< 最大VRAM使用量（0で自動）
        float evictionThreshold = 0.9f;         ///< 退避開始閾値
        float evictionTarget = 0.7f;            ///< 退避目標使用率
        uint32 unusedFramesBeforeEvict = 60;    ///< 自動退避までの未使用フレーム数
        bool enableBackgroundOperations = true; ///< バックグラウンド常駐操作
    };

    /// 常駐マネージャー
    /// VRAMの自動管理を行う
    class RHI_API RHIResidencyManager
    {
    public:
        RHIResidencyManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        const RHIResidencyManagerConfig& config = {},
                        IRHIFence* fence = nullptr,
                        IRHIQueue* queue = nullptr);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(uint64 frameNumber);

        /// フレーム終了（未使用リソースの退避判定）
        void EndFrame();

        //=====================================================================
        // リソース登録
        //=====================================================================

        void RegisterResource(IRHIResidentResource* resource);
        void UnregisterResource(IRHIResidentResource* resource);

        /// リソース使用マーク
        void MarkUsed(IRHIResidentResource* resource, uint64 fenceValue = 0);

        /// 複数リソース使用マーク
        void MarkUsed(IRHIResidentResource* const* resources, uint32 count, uint64 fenceValue = 0);

        //=====================================================================
        // 手動制御
        //=====================================================================

        /// 指定リソースの常駐を保証
        bool EnsureResident(IRHIResidentResource* resource);

        /// 指定リソース群の常駐を保証
        bool EnsureResident(IRHIResidentResource* const* resources, uint32 count);

        /// メモリ圧迫時の退避実行
        void PerformEviction();

        /// 非同期常駐要求
        bool EnqueueMakeResident(IRHIResidentResource* const* resources,
                                 uint32 count,
                                 IRHIFence* fenceToSignal,
                                 uint64 fenceValue);

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetCurrentVideoMemoryUsage() const { return m_currentUsage; }
        uint64 GetVideoMemoryBudget() const { return m_budget; }

        float GetUsageRatio() const { return m_budget > 0 ? static_cast<float>(m_currentUsage) / m_budget : 0.0f; }

        uint32 GetResidentResourceCount() const { return m_residentCount; }
        uint32 GetEvictedResourceCount() const { return m_evictedCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIResidencyManagerConfig m_config;

        uint64 m_budget = 0;
        uint64 m_currentUsage = 0;
        uint64 m_currentFrame = 0;

        IRHIFence* m_fence = nullptr;
        IRHIQueue* m_queue = nullptr;

        struct TrackedResource
        {
            IRHIResidentResource* resource;
            uint64 lastUsedFrame;
            uint64 lastUsedFenceValue = 0;
            ERHIResidencyStatus status;
        };
        TrackedResource* m_trackedResources = nullptr;
        uint32 m_trackedCount = 0;
        uint32 m_trackedCapacity = 0;

        uint32 m_residentCount = 0;
        uint32 m_evictedCount = 0;

        /// LRU退避候補選択
        void SelectEvictionCandidates(TrackedResource** outCandidates, uint32& outCount, uint64 targetSize);
    };

    //=========================================================================
    // ストリーミング (11-05)
    //=========================================================================

    /// ストリーミングレベル
    enum class ERHIStreamingLevel : uint8
    {
        Unloaded,  ///< 非ロード
        Thumbnail, ///< 最低品質
        Low,       ///< 低品質
        Medium,    ///< 中品質
        High,      ///< 高品質
        Highest,   ///< 最高品質（フル解像度）
    };

    /// ストリーミングリソースインターフェース
    class RHI_API IRHIStreamingResource
    {
    public:
        virtual ~IRHIStreamingResource() = default;

        virtual ERHIStreamingLevel GetCurrentStreamingLevel() const = 0;
        virtual ERHIStreamingLevel GetRequestedStreamingLevel() const = 0;
        virtual void RequestStreamingLevel(ERHIStreamingLevel level) = 0;
        virtual bool IsStreamingComplete() const = 0;
        virtual uint64 GetMemorySizeForLevel(ERHIStreamingLevel level) const = 0;
    };

    /// テクスチャストリーミングマネージャー
    class RHI_API RHITextureStreamingManager
    {
    public:
        RHITextureStreamingManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, RHIResidencyManager* residencyManager, uint64 streamingBudget);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // リソース管理
        //=====================================================================

        void RegisterResource(IRHIStreamingResource* resource);
        void UnregisterResource(IRHIStreamingResource* resource);

        /// 使用距離更新（優先度計算用）
        void UpdateResourceDistance(IRHIStreamingResource* resource, float distance);

        //=====================================================================
        // ストリーミング制御
        //=====================================================================

        void SetStreamingBudget(uint64 budget);

        /// 強制ロード（カットシーン等）
        void ForceLoad(IRHIStreamingResource* resource, ERHIStreamingLevel level);

        /// 処理優先度計算と更新
        void ProcessStreaming();

    private:
        IRHIDevice* m_device = nullptr;
        RHIResidencyManager* m_residencyManager = nullptr;
        uint64 m_budget = 0;

        struct StreamingEntry
        {
            IRHIStreamingResource* resource;
            float distance;
            float priority;
        };
        StreamingEntry* m_entries = nullptr;
        uint32 m_entryCount = 0;
        uint32 m_entryCapacity = 0;
    };

}} // namespace NS::RHI
