/// @file RHIDescriptorHeap.cpp
/// @brief ディスクリプタヒープアロケーター実装
#include "RHIDescriptorHeap.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIDescriptorHeapAllocator
    //=========================================================================

    bool RHIDescriptorHeapAllocator::Initialize(IRHIDescriptorHeap* heap)
    {
        m_heap = heap;
        uint32 const numDescriptors = heap->GetNumDescriptors();

        m_freeRangeCapacity = 64;
        m_freeRanges = new FreeRange[m_freeRangeCapacity];
        m_freeRanges[0] = {.start=0, .count=numDescriptors};
        m_freeRangeCount = 1;
        m_freeCount = numDescriptors;

        return true;
    }

    void RHIDescriptorHeapAllocator::Shutdown()
    {
        delete[] m_freeRanges;
        m_freeRanges = nullptr;
        m_freeRangeCount = 0;
        m_freeRangeCapacity = 0;
        m_freeCount = 0;
        m_heap = nullptr;
    }

    RHIDescriptorAllocation RHIDescriptorHeapAllocator::Allocate(uint32 count)
    {
        RHIDescriptorAllocation result;
        if ((m_heap == nullptr) || count == 0) {
            return result;
}

        // First-fit
        for (uint32 i = 0; i < m_freeRangeCount; ++i)
        {
            if (m_freeRanges[i].count >= count)
            {
                uint32 const startIndex = m_freeRanges[i].start;

                result.heap = m_heap;
                result.heapIndex = startIndex;
                result.count = count;
                result.cpuHandle = m_heap->GetCPUDescriptorHandle(startIndex);
                if (m_heap->IsShaderVisible()) {
                    result.gpuHandle = m_heap->GetGPUDescriptorHandle(startIndex);
}

                m_freeRanges[i].start += count;
                m_freeRanges[i].count -= count;

                if (m_freeRanges[i].count == 0)
                {
                    m_freeRanges[i] = m_freeRanges[m_freeRangeCount - 1];
                    --m_freeRangeCount;
                }

                m_freeCount -= count;
                return result;
            }
        }

        return result;
    }

    void RHIDescriptorHeapAllocator::Free(const RHIDescriptorAllocation& allocation)
    {
        if (!allocation.IsValid() || allocation.heap != m_heap) {
            return;
}

        const uint32 freeStart = allocation.heapIndex;
        const uint32 freeEnd = freeStart + allocation.count;
        m_freeCount += allocation.count;

        // 隣接フリー範囲との O(n) マージ
        // 解放範囲の左隣・右隣を1パスで探す
        int32 mergeLeft = -1;
        int32 mergeRight = -1;

        for (uint32 i = 0; i < m_freeRangeCount; ++i)
        {
            const uint32 rangeEnd = m_freeRanges[i].start + m_freeRanges[i].count;
            if (rangeEnd == freeStart) {
                mergeLeft = static_cast<int32>(i);
            } else if (m_freeRanges[i].start == freeEnd) {
                mergeRight = static_cast<int32>(i);
}
        }

        if (mergeLeft >= 0 && mergeRight >= 0)
        {
            // 左右両方とマージ: 左を拡張し、右を削除
            m_freeRanges[mergeLeft].count += allocation.count + m_freeRanges[mergeRight].count;
            m_freeRanges[mergeRight] = m_freeRanges[m_freeRangeCount - 1];
            --m_freeRangeCount;
        }
        else if (mergeLeft >= 0)
        {
            // 左のみマージ
            m_freeRanges[mergeLeft].count += allocation.count;
        }
        else if (mergeRight >= 0)
        {
            // 右のみマージ
            m_freeRanges[mergeRight].start = freeStart;
            m_freeRanges[mergeRight].count += allocation.count;
        }
        else
        {
            // マージ不可: 新規レンジ追加
            if (m_freeRangeCount >= m_freeRangeCapacity)
            {
                uint32 const newCap = m_freeRangeCapacity * 2;
                auto* newRanges = new FreeRange[newCap];
                std::memcpy(newRanges, m_freeRanges, sizeof(FreeRange) * m_freeRangeCount);
                delete[] m_freeRanges;
                m_freeRanges = newRanges;
                m_freeRangeCapacity = newCap;
            }
            m_freeRanges[m_freeRangeCount++] = {.start = freeStart, .count = allocation.count};
        }
    }

    void RHIDescriptorHeapAllocator::Reset()
    {
        if (m_heap == nullptr) {
            return;
}

        uint32 const total = m_heap->GetNumDescriptors();
        m_freeRanges[0] = {.start=0, .count=total};
        m_freeRangeCount = 1;
        m_freeCount = total;
    }

} // namespace NS::RHI
