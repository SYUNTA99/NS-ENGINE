/// @file IRHIBuffer.cpp
/// @brief バッファ便利メソッド実装
#include "IRHIBuffer.h"
#include "IRHICommandContext.h"

namespace NS::RHI
{
    //=========================================================================
    // IRHIBuffer: 頂点/インデックスバッファビュー
    //=========================================================================

    RHIVertexBufferView IRHIBuffer::GetVertexBufferView(MemoryOffset offset, MemorySize size, uint32 stride) const
    {
        RHIVertexBufferView view;
        view.bufferAddress = GetGPUVirtualAddress() + offset;
        view.size = static_cast<uint32>(size > 0 ? size : GetSize() - offset);
        view.stride = stride > 0 ? stride : GetStride();
        return view;
    }

    RHIIndexBufferView IRHIBuffer::GetIndexBufferView(ERHIIndexFormat format,
                                                      MemoryOffset offset,
                                                      MemorySize size) const
    {
        RHIIndexBufferView view;
        view.bufferAddress = GetGPUVirtualAddress() + offset;
        view.size = static_cast<uint32>(size > 0 ? size : GetSize() - offset);
        view.format = format;
        return view;
    }

} // namespace NS::RHI
