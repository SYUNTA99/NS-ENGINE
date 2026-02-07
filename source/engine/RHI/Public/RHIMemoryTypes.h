/// @file RHIMemoryTypes.h
/// @brief GPUメモリヒープ型とインターフェース
/// @details メモリプール、ヒープフラグ、IRHIHeap、配置リソース、エイリアシングバリアを提供。
/// @see 11-01-heap-types.md
#pragma once

#include "IRHIResource.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHIMemoryPool (11-01)
    //=========================================================================

    /// メモリプールタイプ
    enum class ERHIMemoryPool : uint8
    {
        Unknown, ///< デフォルト
        L0,      ///< システムメモリ（CPU側）
        L1,      ///< ビデオメモリ（GPU専用）
    };

    //=========================================================================
    // ERHICPUPageProperty (11-01)
    //=========================================================================

    /// CPUページプロパティ
    enum class ERHICPUPageProperty : uint8
    {
        Unknown,
        NotAvailable, ///< CPUアクセス不可
        WriteCombine, ///< 書き込み結合
        WriteBack,    ///< ライトバック
    };

    //=========================================================================
    // ERHIHeapFlags (11-01)
    //=========================================================================

    /// ヒープフラグ
    enum class ERHIHeapFlags : uint32
    {
        None = 0,
        ShaderVisible = 1 << 0,
        AllowOnlyBuffers = 1 << 1,
        AllowOnlyNonRTDSTextures = 1 << 2,
        AllowOnlyRTDSTextures = 1 << 3,
        DenyMSAATextures = 1 << 4,
        AllowRaytracingAccelerationStructure = 1 << 5,
        CreateNotResident = 1 << 6,
        SharedCrossAdapter = 1 << 7,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIHeapFlags)

    //=========================================================================
    // RHIHeapDesc (11-01)
    //=========================================================================

    /// ヒープ記述
    struct RHI_API RHIHeapDesc
    {
        uint64 sizeInBytes = 0;
        ERHIHeapType type = ERHIHeapType::Default;
        ERHIMemoryPool memoryPool = ERHIMemoryPool::Unknown;
        ERHICPUPageProperty cpuPageProperty = ERHICPUPageProperty::Unknown;
        ERHIHeapFlags flags = ERHIHeapFlags::None;
        uint64 alignment = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIHeapDesc Default(uint64 size, ERHIHeapFlags f = ERHIHeapFlags::None)
        {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Default;
            desc.flags = f;
            return desc;
        }

        static RHIHeapDesc Upload(uint64 size)
        {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Upload;
            return desc;
        }

        static RHIHeapDesc Readback(uint64 size)
        {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Readback;
            return desc;
        }

        static RHIHeapDesc BufferHeap(uint64 size, ERHIHeapType heapType = ERHIHeapType::Default)
        {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = heapType;
            desc.flags = ERHIHeapFlags::AllowOnlyBuffers;
            return desc;
        }

        static RHIHeapDesc TextureHeap(uint64 size, bool allowRT = false)
        {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Default;
            desc.flags = allowRT ? ERHIHeapFlags::AllowOnlyRTDSTextures : ERHIHeapFlags::AllowOnlyNonRTDSTextures;
            return desc;
        }
    };

    //=========================================================================
    // IRHIHeap (11-01)
    //=========================================================================

    /// GPUメモリヒープ
    class RHI_API IRHIHeap : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Heap)

        virtual ~IRHIHeap() = default;

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// サイズ取得
        virtual uint64 GetSize() const = 0;

        /// ヒープタイプ取得
        virtual ERHIHeapType GetType() const = 0;

        /// フラグ取得
        virtual ERHIHeapFlags GetFlags() const = 0;

        /// アライメント取得
        virtual uint64 GetAlignment() const = 0;

        /// 常駐しているか
        virtual bool IsResident() const = 0;

        /// GPUアドレス取得（バッファヒープ用）
        virtual uint64 GetGPUVirtualAddress() const = 0;
    };

    using RHIHeapRef = TRefCountPtr<IRHIHeap>;

} // namespace NS::RHI
