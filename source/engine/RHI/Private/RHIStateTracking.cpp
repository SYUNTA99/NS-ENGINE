/// @file RHIStateTracking.cpp
/// @brief リソース状態追跡・グローバル状態管理・自動バリア・状態検証実装
#include "RHIStateTracking.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace NS::RHI
{
    //=========================================================================
    // RHIResourceStateTracker
    //=========================================================================

    bool RHIResourceStateTracker::Initialize(uint32 maxTrackedResources)
    {
        m_trackedResources = new TrackedResource[maxTrackedResources];
        m_trackedCapacity = maxTrackedResources;
        m_trackedCount = 0;

        // バリアバッファ（リソース数の半分を初期容量）
        uint32 barrierCapacity = maxTrackedResources / 2;
        barrierCapacity = std::max<uint32>(barrierCapacity, 64);

        m_pendingBarriers = new RHITransitionBarrier[barrierCapacity];
        m_pendingBarrierCapacity = barrierCapacity;
        m_pendingBarrierCount = 0;

        return true;
    }

    void RHIResourceStateTracker::Shutdown()
    {
        delete[] m_trackedResources;
        m_trackedResources = nullptr;
        m_trackedCount = 0;
        m_trackedCapacity = 0;

        delete[] m_pendingBarriers;
        m_pendingBarriers = nullptr;
        m_pendingBarrierCount = 0;
        m_pendingBarrierCapacity = 0;
    }

    void RHIResourceStateTracker::Reset()
    {
        m_trackedCount = 0;
        m_pendingBarrierCount = 0;
    }

    void RHIResourceStateTracker::TrackResource(IRHIResource* resource, ERHIResourceState initialState)
    {
        if ((resource == nullptr) || m_trackedCount >= m_trackedCapacity)
        {
            return;
        }

        // 既に追跡中なら更新
        TrackedResource* existing = FindTrackedResource(resource);
        if (existing != nullptr)
        {
            existing->stateMap.SetAllSubresourcesState(initialState);
            return;
        }

        auto& tracked = m_trackedResources[m_trackedCount++];
        tracked.resource = resource;
        tracked.stateMap.SetAllSubresourcesState(initialState);
    }

    void RHIResourceStateTracker::UntrackResource(IRHIResource* resource)
    {
        for (uint32 i = 0; i < m_trackedCount; ++i)
        {
            if (m_trackedResources[i].resource == resource)
            {
                // 末尾と入れ替え削除
                m_trackedResources[i] = m_trackedResources[m_trackedCount - 1];
                --m_trackedCount;
                return;
            }
        }
    }

    ERHIResourceState RHIResourceStateTracker::GetCurrentState(IRHIResource* resource) const
    {
        const TrackedResource* tracked = FindTrackedResource(resource);
        if (tracked != nullptr)
        {
            return tracked->stateMap.GetUniformState();
        }
        return ERHIResourceState::Common;
    }

    ERHIResourceState RHIResourceStateTracker::GetSubresourceState(IRHIResource* resource, uint32 subresource) const
    {
        const TrackedResource* tracked = FindTrackedResource(resource);
        if (tracked == nullptr)
        {
            return ERHIResourceState::Common;
        }

        // サブリソース毎の追跡がなければ全体状態を返す
        if (subresource == kAllSubresources || tracked->stateMap.IsUniform())
        {
            return tracked->stateMap.GetUniformState();
        }

        return tracked->stateMap.GetSubresourceState(subresource);
    }

    void RHIResourceStateTracker::RequireState(IRHIResource* resource,
                                               ERHIResourceState requiredState,
                                               uint32 subresource)
    {
        if (resource == nullptr)
        {
            return;
        }

        ERHIResourceState currentState =
            (subresource == kAllSubresources) ? GetCurrentState(resource) : GetSubresourceState(resource, subresource);

        // 同一状態なら遷移不要
        if (currentState == requiredState)
        {
            return;
        }

        // バリアを追加
        if (m_pendingBarrierCount < m_pendingBarrierCapacity)
        {
            auto& barrier = m_pendingBarriers[m_pendingBarrierCount++];
            barrier.resource = resource;
            barrier.subresource = subresource;
            barrier.stateBefore = currentState;
            barrier.stateAfter = requiredState;
            barrier.flags = ERHIBarrierFlags::None;
        }

        // 状態を更新
        TrackedResource* tracked = FindTrackedResource(resource);
        if (tracked != nullptr)
        {
            if (subresource == kAllSubresources)
            {
                tracked->stateMap.SetAllSubresourcesState(requiredState);
            }
            else
            {
                tracked->stateMap.SetSubresourceState(subresource, requiredState);
            }
        }
        else
        {
            // 未追跡リソース: 新規登録（全サブリソースを初期状態として登録）
            TrackResource(resource, requiredState);
        }
    }

    uint32 RHIResourceStateTracker::GetPendingBarriers(RHITransitionBarrier* outBarriers, uint32 maxBarriers)
    {
        uint32 const count = (m_pendingBarrierCount < maxBarriers) ? m_pendingBarrierCount : maxBarriers;
        std::memcpy(outBarriers, m_pendingBarriers, count * sizeof(RHITransitionBarrier));
        return count;
    }

    void RHIResourceStateTracker::ClearPendingBarriers()
    {
        m_pendingBarrierCount = 0;
    }

    RHIResourceStateTracker::TrackedResource* RHIResourceStateTracker::FindTrackedResource(IRHIResource* resource)
    {
        for (uint32 i = 0; i < m_trackedCount; ++i)
        {
            if (m_trackedResources[i].resource == resource)
            {
                return &m_trackedResources[i];
            }
        }
        return nullptr;
    }

    const RHIResourceStateTracker::TrackedResource* RHIResourceStateTracker::FindTrackedResource(
        IRHIResource* resource) const
    {
        for (uint32 i = 0; i < m_trackedCount; ++i)
        {
            if (m_trackedResources[i].resource == resource)
            {
                return &m_trackedResources[i];
            }
        }
        return nullptr;
    }

    //=========================================================================
    // RHIGlobalResourceStateManager
    //=========================================================================

    bool RHIGlobalResourceStateManager::Initialize(IRHIDevice* device)
    {
        m_device = device;
        m_mutex = new std::mutex();
        return true;
    }

    void RHIGlobalResourceStateManager::Shutdown()
    {
        delete static_cast<std::mutex*>(m_mutex);
        m_mutex = nullptr;
        m_device = nullptr;
    }

    void RHIGlobalResourceStateManager::RegisterResource(IRHIResource* resource,
                                                         ERHIResourceState initialState,
                                                         uint32 subresourceCount)
    {
        (void)resource;
        (void)initialState;
        (void)subresourceCount;
        // 実際はハッシュマップに登録
    }

    void RHIGlobalResourceStateManager::UnregisterResource(IRHIResource* resource)
    {
        (void)resource;
    }

    ERHIResourceState RHIGlobalResourceStateManager::GetGlobalState(IRHIResource* resource) const
    {
        (void)resource;
        return ERHIResourceState::Common;
    }

    ERHIResourceState RHIGlobalResourceStateManager::GetSubresourceGlobalState(IRHIResource* resource,
                                                                               uint32 subresource) const
    {
        (void)resource;
        (void)subresource;
        return ERHIResourceState::Common;
    }

    uint32 RHIGlobalResourceStateManager::ResolveBarriers(const RHIResourceStateTracker& localTracker,
                                                          RHITransitionBarrier* outBarriers,
                                                          uint32 maxBarriers)
    {
        (void)localTracker;
        (void)outBarriers;
        (void)maxBarriers;
        // ローカルトラッカーのknown state とグローバル状態を比較し
        // 差分のバリアを生成する（バックエンド依存）
        return 0;
    }

    void RHIGlobalResourceStateManager::CommitLocalStates(const RHIResourceStateTracker& localTracker)
    {
        (void)localTracker;
        // ローカルトラッカーの最終状態をグローバルに反映
    }

    //=========================================================================
    // RHIAutoBarrierContext
    //=========================================================================

    void RHIAutoBarrierContext::Initialize(IRHICommandContext* context, RHIGlobalResourceStateManager* globalManager)
    {
        m_context = context;
        m_globalManager = globalManager;
        m_localTracker.Initialize();
        m_pendingBarriers = RHIBarrierBatch(context);
    }

    void RHIAutoBarrierContext::Finalize()
    {
        FlushBarriers();

        if (m_globalManager != nullptr)
        {
            m_globalManager->CommitLocalStates(m_localTracker);
        }

        m_localTracker.Shutdown();
        m_context = nullptr;
        m_globalManager = nullptr;
    }

    void RHIAutoBarrierContext::UseAsShaderResource(IRHITexture* texture, uint32 subresource)
    {
        m_localTracker.RequireState(
            static_cast<IRHIResource*>(texture), ERHIResourceState::ShaderResource, subresource);
    }

    void RHIAutoBarrierContext::UseAsRenderTarget(IRHITexture* texture, uint32 subresource)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(texture), ERHIResourceState::RenderTarget, subresource);
    }

    void RHIAutoBarrierContext::UseAsDepthStencil(IRHITexture* texture, bool write)
    {
        ERHIResourceState const state = write ? ERHIResourceState::DepthWrite : ERHIResourceState::DepthRead;
        m_localTracker.RequireState(static_cast<IRHIResource*>(texture), state);
    }

    void RHIAutoBarrierContext::UseAsUAV(IRHITexture* texture, uint32 subresource)
    {
        m_localTracker.RequireState(
            static_cast<IRHIResource*>(texture), ERHIResourceState::UnorderedAccess, subresource);
    }

    void RHIAutoBarrierContext::UseAsCopyDest(IRHITexture* texture)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(texture), ERHIResourceState::CopyDest);
    }

    void RHIAutoBarrierContext::UseAsCopySource(IRHITexture* texture)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(texture), ERHIResourceState::CopySource);
    }

    void RHIAutoBarrierContext::UseAsVertexBuffer(IRHIBuffer* buffer)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(buffer), ERHIResourceState::VertexBuffer);
    }

    void RHIAutoBarrierContext::UseAsIndexBuffer(IRHIBuffer* buffer)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(buffer), ERHIResourceState::IndexBuffer);
    }

    void RHIAutoBarrierContext::UseAsConstantBuffer(IRHIBuffer* buffer)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(buffer), ERHIResourceState::ConstantBuffer);
    }

    void RHIAutoBarrierContext::UseAsUAV(IRHIBuffer* buffer)
    {
        m_localTracker.RequireState(static_cast<IRHIResource*>(buffer), ERHIResourceState::UnorderedAccess);
    }

    void RHIAutoBarrierContext::FlushBarriers()
    {
        if (!m_context)
        {
            return;
        }

        RHITransitionBarrier barriers[RHIBarrierBatch::kMaxBarriers];
        uint32 const count = m_localTracker.GetPendingBarriers(barriers, RHIBarrierBatch::kMaxBarriers);

        for (uint32 i = 0; i < count; ++i)
        {
            m_context->TransitionBarrier(
                barriers[i].resource, barriers[i].stateBefore, barriers[i].stateAfter, barriers[i].subresource);
        }

        if (count > 0)
            m_context->FlushBarriers();

        m_localTracker.ClearPendingBarriers();
    }

    void RHIAutoBarrierContext::UAVBarrier(IRHIResource* resource)
    {
        if (m_context)
        {
            m_context->UAVBarrier(resource);
            m_context->FlushBarriers();
        }
    }

    //=========================================================================
    // RHIResourceStateValidator
    //=========================================================================

    bool RHIResourceStateValidator::ValidateAccess(IRHIResource* resource,
                                                   ERHIResourceState requiredState,
                                                   ERHIResourceState actualState,
                                                   uint32 subresource)
    {
        if (!m_enabled)
        {
            return true;
        }

        // 読み取り専用状態は組み合わせ可能
        bool const requiredIsReadOnly =
            (requiredState == ERHIResourceState::ShaderResource || requiredState == ERHIResourceState::CopySource ||
             requiredState == ERHIResourceState::DepthRead || requiredState == ERHIResourceState::IndirectArgument ||
             requiredState == ERHIResourceState::VertexBuffer || requiredState == ERHIResourceState::IndexBuffer ||
             requiredState == ERHIResourceState::ConstantBuffer);

        if (requiredIsReadOnly)
        {
            // 読み取り状態は他の読み取り状態と組み合わせ可能
            if ((static_cast<uint32>(actualState) & static_cast<uint32>(requiredState)) != 0)
            {
                return true;
            }
        }

        if (actualState != requiredState)
        {
            std::snprintf(m_lastError,
                          sizeof(m_lastError),
                          "Resource %p subresource %u: required state 0x%x but actual state 0x%x",
                          static_cast<void*>(resource),
                          subresource,
                          static_cast<uint32>(requiredState),
                          static_cast<uint32>(actualState));
            return false;
        }
        return true;
    }

    bool RHIResourceStateValidator::ValidateTransition(IRHIResource* resource,
                                                       ERHIResourceState before,
                                                       ERHIResourceState after,
                                                       uint32 subresource)
    {
        if (!m_enabled)
        {
            return true;
        }

        if (before == after)
        {
            std::snprintf(m_lastError,
                          sizeof(m_lastError),
                          "Resource %p subresource %u: redundant transition 0x%x -> 0x%x",
                          static_cast<void*>(resource),
                          subresource,
                          static_cast<uint32>(before),
                          static_cast<uint32>(after));
            return false;
        }

        return true;
    }

} // namespace NS::RHI
