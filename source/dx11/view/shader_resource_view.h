//----------------------------------------------------------------------------
//! @file   shader_resource_view.h
//! @brief  シェーダーリソースビュー（SRV）ラッパークラス
//!
//! @note このファイルは後方互換性のために残されています。
//!       新規コードでは view_wrapper.h を直接インクルードしてください。
//----------------------------------------------------------------------------
#pragma once

#include "dx11/view/view_wrapper.h"

// ShaderResourceView は view_wrapper.h で定義された型エイリアス
// using ShaderResourceView = ViewWrapper<ID3D11ShaderResourceView>;
