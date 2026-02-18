/// @file D3D12Queue.cpp
/// @brief D3D12コマンドキュー実装
#include "D3D12Queue.h"
#include "D3D12Device.h"
#include "D3D12Fence.h"
#include <cstdio>
#include <cstring>

namespace NS::D3D12RHI
{
    //=========================================================================
    // 初期化 / シャットダウン
    //=========================================================================

    bool D3D12Queue::Init(D3D12Device* device, NS::RHI::ERHIQueueType queueType, uint32 queueIndex)
    {
        device_ = device;
        queueType_ = queueType;
        queueIndex_ = queueIndex;

        ID3D12Device* d3dDevice = device->GetD3DDevice();
        if (!d3dDevice)
            return false;

        // コマンドキュー作成
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = ToD3D12CommandListType(queueType);
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT hr = d3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue_));
        if (FAILED(hr))
        {
            char buf[128];
            snprintf(buf,
                     sizeof(buf),
                     "D3D12Queue: CreateCommandQueue failed (type=%s, hr=0x%08lX)",
                     NS::RHI::GetQueueTypeName(queueType),
                     hr);
            LOG_ERROR(std::string(buf));
            return false;
        }

        // キュー専用フェンス作成
        queueFence_ = new D3D12Fence();
        if (!queueFence_->Init(device, 0))
        {
            LOG_ERROR(std::string("D3D12Queue: CreateFence failed"));
            delete queueFence_;
            queueFence_ = nullptr;
            return false;
        }

        // デバッグ名設定
        const char* typeName = NS::RHI::GetQueueTypeName(queueType);
        snprintf(description_, sizeof(description_), "D3D12 %s Queue[%u]", typeName, queueIndex);

        // ネイティブオブジェクトにデバッグ名を設定
        wchar_t wname[64];
        size_t nameLen = strlen(description_);
        for (size_t i = 0; i <= nameLen && i < 63; ++i)
            wname[i] = static_cast<wchar_t>(description_[i]);
        wname[63] = L'\0';
        commandQueue_->SetName(wname);

        char buf[128];
        snprintf(buf, sizeof(buf), "D3D12Queue: %s created", description_);
        LOG_INFO(std::string(buf));

        return true;
    }

    void D3D12Queue::Shutdown()
    {
        // GPU完了待ち
        if (commandQueue_ && queueFence_ && lastSubmittedFenceValue_ > 0)
        {
            Flush();
        }

        if (queueFence_)
        {
            queueFence_->Release();
            queueFence_ = nullptr;
        }

        commandQueue_.Reset();
        device_ = nullptr;
    }

    //=========================================================================
    // 基本プロパティ
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12Queue::GetDevice() const
    {
        return device_;
    }

    bool D3D12Queue::SupportsTimestampQueries() const
    {
        // Graphics/Computeキューのみタイムスタンプ対応
        return queueType_ != NS::RHI::ERHIQueueType::Copy;
    }

    bool D3D12Queue::SupportsTileMapping() const
    {
        // Graphicsキューのみタイルマッピング対応
        return queueType_ == NS::RHI::ERHIQueueType::Graphics;
    }

    //=========================================================================
    // コマンド実行
    //=========================================================================

    void D3D12Queue::ExecuteCommandLists(NS::RHI::IRHICommandList* const* /*commandLists*/, uint32 /*count*/)
    {
        // サブ計画11(CommandList)で実装
        // D3D12CommandListからID3D12CommandListを取得してExecuteCommandListsを呼ぶ
    }

    void D3D12Queue::ExecuteContext(NS::RHI::IRHICommandContext* /*context*/)
    {
        // サブ計画12(CommandContext)で実装
    }

    //=========================================================================
    // 同期
    //=========================================================================

    void D3D12Queue::Signal(NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (!commandQueue_ || !fence)
            return;

        auto* d3dFence = static_cast<D3D12Fence*>(fence);
        ID3D12Fence* nativeFence = d3dFence->GetD3DFence();
        if (nativeFence)
        {
            commandQueue_->Signal(nativeFence, value);
        }
    }

    void D3D12Queue::Wait(NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (!commandQueue_ || !fence)
            return;

        auto* d3dFence = static_cast<D3D12Fence*>(fence);
        ID3D12Fence* nativeFence = d3dFence->GetD3DFence();
        if (nativeFence)
        {
            commandQueue_->Wait(nativeFence, value);
        }
    }

    void D3D12Queue::Flush()
    {
        if (!commandQueue_ || !queueFence_)
            return;

        // 現在のフェンス値でシグナル
        uint64 fenceValue = nextFenceValue_++;
        ID3D12Fence* nativeFence = queueFence_->GetD3DFence();
        HRESULT hr = commandQueue_->Signal(nativeFence, fenceValue);
        if (FAILED(hr))
            return;

        lastSubmittedFenceValue_ = fenceValue;

        // CPU側で完了待ち
        queueFence_->Wait(fenceValue, UINT64_MAX);
    }

    //=========================================================================
    // タイミング
    //=========================================================================

    uint64 D3D12Queue::GetGPUTimestamp() const
    {
        if (!commandQueue_)
            return 0;

        uint64 timestamp = 0;
        uint64 frequency = 0;
        commandQueue_->GetClockCalibration(&timestamp, &frequency);
        return timestamp;
    }

    uint64 D3D12Queue::GetTimestampFrequency() const
    {
        if (!commandQueue_)
            return 0;

        uint64 frequency = 0;
        commandQueue_->GetTimestampFrequency(&frequency);
        return frequency;
    }

    //=========================================================================
    // 診断
    //=========================================================================

    const char* D3D12Queue::GetDescription() const
    {
        return description_;
    }

    void D3D12Queue::InsertDebugMarker(const char* /*name*/, uint32 /*color*/)
    {
        // PIXマーカー — サブ計画11で実装
    }

    void D3D12Queue::BeginDebugEvent(const char* /*name*/, uint32 /*color*/)
    {
        // PIXマーカー — サブ計画11で実装
    }

    void D3D12Queue::EndDebugEvent()
    {
        // PIXマーカー — サブ計画11で実装
    }

    //=========================================================================
    // キュー専用フェンス
    //=========================================================================

    NS::RHI::IRHIFence* D3D12Queue::GetFence() const
    {
        return queueFence_;
    }

    uint64 D3D12Queue::GetLastCompletedFenceValue() const
    {
        if (!queueFence_)
            return 0;
        return queueFence_->GetCompletedValue();
    }

    uint64 D3D12Queue::AdvanceFence()
    {
        if (!commandQueue_ || !queueFence_)
            return 0;

        uint64 fenceValue = nextFenceValue_++;
        ID3D12Fence* nativeFence = queueFence_->GetD3DFence();
        HRESULT hr = commandQueue_->Signal(nativeFence, fenceValue);
        if (FAILED(hr))
            return 0;

        lastSubmittedFenceValue_ = fenceValue;
        return fenceValue;
    }

    //=========================================================================
    // フェンス待ち
    //=========================================================================

    void* D3D12Queue::GetFenceEventHandle() const
    {
        // D3D12Fenceの内部イベントは非公開。外部使用不可。
        return nullptr;
    }

    bool D3D12Queue::WaitForFence(uint64 fenceValue, uint32 timeoutMs)
    {
        if (!queueFence_)
            return false;

        uint64 timeout = (timeoutMs == 0) ? UINT64_MAX : static_cast<uint64>(timeoutMs);
        return queueFence_->Wait(fenceValue, timeout);
    }

    //=========================================================================
    // キュー間同期
    //=========================================================================

    void D3D12Queue::WaitForQueue(NS::RHI::IRHIQueue* otherQueue, uint64 fenceValue)
    {
        if (!otherQueue || !commandQueue_)
            return;

        auto* other = static_cast<D3D12Queue*>(otherQueue);
        if (other->queueFence_)
        {
            ID3D12Fence* nativeFence = other->queueFence_->GetD3DFence();
            if (nativeFence)
            {
                commandQueue_->Wait(nativeFence, fenceValue);
            }
        }
    }

    void D3D12Queue::WaitForExternalFence(NS::RHI::IRHIFence* fence, uint64 value)
    {
        if (!commandQueue_ || !fence)
            return;

        auto* d3dFence = static_cast<D3D12Fence*>(fence);
        ID3D12Fence* nativeFence = d3dFence->GetD3DFence();
        if (nativeFence)
        {
            commandQueue_->Wait(nativeFence, value);
        }
    }

    //=========================================================================
    // 統計
    //=========================================================================

    void D3D12Queue::ResetStats()
    {
        stats_ = {};
    }

    //=========================================================================
    // GPU診断
    //=========================================================================

    void D3D12Queue::EnableGPUCrashDump(bool /*enable*/)
    {
        // DRED/Aftermath統合 — サブ計画39で実装
    }

    void D3D12Queue::InsertBreadcrumb(uint32 /*value*/)
    {
        // Breadcrumb — サブ計画39で実装
    }

} // namespace NS::D3D12RHI
