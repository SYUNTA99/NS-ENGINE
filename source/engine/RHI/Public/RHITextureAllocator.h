/// @file RHITextureAllocator.h
/// @brief テクスチャメモリアロケーター
/// @details テクスチャプール、トランジェントアロケーター、レンダーターゲットプール、アトラスを提供。
/// @see 11-03-texture-allocator.md
#pragma once

#include "IRHITexture.h"
#include "RHIMacros.h"
#include "RHIMemoryTypes.h"

namespace NS::RHI
{
    class IRHIDevice;
    class IRHICommandContext;

    //=========================================================================
    // RHITextureAllocation (11-03)
    //=========================================================================

    /// テクスチャアロケーション
    struct RHI_API RHITextureAllocation
    {
        IRHITexture* texture = nullptr; ///< テクスチャ
        IRHIHeap* heap = nullptr;       ///< ヒープ
        uint64 heapOffset = 0;          ///< ヒープ内オフセット
        uint64 size = 0;                ///< サイズ

        bool IsValid() const { return texture != nullptr; }
    };

    //=========================================================================
    // RHITexturePoolConfig / RHITexturePool (11-03)
    //=========================================================================

    /// テクスチャプール設定
    struct RHI_API RHITexturePoolConfig
    {
        RHITextureDesc desc;     ///< テクスチャ記述（テンプレート）
        uint32 initialCount = 4; ///< 初期数
        uint32 maxCount = 0;     ///< 最大数（0で無制限）
    };

    /// テクスチャプール
    /// 同一記述のテクスチャをプール管理
    class RHI_API RHITexturePool
    {
    public:
        RHITexturePool() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, const RHITexturePoolConfig& config);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 割り当て/解放
        //=====================================================================

        IRHITexture* Acquire();
        void Release(IRHITexture* texture);

        //=====================================================================
        // 情報
        //=====================================================================

        const RHITextureDesc& GetTextureDesc() const { return m_config.desc; }
        uint32 GetAvailableCount() const { return m_freeCount; }
        uint32 GetTotalCount() const { return m_totalCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHITexturePoolConfig m_config;

        IRHITexture** m_freeList = nullptr;
        uint32 m_freeCount = 0;
        uint32 m_freeCapacity = 0;
        uint32 m_totalCount = 0;
    };

    //=========================================================================
    // RHITransientTextureAllocator (11-03)
    //=========================================================================

    /// トランジェントテクスチャ要求
    struct RHI_API RHITransientTextureRequest
    {
        RHITextureDesc desc;     ///< テクスチャ記述
        uint32 firstUsePass = 0; ///< 使用開始パス
        uint32 lastUsePass = 0;  ///< 使用終了パス
        const char* debugName = nullptr;
    };

    /// トランジェントテクスチャアロケーター
    /// フレーム内のメモリエイリアシングを行うテクスチャ管理
    class RHI_API RHITransientTextureAllocator
    {
    public:
        RHITransientTextureAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint64 heapSize);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // テクスチャ要求
        //=====================================================================

        /// トランジェントテクスチャ要求
        /// @return ハンドル（確定後にGetTextureで取得）
        uint32 Request(const RHITransientTextureRequest& request);

        /// 複数テクスチャ要求
        void RequestBatch(const RHITransientTextureRequest* requests, uint32 count, uint32* outHandles);

        //=====================================================================
        // 確定
        //=====================================================================

        /// 割り当て確定（エイリアシング最適化実行）
        bool Finalize();

        //=====================================================================
        // テクスチャ取得
        //=====================================================================

        /// 確定後にテクスチャ取得
        IRHITexture* GetTexture(uint32 handle) const;

        /// エイリアシングバリアが必要か
        bool NeedsAliasingBarrier(uint32 handle, uint32 passIndex) const;

        /// エイリアス元テクスチャ取得
        IRHITexture* GetPreviousAliasedTexture(uint32 handle) const;

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetHeapSize() const { return m_heapSize; }
        uint64 GetUsedSize() const { return m_usedSize; }
        uint32 GetTextureCount() const { return m_textureCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIHeapRef m_heap;
        uint64 m_heapSize = 0;
        uint64 m_usedSize = 0;
        uint32 m_textureCount = 0;

        struct TextureEntry
        {
            RHITextureAllocation allocation;
            uint32 firstPass;
            uint32 lastPass;
            uint32 aliasedFrom; ///< エイリアス元（UINT32_MAXで非エイリアス）
        };
        TextureEntry* m_entries = nullptr;
        uint32 m_entryCount = 0;
        uint32 m_entryCapacity = 0;
    };

