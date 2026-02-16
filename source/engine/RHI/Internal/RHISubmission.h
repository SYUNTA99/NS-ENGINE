/// @file RHISubmission.h
/// @brief RHIPayload・サブミッションキュー型定義
/// @details レンダースレッドからサブミッションスレッドへの送信単位を定義。
/// @see 01-17b-submission-pipeline.md
#pragma once

#include "Common/Utility/Types.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIPayload: サブミッション単位
    //=========================================================================

    /// サブミッション単位
    /// レンダースレッドが作成し、サブミッションスレッドがGPUに送信する
    struct RHI_API RHIPayload
    {
        //=====================================================================
        // コマンド
        //=====================================================================

        /// 送信先キュータイプ
        ERHIQueueType queueType = ERHIQueueType::Graphics;

        /// 送信するコマンドリスト群
        IRHICommandList** commandLists = nullptr;
        uint32 commandListCount = 0;

        //=====================================================================
        // 同期
        //=====================================================================

        /// このPayload完了時のフェンス値
        uint64 completionFenceValue = 0;

        //=====================================================================
        // リソース参照
        //=====================================================================

        /// このPayloadで使用したコマンドアロケーター群
        /// 完了後にプールに返却される
        IRHICommandAllocator** usedAllocators = nullptr;
        uint32 usedAllocatorCount = 0;
    };

}} // namespace NS::RHI
