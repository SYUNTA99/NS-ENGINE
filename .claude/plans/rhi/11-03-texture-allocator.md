# 11-03: テクスチャアロケーター

## 目的

効率的なテクスチャメモリ割り当て機構を定義する。

## 参照ドキュメント

- 11-01-heap-types.md (IRHIHeap)
- 04-01-texture-desc.md (RHITextureDesc)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHITextureAllocator.h`

## TODO

### 1. テクスチャアロケーション

```cpp
#pragma once

#include "IRHITexture.h"
#include "IRHIHeap.h"

namespace NS::RHI
{
    /// テクスチャアロケーション
    struct RHI_API RHITextureAllocation
    {
        /// テクスチャ
        IRHITexture* texture = nullptr;

        /// ヒープ
        IRHIHeap* heap = nullptr;

        /// ヒープのオフセット
        uint64 heapOffset = 0;

        /// サイズ
        uint64 size = 0;

        /// 有効か
        bool IsValid() const { return texture != nullptr; }
    };
}
```

- [ ] RHITextureAllocation 構造体

### 2. テクスチャプール

```cpp
namespace NS::RHI
{
    /// テクスチャプール設定
    struct RHI_API RHITexturePoolConfig
    {
        /// テクスチャ記述（テンプレート）
        RHITextureDesc desc;

        /// 初期数
        uint32 initialCount = 4;

        /// 最大数（0で無制限）
        uint32 maxCount = 0;
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

        /// テクスチャ取得
        IRHITexture* Acquire();

        /// テクスチャ返却
        void Release(IRHITexture* texture);

        //=====================================================================
        // 情報
        //=====================================================================

        /// テクスチャ記述取得
        const RHITextureDesc& GetTextureDesc() const { return m_config.desc; }

        /// 利用可能数
        uint32 GetAvailableCount() const { return m_freeCount; }

        /// 総数
        uint32 GetTotalCount() const { return m_totalCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHITexturePoolConfig m_config;

        IRHITexture** m_freeList = nullptr;
        uint32 m_freeCount = 0;
        uint32 m_freeCapacity = 0;
        uint32 m_totalCount = 0;
    };
}
```

- [ ] RHITexturePoolConfig 構造体
- [ ] RHITexturePool クラス

### 3. トランジェントテクスチャアロケーター

```cpp
namespace NS::RHI
{
    /// トランジェントテクスチャ要求
    struct RHI_API RHITransientTextureRequest
    {
        /// テクスチャ記述
        RHITextureDesc desc;

        /// 使用開始パス
        uint32 firstUsePass = 0;

        /// 使用終了パス
        uint32 lastUsePass = 0;

        /// デバッグ名
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

        /// フレーム開始（要求をクリア）。
        void BeginFrame();

        /// フレーム終了
        void EndFrame();

        //=====================================================================
        // テクスチャ要求
        //=====================================================================

        /// トランジェントテクスチャ要求
        /// @return テクスチャハンドル（実際のテクスチャは確定後に取得）
        uint32 Request(const RHITransientTextureRequest& request);

        /// 複数テクスチャ要求
        void RequestBatch(
            const RHITransientTextureRequest* requests,
            uint32 count,
            uint32* outHandles);

        //=====================================================================
        // 確定
        //=====================================================================

        /// 割り当て確定
        /// すべての要求を分割し、メモリエイリアシングを最適化
        bool Finalize();

        //=====================================================================
        // テクスチャ取得
        //=====================================================================

        /// 確定後にテクスチャ取得
        IRHITexture* GetTexture(uint32 handle) const;

        /// エイリアシングバリアが必要か
        bool NeedsAliasingBarrier(uint32 handle, uint32 passIndex) const;

        /// エイリアシングバリア情報を取得
        IRHITexture* GetPreviousAliasedTexture(uint32 handle) const;

        //=====================================================================
        // 情報
        //=====================================================================

        /// ヒープサイズ
        uint64 GetHeapSize() const { return m_heapSize; }

        /// 現在フレームの使用量
        uint64 GetUsedSize() const { return m_usedSize; }

