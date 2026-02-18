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
        IWindowsMessageHandler() = default;
        virtual ~IWindowsMessageHandler() = default;
        NS_DISALLOW_COPY_AND_MOVE(IWindowsMessageHandler);

    public:
        /// @return true: メッセージ処理済み、falseで通常処理続行
        virtual bool ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam, int32_t& outResult) = 0;
    };

    // =========================================================================
    // DeferredWindowsMessage — 遅延メッセージ
    // =========================================================================

    struct DeferredWindowsMessage
    {
        std::weak_ptr<WindowsWindow> nativeWindow;
        HWND hWnd = nullptr;
        uint32_t msg = 0;
        WPARAM wParam = 0;
        LPARAM lParam = 0;
        int32_t x = 0;
        int32_t y = 0;
        uint32_t rawInputFlags = 0;
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
        NS_DISALLOW_COPY_AND_MOVE(WindowsApplication);

    public:
        /// 外部メッセージハンドラ管理
        void AddMessageHandler(IWindowsMessageHandler& handler);
        void RemoveMessageHandler(IWindowsMessageHandler& handler);

        /// インスタンスハンドル
        HINSTANCE GetInstanceHandle() const { return m_hInstance; }

        /// WndProc（WindowsWindow::Initialize で設定）
        static LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // =================================================================
        // GenericApplication overrides
        // =================================================================
        void PumpMessages(float timeDelta) override;
        void PollGameDeviceState(float timeDelta) override;
        void ProcessDeferredEvents(float timeDelta) override;
        void Tick(float timeDelta) override;

        std::shared_ptr<GenericWindow> MakeWindow() override;
        void InitializeWindow(const std::shared_ptr<GenericWindow>& window,
                              const GenericWindowDefinition& definition,
                              const std::shared_ptr<GenericWindow>& parent,
                              bool bShowImmediately) override;

        void SetCapture(const std::shared_ptr<GenericWindow>& window) override;
        void* GetCapture() const override;
        std::shared_ptr<GenericWindow> GetWindowUnderCursor() const override;

        ModifierKeysState GetModifierKeys() const override;
        void SetHighPrecisionMouseMode(bool bEnable, const std::shared_ptr<GenericWindow>& window) override;
        bool IsMouseAttached() const override;
        bool IsGamepadAttached() const override;
        bool IsUsingHighPrecisionMouseMode() const override;

        void DestroyApplication() override;
        PlatformRect GetWorkArea(const PlatformRect& currentWindow) const override;
        WindowTransparency GetWindowTransparencySupport() const override;

        /// テキスト入力メソッドシステム (IME)
        ITextInputMethodSystem* GetTextInputMethodSystem() override;

        /// Force Feedback (XInput)
        void SetForceFeedbackChannelValue(int32_t controllerId, int32_t channelType, float value);
        void SetForceFeedbackChannelValues(int32_t controllerId, const ForceFeedbackValues& values);

    private:
        explicit WindowsApplication(HINSTANCE hInstance, HICON hIcon);

        /// メッセージルーティング
        int32_t ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);
        void DeferMessage(const std::shared_ptr<WindowsWindow>& window,
                          HWND hWnd,
                          uint32_t msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          int32_t x = 0,
                          int32_t y = 0,
                          uint32_t rawInputFlags = 0);
        void ProcessDeferredMessage(const DeferredWindowsMessage& msg);

        /// ウィンドウ検索
        std::shared_ptr<WindowsWindow> FindWindowByHWND(HWND hwnd) const;

        /// 修飾キー状態更新
        void UpdateModifierKeyState(WPARAM wParam, LPARAM lParam, bool bKeyDown);

        /// XInput
        void PollXInput();

        /// Raw Input (13-01)
        void ProcessRawMouseInput(const RAWMOUSE& mouseData);

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
        float m_lastTimeDelta = 1.0F / 60.0F;
        static constexpr int32_t kMaxXInputControllers = 4;
        struct XInputControllerState
        {
            XINPUT_STATE lastState = {};
            XINPUT_VIBRATION lastVibration = {};
            bool bConnected = false;
            float disconnectedCooldown = 0.0F;
        };
        std::array<XInputControllerState, kMaxXInputControllers> m_xinputStates = {};
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
