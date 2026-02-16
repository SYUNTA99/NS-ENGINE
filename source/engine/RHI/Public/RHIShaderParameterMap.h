/// @file RHIShaderParameterMap.h
/// @brief シェーダーパラメータマップ・マテリアルバインディング
/// @details パラメータ名からバインディング位置へのマッピングと型安全なハンドルを提供。
/// @see 24-02-shader-parameter-map.md
#pragma once

#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <unordered_map>
#include <vector>

namespace NS { namespace RHI {
    // 前方宣言
    class IRHIBuffer;
    class IRHITexture;
    class IRHISampler;
    class IRHIRootSignature;
    class IRHICommandContext;

    //=========================================================================
    // ERHIShaderParameterType (24-02)
    //=========================================================================

    /// パラメータタイプ
    enum class ERHIShaderParameterType : uint8
    {
        ConstantBuffer,        ///< cbuffer / uniform block
        Texture,               ///< Texture2D, TextureCube等
        Buffer,                ///< StructuredBuffer, ByteAddressBuffer
        Sampler,               ///< SamplerState
        UAV,                   ///< RWTexture, RWBuffer
        RootConstant,          ///< インラインルート定数
        AccelerationStructure, ///< RTアクセラレーション構造
    };

    //=========================================================================
    // RHIShaderParameterBinding (24-02)
    //=========================================================================

    /// パラメータバインディング情報
    struct RHIShaderParameterBinding
    {
        const char* name = nullptr; ///< パラメータ名
        ERHIShaderParameterType type = ERHIShaderParameterType::ConstantBuffer;
        uint32 bindPoint = 0;                              ///< register番号
        uint32 bindCount = 1;                              ///< 配列サイズ
        uint32 space = 0;                                  ///< register space
        EShaderFrequency stage = EShaderFrequency::Vertex; ///< 使用シェーダーステージ
        uint32 rootParameterIndex = UINT32_MAX;            ///< ルートシグネチャ内インデックス
        uint32 descriptorTableOffset = 0;                  ///< ディスクリプタテーブル内のオフセット
    };

    //=========================================================================
    // RHIShaderParameterMap (24-02)
    //=========================================================================

    /// シェーダーパラメータマップ
    /// リフレクションから構築され、バインディング時に参照
    class RHI_API RHIShaderParameterMap
    {
    public:
        RHIShaderParameterMap() = default;

        /// パラメータを追加
        void AddParameter(const RHIShaderParameterBinding& binding);

        /// 名前でパラメータを検索
        const RHIShaderParameterBinding* FindParameter(const char* name) const;

        /// 名前でパラメータを検索（ハッシュ版、高速）
        const RHIShaderParameterBinding* FindParameter(uint64 nameHash) const;

        /// 全パラメータ取得
        const std::vector<RHIShaderParameterBinding>& GetAllParameters() const { return m_parameters; }

        /// タイプ別パラメータ数
        uint32 GetParameterCount(ERHIShaderParameterType type) const;

        /// パラメータが存在するか
        bool HasParameter(const char* name) const { return FindParameter(name) != nullptr; }

        /// ルートシグネチャとの互換性チェック
        bool IsCompatibleWith(const IRHIRootSignature* rootSignature) const;

    private:
        std::vector<RHIShaderParameterBinding> m_parameters;
        std::unordered_map<uint64, uint32> m_nameHashToIndex;
    };

    //=========================================================================
    // TRHIShaderParameterHandle (24-02)
    //=========================================================================

    /// シェーダーパラメータハンドル
    /// 高速なバインディング用にキャッシュ
    template <typename T> class TRHIShaderParameterHandle
    {
    public:
        TRHIShaderParameterHandle() = default;
        explicit TRHIShaderParameterHandle(const RHIShaderParameterBinding* binding) : m_binding(binding) {}

        bool IsValid() const { return m_binding != nullptr; }

        uint32 GetRootParameterIndex() const { return m_binding ? m_binding->rootParameterIndex : UINT32_MAX; }

        uint32 GetDescriptorTableOffset() const { return m_binding ? m_binding->descriptorTableOffset : 0; }

        const RHIShaderParameterBinding* GetBinding() const { return m_binding; }

    private:
        const RHIShaderParameterBinding* m_binding = nullptr;
    };

    using RHIConstantBufferHandle = TRHIShaderParameterHandle<IRHIBuffer>;
    using RHITextureHandle = TRHIShaderParameterHandle<IRHITexture>;
    using RHISamplerHandle = TRHIShaderParameterHandle<IRHISampler>;
    using RHIUAVHandle = TRHIShaderParameterHandle<void>;

    //=========================================================================
    // RHIMaterialParameterSet (24-02)
    //=========================================================================

    /// マテリアルパラメータセット
    /// マテリアル固有のパラメータをまとめて管理
    class RHI_API RHIMaterialParameterSet
    {
    public:
        explicit RHIMaterialParameterSet(const RHIShaderParameterMap* parameterMap);

        /// テクスチャ設定
        void SetTexture(const char* name, IRHITexture* texture);
        void SetTexture(RHITextureHandle handle, IRHITexture* texture);

        /// 定数バッファ設定
        void SetConstantBuffer(const char* name, IRHIBuffer* buffer);
        void SetConstantBuffer(RHIConstantBufferHandle handle, IRHIBuffer* buffer);

        /// サンプラー設定
        void SetSampler(const char* name, IRHISampler* sampler);
        void SetSampler(RHISamplerHandle handle, IRHISampler* sampler);

        /// スカラー値設定（ルート定数用）
        template <typename T> void SetValue(const char* name, const T& value);

        /// コマンドコンテキストにバインド
        void Bind(IRHICommandContext* context) const;

        /// パラメータマップからハンドルを取得
        RHITextureHandle GetTextureHandle(const char* name) const;
        RHIConstantBufferHandle GetConstantBufferHandle(const char* name) const;
        RHISamplerHandle GetSamplerHandle(const char* name) const;

    private:
        const RHIShaderParameterMap* m_parameterMap;
        std::unordered_map<uint32, IRHITexture*> m_textures;
        std::unordered_map<uint32, IRHIBuffer*> m_constantBuffers;
        std::unordered_map<uint32, IRHISampler*> m_samplers;
        std::unordered_map<uint32, std::vector<uint8>> m_rootConstants;
    };

}} // namespace NS::RHI
