/// @file WindowsApplication.h
/// @brief Windows固有アプリケーション実装
#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // must precede xinput.h

#include <shobjidl.h>
#include <xinput.h>

#undef IsMaximized
#undef IsMinimized
#endif

#include "ApplicationCore/GamepadTypes.h"
#include "GenericPlatform/GenericApplication.h"
#include "Windows/WindowsTextInputMethodSystem.h"
#include "Windows/WindowsWindow.h"
#include <array>
#include <memory>
#include <vector>
#include <wrl/client.h>

namespace NS
{
    // =========================================================================
    // IWindowsMessageHandler
    // =========================================================================

    /// 外部メッセージハンドラインターフェース
    class IWindowsMessageHandler
    {
    public:
        virtual ~IWindowsMessageHandler() = default;

        /// @return true: メッセージ処理済み、falseで通常処理続行
        virtual bool ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam, int32_t& OutResult) = 0;
    };

    // =========================================================================
    // DeferredWindowsMessage — 遅延メッセージ
    // =========================================================================

    struct DeferredWindowsMessage
    {
        std::weak_ptr<WindowsWindow> NativeWindow;
        HWND hWnd = nullptr;
        uint32_t Msg = 0;
        WPARAM wParam = 0;
        LPARAM lParam = 0;
        int32_t X = 0;
        int32_t Y = 0;
        uint32_t RawInputFlags = 0;
    };

    // =========================================================================
    // WindowsApplication
    // =========================================================================

    /// Windows固有アプリケーション
    class WindowsApplication : public GenericApplication
    {
    public:
        /// ファクトリ
        static std::shared_ptr<WindowsApplication> CreateWindowsApplication(HINSTANCE hInstance, HICON hIcon);
        ~WindowsApplication() override;

        /// 外部メッセージハンドラ管理
        void AddMessageHandler(IWindowsMessageHandler& Handler);
        void RemoveMessageHandler(IWindowsMessageHandler& Handler);

        /// インスタンスハンドル
        HINSTANCE GetInstanceHandle() const { return m_hInstance; }

        /// WndProc（WindowsWindow::Initialize で設定）
        static LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // =================================================================
        // GenericApplication overrides
        // =================================================================
        void PumpMessages(float TimeDelta) override;
        void PollGameDeviceState(float TimeDelta) override;
        void ProcessDeferredEvents(float TimeDelta) override;
        void Tick(float TimeDelta) override;

        std::shared_ptr<GenericWindow> MakeWindow() override;
        void InitializeWindow(const std::shared_ptr<GenericWindow>& Window,
                              const GenericWindowDefinition& Definition,
                              const std::shared_ptr<GenericWindow>& Parent,
                              bool bShowImmediately) override;

        void SetCapture(const std::shared_ptr<GenericWindow>& Window) override;
        void* GetCapture() const override;
        std::shared_ptr<GenericWindow> GetWindowUnderCursor() const override;

        ModifierKeysState GetModifierKeys() const override;
        void SetHighPrecisionMouseMode(bool bEnable, const std::shared_ptr<GenericWindow>& Window) override;
        bool IsMouseAttached() const override;
        bool IsGamepadAttached() const override;
        bool IsUsingHighPrecisionMouseMode() const override;

        void DestroyApplication() override;
        PlatformRect GetWorkArea(const PlatformRect& CurrentWindow) const override;
        WindowTransparency GetWindowTransparencySupport() const override;

        /// テキスト入力メソッドシステム (IME)
        ITextInputMethodSystem* GetTextInputMethodSystem() override;

        /// Force Feedback (XInput)
        void SetForceFeedbackChannelValue(int32_t ControllerId, int32_t ChannelType, float Value);
        void SetForceFeedbackChannelValues(int32_t ControllerId, const ForceFeedbackValues& Values);

    private:
        explicit WindowsApplication(HINSTANCE hInstance, HICON hIcon);

        /// メッセージルーティング
        int32_t ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);
        void DeferMessage(const std::shared_ptr<WindowsWindow>& Window,
                          HWND hWnd,
                          uint32_t Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          int32_t X = 0,
                          int32_t Y = 0,
                          uint32_t RawInputFlags = 0);
        void ProcessDeferredMessage(const DeferredWindowsMessage& Msg);

        /// ウィンドウ検索
        std::shared_ptr<WindowsWindow> FindWindowByHWND(HWND hwnd) const;

        /// 修飾キー状態更新
        void UpdateModifierKeyState(WPARAM wParam, LPARAM lParam, bool bKeyDown);

        /// XInput
        void PollXInput();

        /// Raw Input (13-01)
        void ProcessRawMouseInput(const RAWMOUSE& MouseData);

        // メンバ
        HINSTANCE m_hInstance = nullptr;
        std::vector<std::shared_ptr<WindowsWindow>> m_windows;
        std::vector<IWindowsMessageHandler*> m_messageHandlers;
        std::vector<DeferredWindowsMessage> m_deferredMessages;

        // 修飾キー状態 (L/R Shift, Ctrl, Alt, Command, CapsLock)
        enum ModifierKeyIndex
        {
            LeftShift = 0,
            RightShift,
            LeftControl,
            RightControl,
            LeftAlt,
            RightAlt,
            LeftCommand,
            RightCommand,
            CapsLock,
            ModifierKeyCount
        };
        std::array<bool, ModifierKeyCount> m_modifierKeyState = {};

        // マウス
        bool m_bIsMouseAttached = true;
        bool m_bUsingHighPrecisionMouse = false;

        // OLE
        bool m_bOleInitialized = false;

        // Raw Input (13-01)
        int32_t m_lastRawMouseX = 0;
        int32_t m_lastRawMouseY = 0;
        bool m_bRawMouseFirstMove = true;

        // XInput
        float m_lastTimeDelta = 1.0f / 60.0f;
        static constexpr int32_t MaxXInputControllers = 4;
        struct XInputControllerState
        {
            XINPUT_STATE LastState = {};
            XINPUT_VIBRATION LastVibration = {};
            bool bConnected = false;
            float DisconnectedCooldown = 0.0f;
        };
        std::array<XInputControllerState, MaxXInputControllers> m_xinputStates = {};
        bool m_bGamepadEnabled = true;

        // IME
        WindowsTextInputMethodSystem m_textInputMethodSystem;

        // ITaskbarList3 (タスクバー進捗)
        Microsoft::WRL::ComPtr<ITaskbarList3> m_taskbarList;

        // アクセシビリティキー保存
        STICKYKEYS m_savedStickyKeys = {};
        TOGGLEKEYS m_savedToggleKeys = {};
        FILTERKEYS m_savedFilterKeys = {};
    };

} // namespace NS
