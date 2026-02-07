/// @file RHIPlatformWorkarounds.h
/// @brief プラットフォーム回避策フラグ
/// @details GPU/ドライバ固有のバグ・制限を回避するためのフラグシステム。
/// @see 11-07-platform-workarounds.md
#pragma once

#include "RHIMacros.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIPlatformWorkarounds (11-07)
    //=========================================================================

    /// プラットフォーム回避策フラグ
    struct RHI_API RHIPlatformWorkarounds
    {
        //=====================================================================
        // リソース状態遷移関連
        //=====================================================================

        /// COPYSRC/COPYDEST状態への追加遷移が必要
        bool needsExtraTransitions = false;

        /// Transient ResourceのDiscard状態追跡が必要
        bool needsTransientDiscardStateTracking = false;

        /// AsyncCompute→GraphicsのDiscard遷移回避
        bool needsTransientDiscardOnGraphicsWorkaround = false;

        /// 非ピクセルシェーダーSRVの手動遷移が必要
        bool needsSRVGraphicsNonPixelWorkaround = false;

        //=====================================================================
        // リソース削除関連
        //=====================================================================

        /// リソース削除に追加の遅延が必要
        bool needsExtraDeletionLatency = false;

        /// ストリーミングテクスチャの削除遅延を強制無効化
        bool forceNoDeletionLatencyForStreamingTextures = false;

        //=====================================================================
        // シェーダー関連
        //=====================================================================

        /// 明示的なシェーダーアンバインドが必要
        bool needsShaderUnbinds = false;

        //=====================================================================
        // レンダリング関連
        //=====================================================================

        /// カスケードシャドウマップのアトラス化回避
        bool needsUnatlasedCSMDepthsWorkaround = false;

        //=====================================================================
        // フォーマット関連
        //=====================================================================

        /// BC4フォーマットエミュレーションが必要
        bool needsBC4FormatEmulation = false;

        /// BC5フォーマットエミュレーションが必要
        bool needsBC5FormatEmulation = false;

        //=====================================================================
        // 同期関連
        //=====================================================================

        /// フェンス値の追加マージンが必要
        bool needsFenceValuePadding = false;

        /// コマンドリスト間の追加同期が必要
        bool needsExplicitCommandListSync = false;
    };

    //=========================================================================
    // グローバルアクセス (11-07)
    //=========================================================================

    /// グローバル回避策フラグ
    RHI_API extern RHIPlatformWorkarounds GRHIPlatformWorkarounds;

/// マクロアクセス（互換性のため）
#define GRHINeedsExtraTransitions GRHIPlatformWorkarounds.needsExtraTransitions

#define GRHINeedsTransientDiscardStateTracking GRHIPlatformWorkarounds.needsTransientDiscardStateTracking

#define GRHINeedsTransientDiscardOnGraphicsWorkaround GRHIPlatformWorkarounds.needsTransientDiscardOnGraphicsWorkaround

#define GRHINeedsSRVGraphicsNonPixelWorkaround GRHIPlatformWorkarounds.needsSRVGraphicsNonPixelWorkaround

#define GRHINeedsExtraDeletionLatency GRHIPlatformWorkarounds.needsExtraDeletionLatency

#define GRHIForceNoDeletionLatencyForStreamingTextures                                                                 \
    GRHIPlatformWorkarounds.forceNoDeletionLatencyForStreamingTextures

#define GRHINeedsShaderUnbinds GRHIPlatformWorkarounds.needsShaderUnbinds

#define GRHINeedsUnatlasedCSMDepthsWorkaround GRHIPlatformWorkarounds.needsUnatlasedCSMDepthsWorkaround

    //=========================================================================
    // 初期化関数 (11-07)
    //=========================================================================

    /// プラットフォーム固有の回避策フラグを設定
    /// @note RHI初期化時にバックエンドが呼び出す
    RHI_API void InitializePlatformWorkarounds(RHIPlatformWorkarounds& outWorkarounds);

} // namespace NS::RHI
