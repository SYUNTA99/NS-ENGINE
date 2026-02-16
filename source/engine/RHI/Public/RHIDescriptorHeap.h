/// @file RHIDescriptorHeap.h
/// @brief ディスクリプタヒープインターフェースとアロケーター
/// @details ディスクリプタヒープの作成、管理、割り当て/解放を提供。
/// @see 10-01-descriptor-heap.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIDescriptorHeapFlags (10-01)
    //=========================================================================

    /// ディスクリプタヒープフラグ
    enum class ERHIDescriptorHeapFlags : uint32
    {
        None = 0,

        /// シェーダーから参照可能（GPUヒープ）
        ShaderVisible = 1 << 0,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIDescriptorHeapFlags)

    //=========================================================================
    // RHIDescriptorHeapDesc (10-01)
    //=========================================================================

    /// ディスクリプタヒープ記述
    struct RHI_API RHIDescriptorHeapDesc
    {
        /// ヒープタイプ
        ERHIDescriptorHeapType type = ERHIDescriptorHeapType::CBV_SRV_UAV;

        /// ディスクリプタ数
        uint32 numDescriptors = 0;

        /// フラグ
        ERHIDescriptorHeapFlags flags = ERHIDescriptorHeapFlags::None;

        /// ノードマスク（マルチGPU用）
        uint32 nodeMask = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// CBV/SRV/UAVヒープ
        static RHIDescriptorHeapDesc CBVSRVUAV(uint32 count, bool shaderVisible = true)
        {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::CBV_SRV_UAV;
            desc.numDescriptors = count;
            desc.flags = shaderVisible ? ERHIDescriptorHeapFlags::ShaderVisible : ERHIDescriptorHeapFlags::None;
            return desc;
        }

        /// サンプラーヒープ
        static RHIDescriptorHeapDesc Sampler(uint32 count, bool shaderVisible = true)
        {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::Sampler;
            desc.numDescriptors = count;
            desc.flags = shaderVisible ? ERHIDescriptorHeapFlags::ShaderVisible : ERHIDescriptorHeapFlags::None;
            return desc;
        }

        /// RTVヒープ
        static RHIDescriptorHeapDesc RTV(uint32 count)
        {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::RTV;
            desc.numDescriptors = count;
            desc.flags = ERHIDescriptorHeapFlags::None;
            return desc;
        }

        /// DSVヒープ
        static RHIDescriptorHeapDesc DSV(uint32 count)
        {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::DSV;
            desc.numDescriptors = count;
            desc.flags = ERHIDescriptorHeapFlags::None;
            return desc;
        }
    };

    //=========================================================================
    // IRHIDescriptorHeap (10-01)
    //=========================================================================

    /// ディスクリプタヒープ
    class RHI_API IRHIDescriptorHeap : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(DescriptorHeap)

        virtual ~IRHIDescriptorHeap() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// ヒープタイプ取得
        virtual ERHIDescriptorHeapType GetType() const = 0;

        /// ディスクリプタ数取得
        virtual uint32 GetNumDescriptors() const = 0;

        /// シェーダー可視か
        virtual bool IsShaderVisible() const = 0;

        /// ディスクリプタインクリメントサイズ取得
        virtual uint32 GetDescriptorIncrementSize() const = 0;

        //=====================================================================
        // ハンドル取得
        //=====================================================================

        /// CPUハンドル取得（先頭）
        virtual RHICPUDescriptorHandle GetCPUDescriptorHandleForHeapStart() const = 0;

        /// GPUハンドル取得（先頭、シェーダー可視のみ）
        virtual RHIGPUDescriptorHandle GetGPUDescriptorHandleForHeapStart() const = 0;

        /// 指定インデックスのCPUハンドル取得
        RHICPUDescriptorHandle GetCPUDescriptorHandle(uint32 index) const
        {
            RHICPUDescriptorHandle handle = GetCPUDescriptorHandleForHeapStart();
            handle.ptr += static_cast<size_t>(index) * GetDescriptorIncrementSize();
            return handle;
        }

        /// 指定インデックスのGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUDescriptorHandle(uint32 index) const
        {
            RHIGPUDescriptorHandle handle = GetGPUDescriptorHandleForHeapStart();
            handle.ptr += static_cast<uint64>(index) * GetDescriptorIncrementSize();
            return handle;
        }
    };

    using RHIDescriptorHeapRef = TRefCountPtr<IRHIDescriptorHeap>;

    //=========================================================================
    // RHIDescriptorAllocation (10-01)
    //=========================================================================

    /// ディスクリプタアロケーション
    struct RHI_API RHIDescriptorAllocation
    {
        /// CPUハンドル
        RHICPUDescriptorHandle cpuHandle = {};

        /// GPUハンドル（シェーダー可視時のみ有効）
        RHIGPUDescriptorHandle gpuHandle = {};

        /// ヒープ内インデックス
        uint32 heapIndex = 0;

        /// 連続ディスクリプタ数
        uint32 count = 0;

        /// 所属ヒープ
        IRHIDescriptorHeap* heap = nullptr;

        /// 有効か
        bool IsValid() const { return heap != nullptr && count > 0; }

        /// N番目のCPUハンドル取得
        RHICPUDescriptorHandle GetCPUHandle(uint32 offset = 0) const
        {
            RHICPUDescriptorHandle h = cpuHandle;
            h.ptr += static_cast<size_t>(offset) * heap->GetDescriptorIncrementSize();
            return h;
        }

        /// N番目のGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(uint32 offset = 0) const
        {
            RHIGPUDescriptorHandle h = gpuHandle;
            h.ptr += static_cast<uint64>(offset) * heap->GetDescriptorIncrementSize();
            return h;
        }
    };

    //=========================================================================
    // RHIDescriptorHeapAllocator (10-01)
    //=========================================================================

    /// ディスクリプタヒープアロケーター
    /// ヒープからディスクリプタを動的に割り当て
    /// @threadsafety スレッドセーフではない。外部同期が必要。
    class RHI_API RHIDescriptorHeapAllocator
    {
    public:
        RHIDescriptorHeapAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDescriptorHeap* heap);

        /// シャットダウン
        void Shutdown();

        /// ディスクリプタ割り当て
        /// @return 有効な割り当て。枯渇時はIsValid()==falseを返す。
        RHIDescriptorAllocation Allocate(uint32 count = 1);

        /// ディスクリプタ解放
        void Free(const RHIDescriptorAllocation& allocation);

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const { return m_freeCount; }

        /// 総ディスクリプタ数
        uint32 GetTotalCount() const { return m_heap ? m_heap->GetNumDescriptors() : 0; }

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// リセット（全ディスクリプタを解放）
        void Reset();

    private:
        IRHIDescriptorHeap* m_heap = nullptr;

        /// フリーリスト管理
        struct FreeRange
        {
            uint32 start;
            uint32 count;
        };
        FreeRange* m_freeRanges = nullptr;
        uint32 m_freeRangeCount = 0;
        uint32 m_freeRangeCapacity = 0;
        uint32 m_freeCount = 0;
    };

}} // namespace NS::RHI
