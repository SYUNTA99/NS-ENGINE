//----------------------------------------------------------------------------
//! @file   unordered_access_view.h
//! @brief  アンオーダードアクセスビュー（UAV）ラッパークラス
//!
//! @note このファイルは後方互換性のために残されています。
//!       新規コードでは view_wrapper.h を直接インクルードしてください。
//----------------------------------------------------------------------------
#pragma once

#include "dx11/view/view_wrapper.h"

// UnorderedAccessView は view_wrapper.h で定義された型エイリアス
// using UnorderedAccessView = ViewWrapper<ID3D11UnorderedAccessView>;
