/// @file RHISubmissionThread.h
/// @brief サブミッション/インタラプトスレッド
/// @details 3段パイプライン（レンダー→サブミッション→インタラプト）のスレッド実装。
/// @see 01-17b-submission-pipeline.md
#pragma once

#include "Common/Utility/Macros.h"
#include "Common/Utility/Types.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace NS { namespace RHI {
    struct RHIPayload;
}

namespace NS { namespace RHI { namespace Private {
    class RHIObjectPool;

    //=========================================================================
    // PendingInterrupt: 完了待ちエントリ
    //=========================================================================

    /// 完了待ちエントリ
    struct PendingInterrupt
    {
        ERHIQueueType queueType = ERHIQueueType::Graphics;
        IRHIFence* fence = nullptr;
        uint64 fenceValue = 0;

        /// 完了時に解放するアロケーター群
        IRHICommandAllocator** allocators = nullptr;
        uint32 allocatorCount = 0;

        /// 完了時コールバック（オプション）
        void (*callback)(void* userData) = nullptr;
        void* callbackUserData = nullptr;
    };

    //=========================================================================
    // RHISubmissionThread
    //=========================================================================

    /// サブミッションスレッド
    /// PendingSubmissionキューからPayloadを取り出し、GPUに送信する
    class RHISubmissionThread
    {
    public:
        RHISubmissionThread() = default;
        ~RHISubmissionThread();

        NS_DISALLOW_COPY(RHISubmissionThread);

        /// スレッド開始
        void Start();

        /// スレッド停止（全Payload処理完了後）
        void Stop(uint64 timeoutMs = 5000);

        /// Payloadをキューに投入（レンダースレッドから呼ばれる）
        void EnqueuePayload(RHIPayload&& payload);

        /// キューテーブル設定
        void SetQueue(ERHIQueueType type, IRHIQueue* queue);

        /// フェンステーブル設定
        void SetQueueFence(ERHIQueueType type, IRHIFence* fence);

        /// インタラプトスレッド参照設定
        void SetInterruptThread(class RHIInterruptThread* interruptThread);

    private:
        void ThreadMain();
        void ProcessPayload(const RHIPayload& payload);
        void BatchAndSubmit(IRHIQueue* queue, IRHICommandList* const* commandLists, uint32 count);

        std::thread m_thread;
        std::atomic<bool> m_running{false};

        std::queue<RHIPayload> m_pendingQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_wakeEvent;

        IRHIQueue* m_queues[static_cast<uint8>(ERHIQueueType::Count)] = {};
        IRHIFence* m_queueFences[static_cast<uint8>(ERHIQueueType::Count)] = {};

        RHIInterruptThread* m_interruptThread = nullptr;

        static constexpr uint32 kMaxBatchSize = 64;
    };

    //=========================================================================
    // RHIInterruptThread
    //=========================================================================

    /// インタラプトスレッド
    /// GPU完了を監視し、リソース解放・プール返却・コールバックを実行する
    class RHIInterruptThread
    {
    public:
        RHIInterruptThread() = default;
        ~RHIInterruptThread();

        NS_DISALLOW_COPY(RHIInterruptThread);

        /// スレッド開始
        void Start();

        /// スレッド停止
        void Stop(uint64 timeoutMs = 5000);

        /// 完了待ちエントリを投入（サブミッションスレッドから呼ばれる）
        void EnqueueInterrupt(PendingInterrupt&& entry);

        /// オブジェクトプール参照設定
        void SetObjectPool(ERHIQueueType type, RHIObjectPool* pool);

    private:
        void ThreadMain();
        void ProcessCompletion(const PendingInterrupt& entry);

        std::thread m_thread;
        std::atomic<bool> m_running{false};

        std::queue<PendingInterrupt> m_pendingQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_wakeEvent;

        RHIObjectPool* m_objectPools[static_cast<uint8>(ERHIQueueType::Count)] = {};
    };

}}} // namespace NS::RHI::Private
