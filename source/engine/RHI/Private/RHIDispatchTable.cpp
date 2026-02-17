/// @file RHIDispatchTable.cpp
/// @brief RHIディスパッチテーブル グローバルインスタンス
#include "RHIDispatchTable.h"

namespace NS::RHI
{
    /// グローバルディスパッチテーブル
    /// バックエンドモジュール（D3D12, Vulkan等）の初期化時に関数ポインタが設定される
    RHIDispatchTable GRHIDispatchTable = {};

} // namespace NS::RHI
