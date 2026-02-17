/// @file RHIBoundShaderState.cpp
/// @brief バウンドシェーダーステート・キャッシュ実装
#include "RHIBoundShaderState.h"
#include "IRHIShader.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIBoundShaderStateKey
    //=========================================================================

    uint64 RHIBoundShaderStateKey::GetCombinedHash() const
    {
        uint64 hash = 14695981039346656037ULL;

        auto combine = [&hash](uint64 value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };

        combine(vertexShaderHash);
        combine(pixelShaderHash);
        combine(geometryShaderHash);
        combine(hullShaderHash);
        combine(domainShaderHash);
        combine(meshShaderHash);
        combine(amplificationShaderHash);

        return hash;
    }

    //=========================================================================
    // RHIBoundShaderState
    //=========================================================================

    RHIBoundShaderState::RHIBoundShaderState(const RHIBoundShaderStateDesc& desc) : m_inputLayout(desc.inputLayout)
    {
        m_vertexShader = desc.vertexShader;
        m_pixelShader = desc.pixelShader;
        m_geometryShader = desc.geometryShader;
        m_hullShader = desc.hullShader;
        m_domainShader = desc.domainShader;
        m_amplificationShader = desc.amplificationShader;
        m_meshShader = desc.meshShader;
        

        // キー構築（128bitハッシュをXOR縮約して64bitキーに）
        auto shaderHash = [](IRHIShader* shader) -> uint64 {
            if (shader == nullptr) { return 0; }
            RHIShaderHash h = shader->GetHash();
            return h.hash[0] ^ h.hash[1];
        };

        m_key.vertexShaderHash = shaderHash(desc.vertexShader);
        m_key.pixelShaderHash = shaderHash(desc.pixelShader);
        m_key.geometryShaderHash = shaderHash(desc.geometryShader);
        m_key.hullShaderHash = shaderHash(desc.hullShader);
        m_key.domainShaderHash = shaderHash(desc.domainShader);
        m_key.meshShaderHash = shaderHash(desc.meshShader);
        m_key.amplificationShaderHash = shaderHash(desc.amplificationShader);

        BuildParameterMap();
    }

    void RHIBoundShaderState::BuildParameterMap()
    {
        // 全シェーダーステージのリフレクション情報を統合
        // 実際のリフレクションはバックエンド依存
    }

    //=========================================================================
    // RHIBoundShaderStateCache
    //=========================================================================

    RHIBoundShaderStateRef RHIBoundShaderStateCache::GetOrCreate(const RHIBoundShaderStateDesc& desc)
    {
        // 暫定キー構築
        RHIBoundShaderStateKey key;
        // descからハッシュを構築する簡易方法
        key.vertexShaderHash = reinterpret_cast<uint64>(desc.vertexShader);
        key.pixelShaderHash = reinterpret_cast<uint64>(desc.pixelShader);
        key.geometryShaderHash = reinterpret_cast<uint64>(desc.geometryShader);
        key.hullShaderHash = reinterpret_cast<uint64>(desc.hullShader);
        key.domainShaderHash = reinterpret_cast<uint64>(desc.domainShader);
        key.meshShaderHash = reinterpret_cast<uint64>(desc.meshShader);
        key.amplificationShaderHash = reinterpret_cast<uint64>(desc.amplificationShader);

        std::lock_guard<std::mutex> const lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end())
        {
            ++m_cacheHits;
            return it->second;
        }

        ++m_cacheMisses;
        auto bss = std::make_shared<RHIBoundShaderState>(desc);
        m_cache[bss->GetKey()] = bss;
        return bss;
    }

    void RHIBoundShaderStateCache::Clear()
    {
        std::lock_guard<std::mutex> const lock(m_mutex);
        m_cache.clear();
        m_cacheHits = 0;
        m_cacheMisses = 0;
    }

} // namespace NS::RHI
