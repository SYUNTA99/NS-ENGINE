/// @file RHIResourceTable.cpp
/// @brief 繝ｪ繧ｽ繝ｼ繧ｹ繝・・繝悶Ν繝ｻBindless繝ｪ繧ｽ繝ｼ繧ｹ繝・・繝悶Ν螳溯｣・
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

    // 譌｢蟄倥お繝ｳ繝医Μ繧呈峩譁ｰ縲√↑縺代ｌ縺ｰ霑ｽ蜉
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
        // 繝ｪ繧ｽ繝ｼ繧ｹ繝・・繝悶Ν縺ｮ繝舌う繝ｳ繝峨・繝舌ャ繧ｯ繧ｨ繝ｳ繝我ｾ晏ｭ・
        // 蜷・お繝ｳ繝医Μ縺ｮ繧ｿ繧､繝励↓蠢懊§縺ｦSetGraphicsRootDescriptorTable遲峨ｒ蜻ｼ縺ｶ
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
        // Bindless繝偵・繝励・蜿門ｾ励・繝舌ャ繧ｯ繧ｨ繝ｳ繝我ｾ晏ｭ・
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
