/// @file D3D12Bindless.h
/// @brief D3D12 Bindlessディスクリプタ管理
#pragma once

#include "D3D12Descriptors.h"
#include "D3D12RHIPrivate.h"

#include "RHI/Public/RHITypes.h"

#include <mutex>
#include <vector>

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12BindlessDescriptorHeap — 永続GPU可視ヒープ管理
    //=========================================================================

    /// CBV/SRV/UAV用のGPU可視ディスクリプタヒープ。
    /// フリーリストでインデックスを管理し、ディスクリプタのコピーを行う。
    class D3D12BindlessDescriptorHeap
    {
    public:
        static constexpr uint32 kMaxDescriptors = 1000000;

        D3D12BindlessDescriptorHeap() = default;
        ~D3D12BindlessDescriptorHeap() = default;

        /// 初期化: GPU可視 CBV_SRV_UAV ヒープ作成
        bool Init(D3D12Device* device, uint32 numDescriptors = kMaxDescriptors);

        /// ディスクリプタインデックス割り当て
        NS::RHI::BindlessIndex Allocate();

        /// ディスクリプタインデックス解放
        void Free(NS::RHI::BindlessIndex index);

        /// ディスクリプタコピー（CPUハンドル→ヒープ内インデックス）
        void CopyToIndex(NS::RHI::BindlessIndex index, D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle);

        /// ネイティブヒープ取得
        ID3D12DescriptorHeap* GetD3DHeap() const { return heap_.Get(); }

        /// GPU ハンドル取得
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(NS::RHI::BindlessIndex index) const;

        /// CPU ハンドル取得（コピー先）
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(NS::RHI::BindlessIndex index) const;

        uint32 GetIncrementSize() const { return incrementSize_; }

    private:
        ComPtr<ID3D12DescriptorHeap> heap_;
        ID3D12Device* device_ = nullptr;
        uint32 numDescriptors_ = 0;
        uint32 incrementSize_ = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart_ = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart_ = {};

        /// フリーリスト
        std::vector<uint32> freeList_;
        uint32 nextFreshIndex_ = 0;
        std::mutex mutex_;
    };

    //=========================================================================
    // D3D12BindlessSamplerHeap — サンプラー用永続ヒープ
    //=========================================================================

    class D3D12BindlessSamplerHeap
    {
    public:
        static constexpr uint32 kMaxSamplers = 2048;

        D3D12BindlessSamplerHeap() = default;
        ~D3D12BindlessSamplerHeap() = default;

        bool Init(D3D12Device* device, uint32 numSamplers = kMaxSamplers);

        NS::RHI::BindlessSamplerIndex Allocate();
        void Free(NS::RHI::BindlessSamplerIndex index);

        void CopyToIndex(NS::RHI::BindlessSamplerIndex index, D3D12_CPU_DESCRIPTOR_HANDLE srcCpuHandle);

        ID3D12DescriptorHeap* GetD3DHeap() const { return heap_.Get(); }
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(NS::RHI::BindlessSamplerIndex index) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(NS::RHI::BindlessSamplerIndex index) const;

    private:
        ComPtr<ID3D12DescriptorHeap> heap_;
        ID3D12Device* device_ = nullptr;
        uint32 numSamplers_ = 0;
        uint32 incrementSize_ = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart_ = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart_ = {};

        std::vector<uint32> freeList_;
        uint32 nextFreshIndex_ = 0;
        std::mutex mutex_;
    };

    //=========================================================================
    // D3D12BindlessManager — 統合管理
    //=========================================================================

    /// Bindlessディスクリプタの統合マネージャー。
    /// SRV/UAVの割り当て・解放とコンテキストへのヒープ設定を行う。
    class D3D12BindlessManager
    {
    public:
        D3D12BindlessManager() = default;
        ~D3D12BindlessManager() = default;

        bool Init(D3D12Device* device);
        void Shutdown();

        /// SRV割り当て: ビューのCPUハンドルをBindlessヒープにコピー
        NS::RHI::BindlessSRVIndex AllocateSRV(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle);

        /// UAV割り当て: ビューのCPUハンドルをBindlessヒープにコピー
        NS::RHI::BindlessUAVIndex AllocateUAV(D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle);

        /// SRV解放
        void FreeSRV(NS::RHI::BindlessSRVIndex index);

        /// UAV解放
        void FreeUAV(NS::RHI::BindlessUAVIndex index);

        /// ヒープ取得
        D3D12BindlessDescriptorHeap& GetResourceHeap() { return resourceHeap_; }
        D3D12BindlessSamplerHeap& GetSamplerHeap() { return samplerHeap_; }

        /// IRHIDescriptorHeapラッパー取得（RHIBindlessResourceTable用）
        D3D12DescriptorHeap* GetResourceHeapWrapper() { return &resourceHeapWrapper_; }
        D3D12DescriptorHeap* GetSamplerHeapWrapper() { return &samplerHeapWrapper_; }

        /// コマンドリストにBindlessヒープを設定
        void SetHeapsOnCommandList(ID3D12GraphicsCommandList* cmdList);

    private:
        D3D12Device* device_ = nullptr;
        D3D12BindlessDescriptorHeap resourceHeap_;
        D3D12BindlessSamplerHeap samplerHeap_;

        /// IRHIDescriptorHeapとして公開するためのラッパー
        D3D12DescriptorHeap resourceHeapWrapper_;
        D3D12DescriptorHeap samplerHeapWrapper_;
    };

} // namespace NS::D3D12RHI
