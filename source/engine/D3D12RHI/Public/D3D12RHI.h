/// @file D3D12RHI.h
/// @brief D3D12RHIモジュール公開定数・型
/// @details Gameコードや他モジュールが参照可能な定数とエントリポイント宣言。
#pragma once

#include "RHI/Public/RHIMacros.h"

#include <cstdint>

namespace NS::D3D12RHI
{
    //=========================================================================
    // リソースバインディング上限
    //=========================================================================

    inline constexpr uint32_t kMaxSRVs = 64;     ///< SRVスロット数（64-bitマスク制限）
    inline constexpr uint32_t kMaxSamplers = 32; ///< サンプラースロット数
    inline constexpr uint32_t kMaxUAVs = 16;     ///< UAVスロット数
    inline constexpr uint32_t kMaxCBVs = 16;     ///< CBVスロット数
    inline constexpr uint32_t kMaxRootCBVs = 16; ///< ルートディスクリプタCBV数

    //=========================================================================
    // フレーム管理
    //=========================================================================

    inline constexpr uint32_t kMaxBackBufferCount = 3; ///< スワップチェーンバッファ数

    //=========================================================================
    // MSAA
    //=========================================================================

    inline constexpr uint32_t kMaxMSAACount = 8; ///< MSAA最大サンプル数

    //=========================================================================
    // ルートシグネチャ
    //=========================================================================

    inline constexpr uint32_t kRootSignatureMaxDWORDs = 64; ///< D3D12ハード上限

} // namespace NS::D3D12RHI
