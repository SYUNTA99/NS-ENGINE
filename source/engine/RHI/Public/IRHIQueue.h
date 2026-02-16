/// @file IRHIQueue.h
/// @brief コマンドキューインターフェース
/// @details GPUにコマンドを送信するためのキュー。同期、タイミング、デバッグ機能を含む。
/// @see 01-25-queue-interface.md
#pragma once

#include "Common/Utility/Macros.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIQueueStats: キュー統計
    //=========================================================================

    /// キュー統計
    struct RHIQueueStats
    {
        uint64 commandListsSubmitted = 0;
        uint64 drawCalls = 0;
        uint64 dispatches = 0;
        uint64 barriers = 0;
        float averageGPUTimeMs = 0.0f;
    };

    //=========================================================================
    // IRHIQueue
    //=========================================================================

    /// コマンドキューインターフェース
    /// GPUにコマンドを送信するためのキュー
    class RHI_API IRHIQueue
    {
    public:
        virtual ~IRHIQueue() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 親デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// キュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        /// キューインデックス取得（同タイプ中での番号）
        virtual uint32 GetQueueIndex() const = 0;

        /// キュー名取得（デバッグ用）
        const char* GetQueueTypeName() const { return NS::RHI::GetQueueTypeName(GetQueueType()); }

        //=====================================================================
        // 機能サポートクエリ
        //=====================================================================

        /// グラフィックス操作をサポートするか
        bool SupportsGraphics() const { return GetQueueType() == ERHIQueueType::Graphics; }

        /// コンピュート操作をサポートするか
        bool SupportsCompute() const { return GetQueueType() != ERHIQueueType::Copy; }

        /// コピー操作をサポートするか
        bool SupportsCopy() const { return true; }

        /// タイムスタンプクエリをサポートするか
        virtual bool SupportsTimestampQueries() const = 0;

        /// タイルマッピングをサポートするか
        virtual bool SupportsTileMapping() const = 0;

        //=====================================================================
        // コマンド実行
        //=====================================================================

        /// コマンドリストを実行
        virtual void ExecuteCommandLists(IRHICommandList* const* commandLists, uint32 count) = 0;

        /// 単一コマンドリストを実行
        void ExecuteCommandList(IRHICommandList* commandList) { ExecuteCommandLists(&commandList, 1); }

        /// コンテキストを直接実行
        virtual void ExecuteContext(IRHICommandContext* context) = 0;

        //=====================================================================
        // 同期
        //=====================================================================

        /// フェンスシグナル
        virtual void Signal(IRHIFence* fence, uint64 value) = 0;

        /// フェンス待ち（GPU側）
        virtual void Wait(IRHIFence* fence, uint64 value) = 0;

        /// キューの全コマンド完了待機（CPU側）
        virtual void Flush() = 0;

        //=====================================================================
        // タイミング
        //=====================================================================

        /// GPUタイムスタンプ取得
        virtual uint64 GetGPUTimestamp() const = 0;

        /// タイムスタンプ周波数取得
        virtual uint64 GetTimestampFrequency() const = 0;

        //=====================================================================
        // 診断
        //=====================================================================

        /// キュー説明文字列取得（デバッグ用）
        virtual const char* GetDescription() const = 0;

        /// デバッグマーカー挿入
        virtual void InsertDebugMarker(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント開始
        virtual void BeginDebugEvent(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント終了
        virtual void EndDebugEvent() = 0;

        //=====================================================================
        // キュー専用フェンス (01-26)
        //=====================================================================

        /// キュー専用フェンス取得
        virtual IRHIFence* GetFence() const = 0;

        /// 最後に送信されたフェンス値
        virtual uint64 GetLastSubmittedFenceValue() const = 0;

        /// 最後に完了したフェンス値
        virtual uint64 GetLastCompletedFenceValue() const = 0;

        /// 指定フェンス値が完了しているか
        bool IsFenceComplete(uint64 fenceValue) const
        {
            return GetLastCompletedFenceValue() >= fenceValue;
        }

        /// 次のフェンス値を発行
        virtual uint64 AdvanceFence() = 0;

        //=====================================================================
        // フェンス待ち
        //=====================================================================

        /// CPU側でフェンス完了を待つ
        virtual bool WaitForFence(uint64 fenceValue, uint32 timeoutMs = 0) = 0;

        /// 最新の送信コマンドが完了するまで待つ
        bool WaitForIdle(uint32 timeoutMs = 0)
        {
            return WaitForFence(GetLastSubmittedFenceValue(), timeoutMs);
        }

        /// フェンスイベントオブジェクト取得（Windows用）
        virtual void* GetFenceEventHandle() const = 0;

        //=====================================================================
        // キュー間同期
        //=====================================================================

        /// 他キューのフェンスを待つ
        virtual void WaitForQueue(IRHIQueue* otherQueue, uint64 fenceValue) = 0;

        /// 他キューの最新コマンドを待つ
        void WaitForQueueIdle(IRHIQueue* otherQueue)
        {
            WaitForQueue(otherQueue, otherQueue->GetLastSubmittedFenceValue());
        }

        /// 外部フェンスを待つ
        virtual void WaitForExternalFence(IRHIFence* fence, uint64 value) = 0;

        //=====================================================================
        // 統計
        //=====================================================================

        /// キュー統計取得
        virtual RHIQueueStats GetStats() const = 0;

        /// 統計リセット
        virtual void ResetStats() = 0;

        //=====================================================================
        // GPU診断
        //=====================================================================

        /// GPUクラッシュダンプ有効化
        virtual void EnableGPUCrashDump(bool enable) = 0;

        /// Breadcrumb挿入（GPUハング時の原因特定用）
        virtual void InsertBreadcrumb(uint32 value) = 0;
    };

    //=========================================================================
    // デバッグイベントスコープ（RAII）
    //=========================================================================

    struct RHIQueueDebugEventScope
    {
        NS_DISALLOW_COPY(RHIQueueDebugEventScope);

        IRHIQueue* queue;

        RHIQueueDebugEventScope(IRHIQueue* q, const char* name, uint32 color = 0) : queue(q)
        {
            if (queue)
                queue->BeginDebugEvent(name, color);
        }

        ~RHIQueueDebugEventScope()
        {
            if (queue)
                queue->EndDebugEvent();
        }
    };

#define RHI_QUEUE_DEBUG_EVENT(queue, name)                                                                             \
    ::NS::RHI::RHIQueueDebugEventScope NS_MACRO_CONCATENATE(_rhiQueueEvent, __LINE__)(queue, name)

    //=========================================================================
    // RHISyncPoint: 同期ポイント
    //=========================================================================

    /// 同期ポイント
    /// キュー + フェンス値のペアで特定のGPU時点を表す
    struct RHISyncPoint
    {
        IRHIQueue* queue = nullptr;
        uint64 fenceValue = 0;

        RHISyncPoint() = default;
        RHISyncPoint(IRHIQueue* q, uint64 v) : queue(q), fenceValue(v) {}

        bool IsValid() const { return queue != nullptr; }

        bool IsComplete() const { return queue && queue->IsFenceComplete(fenceValue); }

        void Wait(uint32 timeoutMs = 0) const
        {
            if (queue)
                queue->WaitForFence(fenceValue, timeoutMs);
        }
    };

}} // namespace NS::RHI
