/// @file RHIBarrier.cpp
/// @brief リソースバリアバッチ・スプリットバリア実装
#include "RHIBarrier.h"
#include "IRHICommandContext.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIBarrierBatch
    //=========================================================================

    RHIBarrierBatch& RHIBarrierBatch::AddTransition(IRHIResource* resource,
                                                    ERHIResourceState before,
                                                    ERHIResourceState after,
                                                    uint32 subresource)
    {
        // 同一状態なら追加不要
        if (before == after)
        {
            return *this;
        }

        // 自動フラッシュ: コンテキスト有りかつ容量到達
        if (m_transitionCount >= kMaxBarriers)
        {
            if (m_context)
            {
                Submit(m_context);
            }
            else
            {
                return *this; // 容量超過
            }
        }

        auto& barrier = m_transitions[m_transitionCount++];
        barrier.resource = resource;
        barrier.subresource = subresource;
        barrier.stateBefore = before;
        barrier.stateAfter = after;
        barrier.flags = ERHIBarrierFlags::None;

        return *this;
    }

    RHIBarrierBatch& RHIBarrierBatch::AddTransition(const RHITransitionBarrier& barrier)
    {
        if (barrier.stateBefore == barrier.stateAfter)
        {
            return *this;
        }

        if (m_transitionCount >= kMaxBarriers)
        {
            if (m_context)
            {
                Submit(m_context);
            }
            else
            {
                return *this;
            }
        }

        m_transitions[m_transitionCount++] = barrier;
        return *this;
    }

    RHIBarrierBatch& RHIBarrierBatch::AddUAV(IRHIResource* resource)
    {
        if (m_uavCount >= kMaxBarriers)
        {
            if (m_context)
            {
                Submit(m_context);
            }
            else
            {
                return *this;
            }
        }

        m_uavs[m_uavCount++].resource = resource;
        return *this;
    }

    RHIBarrierBatch& RHIBarrierBatch::AddAliasing(IRHIResource* before, IRHIResource* after)
    {
        if (m_aliasingCount >= kMaxBarriers)
        {
            if (m_context)
            {
                Submit(m_context);
            }
            else
            {
                return *this;
            }
        }

        auto& barrier = m_aliasings[m_aliasingCount++];
        barrier.resourceBefore = before;
        barrier.resourceAfter = after;
        return *this;
    }

    void RHIBarrierBatch::Submit(IRHICommandContext* context)
    {
        if ((context == nullptr) || IsEmpty())
        {
            return;
        }

        // 遷移バリア
        for (uint32 i = 0; i < m_transitionCount; ++i)
        {
            const auto& b = m_transitions[i];
            context->TransitionBarrier(b.resource, b.stateBefore, b.stateAfter, b.subresource);
        }

        // UAVバリア
        for (uint32 i = 0; i < m_uavCount; ++i)
        {
            context->UAVBarrier(m_uavs[i].resource);
        }

        // エイリアシングバリア
        for (uint32 i = 0; i < m_aliasingCount; ++i)
        {
            context->AliasingBarrier(m_aliasings[i].resourceBefore, m_aliasings[i].resourceAfter);
        }

        context->FlushBarriers();
        Clear();
    }

    void RHIBarrierBatch::Clear()
    {
        m_transitionCount = 0;
        m_uavCount = 0;
        m_aliasingCount = 0;
    }

    //=========================================================================
    // RHISplitBarrier
    //=========================================================================

    void RHISplitBarrier::Begin(IRHICommandContext* context,
                                IRHIResource* resource,
                                ERHIResourceState before,
                                ERHIResourceState after,
                                uint32 subresource)
    {
        m_resource = resource;
        m_stateBefore = before;
        m_stateAfter = after;
        m_subresource = subresource;

        // BeginOnlyフラグ付きで遷移開始
        RHITransitionBarrier barrier;
        barrier.resource = resource;
        barrier.subresource = subresource;
        barrier.stateBefore = before;
        barrier.stateAfter = after;
        barrier.flags = ERHIBarrierFlags::BeginOnly;

        context->TransitionBarrier(barrier.resource, barrier.stateBefore, barrier.stateAfter, barrier.subresource);
        context->FlushBarriers();
    }

    void RHISplitBarrier::End(IRHICommandContext* context)
    {
        if (!m_resource)
        {
            return;
        }

        // EndOnlyフラグ付きで遷移完了
        context->TransitionBarrier(m_resource, m_stateBefore, m_stateAfter, m_subresource);
        context->FlushBarriers();

        m_resource = nullptr;
    }

    //=========================================================================
    // RHISplitBarrierBatch
    //=========================================================================

    void RHISplitBarrierBatch::BeginBarrier(IRHICommandContext* context,
                                            IRHIResource* resource,
                                            ERHIResourceState before,
                                            ERHIResourceState after,
                                            uint32 subresource)
    {
        if (m_count >= kMaxSplitBarriers)
        {
            return;
        }

        m_barriers[m_count].Begin(context, resource, before, after, subresource);
        ++m_count;
    }

    void RHISplitBarrierBatch::EndAll(IRHICommandContext* context)
    {
        for (uint32 i = 0; i < m_count; ++i)
        {
            m_barriers[i].End(context);
        }
        m_count = 0;
    }

} // namespace NS::RHI
