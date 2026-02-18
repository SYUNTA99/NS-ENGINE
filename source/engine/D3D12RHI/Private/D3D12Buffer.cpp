/// @file D3D12Buffer.cpp
/// @brief D3D12 バッファ — IRHIBuffer実装
#include "D3D12Buffer.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // コンストラクタ / デストラクタ
    //=========================================================================

    D3D12Buffer::D3D12Buffer() = default;

    D3D12Buffer::~D3D12Buffer()
    {
        gpuResource_.Release();
        device_ = nullptr;
    }

    //=========================================================================
    // 初期化
    //=========================================================================

    bool D3D12Buffer::Init(D3D12Device* device, const NS::RHI::RHIBufferDesc& desc, const void* initialData)
    {
        if (!device || desc.size == 0)
        {
            return false;
        }

        device_ = device;
        usage_ = desc.usage;
        heapType_ = DetermineHeapType(usage_);

        // ヒーププロパティ
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = heapType_;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // リソースフラグ
        D3D12_RESOURCE_FLAGS resourceFlags = ConvertResourceFlags(usage_);

        // バッファサイズアライメント
        NS::RHI::MemorySize alignedSize = desc.size;
        if (EnumHasAnyFlags(usage_, NS::RHI::ERHIBufferUsage::ConstantBuffer))
        {
            // 定数バッファは256Bアライメント必須
            alignedSize = NS::RHI::AlignUp(
                alignedSize, static_cast<NS::RHI::MemorySize>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
        }
        if (desc.alignment > 0)
        {
            alignedSize = NS::RHI::AlignUp(alignedSize, static_cast<NS::RHI::MemorySize>(desc.alignment));
        }

        // リソースデスクリプタ
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = alignedSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = resourceFlags;

        // 初期状態決定
        NS::RHI::ERHIResourceState initialState = NS::RHI::ERHIResourceState::Common;
        if (heapType_ == D3D12_HEAP_TYPE_UPLOAD)
        {
            initialState = NS::RHI::ERHIResourceState::GenericRead;
        }
        else if (heapType_ == D3D12_HEAP_TYPE_READBACK)
        {
            initialState = NS::RHI::ERHIResourceState::CopyDest;
        }

        // CommittedResource作成
        if (!gpuResource_.InitCommitted(device_, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, initialState))
        {
            LOG_ERROR("[D3D12RHI] Failed to create buffer resource");
            return false;
        }

        // IRHIBuffer protectedメンバー設定
        m_size = desc.size;
        m_stride = desc.stride;
        m_gpuVirtualAddress = gpuResource_.GetGPUVirtualAddress();

        // デバッグ名
        if (desc.debugName)
        {
            SetDebugName(desc.debugName);
        }

        // 初期データアップロード（Uploadヒープのみ直接コピー）
        if (initialData && heapType_ == D3D12_HEAP_TYPE_UPLOAD)
        {
            void* mapped = gpuResource_.Map(0, nullptr);
            if (mapped)
            {
                std::memcpy(mapped, initialData, desc.size);
                gpuResource_.Unmap(0, nullptr);
            }
        }
        // DEFAULTヒープへの初期データはUpload Heap経由（サブ計画18で実装）

        return true;
    }

    //=========================================================================
    // IRHIBuffer — 基本プロパティ
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12Buffer::GetDevice() const
    {
        return device_;
    }

    //=========================================================================
    // IRHIBuffer — メモリ情報
    //=========================================================================

    NS::RHI::RHIBufferMemoryInfo D3D12Buffer::GetMemoryInfo() const
    {
        NS::RHI::RHIBufferMemoryInfo info;
        info.usableSize = m_size;
        info.alignment = 0;
        info.heapOffset = 0;

        // ヒープタイプ変換
        switch (heapType_)
        {
        case D3D12_HEAP_TYPE_UPLOAD:
            info.heapType = NS::RHI::ERHIHeapType::Upload;
            break;
        case D3D12_HEAP_TYPE_READBACK:
            info.heapType = NS::RHI::ERHIHeapType::Readback;
            break;
        default:
            info.heapType = NS::RHI::ERHIHeapType::Default;
            break;
        }

        // 実際に割り当てられたサイズ（D3D12リソースDescから取得）
        if (gpuResource_.IsValid())
        {
            D3D12_RESOURCE_DESC desc = gpuResource_.GetDesc();
            info.allocatedSize = static_cast<NS::RHI::MemorySize>(desc.Width);
        }
        else
        {
            info.allocatedSize = m_size;
        }

        return info;
    }

    //=========================================================================
    // IRHIBuffer — Map/Unmap
    //=========================================================================

    NS::RHI::RHIMapResult D3D12Buffer::Map(NS::RHI::ERHIMapMode mode,
                                           NS::RHI::MemoryOffset offset,
                                           NS::RHI::MemorySize size)
    {
        NS::RHI::RHIMapResult result;

        // 読み取りアクセスの場合のみ読み取り範囲を指定
        D3D12_RANGE readRange{};
        if (NS::RHI::MapModeHasRead(mode))
        {
            readRange.Begin = static_cast<SIZE_T>(offset);
            readRange.End = static_cast<SIZE_T>(offset + (size > 0 ? size : m_size));
        }
        // 書き込み専用の場合は空の読み取り範囲（CPUキャッシュ最適化）

        void* mapped = gpuResource_.Map(0, &readRange);
        if (!mapped)
        {
            return result;
        }

        result.data = static_cast<uint8*>(mapped) + offset;
        result.size = (size > 0) ? size : m_size;
        return result;
    }

    void D3D12Buffer::Unmap(NS::RHI::MemoryOffset offset, NS::RHI::MemorySize size)
    {
        D3D12_RANGE writtenRange{};
        writtenRange.Begin = static_cast<SIZE_T>(offset);
        writtenRange.End = static_cast<SIZE_T>(offset + (size > 0 ? size : m_size));

        gpuResource_.Unmap(0, &writtenRange);
    }

    //=========================================================================
    // IRHIResource — デバッグ
    //=========================================================================

    void D3D12Buffer::SetDebugName(const char* name)
    {
        gpuResource_.SetDebugName(name);
    }

    //=========================================================================
    // ヒープタイプ判定
    //=========================================================================

    D3D12_HEAP_TYPE D3D12Buffer::DetermineHeapType(NS::RHI::ERHIBufferUsage usage)
    {
        using NS::RHI::ERHIBufferUsage;

        // CPU書き込み可能 or Dynamic → UPLOAD
        if (EnumHasAnyFlags(usage, ERHIBufferUsage::CPUWritable | ERHIBufferUsage::Dynamic))
        {
            return D3D12_HEAP_TYPE_UPLOAD;
        }

        // CPU読み取り可能 → READBACK
        if (EnumHasAnyFlags(usage, ERHIBufferUsage::CPUReadable))
        {
            return D3D12_HEAP_TYPE_READBACK;
        }

        // デフォルト → GPU専用
        return D3D12_HEAP_TYPE_DEFAULT;
    }

    //=========================================================================
    // リソースフラグ変換
    //=========================================================================

    D3D12_RESOURCE_FLAGS D3D12Buffer::ConvertResourceFlags(NS::RHI::ERHIBufferUsage usage)
    {
        using NS::RHI::ERHIBufferUsage;

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        // UAV
        if (EnumHasAnyFlags(usage, ERHIBufferUsage::UnorderedAccess))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        // SRVが不要な場合（D3D12ではバッファにDENY_SHADER_RESOURCEは無効なので省略）
        // D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCEはテクスチャ専用

        return flags;
    }

} // namespace NS::D3D12RHI
