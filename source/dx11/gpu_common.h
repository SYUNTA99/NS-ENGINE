//----------------------------------------------------------------------------
//! @file   gpu_common.h
//! @brief  DirectX11汎用クラス 共通ヘッダー
//! @note   全てのgpu関連ヘッダーファイルがこのファイルをインクルードします
//----------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------
// 共通ライブラリ
//--------------------------------------------------------------
#include "common/stl/stl_common.h"
#include "common/stl/stl_containers.h"
#include "common/platform/com_ptr.h"
#include "common/utility/non_copyable.h"

//--------------------------------------------------------------
// DirectX11関連
//--------------------------------------------------------------
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

//--------------------------------------------------------------
// ヘルパー関数
//--------------------------------------------------------------

//! D3D11_USAGEからCPUアクセスフラグを自動推論
//! @param [in] usage 使用法
//! @return CPUアクセスフラグ
inline D3D11_CPU_ACCESS_FLAG GetCpuAccessFlags(D3D11_USAGE usage) noexcept
{
    switch (usage) {
        case D3D11_USAGE_DYNAMIC:
            return D3D11_CPU_ACCESS_WRITE;
        case D3D11_USAGE_STAGING:
            return static_cast<D3D11_CPU_ACCESS_FLAG>(
                D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE
            );
        case D3D11_USAGE_IMMUTABLE:
        case D3D11_USAGE_DEFAULT:
        default:
            return static_cast<D3D11_CPU_ACCESS_FLAG>(0);
    }
}

