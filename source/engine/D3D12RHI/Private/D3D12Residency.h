/// @file D3D12Residency.h
/// @brief D3D12 レジデンシー管理
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIResidency.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12ResidencyManager
    //=========================================================================

    /// D3D12バックエンドのレジデンシー管理
    /// IDXGIAdapter3::QueryVideoMemoryInfo + ID3D12Device::MakeResident/Evict
    class D3D12ResidencyManager
    {
    public:
        D3D12ResidencyManager() = default;
        ~D3D12ResidencyManager();

        /// 初期化
        bool Init(D3D12Device* device, IDXGIAdapter1* adapter);

        /// シャットダウン
        void Shutdown();

        /// メモリ予算情報を更新
        void UpdateMemoryBudget();

        /// リソースを常駐状態にする
        bool MakeResident(ID3D12Pageable* const* objects, uint32 count);

        /// リソースを非常駐状態にする
        bool Evict(ID3D12Pageable* const* objects, uint32 count);

        /// メモリ統計
        uint64 GetDedicatedBudget() const { return dedicatedBudget_; }
        uint64 GetDedicatedUsage() const { return dedicatedUsage_; }
        uint64 GetSharedBudget() const { return sharedBudget_; }
        uint64 GetSharedUsage() const { return sharedUsage_; }
        uint64 GetDedicatedVideoMemory() const { return dedicatedVideoMemory_; }
        uint64 GetSharedSystemMemory() const { return sharedSystemMemory_; }

        /// メモリ圧迫コールバック
        using MemoryPressureCallback = void (*)(uint64 bytesNeeded);
        void SetMemoryPressureCallback(MemoryPressureCallback callback) { pressureCallback_ = callback; }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<IDXGIAdapter3> adapter3_;

        uint64 dedicatedBudget_ = 0;
        uint64 dedicatedUsage_ = 0;
        uint64 sharedBudget_ = 0;
        uint64 sharedUsage_ = 0;
        uint64 dedicatedVideoMemory_ = 0;
        uint64 sharedSystemMemory_ = 0;

        MemoryPressureCallback pressureCallback_ = nullptr;
    };

} // namespace NS::D3D12RHI