        /// 割り当てテクスチャ数
        uint32 GetTextureCount() const { return m_textureCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIHeapRef m_heap;
        uint64 m_heapSize = 0;
        uint64 m_usedSize = 0;
        uint32 m_textureCount = 0;

        struct TextureEntry {
            RHITextureAllocation allocation;
            uint32 firstPass;
            uint32 lastPass;
            uint32 aliasedFrom;  // エイリアス元（無効値で非エイリアス）。
        };
        TextureEntry* m_entries = nullptr;
        uint32 m_entryCount = 0;
        uint32 m_entryCapacity = 0;
    };
}
```

- [ ] RHITransientTextureRequest 構造体
- [ ] RHITransientTextureAllocator クラス

### 4. レンダーターゲットプール

```cpp
namespace NS::RHI
{
    /// レンダーターゲット記述キー
    struct RHI_API RHIRenderTargetKey
    {
        uint32 width = 0;
        uint32 height = 0;
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;
        uint32 sampleCount = 1;

        bool operator==(const RHIRenderTargetKey& other) const {
            return width == other.width &&
                   height == other.height &&
                   format == other.format &&
                   sampleCount == other.sampleCount;
        }

        static RHIRenderTargetKey FromDesc(const RHITextureDesc& desc) {
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

        /// フレーム開始（使用マスクをクリア）。
        void BeginFrame();

        /// フレーム終了（未使用RTを解放候補にする）。
        void EndFrame();

        //=====================================================================
        // RT取得返却
        //=====================================================================

        /// RT取得（プールから、なければ作成）。
        IRHITexture* Acquire(const RHIRenderTargetKey& key, const char* debugName = nullptr);

        /// RT取得（記述から）。
        IRHITexture* Acquire(
            uint32 width,
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
        /// @param maxAge 最大未使用フレーム数
        void Trim(uint32 maxAge = 3);

        /// 全RTをクリア
        void Clear();

        //=====================================================================
        // 情報
        //=====================================================================

        /// プール内T数
        uint32 GetPooledCount() const { return m_pooledCount; }

        /// 使用中RT数
        uint32 GetInUseCount() const { return m_inUseCount; }

        /// 総メモリ使用量
        uint64 GetTotalMemoryUsage() const { return m_totalMemory; }

    private:
        IRHIDevice* m_device = nullptr;

        struct PooledRT {
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
}
```

- [ ] RHIRenderTargetKey 構造体
- [ ] RHIRenderTargetPool クラス

### 5. テクスチャメモリアトラス

```cpp
namespace NS::RHI
{
    /// アトラスリージョン
    struct RHI_API RHIAtlasRegion
    {
        /// アトラステクスチャ
        IRHITexture* atlas = nullptr;

        /// 領域（ピクセル）。
        uint32 x = 0;
        uint32 y = 0;
        uint32 width = 0;
        uint32 height = 0;

        /// UV座標（0-1）。
        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 1.0f;
        float v1 = 1.0f;

        /// 有効か
        bool IsValid() const { return atlas != nullptr && width > 0 && height > 0; }
    };

    /// テクスチャアトラスアロケーター
    /// 小さなテクスチャを1つの大きなテクスチャにパック
    class RHI_API RHITextureAtlasAllocator
    {
    public:
        RHITextureAtlasAllocator() = default;

        /// 初期化
        /// @param device デバイス
        /// @param width アトラス幅
        /// @param height アトラス高さ
        /// @param format ピクセルフォーマット
        bool Initialize(
            IRHIDevice* device,
            uint32 width,
            uint32 height,
            ERHIPixelFormat format);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 領域割り当て
        //=====================================================================

        /// 領域割り当て
        RHIAtlasRegion Allocate(uint32 width, uint32 height);

        /// 領域解放
        void Free(const RHIAtlasRegion& region);

        //=====================================================================
        // データアップロード
        //=====================================================================

        /// 領域にデータをアップロード
        void Upload(
            IRHICommandContext* context,
            const RHIAtlasRegion& region,
            const void* data,
            uint32 rowPitch);

        //=====================================================================
        // 情報
        //=====================================================================

        /// アトラステクスチャ取得
        IRHITexture* GetAtlasTexture() const { return m_texture; }

        /// 利用率
        float GetOccupancy() const;

    private:
        IRHIDevice* m_device = nullptr;
        RHITextureRef m_texture;
        uint32 m_width = 0;
        uint32 m_height = 0;

        /// 矩形パッキングアルゴリズム実装
        // Shelf or Guillotine algorithm
    };
}
```

- [ ] RHIAtlasRegion 構造体
- [ ] RHITextureAtlasAllocator クラス

## 検証方法

- [ ] テクスチャプールの取得返却
- [ ] トランジェントテクスチャのエイリアシング
- [ ] レンダーターゲットプールの管理
- [ ] アトラスのパッキング
