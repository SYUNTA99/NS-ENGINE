/// @file RHIAsyncReadback.cpp
/// @brief オクルージョンクエリ用非同期リードバック実装
#include "RHIAsyncReadback.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIOcclusionQueryReadback
    //=========================================================================

    RHIOcclusionQueryReadback::RHIOcclusionQueryReadback(IRHIDevice* device, uint32 maxQueries)
        :  m_maxQueries(maxQueries)
    {
        RHIBufferReadbackDesc desc;
        desc.maxSize = maxQueries * sizeof(uint64);
        desc.debugName = "OcclusionQueryReadback";

        for (uint32 i = 0; i < kFrameLatency; ++i)
        {
            m_readbacks[i] = device->CreateBufferReadback(desc);
            m_cachedResults[i].resize(maxQueries, 0);
        }
    }

    void RHIOcclusionQueryReadback::EnqueueReadback(IRHICommandContext* context,
                                                    IRHIQueryHeap* queryHeap,
                                                    uint32 startQuery,
                                                    uint32 queryCount)
    {
        if ((context == nullptr) || (queryHeap == nullptr)) {
            return;
}

        // クエリ結果をリードバックバッファにコピー
        // ResolveQueryDataは中間IRHIBuffer*が必要。
        // 実装時は中間バッファ経由でReadbackにコピーする。
        (void)queryHeap;
        (void)startQuery;
        (void)queryCount;
    }

    void RHIOcclusionQueryReadback::OnFrameEnd()
    {
        // 読み取り可能なフレームの結果をキャッシュ
        uint32 const readFrame = (m_currentFrame + 1) % kFrameLatency;
        if (m_readbacks[readFrame] && m_readbacks[readFrame]->IsReady())
        {
            const void* data = m_readbacks[readFrame]->Lock();
            if (data != nullptr)
            {
                const auto* results = static_cast<const uint64*>(data);
                for (uint32 i = 0; i < m_maxQueries; ++i) {
                    m_cachedResults[readFrame][i] = results[i];
}
                m_readbacks[readFrame]->Unlock();
            }
        }

        m_currentFrame = (m_currentFrame + 1) % kFrameLatency;
    }

    bool RHIOcclusionQueryReadback::GetQueryResult(uint32 queryIndex, uint64& outSamplesPassed) const
    {
        if (queryIndex >= m_maxQueries) {
            return false;
}

        // 最新の有効な結果を返す
        uint32 const readFrame = (m_currentFrame + 1) % kFrameLatency;
        outSamplesPassed = m_cachedResults[readFrame][queryIndex];
        return true;
    }

    bool RHIOcclusionQueryReadback::IsVisible(uint32 queryIndex, uint64 sampleThreshold) const
    {
        uint64 samples = 0;
        if (!GetQueryResult(queryIndex, samples)) {
            return true; // データなし→可視として扱う
}
        return samples >= sampleThreshold;
    }

} // namespace NS::RHI
