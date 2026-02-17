/// @file RHIMeshDispatch.cpp
/// @brief メッシュレット描画マネージャー実装
#include "RHIMeshDispatch.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"
#include "RHIMeshPipelineState.h"

#include <algorithm>

namespace NS::RHI
{
    //=========================================================================
    // RHIMeshletDrawManager
    //=========================================================================

    RHIMeshletDrawManager::RHIMeshletDrawManager(IRHIDevice* device, uint32 maxBatches) : m_maxBatches(maxBatches)
    {
        m_batches.reserve(maxBatches);

        // Indirect引数バッファ作成（DispatchMesh引数 = 12 bytes per batch）
        RHIBufferDesc indirectDesc;
        indirectDesc.size = static_cast<uint64>(maxBatches) * 12;
        indirectDesc.usage = ERHIBufferUsage::IndirectArgs;
        indirectDesc.debugName = "MeshletDrawManager_IndirectArgs";
        m_indirectBuffer = device->CreateBuffer(indirectDesc);

        // カウントバッファ（4 bytes）
        RHIBufferDesc countDesc;
        countDesc.size = sizeof(uint32);
        countDesc.usage = ERHIBufferUsage::IndirectArgs;
        countDesc.debugName = "MeshletDrawManager_Count";
        m_countBuffer = device->CreateBuffer(countDesc);
    }

    void RHIMeshletDrawManager::AddBatch(const RHIMeshletBatch& batch)
    {
        if (m_batches.size() < m_maxBatches)
        {
            m_batches.push_back(batch);
        }
    }

    void RHIMeshletDrawManager::SortByMaterial()
    {
        std::sort(m_batches.begin(), m_batches.end(), [](const RHIMeshletBatch& a, const RHIMeshletBatch& b) {
            return a.materialId < b.materialId;
        });
    }

    void RHIMeshletDrawManager::CullBatches(const float* frustumPlanes)
    {
        (void)frustumPlanes;
        // GPUカリングはバックエンド依存
        // CPUフォールバック: 全バッチを可視として扱う
        m_visibleMeshlets = GetTotalMeshlets();
    }

    void RHIMeshletDrawManager::BuildIndirectBuffer(IRHICommandContext* context)
    {
        (void)context;
        // Indirect引数バッファへの書き込みはバックエンド依存
        // 各バッチのメッシュレット数からDispatchMesh引数を生成
    }

    void RHIMeshletDrawManager::DrawAll(IRHICommandContext* context, IRHIMeshPipelineState* pso)
    {
        (void)context;
        (void)pso;
        // context->SetMeshPipelineState(pso);
        // for each batch: context->DispatchMesh(groupX, groupY, groupZ);
    }

    void RHIMeshletDrawManager::DrawByMaterial(
        IRHICommandContext* context, const std::function<IRHIMeshPipelineState*(uint32 materialId)>& psoGetter)
    {
        (void)context;
        (void)psoGetter;
        // マテリアルID変化時にPSOを切り替えて描画
    }

    uint32 RHIMeshletDrawManager::GetTotalMeshlets() const
    {
        uint32 total = 0;
        for (const auto& batch : m_batches)
            total += batch.meshletCount;
        return total;
    }

    uint32 RHIMeshletDrawManager::GetVisibleMeshlets() const
    {
        return m_visibleMeshlets;
    }

    uint32 RHIMeshletDrawManager::GetBatchCount() const
    {
        return static_cast<uint32>(m_batches.size());
    }

    void RHIMeshletDrawManager::Clear()
    {
        m_batches.clear();
        m_visibleMeshlets = 0;
    }

} // namespace NS::RHI
