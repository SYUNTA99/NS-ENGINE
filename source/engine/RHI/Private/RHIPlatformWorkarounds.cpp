/// @file RHIPlatformWorkarounds.cpp
/// @brief プラットフォーム回避策フラグ グローバル定義・初期化
#include "RHIPlatformWorkarounds.h"

namespace NS::RHI
{
    /// グローバル回避策フラグインスタンス
    RHIPlatformWorkarounds GRHIPlatformWorkarounds = {};

    void InitializePlatformWorkarounds(RHIPlatformWorkarounds& outWorkarounds)
    {
        // デフォルトは全て無効
        // バックエンド初期化時にGPU/ドライバ情報に応じて設定する
        outWorkarounds = {};
    }

} // namespace NS::RHI
