/// @file D3D12Resource.cpp
/// @brief D3D12 GPU リソースラッパー実装
#include "D3D12Resource.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // デストラクタ
    //=========================================================================

    D3D12GpuResource::~D3D12GpuResource()
    {
        Release();
    }

    //=========================================================================
    // ムーブ
    //=========================================================================

    D3D12GpuResource::D3D12GpuResource(D3D12GpuResource&& other) noexcept
        : device_(other.device_), resource_(std::move(other.resource_)), heapType_(other.heapType_),
          gpuVirtualAddress_(other.gpuVirtualAddress_), stateMap_(std::move(other.stateMap_)),
          requiresStateTracking_(other.requiresStateTracking_), mappedAddress_(other.mappedAddress_),
          mapCount_(other.mapCount_)
    {
        other.device_ = nullptr;
        other.gpuVirtualAddress_ = 0;
        other.mappedAddress_ = nullptr;
        other.mapCount_ = 0;
    }

    D3D12GpuResource& D3D12GpuResource::operator=(D3D12GpuResource&& other) noexcept
    {
        if (this != &other)
        {
            Release();

            device_ = other.device_;
            resource_ = std::move(other.resource_);
            heapType_ = other.heapType_;
            gpuVirtualAddress_ = other.gpuVirtualAddress_;
            stateMap_ = std::move(other.stateMap_);
            requiresStateTracking_ = other.requiresStateTracking_;
            mappedAddress_ = other.mappedAddress_;
            mapCount_ = other.mapCount_;

            other.device_ = nullptr;
            other.gpuVirtualAddress_ = 0;
            other.mappedAddress_ = nullptr;
            other.mapCount_ = 0;
        }
        return *this;
    }

    //=========================================================================
    // 初期化
    //=========================================================================

    void D3D12GpuResource::InitFromExisting(D3D12Device* device,
                                            ComPtr<ID3D12Resource> resource,
                                            D3D12_HEAP_TYPE heapType,
                                            NS::RHI::ERHIResourceState initialState,
                                            uint32 subresourceCount)
    {
        Release();

        device_ = device;
        resource_ = std::move(resource);
        heapType_ = heapType;

        // 状態追跡初期化
        stateMap_.Initialize(subresourceCount, initialState);

        // Upload/Readbackヒープは状態遷移不要
        requiresStateTracking_ = (heapType == D3D12_HEAP_TYPE_DEFAULT);

        // GPU仮想アドレスキャッシュ（バッファのみ）
        gpuVirtualAddress_ = 0;
        if (resource_)
        {
            D3D12_RESOURCE_DESC desc = resource_->GetDesc();
            if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            {
                gpuVirtualAddress_ = resource_->GetGPUVirtualAddress();
            }
        }
    }

    bool D3D12GpuResource::InitCommitted(D3D12Device* device,
                                         const D3D12_HEAP_PROPERTIES& heapProps,
                                         D3D12_HEAP_FLAGS heapFlags,
                                         const D3D12_RESOURCE_DESC& desc,
                                         NS::RHI::ERHIResourceState initialState,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        Release();

        device_ = device;
        heapType_ = heapProps.Type;

        // D3D12初期状態変換
        D3D12_RESOURCE_STATES d3dState = ConvertToD3D12State(initialState);

        // Upload/Readbackヒープは初期状態が固定
        if (heapProps.Type == D3D12_HEAP_TYPE_UPLOAD)
        {
            d3dState = D3D12_RESOURCE_STATE_GENERIC_READ;
            initialState = NS::RHI::ERHIResourceState::GenericRead;
        }
        else if (heapProps.Type == D3D12_HEAP_TYPE_READBACK)
        {
            d3dState = D3D12_RESOURCE_STATE_COPY_DEST;
            initialState = NS::RHI::ERHIResourceState::CopyDest;
        }

        HRESULT hr = device->GetD3DDevice()->CreateCommittedResource(
            &heapProps, heapFlags, &desc, d3dState, pOptimizedClearValue, IID_PPV_ARGS(&resource_));

        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] CreateCommittedResource failed");
            return false;
        }

        // サブリソース数計算
        uint32 subresourceCount = 1;
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            subresourceCount = desc.MipLevels * desc.DepthOrArraySize;
            // DepthStencilフォーマットは追加プレーンがある
            // 簡易版: 一般的なフォーマットは1プレーン
        }

        // 状態追跡初期化
        stateMap_.Initialize(subresourceCount, initialState);
        requiresStateTracking_ = (heapProps.Type == D3D12_HEAP_TYPE_DEFAULT);

        // GPU仮想アドレスキャッシュ
        gpuVirtualAddress_ = 0;
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            gpuVirtualAddress_ = resource_->GetGPUVirtualAddress();
        }

        return true;
    }

    void D3D12GpuResource::Release()
    {
        // マップ中なら解除
        if (mappedAddress_ && resource_)
        {
            resource_->Unmap(0, nullptr);
            mappedAddress_ = nullptr;
            mapCount_ = 0;
        }

        resource_.Reset();
        gpuVirtualAddress_ = 0;
        stateMap_.Reset();
        device_ = nullptr;
    }

    //=========================================================================
    // アクセサ
    //=========================================================================

    D3D12_RESOURCE_DESC D3D12GpuResource::GetDesc() const
    {
        if (resource_)
        {
            return resource_->GetDesc();
        }
        return {};
    }

    NS::RHI::ERHIResourceState D3D12GpuResource::GetCurrentState() const
    {
        if (stateMap_.IsUniform())
        {
            return stateMap_.GetUniformState();
        }
        // 非均一状態の場合はCommon返却（呼び出し側がサブリソース単位で確認すべき）
        return NS::RHI::ERHIResourceState::Common;
    }

    //=========================================================================
    // Map/Unmap
    //=========================================================================

    void* D3D12GpuResource::Map(uint32 subresource, const D3D12_RANGE* readRange)
    {
        if (!resource_)
            return nullptr;

        if (mapCount_ == 0)
        {
            void* data = nullptr;
            HRESULT hr = resource_->Map(subresource, readRange, &data);
            if (FAILED(hr))
            {
                LOG_HRESULT(hr, "[D3D12RHI] Resource Map failed");
                return nullptr;
            }
            mappedAddress_ = data;
        }

        ++mapCount_;
        return mappedAddress_;
    }

    void D3D12GpuResource::Unmap(uint32 subresource, const D3D12_RANGE* writtenRange)
    {
        if (mapCount_ <= 0)
            return;

        --mapCount_;
        if (mapCount_ == 0 && resource_)
        {
            resource_->Unmap(subresource, writtenRange);
            mappedAddress_ = nullptr;
        }
    }

    //=========================================================================
    // デバッグ
    //=========================================================================

    void D3D12GpuResource::SetDebugName(const char* name)
    {
        if (resource_ && name)
        {
            wchar_t wname[256]{};
            for (int i = 0; i < 255 && name[i]; ++i)
            {
                wname[i] = static_cast<wchar_t>(name[i]);
            }
            resource_->SetName(wname);
        }
    }

    //=========================================================================
    // 状態変換
    //=========================================================================

    D3D12_RESOURCE_STATES D3D12GpuResource::ConvertToD3D12State(NS::RHI::ERHIResourceState state)
    {
        using NS::RHI::ERHIResourceState;

        if (state == ERHIResourceState::Common)
            return D3D12_RESOURCE_STATE_COMMON;

        D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;

        if ((state & ERHIResourceState::VertexBuffer) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if ((state & ERHIResourceState::ConstantBuffer) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if ((state & ERHIResourceState::IndexBuffer) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
        if ((state & ERHIResourceState::RenderTarget) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if ((state & ERHIResourceState::UnorderedAccess) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if ((state & ERHIResourceState::DepthWrite) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if ((state & ERHIResourceState::DepthRead) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if ((state & ERHIResourceState::NonPixelShaderResource) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if ((state & ERHIResourceState::PixelShaderResource) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        if ((state & ERHIResourceState::StreamOut) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_STREAM_OUT;
        if ((state & ERHIResourceState::IndirectArgument) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if ((state & ERHIResourceState::CopyDest) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_COPY_DEST;
        if ((state & ERHIResourceState::CopySource) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if ((state & ERHIResourceState::ResolveDest) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
        if ((state & ERHIResourceState::ResolveSource) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        if ((state & ERHIResourceState::RaytracingAccelerationStructure) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((state & ERHIResourceState::ShadingRateSource) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        if ((state & ERHIResourceState::Present) != ERHIResourceState::Common)
            result |= D3D12_RESOURCE_STATE_PRESENT;

        return result;
    }

} // namespace NS::D3D12RHI
