/// @file WindowsPlatformApplicationMisc.h
/// @brief Windows 固有アプリケーションユーティリティ
#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

namespace NS
{
    /// Windows 固有アプリケーションユーティリティ
    class WindowsPlatformApplicationMisc : public GenericPlatformApplicationMisc
    {
    public:
        // =================================================================
        // ライフサイクル
        // =================================================================

        static void PreInit();
        static void Init();
        static void TearDown();

        // =================================================================
        // アプリケーション生成
        // =================================================================

        static std::shared_ptr<GenericApplication> CreateApplication();

        // =================================================================
        // DPI
        // =================================================================

        /// 高DPIモードの設定 (Per-Monitor V2 / fallback)
        static void SetHighDPIMode();

        /// ポイント座標からDPIスケールファクターを取得
        static float GetDPIScaleFactorAtPoint(int32_t X, int32_t Y);

        // =================================================================
        // スクリーンセーバー
        // =================================================================

        static bool ControlScreensaver(ScreenSaverAction Action);

        // =================================================================
        // クリップボード
        // =================================================================

        static void ClipboardCopy(const wchar_t* Str);
        static void ClipboardPaste(std::wstring& Dest);

        // =================================================================
        // アプリケーション状態
        // =================================================================

        /// フォアグラウンド判定
        static bool IsThisApplicationForeground();

        /// メッセージポンプ (PeekMessage/TranslateMessage/DispatchMessage)
        static void PumpMessages(bool bFromMainLoop);

        // =================================================================
        // ゲームパッド
        // =================================================================

        /// XInput コントローラー名を取得
        static std::wstring GetGamepadControllerName(int32_t ControllerId);

        // =================================================================
        // ユーティリティ
        // =================================================================

        static const wchar_t* GetPlatformName() { return L"Windows"; }
        static bool IsRemoteSession();
    };

} // namespace NS
