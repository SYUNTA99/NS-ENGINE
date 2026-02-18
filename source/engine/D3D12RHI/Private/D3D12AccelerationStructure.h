/// @file D3D12AccelerationStructure.h
/// @brief D3D12 レイトレーシング加速構造
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIRaytracingAS.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12AccelerationStructure — IRHIAccelerationStructure実装
    //=========================================================================

    class D3D12AccelerationStructure final : public NS::RHI::IRHIAccelerationStructure
    {
    public:
        D3D12AccelerationStructure() = default;
        ~D3D12AccelerationStructure() override = default;

        /// 初期化
        bool Init(D3D12Device* device,
                  const NS::RHI::RHIRaytracingAccelerationStructureDesc& desc,
                  const char* debugName);

        // --- IRHIAccelerationStructure ---
        D3D12Device* GetD3D12Device() const { return device_; }
        uint64 GetGPUVirtualAddress() const override { return gpuAddress_; }
        NS::RHI::IRHIBuffer* GetResultBuffer() const override { return resultBuffer_; }
        uint64 GetResultBufferOffset() const override { return resultBufferOffset_; }
        uint64 GetSize() const override { return size_; }

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::IRHIBuffer* resultBuffer_ = nullptr;
        uint64 resultBufferOffset_ = 0;
        uint64 gpuAddress_ = 0;
        uint64 size_ = 0;
    };

    //=========================================================================
    // ヘルパー: RHI → D3D12 変換
    //=========================================================================

    /// ERHIRaytracingBuildFlags → D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS
    inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertBuildFlags(
        NS::RHI::ERHIRaytracingBuildFlags flags)
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3dFlags =
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::AllowUpdate))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::AllowCompaction))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::PreferFastTrace))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::PreferFastBuild))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::MinimizeMemory))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingBuildFlags::PerformUpdate))
            d3dFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        return d3dFlags;
    }

    /// ERHIRaytracingGeometryFlags → D3D12_RAYTRACING_GEOMETRY_FLAGS
    inline D3D12_RAYTRACING_GEOMETRY_FLAGS ConvertGeometryFlags(NS::RHI::ERHIRaytracingGeometryFlags flags)
    {
        D3D12_RAYTRACING_GEOMETRY_FLAGS d3dFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingGeometryFlags::Opaque))
            d3dFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        if (static_cast<uint32>(flags & NS::RHI::ERHIRaytracingGeometryFlags::NoDuplicateAnyHit))
            d3dFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
        return d3dFlags;
    }

    /// ERHIRaytracingCopyMode → D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE
    inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE ConvertCopyMode(NS::RHI::ERHIRaytracingCopyMode mode)
    {
        switch (mode)
        {
        case NS::RHI::ERHIRaytracingCopyMode::Clone:
            return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
        case NS::RHI::ERHIRaytracingCopyMode::Compact:
            return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT;
        case NS::RHI::ERHIRaytracingCopyMode::SerializeToBuffer:
            return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE;
        case NS::RHI::ERHIRaytracingCopyMode::DeserializeFromBuffer:
            return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE;
        default:
            return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE;
        }
    }

    /// ERHIPixelFormat → DXGI_FORMAT（頂点フォーマット用）
    DXGI_FORMAT ConvertVertexFormatForRT(NS::RHI::ERHIPixelFormat format);

    /// ジオメトリ記述変換ヘルパー
    void ConvertGeometryDesc(const NS::RHI::RHIRaytracingGeometryDesc& src, D3D12_RAYTRACING_GEOMETRY_DESC& dest);

    /// ビルド入力変換ヘルパー
    void ConvertBuildInputs(const NS::RHI::RHIRaytracingAccelerationStructureBuildInputs& src,
                            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& dest,
                            D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs,
                            uint32 maxGeometries);

} // namespace NS::D3D12RHI
