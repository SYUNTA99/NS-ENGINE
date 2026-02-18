/// @file D3D12Barriers.h
/// @brief D3D12 バリアシステム（Legacy + Enhanced）
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIBarrier.h"
#include "RHI/Public/RHIResourceState.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12BarrierBatcher (Legacy)
    //=========================================================================

    /// D3D12_RESOURCE_BARRIERのバッチ蓄積・一括発行
    /// Transition/UAV/Aliasingを単一配列に蓄積し、
    /// FlushでCommandList->ResourceBarrier()を1回呼び出す。
    class D3D12BarrierBatcher
    {
    public:
        static constexpr uint32 kMaxBatchedBarriers = 64;

        //=====================================================================
        // バリア追加
        //=====================================================================

        /// 遷移バリア追加
        /// @return 追加されたバリア数（0=逆遷移キャンセル, 1=追加, -1=エラー）
        int32 AddTransition(ID3D12Resource* resource,
                            D3D12_RESOURCE_STATES before,
                            D3D12_RESOURCE_STATES after,
                            uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                            D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

        /// UAVバリア追加
        void AddUAV(ID3D12Resource* resource = nullptr);

        /// エイリアシングバリア追加
        void AddAliasing(ID3D12Resource* before, ID3D12Resource* after);

        //=====================================================================
        // RHI型からの変換追加
        //=====================================================================

        /// RHI遷移バリアからD3D12バリアに変換して追加
        void AddTransitionFromRHI(NS::RHI::IRHIResource* resource,
                                  NS::RHI::ERHIResourceState before,
                                  NS::RHI::ERHIResourceState after,
                                  uint32 subresource = NS::RHI::kAllSubresources,
                                  NS::RHI::ERHIBarrierFlags flags = NS::RHI::ERHIBarrierFlags::None);

        //=====================================================================
        // 発行
        //=====================================================================

        /// 蓄積バリアをコマンドリストに発行
        void Flush(ID3D12GraphicsCommandList* cmdList);

        /// バリア蓄積をリセット（発行せずクリア）
        void Reset();

        //=====================================================================
        // 情報
        //=====================================================================

        uint32 GetPendingCount() const { return count_; }
        bool IsEmpty() const { return count_ == 0; }

    private:
        D3D12_RESOURCE_BARRIER barriers_[kMaxBatchedBarriers] = {};
        uint32 count_ = 0;

        bool HasCapacity() const { return count_ < kMaxBatchedBarriers; }
    };

    //=========================================================================
    // D3D12EnhancedBarrierBatcher (Enhanced Barriers — FL 12.2+)
    //=========================================================================

    /// D3D12 Enhanced Barrierバッチャー
    /// Global/Texture/Bufferの3タイプ別配列に蓄積し、
    /// ID3D12GraphicsCommandList7::Barrier()で一括発行。
    class D3D12EnhancedBarrierBatcher
    {
    public:
        static constexpr uint32 kMaxBarriers = 64;

        //=====================================================================
        // バリア追加
        //=====================================================================

        /// グローバルバリア追加
        void AddGlobal(D3D12_BARRIER_SYNC syncBefore,
                       D3D12_BARRIER_SYNC syncAfter,
                       D3D12_BARRIER_ACCESS accessBefore,
                       D3D12_BARRIER_ACCESS accessAfter);

        /// テクスチャバリア追加
        void AddTexture(ID3D12Resource* resource,
                        D3D12_BARRIER_SYNC syncBefore,
                        D3D12_BARRIER_SYNC syncAfter,
                        D3D12_BARRIER_ACCESS accessBefore,
                        D3D12_BARRIER_ACCESS accessAfter,
                        D3D12_BARRIER_LAYOUT layoutBefore,
                        D3D12_BARRIER_LAYOUT layoutAfter,
                        uint32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                        D3D12_TEXTURE_BARRIER_FLAGS flags = D3D12_TEXTURE_BARRIER_FLAG_NONE);

        /// バッファバリア追加
        void AddBuffer(ID3D12Resource* resource,
                       D3D12_BARRIER_SYNC syncBefore,
                       D3D12_BARRIER_SYNC syncAfter,
                       D3D12_BARRIER_ACCESS accessBefore,
                       D3D12_BARRIER_ACCESS accessAfter,
                       uint64 offset = 0,
                       uint64 size = UINT64_MAX);

        //=====================================================================
        // RHI型からの変換追加
        //=====================================================================

        /// RHI Enhanced Barrier記述から追加
        void AddFromRHI(const NS::RHI::RHIEnhancedBarrierDesc& desc);

        //=====================================================================
        // 発行
        //=====================================================================

        /// 蓄積バリアをコマンドリスト7に発行
        void Flush(ID3D12GraphicsCommandList7* cmdList);

        /// リセット
        void Reset();

        //=====================================================================
        // 情報
        //=====================================================================

        uint32 GetTotalPendingCount() const { return globalCount_ + textureCount_ + bufferCount_; }
        bool IsEmpty() const { return GetTotalPendingCount() == 0; }

    private:
        D3D12_GLOBAL_BARRIER globalBarriers_[kMaxBarriers] = {};
        uint32 globalCount_ = 0;

        D3D12_TEXTURE_BARRIER textureBarriers_[kMaxBarriers] = {};
        uint32 textureCount_ = 0;

        D3D12_BUFFER_BARRIER bufferBarriers_[kMaxBarriers] = {};
        uint32 bufferCount_ = 0;
    };

    //=========================================================================
    // Enhanced Barrier 変換ヘルパー
    //=========================================================================

    /// ERHIBarrierSync → D3D12_BARRIER_SYNC 変換
    D3D12_BARRIER_SYNC ConvertBarrierSync(NS::RHI::ERHIBarrierSync sync);

    /// ERHIBarrierAccess → D3D12_BARRIER_ACCESS 変換
    D3D12_BARRIER_ACCESS ConvertBarrierAccess(NS::RHI::ERHIBarrierAccess access);

    /// ERHIBarrierLayout → D3D12_BARRIER_LAYOUT 変換
    D3D12_BARRIER_LAYOUT ConvertBarrierLayout(NS::RHI::ERHIBarrierLayout layout);

    //=========================================================================
    // Legacy Barrier ヘルパー
    //=========================================================================

    /// ERHIBarrierFlags → D3D12_RESOURCE_BARRIER_FLAGS 変換
    D3D12_RESOURCE_BARRIER_FLAGS ConvertBarrierFlags(NS::RHI::ERHIBarrierFlags flags);

    /// IRHIResource → ID3D12Resource ヘルパー（Buffer/Texture対応）
    ID3D12Resource* GetD3D12Resource(NS::RHI::IRHIResource* resource);

} // namespace NS::D3D12RHI
