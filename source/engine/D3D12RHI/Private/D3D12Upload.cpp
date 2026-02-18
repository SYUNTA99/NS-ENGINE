/// @file D3D12Upload.cpp
/// @brief D3D12 アップロードヘルパー実装
#include "D3D12Upload.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    ComPtr<ID3D12Resource> D3D12UploadHelper::CreateUploadBuffer(D3D12Device* device,
                                                                 const void* data,
                                                                 uint64 size,
                                                                 uint64 alignment)
    {
        if (!device || !data || size == 0)
            return nullptr;

        // ヒーププロパティ（UPLOAD）
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // リソースデスクリプタ
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = alignment;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> uploadBuffer;
        HRESULT hr = device->GetD3DDevice()->CreateCommittedResource(&heapProps,
                                                                     D3D12_HEAP_FLAG_NONE,
                                                                     &desc,
                                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(&uploadBuffer));

        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create upload buffer");
            return nullptr;
        }

        // データをコピー
        void* mapped = nullptr;
        D3D12_RANGE readRange{0, 0}; // 読み取りなし
        hr = uploadBuffer->Map(0, &readRange, &mapped);
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to map upload buffer");
            return nullptr;
        }

        std::memcpy(mapped, data, size);
        uploadBuffer->Unmap(0, nullptr);

        return uploadBuffer;
    }

} // namespace NS::D3D12RHI
