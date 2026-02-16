/// @file RHISyncPoint.h
/// @brief 同期ポイント
/// @details フレーム同期、パイプライン同期、複数同期ポイント待機、タイムライン同期を提供。
/// @see 09-02-sync-point.md
#pragma once

#include "IRHIFence.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHISyncPoint (09-02)
    //=========================================================================

    /// 同期ポイント
    /// フェンスと値のペアで特定のGPU処理完了を表す
    struct RHI_API RHISyncPoint
    {
        IRHIFence* fence = nullptr; ///< フェンス
        uint64 value = 0;           ///< フェンス値

        /// 有効か
        bool IsValid() const { return fence != nullptr; }

        /// 完了しているか
        bool IsCompleted() const { return fence && fence->IsCompleted(value); }

        /// CPU待機
        /// @param timeoutMs タイムアウト（ミリ秒）。デフォルト30秒。
        ///        タイムアウト時はfalseを返す。
        ///        呼び出し側でfalse時にデバイスロスト状態をチェックすることを推奨。
        bool Wait(uint64 timeoutMs = 30000) const { return fence ? fence->Wait(value, timeoutMs) : true; }

        bool operator==(const RHISyncPoint& other) const { return fence == other.fence && value == other.value; }
        bool operator!=(const RHISyncPoint& other) const { return !(*this == other); }

        /// 無効な同期ポイント
        static RHISyncPoint Invalid() { return {}; }
    };

    //=========================================================================
    // RHIFrameSync (09-02)
    //=========================================================================

    /// フレーム同期管理
    /// ダブル/トリプルバッファリングのフレーム同期を管理
    class RHI_API RHIFrameSync
    {
    public:
        static constexpr uint32 kMaxBufferedFrames = 4;

        RHIFrameSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 numBufferedFrames);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始（前のフレームのGPU処理完了を待機）
        void BeginFrame();

        /// フレーム終了（GPUキューにフレーム完了シグナルを発行）
        void EndFrame(IRHIQueue* queue);

        //=====================================================================
        // フレーム情報
        //=====================================================================

        /// 現在のフレームインデックス（0〜numBufferedFrames-1）
        uint32 GetCurrentFrameIndex() const { return m_currentFrameIndex; }

        /// 現在のCPUフレーム番号
        uint64 GetCurrentFrameNumber() const { return m_frameNumber; }

        /// 現在のGPU完了フレーム番号
        uint64 GetCompletedFrameNumber() const;

        /// フレームインフライト数
        uint32 GetFramesInFlight() const;

        /// バッファリングフレーム数
        uint32 GetNumBufferedFrames() const { return m_numBufferedFrames; }

        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// 現在フレームの同期ポイント取得
        RHISyncPoint GetCurrentFrameSyncPoint() const;

        /// 指定フレームの同期ポイント取得
        RHISyncPoint GetFrameSyncPoint(uint64 frameNumber) const;

        /// 指定フレームの完了を待機
        /// @param timeoutMs デフォルト30秒。超過時はログ警告+デバイスロストチェック
        bool WaitForFrame(uint64 frameNumber, uint64 timeoutMs = 30000);

        /// すべてのインフライトフレームの完了を待機
        /// 各フレームに30秒タイムアウトを適用し、超過時はログ警告+デバイスロストチェック
        void WaitForAllFrames();

    private:
        IRHIDevice* m_device = nullptr;
        RHIFenceRef m_frameFence;

        uint32 m_numBufferedFrames = 2;
        uint32 m_currentFrameIndex = 0;
        uint64 m_frameNumber = 0;
        uint64 m_frameFenceValues[kMaxBufferedFrames] = {};
    };

    //=========================================================================
    // ERHIPipelineSyncType (09-02)
    //=========================================================================

    /// パイプライン同期タイプ
    enum class ERHIPipelineSyncType : uint8
    {
        GraphicsToGraphics,
        GraphicsToCompute,
        ComputeToGraphics,
        ComputeToCompute,
        CopyToGraphics,
        CopyToCompute,
        GraphicsToCopy,
        ComputeToCopy,
    };

    //=========================================================================
    // RHIPipelineSync (09-02)
    //=========================================================================

    /// パイプライン同期ヘルパー
    class RHI_API RHIPipelineSync
    {
    public:
        RHIPipelineSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // キュー間同期
        //=====================================================================

        /// 同期ポイント挿入
        RHISyncPoint InsertSyncPoint(IRHIQueue* fromQueue);

        /// 同期ポイント待機
        void WaitForSyncPoint(IRHIQueue* queue, const RHISyncPoint& syncPoint);

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        /// キュー間同期（発行と待機）
        /// @note デッドロック防止: 同一フレーム内で循環待ち
        ///       （A→B かつ B→A）が発生しないよう呼び出し側が保証すること。
        void SyncQueues(IRHIQueue* fromQueue, IRHIQueue* toQueue);

        /// グラフィックス→コンピュート同期
        void GraphicsToCompute(IRHIQueue* graphicsQueue, IRHIQueue* computeQueue)
        {
            SyncQueues(graphicsQueue, computeQueue);
        }

        /// コンピュート→グラフィックス同期
        void ComputeToGraphics(IRHIQueue* computeQueue, IRHIQueue* graphicsQueue)
        {
            SyncQueues(computeQueue, graphicsQueue);
        }

    private:
        IRHIDevice* m_device = nullptr;
        RHIFenceRef m_syncFence;
        uint64 m_nextSyncValue = 1;

        //=====================================================================
        // 循環デッドロック検出（デバッグビルドのみ）
        //=====================================================================

        static constexpr uint32 kMaxQueues = 8;
        uint32 m_syncGraph[kMaxQueues][kMaxQueues] = {};

        bool ValidateNoCircularDependency(uint32 fromQueue, uint32 toQueue) const;
        void ResetFrameGraph();
    };

    //=========================================================================
    // RHISyncPointWaiter (09-02)
    //=========================================================================

    /// 複数同期ポイント待機
    class RHI_API RHISyncPointWaiter
    {
    public:
        static constexpr uint32 kMaxSyncPoints = 16;

        RHISyncPointWaiter() = default;

        /// 同期ポイント追加
        void Add(const RHISyncPoint& syncPoint)
        {
            if (m_count < kMaxSyncPoints && syncPoint.IsValid())
            {
                m_syncPoints[m_count++] = syncPoint;
            }
        }

        /// すべて待機（CPU）
        bool WaitAll(uint64 timeoutMs = UINT64_MAX);

        /// いずれか待機（CPU）
        /// @return 完了した同期ポイントのインデックス（-1でタイムアウト）
        int32 WaitAny(uint64 timeoutMs = UINT64_MAX);

        /// すべて完了しているか
        bool AreAllCompleted() const;

        /// いずれか完了しているか
        bool IsAnyCompleted() const;

        /// クリア
        void Clear() { m_count = 0; }

        uint32 GetCount() const { return m_count; }

    private:
        RHISyncPoint m_syncPoints[kMaxSyncPoints];
        uint32 m_count = 0;
    };

    //=========================================================================
    // RHITimelineSync (09-02)
    //=========================================================================

    /// タイムライン同期ポイント
    /// 時系列での同期ポイント管理
    class RHI_API RHITimelineSync
    {
    public:
        RHITimelineSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // タイムライン操作
        //=====================================================================

        /// 現在のタイムライン値取得
        uint64 GetCurrentValue() const;

        /// 次のタイムライン値取得（インクリメント）
        uint64 AcquireNextValue() { return m_nextValue++; }

        /// キューでシグナル
        uint64 Signal(IRHIQueue* queue);

        /// キューで待機
        void Wait(IRHIQueue* queue, uint64 value);

        /// CPU待機
        bool WaitCPU(uint64 value, uint64 timeoutMs = UINT64_MAX);

        /// 同期ポイント取得
        RHISyncPoint GetSyncPoint(uint64 value) const;

        /// フェンス取得
        IRHIFence* GetFence() const { return m_fence; }

    private:
        RHIFenceRef m_fence;
        uint64 m_nextValue = 1;
    };

}} // namespace NS::RHI
