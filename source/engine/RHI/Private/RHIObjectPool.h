/// @file RHIObjectPool.h
/// @brief コンテキスト/アロケーター/コマンドリストプール
/// @details キュータイプごとのプール管理。フェンス値による再利用可能判定を行う。
/// @see 01-17b-submission-pipeline.md
#pragma once

#include "Common/Utility/Macros.h"
#include "Common/Utility/Types.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include <mutex>
#include <vector>

namespace NS::RHI::Private
{
    //=========================================================================
    // PooledAllocator
    //=========================================================================

    /// プール内アロケーターエントリ
    struct PooledAllocator
    {
        IRHICommandAllocator* allocator = nullptr;
        IRHIFence* fence = nullptr;
        uint64 fenceValue = 0;
    };

    //=========================================================================
    // RHIObjectPool
    //=========================================================================

    /// キューレベルのオブジェクトプール
    /// コマンドアロケーター/コマンドリスト/コンテキストをプール管理する
    class RHIObjectPool
    {
    public:
        explicit RHIObjectPool(ERHIQueueType queueType);
        ~RHIObjectPool();

        NS_DISALLOW_COPY(RHIObjectPool);

        //=====================================================================
        // コンテキスト管理
        //=====================================================================

        /// コンテキスト取得（プールから再利用 or 新規作成）
        IRHICommandContext* ObtainContext();

        /// コンテキスト返却
        void ReleaseContext(IRHICommandContext* context);

        //=====================================================================
        // アロケーター管理
        //=====================================================================

        /// コマンドアロケーター取得
        IRHICommandAllocator* ObtainCommandAllocator();

        /// コマンドアロケーター返却
        void ReleaseCommandAllocator(IRHICommandAllocator* allocator, IRHIFence* fence, uint64 fenceValue);

        //=====================================================================
        // コマンドリスト管理
        //=====================================================================

        /// コマンドリスト取得
        IRHICommandList* ObtainCommandList(IRHICommandAllocator* allocator);

        /// コマンドリスト返却
        void ReleaseCommandList(IRHICommandList* commandList);

        //=====================================================================
        // メンテナンス
        //=====================================================================

        /// 完了済みリソースの回収
        void Trim();

        /// 全リソース解放（シャットダウン時）
        void ReleaseAll();

    private:
        ERHIQueueType m_queueType;
        std::mutex m_mutex;

        std::vector<IRHICommandAllocator*> m_availableAllocators;
        std::vector<PooledAllocator> m_pendingAllocators;
        std::vector<IRHICommandList*> m_availableCommandLists;
        std::vector<IRHICommandContext*> m_availableContexts;
    };

} // namespace NS::RHI::Private
