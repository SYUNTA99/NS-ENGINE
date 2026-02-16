/// @file RHIDeferredDelete.h
/// @brief 遅延削除キュー
/// @details GPU使用完了を待ってからリソースを削除する遅延削除キューを定義。
/// @see 01-14-deferred-delete.md
#pragma once

#include "Common/Utility/Macros.h"
#include "Common/Utility/Types.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include <mutex>
#include <vector>

namespace NS { namespace RHI {
    //=========================================================================
    // メモリプレッシャー
    //=========================================================================

    /// メモリプレッシャーレベル
    enum class ERHIMemoryPressure : uint8
    {
        None,
        Low,
        Medium,
        High,
        Critical,
    };

    /// メモリプレッシャーハンドラインターフェース
    class RHIMemoryPressureHandler
    {
    public:
        virtual ~RHIMemoryPressureHandler() = default;
        virtual void NotifyPressureChange(ERHIMemoryPressure level) = 0;
    };

    //=========================================================================
    // RHIDeferredDeleteEntry
    //=========================================================================

    /// 遅延削除エントリ
    struct RHIDeferredDeleteEntry
    {
        /// 削除対象リソース
        const IRHIResource* resource = nullptr;

        /// 待機するフェンス
        IRHIFence* fence = nullptr;

        /// 待機するフェンス値
        uint64 fenceValue = 0;

        /// フレーム番号（フェンスがない場合のフォールバック）
        uint64 frameNumber = 0;
    };

    //=========================================================================
    // RHIDeferredDeleteQueue
    //=========================================================================

    /// 遅延削除キュー
    /// GPUがリソースを使い終わるまで削除を遅延する
    class RHI_API RHIDeferredDeleteQueue
    {
    public:
        RHIDeferredDeleteQueue();
        ~RHIDeferredDeleteQueue();

        NS_DISALLOW_COPY(RHIDeferredDeleteQueue);

        //=====================================================================
        // 設定
        //=====================================================================

        /// 最大遅延フレーム数設定
        /// フェンスがない場合のフォールバック待機フレーム数
        void SetMaxDeferredFrames(uint32 frames);

        /// 現在のフレーム番号を設定
        void SetCurrentFrame(uint64 frameNumber);

        /// メモリプレッシャーハンドラを設定
        void SetMemoryPressureHandler(RHIMemoryPressureHandler* handler);

        /// プレッシャー通知閾値を設定
        void SetPressureThreshold(uint32 threshold);

        //=====================================================================
        // エンキュー
        //=====================================================================

        /// フェンス指定でエンキュー
        void Enqueue(const IRHIResource* resource, IRHIFence* fence, uint64 fenceValue);

        /// フレーム遅延でエンキュー（フェンスなし）
        void EnqueueFrameDeferred(const IRHIResource* resource);

        /// 即座に削除（同期削除）
        /// GPU同期が完了していることを呼び出し側が保証
        static void DeleteImmediate(const IRHIResource* resource);

        //=====================================================================
        // 処理
        //=====================================================================

        /// 完了した削除を処理
        /// @return 削除したリソース数
        uint32 ProcessCompletedDeletions();

        /// 全ての削除を強制実行（シャットダウン時）
        /// GPU同期後に呼び出すこと
        void FlushAll();

        /// ペンディング中のエントリ数
        size_t GetPendingCount() const;

        /// ペンディング中のメモリ推定量
        size_t GetPendingMemoryEstimate() const;

    private:
        std::vector<RHIDeferredDeleteEntry> m_entries;
        mutable std::mutex m_mutex;
        uint64 m_currentFrame = 0;
        uint32 m_maxDeferredFrames = 3;

        RHIMemoryPressureHandler* m_pressureHandler = nullptr;
        uint32 m_pressureThreshold = 256;
    };

}} // namespace NS::RHI
