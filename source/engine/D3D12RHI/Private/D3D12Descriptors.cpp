/// @file D3D12Descriptors.cpp
/// @brief D3D12 ディスクリプタヒープ実装
#include "D3D12Descriptors.h"
#include "D3D12Device.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12DescriptorHeap
    //=========================================================================

    D3D12DescriptorHeap::D3D12DescriptorHeap() = default;

    bool D3D12DescriptorHeap::Init(D3D12Device* device,
                                   const NS::RHI::RHIDescriptorHeapDesc& desc,
                                   const char* debugName)
    {
        if (!device)
            return false;

        device_ = device;
        type_ = desc.type;
        numDescriptors_ = desc.numDescriptors;
        shaderVisible_ =
            (desc.flags & NS::RHI::ERHIDescriptorHeapFlags::ShaderVisible) != NS::RHI::ERHIDescriptorHeapFlags::None;

        // D3D12ヒープ記述
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = ConvertDescriptorHeapType(desc.type);
        heapDesc.NumDescriptors = desc.numDescriptors;
        heapDesc.Flags = shaderVisible_ ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = desc.nodeMask;

        HRESULT hr = device_->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create descriptor heap");
            return false;
        }

        // インクリメントサイズキャッシュ
        incrementSize_ = device_->GetD3DDevice()->GetDescriptorHandleIncrementSize(heapDesc.Type);

        // デバッグ名設定
        if (debugName && debugName[0] != '\0')
        {
            wchar_t wName[256]{};
            MultiByteToWideChar(CP_UTF8, 0, debugName, -1, wName, 256);
            heap_->SetName(wName);
        }

        return true;
    }

    bool D3D12DescriptorHeap::InitFromExisting(D3D12Device* device, ID3D12DescriptorHeap* existingHeap,
                                               NS::RHI::ERHIDescriptorHeapType type, uint32 numDescriptors,
                                               bool shaderVisible)
    {
        if (!device || !existingHeap)
            return false;

        device_ = device;
        type_ = type;
        numDescriptors_ = numDescriptors;
        shaderVisible_ = shaderVisible;
        incrementSize_ = device_->GetD3DDevice()->GetDescriptorHandleIncrementSize(
            ConvertDescriptorHeapType(type));

        // ComPtrにAddRefして保持（所有権は共有）
        heap_ = existingHeap;

        return true;
    }

    NS::RHI::IRHIDevice* D3D12DescriptorHeap::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    NS::RHI::RHICPUDescriptorHandle D3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart() const
    {
        NS::RHI::RHICPUDescriptorHandle handle{};
        if (heap_)
            handle.ptr = heap_->GetCPUDescriptorHandleForHeapStart().ptr;
        return handle;
    }

    NS::RHI::RHIGPUDescriptorHandle D3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart() const
    {
        NS::RHI::RHIGPUDescriptorHandle handle{};
        if (heap_ && shaderVisible_)
            handle.ptr = heap_->GetGPUDescriptorHandleForHeapStart().ptr;
        return handle;
    }

} // namespace NS::D3D12RHI
