/// @file RHITextureAllocator.cpp
/// @brief テクスチャメモリアロケーター実装
#include "RHITextureAllocator.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHITexturePool
    //=========================================================================

    bool RHITexturePool::Initialize(IRHIDevice* device, const RHITexturePoolConfig& config)
    {
        m_device = device;
        m_config = config;

        uint32 const capacity = config.initialCount > 0 ? config.initialCount : 4;
        m_freeList = new IRHITexture*[capacity];
        m_freeCapacity = capacity;
        m_freeCount = 0;
        m_totalCount = 0;

        for (uint32 i = 0; i < config.initialCount; ++i)
        {
            IRHITexture* tex = device->CreateTexture(config.desc, "PoolTexture");
            if (tex != nullptr)
            {
                m_freeList[m_freeCount++] = tex;
                ++m_totalCount;
            }
        }

        return true;
    }

    void RHITexturePool::Shutdown()
    {
        for (uint32 i = 0; i < m_freeCount; ++i)
        {
            if (m_freeList[i] != nullptr)
            {
                m_freeList[i]->Release();
            }
        }

        delete[] m_freeList;
        m_freeList = nullptr;
        m_freeCount = 0;
        m_freeCapacity = 0;
        m_totalCount = 0;
        m_device = nullptr;
    }

    IRHITexture* RHITexturePool::Acquire()
    {
        if (m_freeCount > 0)
        {
            return m_freeList[--m_freeCount];
        }

        if (m_config.maxCount > 0 && m_totalCount >= m_config.maxCount)
        {
            return nullptr;
        }

        IRHITexture* tex = m_device->CreateTexture(m_config.desc, "PoolTexture");
        if (tex != nullptr)
        {
            ++m_totalCount;
        }

        return tex;
    }

    void RHITexturePool::Release(IRHITexture* texture)
    {
        if (texture == nullptr)
        {
            return;
        }

        if (m_freeCount >= m_freeCapacity)
        {
            uint32 const newCap = m_freeCapacity * 2;
            auto** newList = new IRHITexture*[newCap];
            std::memcpy(newList, m_freeList, sizeof(IRHITexture*) * m_freeCount);
            delete[] m_freeList;
            m_freeList = newList;
            m_freeCapacity = newCap;
        }

        m_freeList[m_freeCount++] = texture;
    }

    //=========================================================================
    // RHITransientTextureAllocator
    //=========================================================================

    bool RHITransientTextureAllocator::Initialize(IRHIDevice* device, uint64 heapSize)
    {
        m_device = device;
        m_heapSize = heapSize;
        m_usedSize = 0;
        m_textureCount = 0;

        m_entryCapacity = 64;
        m_entries = new TextureEntry[m_entryCapacity];
        m_entryCount = 0;

        // ヒープ作成はバックエンド依存
        return true;
    }

    void RHITransientTextureAllocator::Shutdown()
    {
        delete[] m_entries;
        m_entries = nullptr;
        m_entryCount = 0;
        m_entryCapacity = 0;
        m_heap = nullptr;
        m_device = nullptr;
        m_heapSize = 0;
        m_usedSize = 0;
        m_textureCount = 0;
    }

    void RHITransientTextureAllocator::BeginFrame()
    {
        m_usedSize = 0;
        m_textureCount = 0;
        m_entryCount = 0;
    }

    void RHITransientTextureAllocator::EndFrame()
    {
        // リソースの解放は次のBeginFrameで行う
    }

    uint32 RHITransientTextureAllocator::Request(const RHITransientTextureRequest& request)
    {
        if (m_entryCount >= m_entryCapacity)
        {
            uint32 const newCap = m_entryCapacity * 2;
            auto* newEntries = new TextureEntry[newCap];
            std::memcpy(newEntries, m_entries, sizeof(TextureEntry) * m_entryCount);
            delete[] m_entries;
            m_entries = newEntries;
            m_entryCapacity = newCap;
        }

        uint32 const handle = m_entryCount;
        auto& entry = m_entries[m_entryCount++];
        entry.allocation = {};
        entry.firstPass = request.firstUsePass;
        entry.lastPass = request.lastUsePass;
        entry.aliasedFrom = UINT32_MAX;

        return handle;
    }

    void RHITransientTextureAllocator::RequestBatch(const RHITransientTextureRequest* requests,
                                                    uint32 count,
                                                    uint32* outHandles)
    {
        for (uint32 i = 0; i < count; ++i)
        {
            outHandles[i] = Request(requests[i]);
        }
    }

    bool RHITransientTextureAllocator::Finalize()
    {
        // エイリアシング最適化を実行
        // ライフタイムが重ならないテクスチャは同じメモリ領域を共有可能
        // 実際の割り当てとエイリアシング解析はバックエンド依存
        m_textureCount = m_entryCount;
        return true;
    }

    IRHITexture* RHITransientTextureAllocator::GetTexture(uint32 handle) const
    {
        if (handle >= m_entryCount)
        {
            return nullptr;
        }
        return m_entries[handle].allocation.texture;
    }

    bool RHITransientTextureAllocator::NeedsAliasingBarrier(uint32 handle, uint32 passIndex) const
    {
        if (handle >= m_entryCount)
        {
            return false;
        }

        const auto& entry = m_entries[handle];
        return entry.firstPass == passIndex && entry.aliasedFrom != UINT32_MAX;
    }

    IRHITexture* RHITransientTextureAllocator::GetPreviousAliasedTexture(uint32 handle) const
    {
        if (handle >= m_entryCount)
        {
            return nullptr;
        }

        uint32 const aliasedFrom = m_entries[handle].aliasedFrom;
        if (aliasedFrom == UINT32_MAX || aliasedFrom >= m_entryCount)
        {
            return nullptr;
        }

        return m_entries[aliasedFrom].allocation.texture;
    }

    //=========================================================================
    // RHIRenderTargetPool
    //=========================================================================

    bool RHIRenderTargetPool::Initialize(IRHIDevice* device)
    {
        m_device = device;
        m_poolCapacity = 32;
        m_pool = new PooledRT[m_poolCapacity];
        m_poolCount = 0;
        m_pooledCount = 0;
        m_inUseCount = 0;
        m_totalMemory = 0;
        m_currentFrame = 0;
        return true;
    }

    void RHIRenderTargetPool::Shutdown()
    {
        Clear();
        delete[] m_pool;
        m_pool = nullptr;
        m_poolCapacity = 0;
        m_device = nullptr;
    }

    void RHIRenderTargetPool::BeginFrame()
    {
        ++m_currentFrame;
    }

    void RHIRenderTargetPool::EndFrame()
    {
        // 自動トリムはオプション
    }

    IRHITexture* RHIRenderTargetPool::Acquire(const RHIRenderTargetKey& key, const char* debugName)
    {
        // プールから一致するRTを検索
        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            if (!m_pool[i].inUse && m_pool[i].key == key)
            {
                m_pool[i].inUse = true;
                m_pool[i].lastUsedFrame = m_currentFrame;
                ++m_inUseCount;
                --m_pooledCount;
                return m_pool[i].texture;
            }
        }

        // 新規作成
        RHITextureDesc desc;
        desc.width = key.width;
        desc.height = key.height;
        desc.format = key.format;
        desc.sampleCount = static_cast<ERHISampleCount>(key.sampleCount);

        IRHITexture* texture = m_device->CreateTexture(desc, debugName);
        if (texture == nullptr)
        {
            return nullptr;
        }

        // プールに追加
        if (m_poolCount >= m_poolCapacity)
        {
            uint32 const newCap = m_poolCapacity * 2;
            auto* newPool = new PooledRT[newCap];
            std::memcpy(newPool, m_pool, sizeof(PooledRT) * m_poolCount);
            delete[] m_pool;
            m_pool = newPool;
            m_poolCapacity = newCap;
        }

        auto& entry = m_pool[m_poolCount++];
        entry.texture = texture;
        entry.key = key;
        entry.lastUsedFrame = m_currentFrame;
        entry.inUse = true;
        ++m_inUseCount;

        return texture;
    }

    void RHIRenderTargetPool::Release(IRHITexture* texture)
    {
        if (texture == nullptr)
        {
            return;
        }

        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            if (m_pool[i].texture == texture && m_pool[i].inUse)
            {
                m_pool[i].inUse = false;
                m_pool[i].lastUsedFrame = m_currentFrame;
                --m_inUseCount;
                ++m_pooledCount;
                return;
            }
        }
    }

    void RHIRenderTargetPool::Trim(uint32 maxAge)
    {
        for (uint32 i = 0; i < m_poolCount;)
        {
            if (!m_pool[i].inUse && m_currentFrame - m_pool[i].lastUsedFrame > maxAge)
            {
                if (m_pool[i].texture != nullptr)
                {
                    m_pool[i].texture->Release();
                }

                // 末尾と入れ替え
                m_pool[i] = m_pool[m_poolCount - 1];
                --m_poolCount;
                --m_pooledCount;
            }
            else
            {
                ++i;
            }
        }
    }

    void RHIRenderTargetPool::Clear()
    {
        for (uint32 i = 0; i < m_poolCount; ++i)
        {
            if (m_pool[i].texture != nullptr)
            {
                m_pool[i].texture->Release();
            }
        }

        m_poolCount = 0;
        m_pooledCount = 0;
        m_inUseCount = 0;
        m_totalMemory = 0;
    }

    //=========================================================================
    // RHITextureAtlasAllocator
    //=========================================================================

    bool RHITextureAtlasAllocator::Initialize(IRHIDevice* device, uint32 width, uint32 height, ERHIPixelFormat format)
    {
        m_device = device;
        m_width = width;
        m_height = height;

        RHITextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.mipLevels = 1;

        m_texture = device->CreateTexture(desc, "TextureAtlas");
        return m_texture != nullptr;
    }

    void RHITextureAtlasAllocator::Shutdown()
    {
        m_texture = nullptr;
        m_device = nullptr;
        m_width = 0;
        m_height = 0;
    }

    RHIAtlasRegion RHITextureAtlasAllocator::Allocate(uint32 width, uint32 height)
    {
        RHIAtlasRegion region;
        // パッキングアルゴリズム（Shelf/Skyline/MaxRects等）はバックエンド実装で提供
        (void)width;
        (void)height;
        return region;
    }

    void RHITextureAtlasAllocator::Free(const RHIAtlasRegion& region)
    {
        // 領域解放はパッキングアルゴリズム依存
        (void)region;
    }

    void RHITextureAtlasAllocator::Upload(IRHICommandContext* context,
                                          const RHIAtlasRegion& region,
                                          const void* data,
                                          uint32 rowPitch)
    {
        // テクスチャ領域へのアップロードはバックエンド依存
        (void)context;
        (void)region;
        (void)data;
        (void)rowPitch;
    }

    float RHITextureAtlasAllocator::GetOccupancy() const
    {
        // 使用率計算はパッキングアルゴリズム依存
        return 0.0F;
    }

} // namespace NS::RHI
