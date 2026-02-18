/// @file RHIResourceTable.cpp
/// @brief リソースチE�Eブル・BindlessリソースチE�Eブル実裁E
#include "RHIResourceTable.h"
#include "IRHICommandContext.h"
#include "IRHIComputeContext.h"
#include "IRHIDevice.h"
#include "IRHISampler.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIResourceTable
    //=========================================================================

    RHIResourceTable::RHIResourceTable(uint32 capacity)
    {
        m_entries.reserve(capacity);
    }

    // 既存エントリを更新、なければ追加
    static void SetEntry(std::vector<RHIResourceTableEntry>& entries,
                         ERHIResourceTableEntryType type,
                         uint32 slot,
                         IRHIResource* resource)
    {
        for (auto& entry : entries)
        {
            if (entry.type == type && entry.slot == slot)
            {
                entry.resource = resource;
                entry.descriptorIndex = 0;
                return;
            }
        }
        entries.push_back({type, slot, resource, 0});
    }

    void RHIResourceTable::SetSRV(uint32 slot, IRHITexture* texture)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::SRV_Texture, slot, static_cast<IRHIResource*>(texture));
    }

    void RHIResourceTable::SetSRV(uint32 slot, IRHIBuffer* buffer)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::SRV_Buffer, slot, static_cast<IRHIResource*>(buffer));
    }

    void RHIResourceTable::SetUAV(uint32 slot, IRHITexture* texture)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::UAV_Texture, slot, static_cast<IRHIResource*>(texture));
    }

    void RHIResourceTable::SetUAV(uint32 slot, IRHIBuffer* buffer)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::UAV_Buffer, slot, static_cast<IRHIResource*>(buffer));
    }

    void RHIResourceTable::SetCBV(uint32 slot, IRHIBuffer* buffer)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::CBV, slot, static_cast<IRHIResource*>(buffer));
    }

    void RHIResourceTable::SetSampler(uint32 slot, IRHISampler* sampler)
    {
        SetEntry(m_entries, ERHIResourceTableEntryType::Sampler, slot, static_cast<IRHIResource*>(sampler));
    }

    const RHIResourceTableEntry* RHIResourceTable::GetEntry(ERHIResourceTableEntryType type, uint32 slot) const
    {
        for (const auto& entry : m_entries)
        {
            if (entry.type == type && entry.slot == slot)
                return &entry;
        }
        return nullptr;
    }

    void RHIResourceTable::Bind(IRHICommandContext* context, EShaderFrequency stage)
    {
        // リソースチE�Eブルのバインド�Eバックエンド依孁E
        // 吁E��ントリのタイプに応じてSetGraphicsRootDescriptorTable等を呼ぶ
        (void)context;
        (void)stage;
    }

    void RHIResourceTable::Bind(IRHIComputeContext* context)
    {
        (void)context;
    }

    //=========================================================================
    // RHIBindlessResourceTable
    //=========================================================================

    RHIBindlessResourceTable::RHIBindlessResourceTable(IRHIDevice* device) : m_device(device)
    {
        if (device)
        {
            m_srvUavHeap = device->GetBindlessSRVUAVHeap();
            m_samplerHeap = device->GetBindlessSamplerHeap();
        }
    }

    uint32 RHIBindlessResourceTable::RegisterTexture(IRHITexture* texture)
    {
        uint32 index = 0;
        if (!m_freeIndices.empty())
        {
            index = m_freeIndices.front();
            m_freeIndices.pop();
            m_resources[index] = static_cast<IRHIResource*>(texture);
        }
        else
        {
            index = static_cast<uint32>(m_resources.size());
            m_resources.push_back(static_cast<IRHIResource*>(texture));
        }
        return index;
    }

    uint32 RHIBindlessResourceTable::RegisterBuffer(IRHIBuffer* buffer)
    {
        uint32 index = 0;
        if (!m_freeIndices.empty())
        {
            index = m_freeIndices.front();
            m_freeIndices.pop();
            m_resources[index] = static_cast<IRHIResource*>(buffer);
        }
        else
        {
            index = static_cast<uint32>(m_resources.size());
            m_resources.push_back(static_cast<IRHIResource*>(buffer));
        }
        return index;
    }

    uint32 RHIBindlessResourceTable::RegisterSampler(IRHISampler* sampler)
    {
        uint32 index = 0;
        if (!m_freeIndices.empty())
        {
            index = m_freeIndices.front();
            m_freeIndices.pop();
            m_resources[index] = static_cast<IRHIResource*>(sampler);
        }
        else
        {
            index = static_cast<uint32>(m_resources.size());
            m_resources.push_back(static_cast<IRHIResource*>(sampler));
        }
        return index;
    }

    IRHITexture* RHIBindlessResourceTable::GetTexture(uint32 index) const
    {
        if (index >= m_resources.size() || (m_resources[index] == nullptr))
        {
            return nullptr;
        }
        return dynamic_cast<IRHITexture*>(m_resources[index]);
    }

    IRHIBuffer* RHIBindlessResourceTable::GetBuffer(uint32 index) const
    {
        if (index >= m_resources.size() || (m_resources[index] == nullptr))
        {
            return nullptr;
        }
        return dynamic_cast<IRHIBuffer*>(m_resources[index]);
    }

    void RHIBindlessResourceTable::Unregister(uint32 index)
    {
        if (index < m_resources.size())
        {
            m_resources[index] = nullptr;
            m_freeIndices.push(index);
        }
    }

    void RHIBindlessResourceTable::BindDescriptorHeaps(IRHICommandContext* context) const
    {
        if (context == nullptr)
        {
            return;
        }

        context->SetDescriptorHeaps(m_srvUavHeap, m_samplerHeap);
    }

} // namespace NS::RHI
