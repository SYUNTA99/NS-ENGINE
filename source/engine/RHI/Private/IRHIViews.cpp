/// @file IRHIViews.cpp
/// @brief ビュー便利メソッド実装
#include "IRHIViews.h"
#include "IRHIBuffer.h"
#include "IRHICommandContext.h"
#include "IRHITexture.h"

namespace NS::RHI
{
    //=========================================================================
    // IRHIShaderResourceView
    //=========================================================================

    IRHIBuffer* IRHIShaderResourceView::GetBuffer() const
    {
        if (!IsBufferView())
        {
            return nullptr;
        }
        return dynamic_cast<IRHIBuffer*>(GetResource());
    }

    IRHITexture* IRHIShaderResourceView::GetTexture() const
    {
        if (IsBufferView())
        {
            return nullptr;
        }
        return dynamic_cast<IRHITexture*>(GetResource());
    }

    //=========================================================================
    // IRHIUnorderedAccessView
    //=========================================================================

    IRHIBuffer* IRHIUnorderedAccessView::GetBuffer() const
    {
        if (!IsBufferView())
        {
            return nullptr;
        }
        return dynamic_cast<IRHIBuffer*>(GetResource());
    }

    IRHITexture* IRHIUnorderedAccessView::GetTexture() const
    {
        if (IsBufferView())
        {
            return nullptr;
        }
        return dynamic_cast<IRHITexture*>(GetResource());
    }

    //=========================================================================
    // IRHIRenderTargetView
    //=========================================================================

    uint32 IRHIRenderTargetView::GetWidth() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return 0;
        }
        return CalculateMipSize(tex->GetWidth(), GetMipSlice());
    }

    uint32 IRHIRenderTargetView::GetHeight() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return 0;
        }
        return CalculateMipSize(tex->GetHeight(), GetMipSlice());
    }

    ERHISampleCount IRHIRenderTargetView::GetSampleCount() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return ERHISampleCount::Count1;
        }
        return tex->GetSampleCount();
    }

    //=========================================================================
    // IRHIDepthStencilView
    //=========================================================================

    uint32 IRHIDepthStencilView::GetWidth() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return 0;
        }
        return CalculateMipSize(tex->GetWidth(), GetMipSlice());
    }

    uint32 IRHIDepthStencilView::GetHeight() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return 0;
        }
        return CalculateMipSize(tex->GetHeight(), GetMipSlice());
    }

    ERHISampleCount IRHIDepthStencilView::GetSampleCount() const
    {
        IRHITexture* tex = GetTexture();
        if (tex == nullptr)
        {
            return ERHISampleCount::Count1;
        }
        return tex->GetSampleCount();
    }

    //=========================================================================
    // RHIRenderTargetArray
    //=========================================================================

    bool RHIRenderTargetArray::ValidateSizeConsistency() const
    {
        if (count == 0)
        {
            return true;
        }

        Extent2D firstSize = {0, 0};
        for (uint32 i = 0; i < count; ++i)
        {
            if (rtvs[i] == nullptr)
            {
                continue;
            }

            Extent2D const size = rtvs[i]->GetSize();
            if (firstSize.width == 0 && firstSize.height == 0)
            {
                firstSize = size;
            }
            else if (size.width != firstSize.width || size.height != firstSize.height)
            {
                return false;
            }
        }
        return true;
    }

    //=========================================================================
    // RHIConstantBufferViewDesc
    //=========================================================================

    uint64 RHIConstantBufferViewDesc::GetEffectiveGPUAddress() const
    {
        if (gpuAddress != 0)
        {
            return gpuAddress;
        }

        if (buffer != nullptr)
        {
            return buffer->GetGPUVirtualAddress() + offset;
        }

        return 0;
    }

    MemorySize RHIConstantBufferViewDesc::GetEffectiveSize() const
    {
        if (size > 0)
        {
            return size;
        }

        if (buffer != nullptr)
        {
            return buffer->GetSize() - offset;
        }

        return 0;
    }

    //=========================================================================
    // IRHIConstantBufferView
    //=========================================================================

    bool IRHIConstantBufferView::UpdateData(const void* data, MemorySize dataSize, MemoryOffset localOffset) const
    {
        IRHIBuffer* buf = GetBuffer();
        if ((buf == nullptr) || !buf->IsCPUWritable())
        {
            return false;
        }

        MemoryOffset const bufferOffset = GetOffset() + localOffset;
        RHIMapResult const map = buf->Map(ERHIMapMode::WriteNoOverwrite, bufferOffset, dataSize);
        if (!map.IsValid())
        {
            return false;
        }

        std::memcpy(map.data, data, dataSize);
        buf->Unmap(bufferOffset, dataSize);
        return true;
    }

    //=========================================================================
    // RHIUAVCounterHelper
    //=========================================================================

    void RHIUAVCounterHelper::ResetCounter(IRHICommandContext* context, IRHIUnorderedAccessView* uav, uint32 value)
    {
        if ((context == nullptr) || (uav == nullptr) || !uav->HasCounter())
        {
            return;
        }

        IRHIBuffer* counterBuf = uav->GetCounterResource();
        if (counterBuf == nullptr)
        {
            return;
        }

        // カウンターバッファに値を書き込み
        counterBuf->WriteData(&value, sizeof(value), uav->GetCounterOffset());
    }

    void RHIUAVCounterHelper::CopyCounterToBuffer(IRHICommandContext* context,
                                                  IRHIUnorderedAccessView* uav,
                                                  IRHIBuffer* destBuffer,
                                                  uint64 destOffset)
    {
        if ((context == nullptr) || (uav == nullptr) || !uav->HasCounter() || (destBuffer == nullptr))
        {
            return;
        }

        IRHIBuffer* counterBuf = uav->GetCounterResource();
        if (counterBuf == nullptr)
        {
            return;
        }

        context->CopyBufferRegion(destBuffer, destOffset, counterBuf, uav->GetCounterOffset(), sizeof(uint32));
    }

    void RHIUAVCounterHelper::SetCounterFromBuffer(IRHICommandContext* context,
                                                   IRHIUnorderedAccessView* uav,
                                                   IRHIBuffer* srcBuffer,
                                                   uint64 srcOffset)
    {
        if ((context == nullptr) || (uav == nullptr) || !uav->HasCounter() || (srcBuffer == nullptr))
        {
            return;
        }

        IRHIBuffer* counterBuf = uav->GetCounterResource();
        if (counterBuf == nullptr)
        {
            return;
        }

        context->CopyBufferRegion(counterBuf, uav->GetCounterOffset(), srcBuffer, srcOffset, sizeof(uint32));
    }

} // namespace NS::RHI
