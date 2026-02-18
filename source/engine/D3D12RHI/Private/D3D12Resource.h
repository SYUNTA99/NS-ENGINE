/// @file D3D12Resource.h
/// @brief D3D12 GPU リソースラッパー
/// @details ID3D12Resourceの内部ラッパー。状態追跡、GPU仮想アドレス、Map/Unmapを提供。
///          D3D12Buffer/D3D12Texture等のGPUリソースクラスがメンバーとして保持する。
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIResourceState.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12GpuResource — ID3D12Resourceラッパー
    //=========================================================================

    class D3D12GpuResource
    {
    public:
        D3D12GpuResource() = default;
        ~D3D12GpuResource();

        D3D12GpuResource(const D3D12GpuResource&) = delete;
        D3D12GpuResource& operator=(const D3D12GpuResource&) = delete;

        D3D12GpuResource(D3D12GpuResource&& other) noexcept;
        D3D12GpuResource& operator=(D3D12GpuResource&& other) noexcept;

        //=====================================================================
        // 初期化
        //=====================================================================

        /// 既存のID3D12Resourceから初期化
        void InitFromExisting(D3D12Device* device,
                              ComPtr<ID3D12Resource> resource,
                              D3D12_HEAP_TYPE heapType,
                              NS::RHI::ERHIResourceState initialState,
                              uint32 subresourceCount = 1);

        /// CommittedResourceを作成して初期化
        bool InitCommitted(D3D12Device* device,
                           const D3D12_HEAP_PROPERTIES& heapProps,
                           D3D12_HEAP_FLAGS heapFlags,
                           const D3D12_RESOURCE_DESC& desc,
                           NS::RHI::ERHIResourceState initialState,
                           const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr);

        /// リソース解放
        void Release();

        //=====================================================================
        // アクセサ
        //=====================================================================

        /// ネイティブリソース取得
        ID3D12Resource* GetD3DResource() const { return resource_.Get(); }

        /// ネイティブリソースComPtr取得
        const ComPtr<ID3D12Resource>& GetD3DResourceComPtr() const { return resource_; }

        /// デバイス取得
        D3D12Device* GetDevice() const { return device_; }

        /// リソースデスクリプタ取得
        D3D12_RESOURCE_DESC GetDesc() const;

        /// ヒープタイプ取得
        D3D12_HEAP_TYPE GetHeapType() const { return heapType_; }

        /// 有効か
        bool IsValid() const { return resource_ != nullptr; }

        //=====================================================================
        // GPU仮想アドレス
        //=====================================================================

        /// GPU仮想アドレス取得（バッファのみ有効）
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return gpuVirtualAddress_; }

        //=====================================================================
        // 状態追跡
        //=====================================================================

        /// 状態マップ取得
        NS::RHI::RHIResourceStateMap& GetStateMap() { return stateMap_; }
        const NS::RHI::RHIResourceStateMap& GetStateMap() const { return stateMap_; }

        /// 現在の統一状態取得（全サブリソースが同じ場合）
        NS::RHI::ERHIResourceState GetCurrentState() const;

        /// 状態追跡が必要か
        bool RequiresStateTracking() const { return requiresStateTracking_; }

        /// 状態追跡フラグ設定
        void SetRequiresStateTracking(bool value) { requiresStateTracking_ = value; }

        //=====================================================================
        // Map/Unmap
        //=====================================================================

        /// CPUアドレスにマップ
        /// @return マップされたアドレス、失敗時nullptr
        void* Map(uint32 subresource = 0, const D3D12_RANGE* readRange = nullptr);

        /// マップ解除
        void Unmap(uint32 subresource = 0, const D3D12_RANGE* writtenRange = nullptr);

        /// マップ済みアドレス取得（Map呼び出し後に有効）
        void* GetMappedAddress() const { return mappedAddress_; }

        /// マップ中か
        bool IsMapped() const { return mapCount_ > 0; }

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名設定
        void SetDebugName(const char* name);

        /// RHIリソース状態をD3D12状態に変換
        static D3D12_RESOURCE_STATES ConvertToD3D12State(NS::RHI::ERHIResourceState state);

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12Resource> resource_;
        D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress_ = 0;

        // 状態追跡
        NS::RHI::RHIResourceStateMap stateMap_;
        bool requiresStateTracking_ = true;

        // Map/Unmap
        void* mappedAddress_ = nullptr;
        int32 mapCount_ = 0;
    };

} // namespace NS::D3D12RHI
