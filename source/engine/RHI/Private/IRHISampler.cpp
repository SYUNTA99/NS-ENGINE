/// @file IRHISampler.cpp
/// @brief サンプラーハッシュ計算、キャッシュ、マネージャー実装
#include "IRHISampler.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // CalculateSamplerDescHash
    //=========================================================================

    uint64 CalculateSamplerDescHash(const RHISamplerDesc& desc)
    {
        // FNV-1a ハッシュ
        uint64 hash = 14695981039346656037ULL;
        const auto* bytes = reinterpret_cast<const uint8*>(&desc);
        for (size_t i = 0; i < sizeof(RHISamplerDesc); ++i)
        {
            hash ^= static_cast<uint64>(bytes[i]);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    //=========================================================================
    // RHISamplerCache
    //=========================================================================

    bool RHISamplerCache::Initialize(IRHIDevice* device, uint32 maxCachedSamplers)
    {
        m_device = device;
        m_cacheCapacity = maxCachedSamplers;
        m_cache = new CacheEntry[maxCachedSamplers]{};
        m_cacheCount = 0;
        m_stats = {};
        return true;
    }

    void RHISamplerCache::Shutdown()
    {
        // プリセットサンプラー解放
        m_pointSampler.Reset();
        m_pointClampSampler.Reset();
        m_linearSampler.Reset();
        m_linearClampSampler.Reset();
        m_shadowPCFSampler.Reset();

        delete[] m_cache;
        m_cache = nullptr;
        m_cacheCount = 0;
        m_cacheCapacity = 0;
        m_device = nullptr;
    }

    IRHISampler* RHISamplerCache::GetOrCreate(const RHISamplerDesc& desc)
    {
        if (m_device == nullptr) {
            return nullptr;
}

        uint64 const hash = CalculateSamplerDescHash(desc);

        // キャッシュ検索
        for (uint32 i = 0; i < m_cacheCount; ++i)
        {
            if (m_cache[i].hash == hash && m_cache[i].sampler)
            {
                ++m_stats.hitCount;
                return m_cache[i].sampler.Get();
            }
        }

        // ミス: 新規作成
        ++m_stats.missCount;
        IRHISampler* sampler = m_device->CreateSampler(desc);
        if (sampler == nullptr) {
            return nullptr;
}

        if (m_cacheCount < m_cacheCapacity)
        {
            m_cache[m_cacheCount].hash = hash;
            m_cache[m_cacheCount].sampler = sampler;
            ++m_cacheCount;
            ++m_stats.cachedCount;
        }

        return sampler;
    }

    IRHISampler* RHISamplerCache::GetPointSampler()
    {
        if (!m_pointSampler) {
            m_pointSampler = GetOrCreate(RHISamplerDesc::Point());
}
        return m_pointSampler.Get();
    }

    IRHISampler* RHISamplerCache::GetPointClampSampler()
    {
        if (!m_pointClampSampler) {
            m_pointClampSampler = GetOrCreate(RHISamplerDesc::PointClamp());
}
        return m_pointClampSampler.Get();
    }

    IRHISampler* RHISamplerCache::GetLinearSampler()
    {
        if (!m_linearSampler) {
            m_linearSampler = GetOrCreate(RHISamplerDesc::Linear());
}
        return m_linearSampler.Get();
    }

    IRHISampler* RHISamplerCache::GetLinearClampSampler()
    {
        if (!m_linearClampSampler) {
            m_linearClampSampler = GetOrCreate(RHISamplerDesc::LinearClamp());
}
        return m_linearClampSampler.Get();
    }

    IRHISampler* RHISamplerCache::GetAnisotropicSampler(uint32 maxAniso)
    {
        // 異方性はパラメータが変わるためプリセットキャッシュせず、通常キャッシュ経由
        return GetOrCreate(RHISamplerDesc::Anisotropic(maxAniso));
    }

    IRHISampler* RHISamplerCache::GetShadowPCFSampler()
    {
        if (!m_shadowPCFSampler) {
            m_shadowPCFSampler = GetOrCreate(RHISamplerDesc::ShadowPCF());
}
        return m_shadowPCFSampler.Get();
    }

    void RHISamplerCache::Clear()
    {
        for (uint32 i = 0; i < m_cacheCount; ++i)
        {
            m_cache[i].hash = 0;
            m_cache[i].sampler.Reset();
        }
        m_cacheCount = 0;
        m_stats.cachedCount = 0;

        m_pointSampler.Reset();
        m_pointClampSampler.Reset();
        m_linearSampler.Reset();
        m_linearClampSampler.Reset();
        m_shadowPCFSampler.Reset();
    }

    //=========================================================================
    // RHISamplerManager
    //=========================================================================

    bool RHISamplerManager::Initialize(IRHIDevice* device)
    {
        m_device = device;
        if (!m_cache.Initialize(device)) {
            return false;
}

        m_namedSamplers = new NamedSampler[64];
        m_namedCount = 0;
        return true;
    }

    void RHISamplerManager::Shutdown()
    {
        m_cache.Shutdown();
        delete[] m_namedSamplers;
        m_namedSamplers = nullptr;
        m_namedCount = 0;
        m_device = nullptr;
    }

    IRHISampler* RHISamplerManager::GetSampler(const RHISamplerDesc& desc)
    {
        return m_cache.GetOrCreate(desc);
    }

    void RHISamplerManager::RegisterSampler(const char* name, IRHISampler* sampler)
    {
        if ((name == nullptr) || (sampler == nullptr) || m_namedCount >= 64) {
            return;
}

        // 既存エントリの上書きチェック
        for (uint32 i = 0; i < m_namedCount; ++i)
        {
            if (std::strcmp(m_namedSamplers[i].name, name) == 0)
            {
                m_namedSamplers[i].sampler = sampler;
                return;
            }
        }

        // 新規登録
        std::strncpy(m_namedSamplers[m_namedCount].name, name, 63);
        m_namedSamplers[m_namedCount].name[63] = '\0';
        m_namedSamplers[m_namedCount].sampler = sampler;
        ++m_namedCount;
    }

    IRHISampler* RHISamplerManager::GetSampler(const char* name) const
    {
        if (name == nullptr) {
            return nullptr;
}

        for (uint32 i = 0; i < m_namedCount; ++i)
        {
            if (std::strcmp(m_namedSamplers[i].name, name) == 0) {
                return m_namedSamplers[i].sampler;
}
        }
        return nullptr;
    }

    BindlessSamplerIndex RHISamplerManager::RegisterBindless(IRHISampler* sampler)
    {
        // バックエンド依存: デバイス経由でBindlessヒープに登録
        // 現時点ではスタブ
        (void)sampler;
        return BindlessSamplerIndex{};
    }

    void RHISamplerManager::UnregisterBindless(BindlessSamplerIndex index)
    {
        // バックエンド依存: スタブ
        (void)index;
    }

} // namespace NS::RHI
