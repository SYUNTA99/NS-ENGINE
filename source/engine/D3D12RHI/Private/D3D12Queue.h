/// @file D3D12Queue.h
/// @brief D3D12コマンドキュー — IRHIQueue実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIQueue.h"

namespace NS::D3D12RHI
{
    class D3D12Device;
    class D3D12Fence;

    //=========================================================================
    // D3D12Queue — IRHIQueue実装
    //=========================================================================

    class D3D12Queue final : public NS::RHI::IRHIQueue
    {
    public:
        D3D12Queue() = default;
        ~D3D12Queue() override = default;

        /// 初期化
        bool Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType, uint32 queueIndex);

        /// シャットダウン
        void Shutdown();

        /// ネイティブキュー取得
        ID3D12CommandQueue* GetD3DCommandQueue() const { return commandQueue_.Get(); }

        /// D3D12Fence取得
        D3D12Fence* GetD3D12Fence() const { return queueFence_; }

        //=====================================================================
        // IRHIQueue — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIQueueType GetQueueType() const override { return queueType_; }
        uint32 GetQueueIndex() const override { return queueIndex_; }
        bool SupportsTimestampQueries() const override;
        bool SupportsTileMapping() const override;

        //=====================================================================
        // IRHIQueue — コマンド実行
        //=====================================================================

        void ExecuteCommandLists(NS::RHI::IRHICommandList* const* commandLists, uint32 count) override;
        void ExecuteContext(NS::RHI::IRHICommandContext* context) override;

        //=====================================================================
        // IRHIQueue — 同期
        //=====================================================================

        void Signal(NS::RHI::IRHIFence* fence, uint64 value) override;
        void Wait(NS::RHI::IRHIFence* fence, uint64 value) override;
        void Flush() override;

        //=====================================================================
        // IRHIQueue — タイミング
        //=====================================================================

        uint64 GetGPUTimestamp() const override;
        uint64 GetTimestampFrequency() const override;

        //=====================================================================
        // IRHIQueue — 診断
        //=====================================================================

        const char* GetDescription() const override;
        void InsertDebugMarker(const char* name, uint32 color) override;
        void BeginDebugEvent(const char* name, uint32 color) override;
        void EndDebugEvent() override;

        //=====================================================================
        // IRHIQueue — キュー専用フェンス
        //=====================================================================

        NS::RHI::IRHIFence* GetFence() const override;
        uint64 GetLastSubmittedFenceValue() const override { return lastSubmittedFenceValue_; }
        uint64 GetLastCompletedFenceValue() const override;
        uint64 AdvanceFence() override;

        //=====================================================================
        // IRHIQueue — フェンス待ち
        //=====================================================================

        bool WaitForFence(uint64 fenceValue, uint32 timeoutMs) override;
        void* GetFenceEventHandle() const override;

        //=====================================================================
        // IRHIQueue — キュー間同期
        //=====================================================================

        void WaitForQueue(NS::RHI::IRHIQueue* otherQueue, uint64 fenceValue) override;
        void WaitForExternalFence(NS::RHI::IRHIFence* fence, uint64 value) override;

        //=====================================================================
        // IRHIQueue — 統計
        //=====================================================================

        NS::RHI::RHIQueueStats GetStats() const override { return stats_; }
        void ResetStats() override;

        //=====================================================================
        // IRHIQueue — GPU診断
        //=====================================================================

        void EnableGPUCrashDump(bool enable) override;
        void InsertBreadcrumb(uint32 value) override;

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::ERHIQueueType queueType_ = NS::RHI::ERHIQueueType::Graphics;
        uint32 queueIndex_ = 0;

        ComPtr<ID3D12CommandQueue> commandQueue_;

        // キュー専用フェンス（D3D12Fence所有）
        D3D12Fence* queueFence_ = nullptr;
        uint64 nextFenceValue_ = 1;
        uint64 lastSubmittedFenceValue_ = 0;

        // 統計
        NS::RHI::RHIQueueStats stats_{};

        // デバッグ名
        char description_[64] = {};
    };

    //=========================================================================
    // D3D12 ERHIQueueType → D3D12_COMMAND_LIST_TYPE 変換
    //=========================================================================

    inline D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(NS::RHI::ERHIQueueType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIQueueType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case NS::RHI::ERHIQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case NS::RHI::ERHIQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

} // namespace NS::D3D12RHI
