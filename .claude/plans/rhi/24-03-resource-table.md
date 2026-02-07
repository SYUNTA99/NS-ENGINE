# 24-03: リソーステーブル

## 概要

シェーダーが参照するリソース（テクスチャ、バッファ等）のテーブル管理する。
Bindless化への移行を見据えた設計、

## ファイル

- `Source/Engine/RHI/Public/RHIResourceTable.h`

## 依存

- 10-04-bindless.md (Bindless)
- 05-01-srv.md (SRV)
- 24-02-shader-parameter-map.md (パラメータマップ）

## 定義

```cpp
namespace NS::RHI
{

/// リソーステーブルエントリタイプ
enum class ERHIResourceTableEntryType : uint8
{
    SRV_Texture,        ///< テクスチャSRV
    SRV_Buffer,         ///< バッファSRV
    UAV_Texture,        ///< テクスチャUAV
    UAV_Buffer,         ///< バッファUAV
    CBV,                ///< 定数バッファビュー
    Sampler,            ///< サンプラー
};

/// リソーステーブルエントリ
struct RHIResourceTableEntry
{
    ERHIResourceTableEntryType type = ERHIResourceTableEntryType::SRV_Texture;
    uint32 slot = 0;                    ///< シェーダーレジスタスロット
    IRHIResource* resource = nullptr;   ///< リソースポインタ
    uint32 descriptorIndex = 0;         ///< Bindlessインデックス
};

/// リソーステーブル
/// シェーダーが使用するリソースをグループ化
class RHIResourceTable
{
public:
    RHIResourceTable() = default;
    explicit RHIResourceTable(uint32 capacity);

    /// SRV追加（テクスチャ）。
    void SetSRV(uint32 slot, IRHITexture* texture);

    /// SRV追加（バッファ）。
    void SetSRV(uint32 slot, IRHIBuffer* buffer);

    /// UAV追加（テクスチャ）。
    void SetUAV(uint32 slot, IRHITexture* texture);

    /// UAV追加（バッファ）。
    void SetUAV(uint32 slot, IRHIBuffer* buffer);

    /// CBV追加
    void SetCBV(uint32 slot, IRHIBuffer* buffer);

    /// サンプラー追加
    void SetSampler(uint32 slot, IRHISampler* sampler);

    /// エントリ取得
    const RHIResourceTableEntry* GetEntry(ERHIResourceTableEntryType type, uint32 slot) const;

    /// 全エントリ取得
    const std::vector<RHIResourceTableEntry>& GetEntries() const { return m_entries; }

    /// エントリ数
    uint32 GetEntryCount() const { return static_cast<uint32>(m_entries.size()); }

    /// クリア
    void Clear() { m_entries.clear(); }

    /// コマンドコンテキストにバインド
    void Bind(IRHIGraphicsContext* context, ERHIShaderStage stage) const;
    void Bind(IRHIComputeContext* context) const;

private:
    std::vector<RHIResourceTableEntry> m_entries;
};

// ════════════════════════════════════════════════════════════════
// マテリアルリソーステーブル
// ════════════════════════════════════════════════════════════════

/// マテリアル用リソーステーブル
/// PBRマテリアルの標準テクスチャスロットを定義
class RHIMaterialResourceTable : public RHIResourceTable
{
public:
    /// 標準のデュアルテクスチャスロット
    enum class ESlot : uint32
    {
        Albedo = 0,
        Normal = 1,
        MetallicRoughness = 2,
        Occlusion = 3,
        Emissive = 4,
        Height = 5,
        // 拡張スロット
        Custom0 = 8,
        Custom1 = 9,
        Custom2 = 10,
        Custom3 = 11,
    };

    void SetAlbedoMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Albedo), texture); }
    void SetNormalMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Normal), texture); }
    void SetMetallicRoughnessMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::MetallicRoughness), texture); }
    void SetOcclusionMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Occlusion), texture); }
    void SetEmissiveMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Emissive), texture); }
    void SetHeightMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Height), texture); }
};

// ════════════════════════════════════════════════════════════════
// グローバルリソーステーブル
// ════════════════════════════════════════════════════════════════

/// フレーム共通リソーステーブル
/// ビュー定数、ライト、シャドウマップ等
class RHIGlobalResourceTable : public RHIResourceTable
{
public:
    /// 標準グローバルスロット
    enum class ESlot : uint32
    {
        // 定数バッファ
        ViewConstants = 0,
        LightConstants = 1,
        ShadowConstants = 2,
        TimeConstants = 3,

        // テクスチャ
        ShadowMap = 16,
        EnvironmentMap = 17,
        IrradianceMap = 18,
        PrefilterMap = 19,
        BRDFLut = 20,
        BlueNoise = 21,
    };

    void SetViewConstants(IRHIBuffer* buffer) { SetCBV(static_cast<uint32>(ESlot::ViewConstants), buffer); }
    void SetLightConstants(IRHIBuffer* buffer) { SetCBV(static_cast<uint32>(ESlot::LightConstants), buffer); }
    void SetShadowMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::ShadowMap), texture); }
    void SetEnvironmentMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::EnvironmentMap), texture); }
};

// ════════════════════════════════════════════════════════════════
// Bindless対応リソーステーブル
// ════════════════════════════════════════════════════════════════

/// Bindlessリソーステーブル
/// 全リソースをグローバルディスクリプタヒープで管理
class RHIBindlessResourceTable
{
public:
    explicit RHIBindlessResourceTable(IRHIDevice* device);

    /// テクスチャを登録してインデックス取得
    uint32 RegisterTexture(IRHITexture* texture);

    /// バッファを登録してインデックス取得
    uint32 RegisterBuffer(IRHIBuffer* buffer);

    /// サンプラーを登録してインデックス取得
    uint32 RegisterSampler(IRHISampler* sampler);

    /// インデックスからリソース取得
    IRHITexture* GetTexture(uint32 index) const;
    IRHIBuffer* GetBuffer(uint32 index) const;

    /// 登録解除
    void Unregister(uint32 index);

    /// ディスクリプタヒープをバインド
    void BindDescriptorHeaps(IRHICommandContext* context) const;

private:
    IRHIDevice* m_device;
    IRHIDescriptorHeap* m_srvUavHeap;
    IRHIDescriptorHeap* m_samplerHeap;
    std::vector<IRHIResource*> m_resources;
    std::queue<uint32> m_freeIndices;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// マテリアルテーブル
RHIMaterialResourceTable materialTable;
materialTable.SetAlbedoMap(albedoTex);
materialTable.SetNormalMap(normalTex);
materialTable.SetMetallicRoughnessMap(pbrTex);

// グローバルテーブル
RHIGlobalResourceTable globalTable;
globalTable.SetViewConstants(viewCB);
globalTable.SetShadowMap(shadowMap);
globalTable.SetEnvironmentMap(envMap);

// 描画時バインド
globalTable.Bind(context, ERHIShaderStage::Vertex);
globalTable.Bind(context, ERHIShaderStage::Pixel);
materialTable.Bind(context, ERHIShaderStage::Pixel);

// Bindless
RHIBindlessResourceTable bindlessTable(device);
uint32 albedoIndex = bindlessTable.RegisterTexture(albedoTex);

// シェーダーでインデックス参照
// Texture2D tex = ResourceDescriptorHeap[albedoIndex];
```

### 6. リソースコレクション

マテリアル単位でリソースをグループ化し、一括バインド・状態遷移を行う。

```cpp
namespace NS::RHI
{
    /// リソースコレクションメンバー
    struct RHI_API RHIResourceCollectionMember
    {
        IRHIResource* resource = nullptr;
        RHIBindlessIndex bindlessIndex;
        ERHIResourceType type = ERHIResourceType::Unknown;
    };

    /// リソースコレクション
    /// テクスチャ・バッファ・サンプラーをグループ化
    class RHI_API RHIResourceCollection : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(ResourceCollection)

        virtual ~RHIResourceCollection() = default;

        /// メンバー更新
        virtual void UpdateMember(
            uint32 index,
            IRHIResource* resource) = 0;

        /// 複数メンバー一括更新
        virtual void UpdateMembers(
            const RHIResourceCollectionMember* members,
            uint32 count) = 0;

        /// Bindlessハンドル取得
        virtual RHIBindlessIndex GetBindlessHandle() const = 0;

        /// メンバー数取得
        virtual uint32 GetMemberCount() const = 0;

        /// メンバー取得
        virtual IRHIResource* GetMember(uint32 index) const = 0;
    };

    using RHIResourceCollectionRef = TRefCountPtr<RHIResourceCollection>;
}
```

- [ ] RHIResourceCollectionMember 構造体
- [ ] RHIResourceCollection クラス

## 検証

- [ ] 全リソースタイプのバインド
- [ ] スロット衝突検出
- [ ] Bindlessインデックス管理
- [ ] コンテキストへのバインド
