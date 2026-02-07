/// @file IRHICommandAllocator.h
/// @brief コマンドアロケーターインターフェース
/// @details コマンドリストのメモリ管理。ライフサイクル、メモリ情報、待機フェンス、プール管理を提供。
/// @see 02-04-command-allocator.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIResourceType.h"

namespace NS::RHI
{
    //=========================================================================
    // IRHICommandAllocator
    //=========================================================================

    /// コマンドアロケーター
    /// コマンドリストのメモリを管理するオブジェクト
    class RHI_API IRHICommandAllocator : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(CommandAllocator)

        virtual ~IRHICommandAllocator() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 対応するキュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// リセット
        /// @note GPU完了後にのみ呼び出し可能
        /// 関連付けられたコマンドリストのメモリを再利用可能にする
        virtual void Reset() = 0;

        /// GPU使用中か
        /// @note 使用中はリセット不可
        virtual bool IsInUse() const = 0;

        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// 割り当て済みメモリサイズ取得
        virtual uint64 GetAllocatedMemory() const = 0;

        /// 使用中メモリサイズ取得
        virtual uint64 GetUsedMemory() const = 0;

        /// メモリ使用率
        float GetMemoryUsageRatio() const
        {
            uint64 allocated = GetAllocatedMemory();
            return allocated > 0 ? static_cast<float>(GetUsedMemory()) / allocated : 0.0f;
        }

        //=====================================================================
        // 待機フェンス
        // アロケーターの再利用可能タイミング追跡用
        //=====================================================================

        /// 待機フェンス設定
        /// @param fence 待機するフェンス
        /// @param value 待機するフェンス値
        virtual void SetWaitFence(IRHIFence* fence, uint64 value) = 0;

        /// 待機フェンス取得
        virtual IRHIFence* GetWaitFence() const = 0;

        /// 待機フェンス値取得
        virtual uint64 GetWaitFenceValue() const = 0;

        /// 待機完了確認
        bool IsWaitComplete() const;
    };

    //=========================================================================
    // IRHICommandAllocatorPool
    //=========================================================================

    /// コマンドアロケータープール
    /// アロケーターの再利用管理
    class RHI_API IRHICommandAllocatorPool
    {
    public:
        virtual ~IRHICommandAllocatorPool() = default;

        /// アロケーター取得
        /// @param queueType キュータイプ
        /// @return 利用可能なアロケーター
        virtual IRHICommandAllocator* Obtain(ERHIQueueType queueType) = 0;

        /// アロケーター返却
        /// @param allocator 返却するアロケーター
        /// @param fence 完了時フェンス
        /// @param fenceValue 完了時フェンス値
        virtual void Release(IRHICommandAllocator* allocator, IRHIFence* fence, uint64 fenceValue) = 0;

        /// 完了したアロケーターを再利用可能にする
        /// @return 再利用可能になったアロケーター数
        virtual uint32 ProcessCompletedAllocators() = 0;

        /// プール内のアロケーター数
        virtual uint32 GetPooledCount(ERHIQueueType queueType) const = 0;

        /// 使用中のアロケーター数
        virtual uint32 GetInUseCount(ERHIQueueType queueType) const = 0;
    };

} // namespace NS::RHI
