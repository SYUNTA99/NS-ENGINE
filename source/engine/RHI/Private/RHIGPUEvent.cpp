/// @file RHIGPUEvent.cpp
/// @brief ブレッドクラムバッファ実装
#include "RHIGPUEvent.h"
#include "IRHIBuffer.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIBreadcrumbBuffer
    //=========================================================================

    bool RHIBreadcrumbBuffer::Initialize(IRHIDevice* device)
    {
        m_device = device;

        // GPU書き込み用バッファ（UAV）
        RHIBufferDesc bufferDesc;
        bufferDesc.size = kMaxEntries * sizeof(uint32);
        bufferDesc.usage = ERHIBufferUsage::UnorderedAccess;
        bufferDesc.debugName = "BreadcrumbBuffer";
        m_buffer = device->CreateBuffer(bufferDesc);

        // Readbackバッファ（CPU読み取り用）
        RHIBufferDesc readbackDesc;
        readbackDesc.size = kMaxEntries * sizeof(uint32);
        readbackDesc.usage = ERHIBufferUsage::None;
        readbackDesc.debugName = "BreadcrumbReadback";
        m_readbackBuffer = device->CreateBuffer(readbackDesc);

        return m_buffer != nullptr && m_readbackBuffer != nullptr;
    }

    void RHIBreadcrumbBuffer::Shutdown()
    {
        m_buffer = nullptr;
        m_readbackBuffer = nullptr;
        m_device = nullptr;
    }

    bool RHIBreadcrumbBuffer::ReadEntries(RHIBreadcrumbEntry* outEntries, uint32& outCount)
    {
        if (!m_readbackBuffer || (outEntries == nullptr))
        {
            outCount = 0;
            return false;
        }

        // Readbackバッファをマップして読み取り
        RHIMapResult mapResult = m_readbackBuffer->Map(ERHIMapMode::Read);
        if (!mapResult.IsValid())
        {
            outCount = 0;
            return false;
        }

        const auto* data = static_cast<const uint32*>(mapResult.data);

        // 書き込まれたエントリを探す
        uint32 count = 0;
        for (uint32 i = 0; i < kMaxEntries && count < outCount; ++i)
        {
            if (data[i] != 0)
            {
                outEntries[count].id = data[i];
                outEntries[count].message = nullptr;
                outEntries[count].timestamp = 0;
                ++count;
            }
        }

        m_readbackBuffer->Unmap();
        outCount = count;
        return true;
    }

    uint32 RHIBreadcrumbBuffer::GetLastWrittenIndex() const
    {
        if (!m_readbackBuffer) {
            return 0;
}

        RHIMapResult mapResult = m_readbackBuffer->Map(ERHIMapMode::Read);
        if (!mapResult.IsValid())
        {
            return 0;
        }

        const auto* data = static_cast<const uint32*>(mapResult.data);

        // 最後の非ゼロエントリを探す
        uint32 lastIndex = 0;
        for (uint32 i = 0; i < kMaxEntries; ++i)
        {
            if (data[i] != 0) {
                lastIndex = i;
}
        }

        m_readbackBuffer->Unmap();
        return lastIndex;
    }

    void RHIBreadcrumbBuffer::Reset()
    {
        // バッファ内容のクリアはバックエンド依存
        // コマンドリストでClearUnorderedAccessViewUintを使用
    }

} // namespace NS::RHI
