/// @file D3D12Fence.h
/// @brief D3D12フェンス — IRHIFence実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIFence.h"

#include <atomic>

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12Fence — IRHIFence実装
    //=========================================================================

    class D3D12Fence final : public NS::RHI::IRHIFence
    {
    public:
        D3D12Fence();
        ~D3D12Fence() override;

        /// 初期化
        bool Init(D3D12Device* device, uint64 initialValue, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);

        /// ネイティブフェンス取得
        ID3D12Fence* GetD3DFence() const { return fence_.Get(); }

        //=====================================================================
        // IRHIFence — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        uint64 GetCompletedValue() const override;
        uint64 GetLastSignaledValue() const override;

        //=====================================================================
        // IRHIFence — シグナル・待機
        //=====================================================================

        void Signal(uint64 value) override;
        bool Wait(uint64 value, uint64 timeoutMs) override;
        bool WaitAny(const uint64* values, uint32 count, uint64 timeoutMs) override;
        bool WaitAll(const uint64* values, uint32 count, uint64 timeoutMs) override;

        //=====================================================================
        // IRHIFence — イベント
        //=====================================================================

        void SetEventOnCompletion(uint64 value, void* eventHandle) override;

        //=====================================================================
        // IRHIFence — 共有
        //=====================================================================

        void* GetSharedHandle() const override;

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12Fence> fence_;
        void* fenceEvent_ = nullptr;
        std::atomic<uint64> lastSignaledValue_{0};
    };

} // namespace NS::D3D12RHI
