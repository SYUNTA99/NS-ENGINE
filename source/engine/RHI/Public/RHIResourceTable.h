/// @file RHIResourceTable.h
/// @brief リソーステーブル・マテリアル/グローバル/Bindlessリソース管理
/// @details シェーダーが参照するリソースのテーブル管理。Bindless化への移行を見据えた設計。
/// @see 24-03-resource-table.md
#pragma once

#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <queue>
#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHIResource;
    class IRHIBuffer;
    class IRHITexture;
    class IRHISampler;
    class IRHIDevice;
    class IRHIDescriptorHeap;
    class IRHICommandContext;
    class IRHIComputeContext;

    //=========================================================================
    // ERHIResourceTableEntryType (24-03)
    //=========================================================================

    /// リソーステーブルエントリタイプ
    enum class ERHIResourceTableEntryType : uint8
    {
        SRV_Texture, ///< テクスチャSRV
        SRV_Buffer,  ///< バッファSRV
        UAV_Texture, ///< テクスチャUAV
        UAV_Buffer,  ///< バッファUAV
        CBV,         ///< 定数バッファビュー
        Sampler,     ///< サンプラー
    };

    //=========================================================================
    // RHIResourceTableEntry (24-03)
    //=========================================================================

    /// リソーステーブルエントリ
    struct RHIResourceTableEntry
    {
        ERHIResourceTableEntryType type = ERHIResourceTableEntryType::SRV_Texture;
        uint32 slot = 0;                  ///< シェーダーレジスタスロット
        IRHIResource* resource = nullptr; ///< リソースポインタ
        uint32 descriptorIndex = 0;       ///< Bindlessインデックス
    };

    //=========================================================================
    // RHIResourceTable (24-03)
    //=========================================================================

    /// リソーステーブル
    /// シェーダーが使用するリソースをグループ化
    class RHI_API RHIResourceTable
    {
    public:
        RHIResourceTable() = default;
        explicit RHIResourceTable(uint32 capacity);

        /// SRV追加（テクスチャ）
        void SetSRV(uint32 slot, IRHITexture* texture);

        /// SRV追加（バッファ）
        void SetSRV(uint32 slot, IRHIBuffer* buffer);

        /// UAV追加（テクスチャ）
        void SetUAV(uint32 slot, IRHITexture* texture);

        /// UAV追加（バッファ）
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
        void Bind(IRHICommandContext* context, EShaderFrequency stage) const;
        void Bind(IRHIComputeContext* context) const;

    private:
        std::vector<RHIResourceTableEntry> m_entries;
    };

    //=========================================================================
    // RHIMaterialResourceTable (24-03)
    //=========================================================================

    /// マテリアル用リソーステーブル
    /// PBRマテリアルの標準テクスチャスロットを定義
    class RHI_API RHIMaterialResourceTable : public RHIResourceTable
    {
    public:
        /// 標準PBRテクスチャスロット
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
        void SetMetallicRoughnessMap(IRHITexture* texture)
        {
            SetSRV(static_cast<uint32>(ESlot::MetallicRoughness), texture);
        }
        void SetOcclusionMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Occlusion), texture); }
        void SetEmissiveMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Emissive), texture); }
        void SetHeightMap(IRHITexture* texture) { SetSRV(static_cast<uint32>(ESlot::Height), texture); }
    };

    //=========================================================================
    // RHIGlobalResourceTable (24-03)
    //=========================================================================

    /// フレーム共通リソーステーブル
    /// ビュー定数、ライト、シャドウマップ等
    class RHI_API RHIGlobalResourceTable : public RHIResourceTable
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

    //=========================================================================
    // RHIBindlessResourceTable (24-03)
    //=========================================================================

    /// Bindlessリソーステーブル
    /// 全リソースをグローバルディスクリプタヒープで管理
    class RHI_API RHIBindlessResourceTable
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
        IRHIDescriptorHeap* m_srvUavHeap = nullptr;
        IRHIDescriptorHeap* m_samplerHeap = nullptr;
        std::vector<IRHIResource*> m_resources;
        std::queue<uint32> m_freeIndices;
    };

    //=========================================================================
    // RHIResourceCollectionMember (24-03)
    //=========================================================================

    /// リソースコレクションメンバー
    struct RHI_API RHIResourceCollectionMember
    {
        IRHIResource* resource = nullptr;
        BindlessIndex bindlessIndex;
        ERHIResourceType type = ERHIResourceType::Unknown;
    };

    //=========================================================================
    // RHIResourceCollection (24-03)
    //=========================================================================

    /// リソースコレクション
    /// テクスチャ・バッファ・サンプラーをグループ化
    class RHI_API RHIResourceCollection : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(ResourceCollection)

        virtual ~RHIResourceCollection() = default;

        /// メンバー更新
        virtual void UpdateMember(uint32 index, IRHIResource* resource) = 0;

        /// 複数メンバー一括更新
        virtual void UpdateMembers(const RHIResourceCollectionMember* members, uint32 count) = 0;

        /// Bindlessハンドル取得
        virtual BindlessIndex GetBindlessHandle() const = 0;

        /// メンバー数取得
        virtual uint32 GetMemberCount() const = 0;

        /// メンバー取得
        virtual IRHIResource* GetMember(uint32 index) const = 0;
    };

    using RHIResourceCollectionRef = TRefCountPtr<RHIResourceCollection>;

} // namespace NS::RHI
