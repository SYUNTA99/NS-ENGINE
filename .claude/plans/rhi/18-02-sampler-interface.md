# 18-02: IRHISampler インターフェース

## 目的

サンプラーオブジェクトポインターフェースを定義する。

## 参照ドキュメント

- 18-01-sampler-desc.md (RHISamplerDesc)
- 10-03-offline-descriptor.md (ディスクリプタ管理

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHISampler.h` (部分

## TODO

### 1. IRHISampler インターフェース

```cpp
namespace NS::RHI
{
    /// サンプラー
    class RHI_API IRHISampler : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Sampler)

        virtual ~IRHISampler() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 記述取得
        virtual const RHISamplerDesc& GetDesc() const = 0;

        //=====================================================================
        // フィルター情報
        //=====================================================================

        /// 縮小フィルター取得
        ERHIFilter GetMinFilter() const { return GetDesc().minFilter; }

        /// 拡大フィルター取得
        ERHIFilter GetMagFilter() const { return GetDesc().magFilter; }

        /// ミップフィルター取得
        ERHIFilter GetMipFilter() const { return GetDesc().mipFilter; }

        /// 異方性サンプラーか
        bool IsAnisotropic() const {
            return GetMinFilter() == ERHIFilter::Anisotropic ||
                   GetMagFilter() == ERHIFilter::Anisotropic;
        }

        /// 最大異方性取得
        uint32 GetMaxAnisotropy() const { return GetDesc().maxAnisotropy; }

        //=====================================================================
        // アドレスモード情報
        //=====================================================================

        /// Uアドレスモード取得
        ERHITextureAddressMode GetAddressU() const { return GetDesc().addressU; }

        /// Vアドレスモード取得
        ERHITextureAddressMode GetAddressV() const { return GetDesc().addressV; }

        /// Wアドレスモード取得
        ERHITextureAddressMode GetAddressW() const { return GetDesc().addressW; }

        //=====================================================================
        // 比較サンプラー情報
        //=====================================================================

        /// 比較サンプラーか
        bool IsComparisonSampler() const { return GetDesc().enableComparison; }

        /// 比較関数取得
        ERHICompareFunc GetComparisonFunc() const { return GetDesc().comparisonFunc; }

        //=====================================================================
        // ディスクリプタ
        //=====================================================================

        /// CPUディスクリプタハンドル取得
        virtual RHICPUDescriptorHandle GetCPUDescriptorHandle() const = 0;
    };

    using RHISamplerRef = TRefCountPtr<IRHISampler>;
}
```

- [ ] IRHISampler インターフェース

### 2. サンプラー作成

```cpp
namespace NS::RHI
{
    /// サンプラー作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// サンプラー作成
        virtual IRHISampler* CreateSampler(
            const RHISamplerDesc& desc,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] CreateSampler

### 3. サンプラーキャッシュ

```cpp
namespace NS::RHI
{
    /// サンプラーキャッシュ
    /// 同一記述のサンプラーを再利用
    class RHI_API RHISamplerCache
    {
    public:
        RHISamplerCache() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 maxCachedSamplers = 256);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // サンプラー取得
        //=====================================================================

        /// サンプラー取得（キャッシュヒット時は既存を返す）。
        IRHISampler* GetOrCreate(const RHISamplerDesc& desc);

        /// プリセットサンプラー取得
        IRHISampler* GetPointSampler();
        IRHISampler* GetPointClampSampler();
        IRHISampler* GetLinearSampler();
        IRHISampler* GetLinearClampSampler();
        IRHISampler* GetAnisotropicSampler(uint32 maxAniso = 16);
        IRHISampler* GetShadowPCFSampler();

        //=====================================================================
        // キャッシュ管理
        //=====================================================================

        /// キャッシュクリア
        void Clear();

        /// キャッシュ統計
        struct CacheStats {
            uint32 cachedCount;
            uint32 hitCount;
            uint32 missCount;
        };
        CacheStats GetStats() const { return m_stats; }

        /// 統計リセット
        void ResetStats() { m_stats = {}; }

    private:
        IRHIDevice* m_device = nullptr;

        struct CacheEntry {
            uint64 hash;
            RHISamplerRef sampler;
        };
        CacheEntry* m_cache = nullptr;
        uint32 m_cacheCount = 0;
        uint32 m_cacheCapacity = 0;

        CacheStats m_stats = {};

        /// プリセットサンプラー
        RHISamplerRef m_pointSampler;
        RHISamplerRef m_pointClampSampler;
        RHISamplerRef m_linearSampler;
        RHISamplerRef m_linearClampSampler;
        RHISamplerRef m_shadowPCFSampler;
    };
}
```

- [ ] RHISamplerCache クラス

### 4. グローバルサンプラーマネージャー

```cpp
namespace NS::RHI
{
    /// グローバルサンプラーマネージャー
    /// デバイス全体でのサンプラー管理
    class RHI_API RHISamplerManager
    {
    public:
        RHISamplerManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // サンプラー取得
        //=====================================================================

        /// 記述からサンプラー取得
        IRHISampler* GetSampler(const RHISamplerDesc& desc);

        /// 名前付きサンプラー登録
        void RegisterSampler(const char* name, IRHISampler* sampler);

        /// 名前付きサンプラー取得
        IRHISampler* GetSampler(const char* name) const;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// 共通サンプラーへのアクセス
        IRHISampler* GetPointSampler() { return m_cache.GetPointSampler(); }
        IRHISampler* GetLinearSampler() { return m_cache.GetLinearSampler(); }
        IRHISampler* GetAnisotropicSampler() { return m_cache.GetAnisotropicSampler(); }
        IRHISampler* GetShadowSampler() { return m_cache.GetShadowPCFSampler(); }

        //=====================================================================
        // Bindless
        //=====================================================================

        /// サンプラーをBindlessヒープに登録
        RHIBindlessIndex RegisterBindless(IRHISampler* sampler);

        /// Bindless登録解除
        void UnregisterBindless(RHIBindlessIndex index);

    private:
        IRHIDevice* m_device = nullptr;
        RHISamplerCache m_cache;

        // 名前→サンプラーマッピング
        struct NamedSampler {
            char name[64];
            IRHISampler* sampler;
        };
        NamedSampler* m_namedSamplers = nullptr;
        uint32 m_namedCount = 0;
    };
}
```

- [ ] RHISamplerManager クラス

### 5. 静的サンプラーとの統合

```cpp
namespace NS::RHI
{
    /// 静的サンプラーとIRHISamplerの変換
    namespace RHISamplerConversion
    {
        /// RHISamplerDescからRHIStaticSamplerDescへ変換
        inline RHIStaticSamplerDesc ToStaticSampler(
            const RHISamplerDesc& desc,
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIStaticSamplerDesc staticDesc;
            staticDesc.shaderRegister = shaderRegister;
            staticDesc.registerSpace = registerSpace;
            staticDesc.shaderVisibility = visibility;

            // フィルター変換
            if (desc.minFilter == ERHIFilter::Anisotropic) {
                staticDesc.filter = ERHIFilterMode::Anisotropic;
            } else if (desc.minFilter == ERHIFilter::Linear) {
                staticDesc.filter = ERHIFilterMode::Linear;
            } else {
                staticDesc.filter = ERHIFilterMode::Point;
            }

            // アドレスモード変換
            staticDesc.addressU = static_cast<ERHIAddressMode>(desc.addressU);
            staticDesc.addressV = static_cast<ERHIAddressMode>(desc.addressV);
            staticDesc.addressW = static_cast<ERHIAddressMode>(desc.addressW);

            staticDesc.mipLODBias = desc.mipLODBias;
            staticDesc.maxAnisotropy = desc.maxAnisotropy;
            staticDesc.comparisonFunc = desc.comparisonFunc;
            staticDesc.minLOD = desc.minLOD;
            staticDesc.maxLOD = desc.maxLOD;

            return staticDesc;
        }

        /// RHIStaticSamplerDescからRHISamplerDescへ変換
        inline RHISamplerDesc FromStaticSampler(const RHIStaticSamplerDesc& staticDesc)
        {
            RHISamplerDesc desc;

            // フィルター変換
            if (staticDesc.filter == ERHIFilterMode::Anisotropic) {
                desc.minFilter = ERHIFilter::Anisotropic;
                desc.magFilter = ERHIFilter::Anisotropic;
            } else if (staticDesc.filter == ERHIFilterMode::Linear) {
                desc.minFilter = ERHIFilter::Linear;
                desc.magFilter = ERHIFilter::Linear;
            } else {
                desc.minFilter = ERHIFilter::Point;
                desc.magFilter = ERHIFilter::Point;
            }
            desc.mipFilter = ERHIFilter::Linear;

            desc.addressU = static_cast<ERHITextureAddressMode>(staticDesc.addressU);
            desc.addressV = static_cast<ERHITextureAddressMode>(staticDesc.addressV);
            desc.addressW = static_cast<ERHITextureAddressMode>(staticDesc.addressW);

            desc.mipLODBias = staticDesc.mipLODBias;
            desc.maxAnisotropy = staticDesc.maxAnisotropy;
            desc.comparisonFunc = staticDesc.comparisonFunc;
            desc.minLOD = staticDesc.minLOD;
            desc.maxLOD = staticDesc.maxLOD;

            return desc;
        }
    }
}
```

- [ ] RHISamplerConversion ヘルパー

## 検証方法

- [ ] サンプラー作成/破棄
- [ ] キャッシュのヒット/ミス
- [ ] プリセットの動作
- [ ] Bindless登録
