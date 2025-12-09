//----------------------------------------------------------------------------
//! @file   render_target_view.h
//! @brief  レンダーターゲットビュー（RTV）ラッパークラス
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <memory>

//===========================================================================
//! レンダーターゲットビュー
//! @brief ID3D11RenderTargetViewのラッパー
//===========================================================================
class RenderTargetView final : private NonCopyable
{
public:
    //----------------------------------------------------------
    //! @name   生成
    //----------------------------------------------------------
    //!@{

    //! バッファからRTVを作成
    //! @param [in] buffer D3D11バッファ
    //! @param [in] desc   RTV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<RenderTargetView> CreateFromBuffer(
        ID3D11Buffer* buffer,
        const D3D11_RENDER_TARGET_VIEW_DESC* desc = nullptr);

    //! Texture1DからRTVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    RTV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<RenderTargetView> CreateFromTexture1D(
        ID3D11Texture1D* texture,
        const D3D11_RENDER_TARGET_VIEW_DESC* desc = nullptr);

    //! Texture2DからRTVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    RTV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<RenderTargetView> CreateFromTexture2D(
        ID3D11Texture2D* texture,
        const D3D11_RENDER_TARGET_VIEW_DESC* desc = nullptr);

    //! Texture3DからRTVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    RTV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<RenderTargetView> CreateFromTexture3D(
        ID3D11Texture3D* texture,
        const D3D11_RENDER_TARGET_VIEW_DESC* desc = nullptr);

    //! 任意のリソースからRTVを作成
    //! @param [in] resource D3D11リソース
    //! @param [in] desc     RTV記述子
    [[nodiscard]] static std::unique_ptr<RenderTargetView> Create(
        ID3D11Resource* resource,
        const D3D11_RENDER_TARGET_VIEW_DESC& desc);

    //! 既存のRTVからラッパーを作成
    //! @param [in] rtv 既存のRenderTargetView
    [[nodiscard]] static std::unique_ptr<RenderTargetView> FromD3DView(
        ComPtr<ID3D11RenderTargetView> rtv);

    //!@}
    //----------------------------------------------------------
    //! @name   アクセサ
    //----------------------------------------------------------
    //!@{

    [[nodiscard]] ID3D11RenderTargetView* Get() const noexcept { return rtv_.Get(); }
    [[nodiscard]] ID3D11RenderTargetView* const* GetAddressOf() const noexcept { return rtv_.GetAddressOf(); }
    [[nodiscard]] bool IsValid() const noexcept { return rtv_ != nullptr; }

    //! 所有権を放棄してComPtrを取得
    [[nodiscard]] ComPtr<ID3D11RenderTargetView> Detach() noexcept { return std::move(rtv_); }

    //! 記述子を取得
    [[nodiscard]] D3D11_RENDER_TARGET_VIEW_DESC GetDesc() const noexcept;

    //!@}
    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    ~RenderTargetView() = default;
    RenderTargetView(RenderTargetView&&) = default;
    RenderTargetView& operator=(RenderTargetView&&) = default;

    //!@}

private:
    RenderTargetView() = default;

    ComPtr<ID3D11RenderTargetView> rtv_;
};
