/// @file D3D12Fence.cpp
/// @brief D3D12フェンス実装
#include "D3D12Fence.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // コンストラクタ / デストラクタ
    //=========================================================================

    D3D12Fence::D3D12Fence() : NS::RHI::IRHIFence() {}

    D3D12Fence::~D3D12Fence()
    {
        if (fenceEvent_)
        {
            CloseHandle(fenceEvent_);
            fenceEvent_ = nullptr;
        }
    }

    //=========================================================================
    // 初期化
    //=========================================================================

    bool D3D12Fence::Init(D3D12Device* device, uint64 initialValue, D3D12_FENCE_FLAGS flags)
    {
        device_ = device;

        ID3D12Device* d3dDevice = device->GetD3DDevice();
        if (!d3dDevice)
            return false;

        HRESULT hr = d3dDevice->CreateFence(initialValue, flags, IID_PPV_ARGS(&fence_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] D3D12Fence::Init CreateFence failed");
            return false;
        }

        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_)
        {
            LOG_ERROR("[D3D12RHI] D3D12Fence::Init CreateEvent failed");
            return false;
        }

        lastSignaledValue_.store(initialValue, std::memory_order_relaxed);
        return true;
    }

    //=========================================================================
    // 基本プロパティ
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12Fence::GetDevice() const
    {
        return device_;
    }

    uint64 D3D12Fence::GetCompletedValue() const
    {
        if (!fence_)
            return 0;
        return fence_->GetCompletedValue();
    }

    uint64 D3D12Fence::GetLastSignaledValue() const
    {
        return lastSignaledValue_.load(std::memory_order_acquire);
    }

    //=========================================================================
    // シグナル・待機
    //=========================================================================

    void D3D12Fence::Signal(uint64 value)
    {
        if (!fence_)
            return;
        fence_->Signal(value);
        lastSignaledValue_.store(value, std::memory_order_release);
    }

    bool D3D12Fence::Wait(uint64 value, uint64 timeoutMs)
    {
        if (!fence_)
            return false;

        if (fence_->GetCompletedValue() >= value)
            return true;

        fence_->SetEventOnCompletion(value, fenceEvent_);
        DWORD waitMs = (timeoutMs == UINT64_MAX) ? INFINITE : static_cast<DWORD>(timeoutMs);
        DWORD result = WaitForSingleObject(fenceEvent_, waitMs);
        return result == WAIT_OBJECT_0;
    }

    bool D3D12Fence::WaitAny(const uint64* values, uint32 count, uint64 timeoutMs)
    {
        if (!fence_ || !values || count == 0)
            return false;

        // 最小値を待つ（いずれかが完了すれば他も完了済み or 不要）
        uint64 minValue = values[0];
        for (uint32 i = 1; i < count; ++i)
        {
            if (values[i] < minValue)
                minValue = values[i];
        }
        return Wait(minValue, timeoutMs);
    }

    bool D3D12Fence::WaitAll(const uint64* values, uint32 count, uint64 timeoutMs)
    {
        if (!fence_ || !values || count == 0)
            return false;

        // 最大値を待つ（全値が完了＝最大値が完了）
        uint64 maxValue = values[0];
        for (uint32 i = 1; i < count; ++i)
        {
            if (values[i] > maxValue)
                maxValue = values[i];
        }
        return Wait(maxValue, timeoutMs);
    }

    //=========================================================================
    // イベント
    //=========================================================================

    void D3D12Fence::SetEventOnCompletion(uint64 value, void* eventHandle)
    {
        if (fence_ && eventHandle)
        {
            fence_->SetEventOnCompletion(value, eventHandle);
        }
    }

    //=========================================================================
    // 共有
    //=========================================================================

    void* D3D12Fence::GetSharedHandle() const
    {
        // 共有フェンスは将来実装
        return nullptr;
    }

} // namespace NS::D3D12RHI
