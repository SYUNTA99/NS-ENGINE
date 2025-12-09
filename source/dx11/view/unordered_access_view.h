//----------------------------------------------------------------------------
//! @file   unordered_access_view.h
//! @brief  アンオーダードアクセスビュー（UAV）ラッパークラス
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <memory>

//===========================================================================
//! アンオーダードアクセスビュー
//! @brief ID3D11UnorderedAccessViewのラッパー
//===========================================================================
class UnorderedAccessView final : private NonCopyable
{
public:
    //----------------------------------------------------------
    //! @name   生成
    //----------------------------------------------------------
    //!@{

    //! バッファからUAVを作成
    //! @param [in] buffer D3D11バッファ
    //! @param [in] desc   UAV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> CreateFromBuffer(
        ID3D11Buffer* buffer,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr);

    //! Texture1DからUAVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    UAV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> CreateFromTexture1D(
        ID3D11Texture1D* texture,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr);

    //! Texture2DからUAVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    UAV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> CreateFromTexture2D(
        ID3D11Texture2D* texture,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr);

    //! Texture3DからUAVを作成
    //! @param [in] texture D3D11テクスチャ
    //! @param [in] desc    UAV記述子（nullptrの場合はデフォルト）
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> CreateFromTexture3D(
        ID3D11Texture3D* texture,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr);

    //! 任意のリソースからUAVを作成
    //! @param [in] resource D3D11リソース
    //! @param [in] desc     UAV記述子
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> Create(
        ID3D11Resource* resource,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc);

    //! 既存のUAVからラッパーを作成
    //! @param [in] uav 既存のUnorderedAccessView
    [[nodiscard]] static std::unique_ptr<UnorderedAccessView> FromD3DView(
        ComPtr<ID3D11UnorderedAccessView> uav);

    //!@}
    //----------------------------------------------------------
    //! @name   アクセサ
    //----------------------------------------------------------
    //!@{

    [[nodiscard]] ID3D11UnorderedAccessView* Get() const noexcept { return uav_.Get(); }
    [[nodiscard]] ID3D11UnorderedAccessView* const* GetAddressOf() const noexcept { return uav_.GetAddressOf(); }
    [[nodiscard]] bool IsValid() const noexcept { return uav_ != nullptr; }

    //! 所有権を放棄してComPtrを取得
    [[nodiscard]] ComPtr<ID3D11UnorderedAccessView> Detach() noexcept { return std::move(uav_); }

    //! 記述子を取得
    [[nodiscard]] D3D11_UNORDERED_ACCESS_VIEW_DESC GetDesc() const noexcept;

    //!@}
    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    ~UnorderedAccessView() = default;
    UnorderedAccessView(UnorderedAccessView&&) = default;
    UnorderedAccessView& operator=(UnorderedAccessView&&) = default;

    //!@}

private:
    UnorderedAccessView() = default;

    ComPtr<ID3D11UnorderedAccessView> uav_;
};
