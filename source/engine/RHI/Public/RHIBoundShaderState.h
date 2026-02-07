/// @file RHIBoundShaderState.h
/// @brief バウンドシェーダーステート・キャッシュ・ビルダー
/// @details シェーダーの組み合わせとバインディングレイアウトを管理。PSO作成の前段階としてシェーダーセットをキャッシュ。
/// @see 24-04-bound-shader-state.md
#pragma once

#include "RHIMacros.h"
#include "RHIShaderParameterMap.h"
#include "RHITypes.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace NS::RHI
{
    // 前方宣言
    class IRHIShader;
    struct RHIInputLayoutDesc;
    struct RHIRootSignatureDesc;

    //=========================================================================
    // RHIBoundShaderStateKey (24-04)
    //=========================================================================

    /// バウンドシェーダーステートキー
    /// シェーダーの組み合わせを一意に識別
    struct RHIBoundShaderStateKey
    {
        uint64 vertexShaderHash = 0;
        uint64 pixelShaderHash = 0;
        uint64 geometryShaderHash = 0;
        uint64 hullShaderHash = 0;
        uint64 domainShaderHash = 0;
        uint64 meshShaderHash = 0;
        uint64 amplificationShaderHash = 0;

        bool operator==(const RHIBoundShaderStateKey& other) const = default;

        uint64 GetCombinedHash() const;
    };

    //=========================================================================
    // RHIBoundShaderStateKeyHash (24-04)
    //=========================================================================

    struct RHIBoundShaderStateKeyHash
    {
        size_t operator()(const RHIBoundShaderStateKey& key) const
        {
            return static_cast<size_t>(key.GetCombinedHash());
        }
    };

    //=========================================================================
    // RHIBoundShaderStateDesc (24-04)
    //=========================================================================

    /// バウンドシェーダーステート記述
    /// @note 各シェーダーのGetFrequency()で正しいステージか検証すること
    struct RHIBoundShaderStateDesc
    {
        // 従来パイプライン
        IRHIShader* vertexShader = nullptr;
        IRHIShader* pixelShader = nullptr;
        IRHIShader* geometryShader = nullptr;
        IRHIShader* hullShader = nullptr;
        IRHIShader* domainShader = nullptr;

        // メッシュシェーダーパイプライン（排他）
        IRHIShader* amplificationShader = nullptr;
        IRHIShader* meshShader = nullptr;

        // 入力レイアウト（従来パイプラインのみ）
        const RHIInputLayoutDesc* inputLayout = nullptr;

        /// パイプラインタイプ判定
        bool IsMeshShaderPipeline() const { return meshShader != nullptr; }
        bool IsTraditionalPipeline() const { return vertexShader != nullptr; }
    };

    //=========================================================================
    // RHIBoundShaderState (24-04)
    //=========================================================================

    /// バウンドシェーダーステート
    /// シェーダーの組み合わせと共通リフレクション情報をキャッシュ
    class RHI_API RHIBoundShaderState
    {
    public:
        explicit RHIBoundShaderState(const RHIBoundShaderStateDesc& desc);

        /// キー取得
        const RHIBoundShaderStateKey& GetKey() const { return m_key; }

        /// シェーダー取得
        IRHIShader* GetVertexShader() const { return m_vertexShader.Get(); }
        IRHIShader* GetPixelShader() const { return m_pixelShader.Get(); }
        IRHIShader* GetGeometryShader() const { return m_geometryShader.Get(); }
        IRHIShader* GetHullShader() const { return m_hullShader.Get(); }
        IRHIShader* GetDomainShader() const { return m_domainShader.Get(); }
        IRHIShader* GetMeshShader() const { return m_meshShader.Get(); }
        IRHIShader* GetAmplificationShader() const { return m_amplificationShader.Get(); }

        /// 入力レイアウト取得
        const RHIInputLayoutDesc* GetInputLayout() const { return m_inputLayout; }

        /// 統合パラメータマップ取得
        const RHIShaderParameterMap& GetParameterMap() const { return m_parameterMap; }

        /// パイプラインタイプ
        bool IsMeshShaderPipeline() const { return m_meshShader != nullptr; }

    private:
        void BuildParameterMap();

        RHIBoundShaderStateKey m_key;

        // シェーダー参照
        RHIShaderRef m_vertexShader;
        RHIShaderRef m_pixelShader;
        RHIShaderRef m_geometryShader;
        RHIShaderRef m_hullShader;
        RHIShaderRef m_domainShader;
        RHIShaderRef m_amplificationShader;
        RHIShaderRef m_meshShader;

        const RHIInputLayoutDesc* m_inputLayout = nullptr;

        // 統合リフレクション
        RHIShaderParameterMap m_parameterMap;
    };

    using RHIBoundShaderStateRef = std::shared_ptr<RHIBoundShaderState>;

    //=========================================================================
    // RHIBoundShaderStateCache (24-04)
    //=========================================================================

    /// バウンドシェーダーステートキャッシュ
    /// 同じシェーダー組み合わせのBSSを再利用
    class RHI_API RHIBoundShaderStateCache
    {
    public:
        /// BSSを取得（なければ作成）
        RHIBoundShaderStateRef GetOrCreate(const RHIBoundShaderStateDesc& desc);

        /// キャッシュをクリア
        void Clear();

        /// 統計
        uint32 GetCachedCount() const { return static_cast<uint32>(m_cache.size()); }
        uint32 GetCacheHits() const { return m_cacheHits; }
        uint32 GetCacheMisses() const { return m_cacheMisses; }

    private:
        std::unordered_map<RHIBoundShaderStateKey, RHIBoundShaderStateRef, RHIBoundShaderStateKeyHash> m_cache;
        uint32 m_cacheHits = 0;
        uint32 m_cacheMisses = 0;
        std::mutex m_mutex;
    };

    //=========================================================================
    // RHIBoundShaderStateBuilder (24-04)
    //=========================================================================

    /// バウンドシェーダーステートビルダー
    class RHIBoundShaderStateBuilder
    {
    public:
        RHIBoundShaderStateBuilder& SetVertexShader(IRHIShader* shader)
        {
            m_desc.vertexShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetPixelShader(IRHIShader* shader)
        {
            m_desc.pixelShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetGeometryShader(IRHIShader* shader)
        {
            m_desc.geometryShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetHullShader(IRHIShader* shader)
        {
            m_desc.hullShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetDomainShader(IRHIShader* shader)
        {
            m_desc.domainShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetMeshShader(IRHIShader* shader)
        {
            m_desc.meshShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetAmplificationShader(IRHIShader* shader)
        {
            m_desc.amplificationShader = shader;
            return *this;
        }

        RHIBoundShaderStateBuilder& SetInputLayout(const RHIInputLayoutDesc* layout)
        {
            m_desc.inputLayout = layout;
            return *this;
        }

        const RHIBoundShaderStateDesc& Build() const { return m_desc; }

    private:
        RHIBoundShaderStateDesc m_desc;
    };

} // namespace NS::RHI
