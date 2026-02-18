/// @file D3D12RayTracingSBT.cpp
/// @brief D3D12 シェーダーバインディングテーブル実装
#include "D3D12RayTracingSBT.h"
#include "D3D12Device.h"
#include "RHI/Public/RHITypes.h"

#include <cstring>

namespace NS::D3D12RHI
{
    D3D12ShaderBindingTable::~D3D12ShaderBindingTable()
    {
        if (buffer_ && mappedData_)
        {
            buffer_->Unmap(0, nullptr);
            mappedData_ = nullptr;
        }
    }

    bool D3D12ShaderBindingTable::Init(D3D12Device* device,
                                       const NS::RHI::RHIShaderBindingTableDesc& desc,
                                       const char* /*debugName*/)
    {
        if (!device)
            return false;

        device_ = device;
        rayGenCount_ = desc.rayGenRecordCount;
        missCount_ = desc.missRecordCount;
        hitGroupCount_ = desc.hitGroupRecordCount;
        callableCount_ = desc.callableRecordCount;

        // レコードストライド: シェーダーID(32B) + ローカルルート引数、32Bアライメント
        uint32 rawStride = NS::RHI::kShaderIdentifierSize + desc.maxLocalRootArgumentsSize;
        recordStride_ = static_cast<uint32>(
            NS::RHI::AlignUp(static_cast<uint64>(rawStride), static_cast<uint64>(NS::RHI::kShaderRecordAlignment)));

        // 各領域のオフセット計算（64Bアライメント）
        rayGenOffset_ = 0;
        uint64 rayGenSize = NS::RHI::AlignUp(static_cast<uint64>(recordStride_) * rayGenCount_,
                                    static_cast<uint64>(NS::RHI::kShaderTableAlignment));

        missOffset_ = rayGenSize;
        uint64 missSize = NS::RHI::AlignUp(static_cast<uint64>(recordStride_) * missCount_,
                                  static_cast<uint64>(NS::RHI::kShaderTableAlignment));

        hitGroupOffset_ = missOffset_ + missSize;
        uint64 hitGroupSize = NS::RHI::AlignUp(static_cast<uint64>(recordStride_) * hitGroupCount_,
                                      static_cast<uint64>(NS::RHI::kShaderTableAlignment));

        callableOffset_ = hitGroupOffset_ + hitGroupSize;
        uint64 callableSize = NS::RHI::AlignUp(static_cast<uint64>(recordStride_) * callableCount_,
                                      static_cast<uint64>(NS::RHI::kShaderTableAlignment));

        totalSize_ = callableOffset_ + callableSize;
        if (totalSize_ == 0)
            return false;

        // Uploadバッファ作成（CPU書き込み → GPU読み取り）
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc{};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = totalSize_;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->GetD3DDevice()->CreateCommittedResource(&heapProps,
                                                                     D3D12_HEAP_FLAG_NONE,
                                                                     &bufferDesc,
                                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(&buffer_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12RHI] Failed to create SBT buffer");
            return false;
        }

        // 永続Map
        hr = buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12RHI] Failed to map SBT buffer");
            return false;
        }

        memset(mappedData_, 0, static_cast<size_t>(totalSize_));
        gpuBaseAddress_ = buffer_->GetGPUVirtualAddress();

        return true;
    }

    void D3D12ShaderBindingTable::WriteRecord(uint64 regionOffset, uint32 index, const NS::RHI::RHIShaderRecord& record)
    {
        if (!mappedData_)
            return;

        uint8* dest = mappedData_ + regionOffset + static_cast<uint64>(index) * recordStride_;

        // シェーダー識別子コピー (32B)
        memcpy(dest, record.identifier.data, NS::RHI::kShaderIdentifierSize);

        // ローカルルート引数コピー
        if (record.localRootArguments && record.localRootArgumentsSize > 0)
        {
            memcpy(dest + NS::RHI::kShaderIdentifierSize, record.localRootArguments, record.localRootArgumentsSize);
        }
    }

    NS::RHI::RHIShaderTableRegion D3D12ShaderBindingTable::GetRayGenRegion() const
    {
        NS::RHI::RHIShaderTableRegion region{};
        if (rayGenCount_ > 0)
        {
            region.startAddress = gpuBaseAddress_ + rayGenOffset_;
            region.size = static_cast<uint64>(recordStride_) * rayGenCount_;
            region.stride = recordStride_;
        }
        return region;
    }

    NS::RHI::RHIShaderTableRegion D3D12ShaderBindingTable::GetMissRegion() const
    {
        NS::RHI::RHIShaderTableRegion region{};
        if (missCount_ > 0)
        {
            region.startAddress = gpuBaseAddress_ + missOffset_;
            region.size = static_cast<uint64>(recordStride_) * missCount_;
            region.stride = recordStride_;
        }
        return region;
    }

    NS::RHI::RHIShaderTableRegion D3D12ShaderBindingTable::GetHitGroupRegion() const
    {
        NS::RHI::RHIShaderTableRegion region{};
        if (hitGroupCount_ > 0)
        {
            region.startAddress = gpuBaseAddress_ + hitGroupOffset_;
            region.size = static_cast<uint64>(recordStride_) * hitGroupCount_;
            region.stride = recordStride_;
        }
        return region;
    }

    NS::RHI::RHIShaderTableRegion D3D12ShaderBindingTable::GetCallableRegion() const
    {
        NS::RHI::RHIShaderTableRegion region{};
        if (callableCount_ > 0)
        {
            region.startAddress = gpuBaseAddress_ + callableOffset_;
            region.size = static_cast<uint64>(recordStride_) * callableCount_;
            region.stride = recordStride_;
        }
        return region;
    }

    void D3D12ShaderBindingTable::SetRayGenRecord(uint32 index, const NS::RHI::RHIShaderRecord& record)
    {
        if (index < rayGenCount_)
            WriteRecord(rayGenOffset_, index, record);
    }

    void D3D12ShaderBindingTable::SetMissRecord(uint32 index, const NS::RHI::RHIShaderRecord& record)
    {
        if (index < missCount_)
            WriteRecord(missOffset_, index, record);
    }

    void D3D12ShaderBindingTable::SetHitGroupRecord(uint32 index, const NS::RHI::RHIShaderRecord& record)
    {
        if (index < hitGroupCount_)
            WriteRecord(hitGroupOffset_, index, record);
    }

    void D3D12ShaderBindingTable::SetCallableRecord(uint32 index, const NS::RHI::RHIShaderRecord& record)
    {
        if (index < callableCount_)
            WriteRecord(callableOffset_, index, record);
    }

} // namespace NS::D3D12RHI
