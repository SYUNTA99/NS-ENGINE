//----------------------------------------------------------------------------
//! @file   rasterizer_state.h
//! @brief  ラスタライザーステート
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <memory>

//===========================================================================
//! ラスタライザーステート
//! @brief ラスタライズ設定をカプセル化
//===========================================================================
class RasterizerState final : private NonCopyable
{
public:
    //----------------------------------------------------------
    //! @name   ファクトリメソッド
    //----------------------------------------------------------
    //!@{

    //! ラスタライザーステートを作成
    //! @param [in] desc ラスタライザー記述子
    //! @return 成功時は有効なunique_ptr、失敗時はnullptr
    [[nodiscard]] static std::unique_ptr<RasterizerState> Create(const D3D11_RASTERIZER_DESC& desc);

    //! デフォルトのラスタライザーステートを作成（ソリッド、バックフェイスカリング）
    [[nodiscard]] static std::unique_ptr<RasterizerState> CreateDefault();

    //! ワイヤーフレーム描画用を作成
    [[nodiscard]] static std::unique_ptr<RasterizerState> CreateWireframe();

    //! カリングなしを作成（両面描画）
    [[nodiscard]] static std::unique_ptr<RasterizerState> CreateNoCull();

    //! フロントフェイスカリングを作成
    [[nodiscard]] static std::unique_ptr<RasterizerState> CreateFrontCull();

    //! シャドウマップ用を作成（深度バイアス付き）
    //! @param [in] depthBias 深度バイアス
    //! @param [in] slopeScaledDepthBias 傾斜スケール深度バイアス
    [[nodiscard]] static std::unique_ptr<RasterizerState> CreateShadowMap(
        int32_t depthBias = 100000,
        float slopeScaledDepthBias = 1.0f);

    //!@}
    //----------------------------------------------------------
    //! @name   アクセサ
    //----------------------------------------------------------
    //!@{

    //! D3D11ラスタライザーステートを取得
    [[nodiscard]] ID3D11RasterizerState* GetD3DRasterizerState() const noexcept { return rasterizer_.Get(); }

    //! 有効性チェック
    [[nodiscard]] bool IsValid() const noexcept { return rasterizer_ != nullptr; }

    //!@}

private:
    RasterizerState() = default;

    ComPtr<ID3D11RasterizerState> rasterizer_;
};