    //=========================================================================
    // RHIRenderTargetPool (11-03)
    //=========================================================================

    /// レンダーターゲット記述キー
    struct RHI_API RHIRenderTargetKey
    {
        uint32 width = 0;
        uint32 height = 0;
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;
        uint32 sampleCount = 1;

        bool operator==(const RHIRenderTargetKey& other) const
        {
            return width == other.width && height == other.height && format == other.format &&
                   sampleCount == other.sampleCount;
        }

        static RHIRenderTargetKey FromDesc(const RHITextureDesc& desc)
        {
            RHIRenderTargetKey key;
            key.width = desc.width;
            key.height = desc.height;
            key.format = desc.format;
            key.sampleCount = desc.sampleCount;
            return key;
        }
    };

    /// レンダーターゲットプール
    /// 動的解像度やポストプロセス用のRT管理
    class RHI_API RHIRenderTargetPool
    {
    public:
        RHIRenderTargetPool() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // RT取得/返却
        //=====================================================================

        /// RT取得（プールから、なければ作成）
        IRHITexture* Acquire(const RHIRenderTargetKey& key, const char* debugName = nullptr);

        /// RT取得（パラメータ直指定）
        IRHITexture* Acquire(uint32 width,
                             uint32 height,
                             ERHIPixelFormat format,
                             uint32 sampleCount = 1,
                             const char* debugName = nullptr)
        {
            RHIRenderTargetKey key;
            key.width = width;
            key.height = height;
            key.format = format;
            key.sampleCount = sampleCount;
            return Acquire(key, debugName);
        }

        /// RT返却
        void Release(IRHITexture* texture);

        //=====================================================================
        // 管理
        //=====================================================================

        /// 未使用RTをトリム
        void Trim(uint32 maxAge = 3);

        /// 全RTをクリア
        void Clear();

        //=====================================================================
        // 情報
        //=====================================================================

        uint32 GetPooledCount() const { return m_pooledCount; }
        uint32 GetInUseCount() const { return m_inUseCount; }
        uint64 GetTotalMemoryUsage() const { return m_totalMemory; }

    private:
        IRHIDevice* m_device = nullptr;

        struct PooledRT
        {
            IRHITexture* texture;
            RHIRenderTargetKey key;
            uint32 lastUsedFrame;
            bool inUse;
        };
        PooledRT* m_pool = nullptr;
        uint32 m_poolCount = 0;
        uint32 m_poolCapacity = 0;
        uint32 m_pooledCount = 0;
        uint32 m_inUseCount = 0;
        uint64 m_totalMemory = 0;
        uint32 m_currentFrame = 0;
    };

    //=========================================================================
    // RHITextureAtlasAllocator (11-03)
    //=========================================================================

    /// アトラスリージョン
    struct RHI_API RHIAtlasRegion
    {
        IRHITexture* atlas = nullptr;
        uint32 x = 0;
        uint32 y = 0;
        uint32 width = 0;
        uint32 height = 0;
        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 1.0f;
        float v1 = 1.0f;

        bool IsValid() const { return atlas != nullptr && width > 0 && height > 0; }
    };

    /// テクスチャアトラスアロケーター
    /// 小さなテクスチャを1つの大きなテクスチャにパック
    class RHI_API RHITextureAtlasAllocator
    {
    public:
        RHITextureAtlasAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 width, uint32 height, ERHIPixelFormat format);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 領域割り当て
        //=====================================================================

        RHIAtlasRegion Allocate(uint32 width, uint32 height);
        void Free(const RHIAtlasRegion& region);

        //=====================================================================
        // データアップロード
        //=====================================================================

        /// 領域にデータをアップロード
        void Upload(IRHICommandContext* context, const RHIAtlasRegion& region, const void* data, uint32 rowPitch);

        //=====================================================================
        // 情報
        //=====================================================================

        IRHITexture* GetAtlasTexture() const { return m_texture; }
        float GetOccupancy() const;

    private:
        IRHIDevice* m_device = nullptr;
        RHITextureRef m_texture;
        uint32 m_width = 0;
        uint32 m_height = 0;
    };

} // namespace NS::RHI
