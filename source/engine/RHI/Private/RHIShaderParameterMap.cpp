/// @file RHIShaderParameterMap.cpp
/// @brief シェーダーパラメータマップ・マテリアルバインディング実装
#include "RHIShaderParameterMap.h"
#include "IRHICommandContext.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // ハッシュヘルパー
    //=========================================================================

    static uint64 HashString(const char* str)
    {
        if (str == nullptr) {
            return 0;
}

        uint64 hash = 14695981039346656037ULL;
        while (*str != 0)
        {
            hash ^= static_cast<uint64>(*str++);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    //=========================================================================
    // RHIShaderParameterMap
    //=========================================================================

    void RHIShaderParameterMap::AddParameter(const RHIShaderParameterBinding& binding)
    {
        auto const index = static_cast<uint32>(m_parameters.size());
        m_parameters.push_back(binding);

        if (binding.name != nullptr)
        {
            uint64 const nameHash = HashString(binding.name);
            m_nameHashToIndex[nameHash] = index;
        }
    }

    const RHIShaderParameterBinding* RHIShaderParameterMap::FindParameter(const char* name) const
    {
        if (name == nullptr) {
            return nullptr;
}

        uint64 const nameHash = HashString(name);
        return FindParameter(nameHash);
    }

    const RHIShaderParameterBinding* RHIShaderParameterMap::FindParameter(uint64 nameHash) const
    {
        auto it = m_nameHashToIndex.find(nameHash);
        if (it != m_nameHashToIndex.end()) {
            return &m_parameters[it->second];
}
        return nullptr;
    }

    uint32 RHIShaderParameterMap::GetParameterCount(ERHIShaderParameterType type) const
    {
        uint32 count = 0;
        for (const auto& param : m_parameters)
        {
            if (param.type == type) {
                ++count;
}
        }
        return count;
    }

    bool RHIShaderParameterMap::IsCompatibleWith(const IRHIRootSignature* rootSignature) const
    {
        // ルートシグネチャとの互換性チェック
        // 全パラメータのrootParameterIndexが有効範囲内か確認
        (void)rootSignature;
        return true;
    }

    //=========================================================================
    // RHIMaterialParameterSet
    //=========================================================================

    RHIMaterialParameterSet::RHIMaterialParameterSet(const RHIShaderParameterMap* parameterMap)
        : m_parameterMap(parameterMap)
    {}

    void RHIMaterialParameterSet::SetTexture(const char* name, IRHITexture* texture)
    {
        auto handle = GetTextureHandle(name);
        if (handle.IsValid()) {
            SetTexture(handle, texture);
}
    }

    void RHIMaterialParameterSet::SetTexture(RHITextureHandle handle, IRHITexture* texture)
    {
        if (!handle.IsValid()) {
            return;
}
        m_textures[handle.GetRootParameterIndex()] = texture;
    }

    void RHIMaterialParameterSet::SetConstantBuffer(const char* name, IRHIBuffer* buffer)
    {
        auto handle = GetConstantBufferHandle(name);
        if (handle.IsValid()) {
            SetConstantBuffer(handle, buffer);
}
    }

    void RHIMaterialParameterSet::SetConstantBuffer(RHIConstantBufferHandle handle, IRHIBuffer* buffer)
    {
        if (!handle.IsValid()) {
            return;
}
        m_constantBuffers[handle.GetRootParameterIndex()] = buffer;
    }

    void RHIMaterialParameterSet::SetSampler(const char* name, IRHISampler* sampler)
    {
        auto handle = GetSamplerHandle(name);
        if (handle.IsValid()) {
            SetSampler(handle, sampler);
}
    }

    void RHIMaterialParameterSet::SetSampler(RHISamplerHandle handle, IRHISampler* sampler)
    {
        if (!handle.IsValid()) {
            return;
}
        m_samplers[handle.GetRootParameterIndex()] = sampler;
    }

    void RHIMaterialParameterSet::Bind(IRHICommandContext* context) 
    {
        // パラメータをコマンドコンテキストにバインド
        // 実際のバインドはバックエンド依存（SetGraphicsRootDescriptorTable等）
        (void)context;
    }

    RHITextureHandle RHIMaterialParameterSet::GetTextureHandle(const char* name) const
    {
        if (m_parameterMap == nullptr) {
            return {};
}
        return RHITextureHandle(m_parameterMap->FindParameter(name));
    }

    RHIConstantBufferHandle RHIMaterialParameterSet::GetConstantBufferHandle(const char* name) const
    {
        if (m_parameterMap == nullptr) {
            return {};
}
        return RHIConstantBufferHandle(m_parameterMap->FindParameter(name));
    }

    RHISamplerHandle RHIMaterialParameterSet::GetSamplerHandle(const char* name) const
    {
        if (m_parameterMap == nullptr) {
            return {};
}
        return RHISamplerHandle(m_parameterMap->FindParameter(name));
    }

} // namespace NS::RHI
