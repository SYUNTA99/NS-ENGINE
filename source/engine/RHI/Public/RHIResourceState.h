/// @file RHIResourceState.h
/// @brief リソース状態定義・遷移管理・サブリソース状態追跡
/// @details D3D12のD3D12_RESOURCE_STATESに対応するリソース状態列挙型、
///          遷移ヘルパー、サブリソース状態マップ、初期状態ヘルパーを提供。
/// @see 16-01-resource-state.md
#pragma once

#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // 全サブリソース定数
    //=========================================================================

    /// 全サブリソースを示す定数
    constexpr uint32 kAllSubresources = ~0u;

    //=========================================================================
    // ERHIResourceState (16-01)
    //=========================================================================

    /// リソース状態
    /// D3D12のD3D12_RESOURCE_STATESに対応
    enum class ERHIResourceState : uint32
    {
        /// 共通状態（任意のアクセス可能）
        Common = 0,

        /// 頂点バッファとして使用
        VertexBuffer = 1 << 0,

        /// 定数バッファとして使用
        ConstantBuffer = 1 << 1,

        /// インデックスバッファとして使用
        IndexBuffer = 1 << 2,

        /// レンダーターゲットとして使用
        RenderTarget = 1 << 3,

        /// アンオーダードアクセス（読み書き）
        UnorderedAccess = 1 << 4,

        /// デプスステンシル書き込み
        DepthWrite = 1 << 5,

        /// デプスステンシル読み取り専用
        DepthRead = 1 << 6,

        /// シェーダーリソース（非ピクセルシェーダー）
        NonPixelShaderResource = 1 << 7,

        /// ピクセルシェーダーリソース
        PixelShaderResource = 1 << 8,

        /// ストリームアウト
        StreamOut = 1 << 9,

        /// 間接引数
        IndirectArgument = 1 << 10,

        /// コピー先
        CopyDest = 1 << 11,

        /// コピー元
        CopySource = 1 << 12,

        /// リゾルブ先
        ResolveDest = 1 << 13,

        /// リゾルブ元
        ResolveSource = 1 << 14,

        /// レイトレーシング加速構造
        RaytracingAccelerationStructure = 1 << 15,

        /// シェーディングレートソース
        ShadingRateSource = 1 << 16,

        /// プレゼント
        Present = 1 << 17,

        /// プレディケーション
        Predication = 1 << 18,

        /// ビデオデコード読み取り
        VideoDecodeRead = 1 << 19,

        /// ビデオデコード書き込み
        VideoDecodeWrite = 1 << 20,

        /// ビデオプロセス読み取り
        VideoProcessRead = 1 << 21,

        /// ビデオプロセス書き込み
        VideoProcessWrite = 1 << 22,

        /// ビデオエンコード読み取り
        VideoEncodeRead = 1 << 23,

        /// ビデオエンコード書き込み
        VideoEncodeWrite = 1 << 24,

        //=====================================================================
        // 複合状態
        //=====================================================================

        /// 汎用読み取り（最適化ヒント）
        GenericRead = VertexBuffer | ConstantBuffer | IndexBuffer | NonPixelShaderResource | PixelShaderResource |
                      IndirectArgument | CopySource,

        /// シェーダーリソース（全ステージ）
        ShaderResource = NonPixelShaderResource | PixelShaderResource,

        /// すべてのシェーダー書き込み
        AllShaderWrite = UnorderedAccess | DepthWrite | RenderTarget,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIResourceState)

    //=========================================================================
    // RHIResourceStateHelper (16-01)
    //=========================================================================

    /// リソース状態ヘルパー
    namespace RHIResourceStateHelper
    {
        /// 読み取り状態か
        inline bool IsReadState(ERHIResourceState state)
        {
            constexpr ERHIResourceState kReadStates =
                ERHIResourceState::VertexBuffer | ERHIResourceState::ConstantBuffer | ERHIResourceState::IndexBuffer |
                ERHIResourceState::NonPixelShaderResource | ERHIResourceState::PixelShaderResource |
                ERHIResourceState::IndirectArgument | ERHIResourceState::CopySource | ERHIResourceState::ResolveSource |
                ERHIResourceState::DepthRead | ERHIResourceState::Predication;

            return (state & kReadStates) != ERHIResourceState::Common;
        }

        /// 書き込み状態か
        inline bool IsWriteState(ERHIResourceState state)
        {
            constexpr ERHIResourceState kWriteStates =
                ERHIResourceState::RenderTarget | ERHIResourceState::UnorderedAccess | ERHIResourceState::DepthWrite |
                ERHIResourceState::StreamOut | ERHIResourceState::CopyDest | ERHIResourceState::ResolveDest;

            return (state & kWriteStates) != ERHIResourceState::Common;
        }

        /// 遷移が必要か
        inline bool NeedsTransition(ERHIResourceState before, ERHIResourceState after)
        {
            if (before == after)
                return false;

            // 読み取りから読み取りへ、両方がGenericReadに含まれるなら不要
            if (IsReadState(before) && IsReadState(after))
            {
                if ((before & ERHIResourceState::GenericRead) == before &&
                    (after & ERHIResourceState::GenericRead) == after)
                {
                    return false;
                }
            }

            return true;
        }

        /// 同時アクセス可能か
        inline bool CanCoexist(ERHIResourceState a, ERHIResourceState b)
        {
            // どちらかが書き込み状態なら共存不可
            if (IsWriteState(a) || IsWriteState(b))
                return false;

            // 両方読み取りなら共存可能
            return true;
        }

        /// 状態名取得
        RHI_API const char* GetStateName(ERHIResourceState state);
    } // namespace RHIResourceStateHelper

    //=========================================================================
    // RHISubresourceState (16-01)
    //=========================================================================

    /// サブリソース状態
    struct RHI_API RHISubresourceState
    {
        /// リソース状態
        ERHIResourceState state = ERHIResourceState::Common;

        /// サブリソースインデックス（kAllSubresourcesで全て）
        uint32 subresource = kAllSubresources;
    };

    //=========================================================================
    // RHIResourceStateMap (16-01)
    //=========================================================================

    /// リソースの完全な状態追跡
    class RHI_API RHIResourceStateMap
    {
    public:
        RHIResourceStateMap() = default;

        /// 初期化（全サブリソース同一状態）
        void Initialize(uint32 subresourceCount, ERHIResourceState initialState);

        /// リセット
        void Reset();

        //=====================================================================
        // 状態取得
        //=====================================================================

        /// 全サブリソースが同一状態か
        bool IsUniform() const { return m_isUniform; }

        /// 統一状態取得（IsUniform時のみ有効）
        ERHIResourceState GetUniformState() const { return m_uniformState; }

        /// サブリソース状態取得
        ERHIResourceState GetSubresourceState(uint32 subresource) const;

        //=====================================================================
        // 状態設定
        //=====================================================================

        /// 全サブリソースの状態設定
        void SetAllSubresourcesState(ERHIResourceState state);

        /// 個別サブリソースの状態設定
        void SetSubresourceState(uint32 subresource, ERHIResourceState state);

        /// 範囲のサブリソース状態設定
        void SetSubresourceRangeState(uint32 firstSubresource, uint32 count, ERHIResourceState state);

        //=====================================================================
        // 情報
        //=====================================================================

        /// サブリソース数
        uint32 GetSubresourceCount() const { return m_subresourceCount; }

    private:
        ERHIResourceState* m_states = nullptr;
        uint32 m_subresourceCount = 0;
        ERHIResourceState m_uniformState = ERHIResourceState::Common;
        bool m_isUniform = true;
    };

    //=========================================================================
    // RHIInitialResourceState (16-01)
    //=========================================================================

    /// リソース初期状態ヘルパー
    namespace RHIInitialResourceState
    {
        /// バッファの初期状態
        inline ERHIResourceState ForBuffer(ERHIBufferUsage usage)
        {
            if (EnumHasAnyFlags(usage, ERHIBufferUsage::CPUWritable))
            {
                return ERHIResourceState::GenericRead;
            }
            if (EnumHasAnyFlags(usage, ERHIBufferUsage::CPUReadable))
            {
                return ERHIResourceState::CopyDest;
            }
            return ERHIResourceState::Common;
        }

        /// テクスチャの初期状態
        inline ERHIResourceState ForTexture(ERHITextureUsage usage)
        {
            if (EnumHasAnyFlags(usage, ERHITextureUsage::RenderTarget))
            {
                return ERHIResourceState::RenderTarget;
            }
            if (EnumHasAnyFlags(usage, ERHITextureUsage::DepthStencil))
            {
                return ERHIResourceState::DepthWrite;
            }
            if (EnumHasAnyFlags(usage, ERHITextureUsage::UnorderedAccess))
            {
                return ERHIResourceState::UnorderedAccess;
            }
            return ERHIResourceState::Common;
        }

        /// スワップチェーンバックバッファの初期状態
        inline ERHIResourceState ForBackBuffer()
        {
            return ERHIResourceState::Present;
        }
    } // namespace RHIInitialResourceState

}} // namespace NS::RHI
