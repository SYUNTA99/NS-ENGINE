//----------------------------------------------------------------------------
//! @file   depth_stencil_state.h
//! @brief  深度ステンシルステート
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <memory>

//===========================================================================
//! 深度ステンシルステート
//! @brief 深度・ステンシルテスト設定をカプセル化
//===========================================================================
class DepthStencilState final : private NonCopyable
{
public:
    //----------------------------------------------------------
    //! @name   ファクトリメソッド
    //----------------------------------------------------------
    //!@{

    //! 深度ステンシルステートを作成
    //! @param [in] desc 深度ステンシル記述子
    //! @return 成功時は有効なunique_ptr、失敗時はnullptr
    [[nodiscard]] static std::unique_ptr<DepthStencilState> Create(const D3D11_DEPTH_STENCIL_DESC& desc);

    //! デフォルト（深度テスト有効、深度書き込み有効）
    [[nodiscard]] static std::unique_ptr<DepthStencilState> CreateDefault();

    //! 深度テストのみ（書き込み無効）
    [[nodiscard]] static std::unique_ptr<DepthStencilState> CreateReadOnly();

    //! 深度テスト無効
    [[nodiscard]] static std::unique_ptr<DepthStencilState> CreateDisabled();

    //! 深度テスト逆転（遠い方を描画）
    [[nodiscard]] static std::unique_ptr<DepthStencilState> CreateReversed();

    //! 深度テスト有効・書き込み無効・LessEqual
    [[nodiscard]] static std::unique_ptr<DepthStencilState> CreateLessEqual();

    //!@}
    //----------------------------------------------------------
    //! @name   アクセサ
    //----------------------------------------------------------
    //!@{

    //! D3D11深度ステンシルステートを取得
    [[nodiscard]] ID3D11DepthStencilState* GetD3DDepthStencilState() const noexcept { return depthStencil_.Get(); }

    //! 有効性チェック
    [[nodiscard]] bool IsValid() const noexcept { return depthStencil_ != nullptr; }

    //!@}

private:
    DepthStencilState() = default;

    ComPtr<ID3D11DepthStencilState> depthStencil_;
};
