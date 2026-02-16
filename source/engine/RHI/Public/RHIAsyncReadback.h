/// @file RHIAsyncReadback.h
/// @brief 非同期リードバック（マルチフレーム遅延対応）
/// @details リングバッファ方式の非同期リードバック、
///          オクルージョンクエリ用リードバックを提供。
/// @see 20-03-async-readback.md
#pragma once

#include "RHIGPUReadback.h"
#include "RHIQuery.h"

#include <array>
#include <vector>

namespace NS { namespace RHI {
    //=========================================================================
    // TRHIAsyncReadbackRing (20-03)
    //=========================================================================

    /// 非同期リードバックリング
    /// 複数フレームの遅延を許容し、待機なしでリードバック
    template <typename T, uint32 BufferCount = 3> class TRHIAsyncReadbackRing
    {
    public:
        static_assert(BufferCount >= 2, "Need at least 2 buffers for async readback");

        explicit TRHIAsyncReadbackRing(IRHIDevice* device)
        {
            RHIBufferReadbackDesc desc{sizeof(T), "AsyncReadback"};
            for (uint32 i = 0; i < BufferCount; ++i)
            {
                m_readbacks[i] = device->CreateBufferReadback(desc);
            }
        }

        /// リードバック要求（現在フレームのデータをエンキュー）
        void EnqueueCopy(IRHICommandContext* context, IRHIBuffer* source, uint64 offset = 0)
        {
            m_readbacks[m_writeIndex]->EnqueueCopy(context, source, offset, sizeof(T));
            m_writeIndex = (m_writeIndex + 1) % BufferCount;
        }

        /// 結果取得（BufferCount-1フレーム前のデータ）
        /// @return データが有効ならtrue
        bool TryGetResult(T& outValue)
        {
            uint32 readIndex = (m_writeIndex + 1) % BufferCount;
            if (m_readbacks[readIndex]->IsReady())
            {
                return m_readbacks[readIndex]->GetData(&outValue, sizeof(T)).IsOk();
            }
            return false;
        }

        /// 最新の有効な結果を取得
        T GetLatestOrDefault(const T& defaultValue = T{})
        {
            for (uint32 i = 0; i < BufferCount; ++i)
            {
                uint32 index = (m_writeIndex + BufferCount - i) % BufferCount;
                if (m_readbacks[index]->IsReady())
                {
                    T value;
                    if (m_readbacks[index]->GetData(&value, sizeof(T)).IsOk())
                    {
                        return value;
                    }
                }
            }
            return defaultValue;
        }

        /// レイテンシ（フレーム数）
        static constexpr uint32 GetLatency() { return BufferCount - 1; }

    private:
        std::array<RHIBufferReadbackRef, BufferCount> m_readbacks;
        uint32 m_writeIndex = 0;
    };

    //=========================================================================
    // RHIOcclusionQueryReadback (20-03)
    //=========================================================================

    /// オクルージョンクエリ用非同期リードバック
    class RHI_API RHIOcclusionQueryReadback
    {
    public:
        explicit RHIOcclusionQueryReadback(IRHIDevice* device, uint32 maxQueries);

        /// クエリ結果のリードバックをエンキュー
        void EnqueueReadback(IRHICommandContext* context,
                             IRHIQueryHeap* queryHeap,
                             uint32 startQuery,
                             uint32 queryCount);

        /// フレーム終了処理
        void OnFrameEnd();

        /// 結果取得（遅延あり）
        bool GetQueryResult(uint32 queryIndex, uint64& outSamplesPassed) const;

        /// 可視判定（閾値サンプル数）
        bool IsVisible(uint32 queryIndex, uint64 sampleThreshold = 1) const;

    private:
        static constexpr uint32 kFrameLatency = 2;
        std::array<RHIBufferReadbackRef, kFrameLatency> m_readbacks;
        std::array<std::vector<uint64>, kFrameLatency> m_cachedResults;
        uint32 m_currentFrame = 0;
        uint32 m_maxQueries;
    };

}} // namespace NS::RHI
