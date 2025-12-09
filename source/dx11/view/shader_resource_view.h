//----------------------------------------------------------------------------
//! @file   shader_resource_view.h
//! @brief  シェーダーリソースビュー（SRV）ラッパークラス
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <memory>

//===========================================================================
//! シェーダーリソースビュー
//! @brief ID3D11ShaderResourceViewのラッパー
//===========================================================================
class ShaderResourceView final : private NonCopyable
{
public:
    //----------------------------------------------------------
    //! @name   生成
    //----------------------------------------------------------
    //!@{

    //! バッファからSRVを作成
    //! @param [in] buffer D3D11バッファ
    //! @param [in] desc   SRV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> CreateFromBuffer(
        ID3D11Buffer* buffer,
        const D3D11_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

    //! Texture1DからSRVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    SRV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> CreateFromTexture1D(
        ID3D11Texture1D* texture,
        const D3D11_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

    //! Texture2DからSRVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    SRV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> CreateFromTexture2D(
        ID3D11Texture2D* texture,
        const D3D11_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

    //! Texture3DからSRVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    SRV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> CreateFromTexture3D(
        ID3D11Texture3D* texture,
        const D3D11_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

    //! 任意のリソースからSRVを作成
    //! @param [in] resource D3D11リソース
    //! @param [in] desc     SRV記述子
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> Create(
        ID3D11Resource* resource,
        const D3D11_SHADER_RESOURCE_VIEW_DESC& desc);

    //! 既存のSRVからラッパーを作成
    //! @param [in] srv 既存のShaderResourceView
    [[nodiscard]] static std::unique_ptr<ShaderResourceView> FromD3DView(
        ComPtr<ID3D11ShaderResourceView> srv);

    //!@}
    //----------------------------------------------------------
    //! @name   アクセサ
    //----------------------------------------------------------
    //!@{

    [[nodiscard]] ID3D11ShaderResourceView* Get() const noexcept { return srv_.Get(); }
    [[nodiscard]] ID3D11ShaderResourceView* const* GetAddressOf() const noexcept { return srv_.GetAddressOf(); }
    [[nodiscard]] bool IsValid() const noexcept { return srv_ != nullptr; }

    //! 所有権を放棄してComPtrを取得
    [[nodiscard]] ComPtr<ID3D11ShaderResourceView> Detach() noexcept { return std::move(srv_); }

    //! 記述子を取得
    [[nodiscard]] D3D11_SHADER_RESOURCE_VIEW_DESC GetDesc() const noexcept;

    //!@}
    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    ~ShaderResourceView() = default;
    ShaderResourceView(ShaderResourceView&&) = default;
    ShaderResourceView& operator=(ShaderResourceView&&) = default;

    //!@}

private:
    ShaderResourceView() = default;

    ComPtr<ID3D11ShaderResourceView> srv_;
};
