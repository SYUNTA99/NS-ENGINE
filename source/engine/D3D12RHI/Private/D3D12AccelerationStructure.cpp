/// @file D3D12AccelerationStructure.cpp
/// @brief D3D12 レイトレーシング加速構造実装
#include "D3D12AccelerationStructure.h"
#include "D3D12Buffer.h"
#include "D3D12Device.h"
#include "D3D12Texture.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12AccelerationStructure
    //=========================================================================

    bool D3D12AccelerationStructure::Init(D3D12Device* device,
                                          const NS::RHI::RHIRaytracingAccelerationStructureDesc& desc,
                                          const char* /*debugName*/)
    {
        if (!device)
            return false;

        device_ = device;
        resultBuffer_ = desc.resultBuffer;
        resultBufferOffset_ = desc.resultBufferOffset;
        size_ = desc.resultDataMaxSize;

        // GPU仮想アドレス計算
        if (resultBuffer_)
        {
            auto* d3dBuffer = static_cast<D3D12Buffer*>(resultBuffer_);
            gpuAddress_ = d3dBuffer->GetD3DResource()->GetGPUVirtualAddress() + resultBufferOffset_;
        }

        return true;
    }

    //=========================================================================
    // ヘルパー関数
    //=========================================================================

    DXGI_FORMAT ConvertVertexFormatForRT(NS::RHI::ERHIPixelFormat format)
    {
        return D3D12Texture::ConvertPixelFormat(format);
    }

    void ConvertGeometryDesc(const NS::RHI::RHIRaytracingGeometryDesc& src, D3D12_RAYTRACING_GEOMETRY_DESC& dest)
    {
        memset(&dest, 0, sizeof(dest));
        dest.Flags = ConvertGeometryFlags(src.flags);

        if (src.type == NS::RHI::ERHIRaytracingGeometryType::Triangles)
        {
            dest.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            auto& tri = dest.Triangles;
            tri.VertexBuffer.StartAddress = src.triangles.vertexBufferAddress;
            tri.VertexBuffer.StrideInBytes = src.triangles.vertexStride;
            tri.VertexCount = src.triangles.vertexCount;
            tri.VertexFormat = ConvertVertexFormatForRT(src.triangles.vertexFormat);

            if (src.triangles.indexBufferAddress != 0)
            {
                tri.IndexBuffer = src.triangles.indexBufferAddress;
                tri.IndexCount = src.triangles.indexCount;
                tri.IndexFormat = (src.triangles.indexFormat == NS::RHI::ERHIIndexFormat::UInt16)
                                      ? DXGI_FORMAT_R16_UINT
                                      : DXGI_FORMAT_R32_UINT;
            }
            else
            {
                tri.IndexFormat = DXGI_FORMAT_UNKNOWN;
                tri.IndexCount = 0;
            }

            tri.Transform3x4 = src.triangles.transformBufferAddress;
        }
        else
        {
            dest.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            dest.AABBs.AABBs.StartAddress = src.aabbs.aabbBufferAddress;
            dest.AABBs.AABBs.StrideInBytes = src.aabbs.aabbStride;
            dest.AABBs.AABBCount = src.aabbs.aabbCount;
        }
    }

    void ConvertBuildInputs(const NS::RHI::RHIRaytracingAccelerationStructureBuildInputs& src,
                            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& dest,
                            D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs,
                            uint32 maxGeometries)
    {
        memset(&dest, 0, sizeof(dest));

        dest.Type = (src.type == NS::RHI::ERHIRaytracingAccelerationStructureType::TopLevel)
                        ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL
                        : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        dest.Flags = ConvertBuildFlags(src.flags);

        if (src.type == NS::RHI::ERHIRaytracingAccelerationStructureType::TopLevel)
        {
            dest.NumDescs = src.instanceCount;
            dest.InstanceDescs = src.instanceDescsAddress;
        }
        else
        {
            uint32 count = (src.geometryCount < maxGeometries) ? src.geometryCount : maxGeometries;
            for (uint32 i = 0; i < count; ++i)
            {
                ConvertGeometryDesc(src.geometries[i], geometryDescs[i]);
            }
            dest.NumDescs = count;
            dest.pGeometryDescs = geometryDescs;
            dest.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        }
    }

} // namespace NS::D3D12RHI
