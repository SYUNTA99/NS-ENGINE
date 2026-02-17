/// @file GenericPlatformApplicationMisc.h
/// @brief プラットフォーム共通アプリケーションユーティリティ
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/InputTypes.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace NS
{
    // 前方宣言
    class GenericApplication;
    class GenericWindow;

    /// プラットフォーム共通アプリケーションユーティリティ基底
    class GenericPlatformApplicationMisc
    {
    public:
        // =================================================================
        // ライフサイクル
        // =================================================================

        /// プラットフォーム固有の事前初期化
        static void PreInit() {}

        /// 初期化
        static void Init() {}

        /// 初期化後処理
        static void PostInit() {}

        /// シャットダウン
        static void TearDown() {}

        /// プリイニシャルモジュール読み込み（将来のモジュールシステム用）
        static void LoadPreInitModules() {}

        /// スタートアップモジュール読み込み（将来のモジュールシステム用）
        static void LoadStartupModules() {}

        // =================================================================
        // アプリケーション生成
        // =================================================================

        /// プラットフォーム固有アプリケーションの生成
        static std::shared_ptr<GenericApplication> CreateApplication();

        // =================================================================
        // DPI
        // =================================================================

        /// 高DPIモードの設定（プラットフォーム初期化時に呼び出す）
        static void SetHighDPIMode() {}

        /// 高DPI対応が有効か
        static bool IsHighDPIAwarenessEnabled() { return true; }

        /// ポイント座標からDPIスケールファクターを取得
        static float GetDPIScaleFactorAtPoint(int32_t X, int32_t Y)
        {
            (void)X;
            (void)Y;
            return 1.0f;
        }

        // =================================================================
        // スクリーンセーバー
        // =================================================================

        /// スクリーンセーバーの有効/無効を制御
        static bool ControlScreensaver(ScreenSaverAction Action)
        {
            (void)Action;
            return true;
        }

        /// スクリーンセーバーを防止（0ピクセル入力送信）
        static void PreventScreenSaver() { ControlScreensaver(ScreenSaverAction::Disable); }

        /// スクリーンセーバーが有効か
        static bool IsScreensaverEnabled() { return true; }

        // =================================================================
        // クリップボード
        // =================================================================

        /// クリップボードにテキストをコピー
        static void ClipboardCopy(const wchar_t* Str) { (void)Str; }

        /// クリップボードからテキストを貼り付け
        static void ClipboardPaste(std::wstring& Dest) { Dest.clear(); }

        // =================================================================
        // アプリケーション状態
        // =================================================================

        /// このアプリケーションがフォアグラウンドか
        static bool IsThisApplicationForeground() { return true; }

        /// メッセージポンプ
        static void PumpMessages(bool bFromMainLoop) { (void)bFromMainLoop; }

        /// 最小化を要求
        static void RequestMinimize() {}

        /// 仮想キーボードが必要か（モバイル等）
        static bool RequiresVirtualKeyboard() { return false; }

        /// ウィンドウ位置を左上に固定するか
        /// @note UE5 API名 "WindowWindow" は意図的なタイポ保存
        static bool AnchorWindowWindowPositionTopLeft() { return false; }

        /// タイトルが一致するウィンドウを検索
        static bool GetWindowTitleMatchingText(const wchar_t* TitleStartsWith, std::wstring& OutTitle)
        {
            (void)TitleStartsWith;
            OutTitle.clear();
            return false;
        }

        // =================================================================
        // ゲームパッド
        // =================================================================

        /// ゲームパッドの使用を許可/禁止
        static void SetGamepadsAllowed(bool bAllowed) { (void)bAllowed; }

        /// コントローラーがゲームパッドに割り当てられているか
        static bool IsControllerAssignedToGamepad(int32_t ControllerId)
        {
            (void)ControllerId;
            return false;
        }

        /// ゲームパッドコントローラー名を取得
        static std::wstring GetGamepadControllerName(int32_t ControllerId)
        {
            (void)ControllerId;
            return {};
        }

        /// ゲームパッド割り当てリセット
        static void ResetGamepadAssignments() {}

        /// コントローラーへのゲームパッド割り当てリセット
        static void ResetGamepadAssignmentToController(int32_t ControllerId) { (void)ControllerId; }

        /// デバイスフィードバックのブロック設定
        static void SetGamepadsBlockDeviceFeedback(bool bBlock) { (void)bBlock; }

        // =================================================================
        // 物理スクリーン情報
        // =================================================================

        /// 物理スクリーン密度 (DPI)
        static ScreenPhysicalAccuracy GetPhysicalScreenDensity(int32_t& OutDensity)
        {
            OutDensity = 0;
            return ScreenPhysicalAccuracy::Unknown;
        }

        /// 物理スクリーン寸法 (インチ)
        static ScreenPhysicalAccuracy GetPhysicalScreenDimensions(float& OutWidth, float& OutHeight)
        {
            OutWidth = 0.0f;
            OutHeight = 0.0f;
            return ScreenPhysicalAccuracy::Unknown;
        }

        /// 物理スクリーンサイズ (対角インチ)
        static ScreenPhysicalAccuracy GetPhysicalScreenSize(float& OutSize)
        {
            OutSize = 0.0f;
            return ScreenPhysicalAccuracy::Unknown;
        }

        /// インチからピクセルへ変換
        template <typename T>
        static void ConvertInchesToPixels(T Inches, T& OutPixels)
        {
            // デフォルト96 DPI
            OutPixels = static_cast<T>(static_cast<float>(Inches) * 96.0f);
        }

        /// ピクセルからインチへ変換
        template <typename T>
        static void ConvertPixelsToInches(T Pixels, T& OutInches)
        {
            OutInches = static_cast<T>(static_cast<float>(Pixels) / 96.0f);
        }

        // =================================================================
        // モーションデータ
        // =================================================================

        /// モーションデータの有効/無効
        static void EnableMotionData(bool bEnable) { (void)bEnable; }

        /// モーションデータが有効か
        static bool IsMotionDataEnabled() { return false; }

        // =================================================================
        // ユーティリティ
        // =================================================================

        /// プラットフォーム名の取得
        static const wchar_t* GetPlatformName() { return L"Generic"; }

        /// リモートセッション検出 (RDP/VNC 等)
        static bool IsRemoteSession() { return false; }

        /// フォーカスウィンドウを設定（プラットフォームフォーカスコールバック経由）
        using PlatformFocusCallback = std::function<void()>;
        static PlatformFocusCallback OnFocusCallback;
    };

} // namespace NS
