/// @file WindowsApplication.cpp
/// @brief WindowsApplication 実装 (06-02 ~ 06-07)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dwmapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include <xinput.h>
#endif

#undef IsMaximized
#undef IsMinimized

#include "ApplicationCore/GamepadTypes.h"
#include "Windows/WindowsApplication.h"

#include <algorithm>
#include <cmath>
#include <utility> // std::swap

#pragma comment(lib, "xinput.lib")

namespace
{
    // XInput デッドゾーン

    // XInput デッドゾーン
    constexpr float XInputLeftThumbDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
    constexpr float XInputRightThumbDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    constexpr float XInputTriggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;

    float ApplyDeadZone(float Value, float DeadZone)
    {
        if (Value > DeadZone)
            return (Value - DeadZone) / (1.0f - DeadZone);
        if (Value < -DeadZone)
            return (Value + DeadZone) / (1.0f - DeadZone);
        return 0.0f;
    }

    NS::MouseButtons::Type XButtonToMouseButton(WPARAM wParam)
    {
        return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? NS::MouseButtons::Thumb01 : NS::MouseButtons::Thumb02;
    }
} // anonymous namespace

namespace NS
{
    // グローバル WindowsApplication インスタンス（AppWndProc で使用）
    static WindowsApplication* g_windowsApp = nullptr;

    // =========================================================================
    // 06-02: ファクトリ・コンストラクタ・デストラクタ
    // =========================================================================

    std::shared_ptr<WindowsApplication> WindowsApplication::CreateWindowsApplication(HINSTANCE hInstance, HICON hIcon)
    {
        // DPI awareness は WindowsPlatformApplicationMisc::SetHighDPIMode() で一元管理

        // ウィンドウクラス登録
        WindowsWindow::Initialize(hInstance, hIcon);

        auto App = std::shared_ptr<WindowsApplication>(new WindowsApplication(hInstance, hIcon));
        g_windowsApp = App.get();

        return App;
    }

    WindowsApplication::WindowsApplication(HINSTANCE hInstance, HICON /*hIcon*/)
        : GenericApplication(nullptr) // カーソルは後で設定 (Phase 7)
          ,
          m_hInstance(hInstance)
    {
        // OLE 初期化
        HRESULT hr = ::OleInitialize(NULL);
        m_bOleInitialized = SUCCEEDED(hr);
        if (!m_bOleInitialized)
        {
            // RPC_E_CHANGED_MODE: COM already initialized with different threading model
            // OLE D&D is disabled in this case
        }

        // アクセシビリティキーの保存
        ::SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &m_savedStickyKeys, 0);

        m_savedToggleKeys.cbSize = sizeof(TOGGLEKEYS);
        ::SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &m_savedToggleKeys, 0);

        m_savedFilterKeys.cbSize = sizeof(FILTERKEYS);
        ::SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &m_savedFilterKeys, 0);

        // ゴースト化防止
        ::DisableProcessWindowsGhosting();

        m_deferredMessages.reserve(64);

        // ITaskbarList3 初期化 (ComPtr RAII)
        HRESULT hrTb =
            ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_taskbarList));
        if (SUCCEEDED(hrTb) && m_taskbarList)
        {
            m_taskbarList->HrInit();
        }
    }

    WindowsApplication::~WindowsApplication()
    {
        // グローバル WindowsApplication インスタンス（AppWndProc で使用）
        g_windowsApp = nullptr;

        // ウィンドウを逆順で破棄
        for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
        {
            if (*it)
            {
                (*it)->Destroy();
            }
        }
        m_windows.clear();

        // アクセシビリティキーの復元
        ::SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &m_savedToggleKeys, 0);
        ::SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &m_savedFilterKeys, 0);

        // TaskbarList 解放 (ComPtr が自動 Release)
        m_taskbarList.Reset();

        // OLE 後処理
        if (m_bOleInitialized)
        {
            ::OleUninitialize();
        }

        // ウィンドウクラス解除
        ::UnregisterClass(WindowsWindow::AppWindowClass, m_hInstance);
    }

    // =========================================================================
    // 06-02: ウィンドウ管理

    std::shared_ptr<GenericWindow> WindowsApplication::MakeWindow()
    {
        auto Window = WindowsWindow::MakeWindow();
        m_windows.push_back(Window);
        return Window;
    }

    void WindowsApplication::InitializeWindow(const std::shared_ptr<GenericWindow>& Window,
                                              const GenericWindowDefinition& Definition,
                                              const std::shared_ptr<GenericWindow>& Parent,
                                              bool bShowImmediately)
    {
        auto WinWindow = std::static_pointer_cast<WindowsWindow>(Window);
        auto WinParent = Parent ? std::static_pointer_cast<WindowsWindow>(Parent) : nullptr;

        WinWindow->Initialize(this, Definition, m_hInstance, WinParent, bShowImmediately);

        // Initialize 失敗時は管理リストから除去（無効エントリ蓄積防止）
        if (WinWindow->GetHWnd() == nullptr)
        {
            m_windows.erase(std::remove(m_windows.begin(), m_windows.end(), WinWindow), m_windows.end());
        }
    }

    void WindowsApplication::SetCapture(const std::shared_ptr<GenericWindow>& Window)
    {
        if (Window)
        {
            auto WinWindow = std::static_pointer_cast<WindowsWindow>(Window);
            ::SetCapture(WinWindow->GetHWnd());
        }
        else
        {
            ::ReleaseCapture();
        }
    }

    void* WindowsApplication::GetCapture() const
    {
        return ::GetCapture();
    }

    std::shared_ptr<GenericWindow> WindowsApplication::GetWindowUnderCursor() const
    {
        POINT pt;
        ::GetCursorPos(&pt);
        HWND hwnd = ::WindowFromPoint(pt);
        return FindWindowByHWND(hwnd);
    }

    std::shared_ptr<WindowsWindow> WindowsApplication::FindWindowByHWND(HWND hwnd) const
    {
        for (const auto& Window : m_windows)
        {
            if (Window && Window->GetHWnd() == hwnd)
            {
                return Window;
            }
        }
        return nullptr;
    }

    // =========================================================================
    // 06-02: メッセージハンドラ管理

    void WindowsApplication::AddMessageHandler(IWindowsMessageHandler& Handler)
    {
        m_messageHandlers.push_back(&Handler);
    }

    void WindowsApplication::RemoveMessageHandler(IWindowsMessageHandler& Handler)
    {
        m_messageHandlers.erase(std::remove(m_messageHandlers.begin(), m_messageHandlers.end(), &Handler),
                                m_messageHandlers.end());
    }

    // =========================================================================
    // 06-03: メッセージポンプ

    void WindowsApplication::PumpMessages(float /*TimeDelta*/)
    {
        MSG msg = {};
        while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    void WindowsApplication::Tick(float TimeDelta)
    {
        PollGameDeviceState(TimeDelta);
    }

    // =========================================================================
    // 06-03: AppWndProc
    // =========================================================================

    LRESULT CALLBACK WindowsApplication::AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (g_windowsApp)
        {
            // 外部メッセージハンドラを先に処理
            auto handlers = g_windowsApp->m_messageHandlers; // コピーしてイテレーション安全
            for (auto* handler : handlers)
            {
                int32_t result = 0;
                if (handler->ProcessMessage(hwnd, msg, wParam, lParam, result))
                {
                    return result;
                }
            }

            int32_t result = g_windowsApp->ProcessMessage(hwnd, msg, wParam, lParam);
            if (result != -1)
            {
                return result;
            }
        }

        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // =========================================================================
    // 06-03 ~ 06-06: ProcessMessage (メインディスパッチ)
    // =========================================================================

    int32_t WindowsApplication::ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
    {
        if (!m_messageHandler.Get())
        {
            return -1;
        }

        auto Window = FindWindowByHWND(hwnd);
        const bool bShouldProcessUserInput = m_messageHandler->ShouldProcessUserInputMessages(Window);
        auto HandledResult = [](bool bHandled) -> int32_t { return bHandled ? 0 : -1; };

        switch (msg)
        {
        // =============================================================
        // 06-04: キーボード
        case WM_CHAR:
        case WM_SYSCHAR:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            bool bIsRepeat = (lParam & (1 << 30)) != 0;
            return HandledResult(m_messageHandler->OnKeyChar(static_cast<NS::TCHAR>(wParam), bIsRepeat));
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            int32_t keyCode = static_cast<int32_t>(wParam);
            bool bIsRepeat = (lParam & (1 << 30)) != 0;
            uint32_t charCode = ::MapVirtualKey(keyCode, MAPVK_VK_TO_CHAR);
            UpdateModifierKeyState(wParam, lParam, true);
            return HandledResult(m_messageHandler->OnKeyDown(keyCode, charCode, bIsRepeat));
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            int32_t keyCode = static_cast<int32_t>(wParam);
            uint32_t charCode = ::MapVirtualKey(keyCode, MAPVK_VK_TO_CHAR);
            UpdateModifierKeyState(wParam, lParam, false);
            return HandledResult(m_messageHandler->OnKeyUp(keyCode, charCode, false));
        }

        // =============================================================
        // 06-05: マウスボタン
        // =============================================================
        case WM_LBUTTONDOWN:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDown(
                Window,
                MouseButtons::Left,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONDOWN:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDown(
                Window,
                MouseButtons::Middle,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONDOWN:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDown(
                Window,
                MouseButtons::Right,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_XBUTTONDOWN:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
        {
            const bool bHandled = m_messageHandler->OnMouseDown(
                Window,
                XButtonToMouseButton(wParam),
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))});
            return bHandled ? TRUE : -1;
        }

        case WM_LBUTTONUP:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseUp(
                MouseButtons::Left,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONUP:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseUp(
                MouseButtons::Middle,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONUP:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseUp(
                MouseButtons::Right,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_XBUTTONUP:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
        {
            const bool bHandled = m_messageHandler->OnMouseUp(
                XButtonToMouseButton(wParam),
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))});
            return bHandled ? TRUE : -1;
        }

        case WM_LBUTTONDBLCLK:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDoubleClick(
                Window,
                MouseButtons::Left,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONDBLCLK:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDoubleClick(
                Window,
                MouseButtons::Middle,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONDBLCLK:
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            return HandledResult(m_messageHandler->OnMouseDoubleClick(
                Window,
                MouseButtons::Right,
                Vector2D{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))}));

        // =============================================================
        // 06-05: マウス移動・ホイール
        // =============================================================
        case WM_MOUSEMOVE:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            // WM_MOUSEMOVE の lParam はクライアント座標
            Vector2D cursorPos{static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam))};
            return HandledResult(m_messageHandler->OnMouseMove(cursorPos));
        }
        case WM_NCMOUSEMOVE:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            // WM_NCMOUSEMOVE の lParam はスクリーン座標 -> クライアント座標に変換
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ScreenToClient(hwnd, &pt);
            return HandledResult(
                m_messageHandler->OnMouseMove(Vector2D{static_cast<float>(pt.x), static_cast<float>(pt.y)}));
        }

        case WM_MOUSEWHEEL:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            // WM_MOUSEWHEEL の lParam はスクリーン座標なので、クライアント座標に変換
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ScreenToClient(hwnd, &pt);
            return HandledResult(
                m_messageHandler->OnMouseWheel(delta, Vector2D{static_cast<float>(pt.x), static_cast<float>(pt.y)}));
        }

        // =============================================================
        // 06-06: ウィンドウイベント
        case WM_SIZE:
        {
            if (Window)
            {
                int32_t w = LOWORD(lParam);
                int32_t h = HIWORD(lParam);
                bool bWasMinimized = (wParam == SIZE_MINIMIZED);
                m_messageHandler->OnSizeChanged(Window, w, h, bWasMinimized);

                if (wParam == SIZE_MAXIMIZED)
                    m_messageHandler->OnWindowAction(Window, WindowAction::Maximize);
                else if (wParam == SIZE_RESTORED)
                    m_messageHandler->OnWindowAction(Window, WindowAction::Restore);
            }
            return 0;
        }
        case WM_MOVE:
        {
            if (Window)
            {
                int32_t x = static_cast<int16_t>(LOWORD(lParam));
                int32_t y = static_cast<int16_t>(HIWORD(lParam));
                m_messageHandler->OnMovedWindow(Window, x, y);
            }
            return 0;
        }
        case WM_ACTIVATE:
        {
            if (Window)
            {
                WindowActivation::Type activation =
                    (LOWORD(wParam) != WA_INACTIVE) ? WindowActivation::Activate : WindowActivation::Deactivate;
                if (LOWORD(wParam) == WA_CLICKACTIVE)
                    activation = WindowActivation::ActivateByMouse;
                m_messageHandler->OnWindowActivationChanged(Window, activation);
            }
            return 0;
        }
        case WM_ACTIVATEAPP:
        {
            m_messageHandler->OnApplicationActivationChanged(wParam != 0);
            return 0;
        }
        case WM_CLOSE:
        {
            if (Window)
            {
                m_messageHandler->OnWindowClose(Window);
            }
            return 0;
        }
        case WM_PAINT:
        {
            if (Window)
            {
                m_messageHandler->OnOSPaint(Window);
            }
            break; // DefWindowProc handles BeginPaint/EndPaint
        }

        // =============================================================
        // 06-04/06-06: 非クライアント領域
        // =============================================================
        case WM_NCHITTEST:
        {
            if (Window && !Window->GetDefinition().HasOSWindowBorder)
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ::ScreenToClient(hwnd, &pt);

                WindowZone::Type zone = m_messageHandler->GetWindowZoneForPoint(Window, pt.x, pt.y);
                return WindowsWindow::WindowZoneToHitTest(zone);
            }
            break;
        }
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        {
            if (Window)
            {
                m_messageHandler->OnWindowAction(Window, WindowAction::ClickedNonClientArea);
            }
            break;
        }

        case WM_GETMINMAXINFO:
        {
            if (Window)
            {
                MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                WindowSizeLimits limits = m_messageHandler->GetSizeLimitsForWindow(Window);
                float dpi = Window->GetDPIScaleFactor();

                if (limits.GetMinWidth().has_value())
                    mmi->ptMinTrackSize.x = static_cast<LONG>(limits.GetMinWidth().value() * dpi);
                if (limits.GetMinHeight().has_value())
                    mmi->ptMinTrackSize.y = static_cast<LONG>(limits.GetMinHeight().value() * dpi);
                if (limits.GetMaxWidth().has_value())
                    mmi->ptMaxTrackSize.x = static_cast<LONG>(limits.GetMaxWidth().value() * dpi);
                if (limits.GetMaxHeight().has_value())
                    mmi->ptMaxTrackSize.y = static_cast<LONG>(limits.GetMaxHeight().value() * dpi);
            }
            return 0;
        }

        case WM_NCCALCSIZE:
        {
            if (wParam == TRUE && Window && !Window->GetDefinition().HasOSWindowBorder)
            {
                // ボーダーレスウィンドウ: 非クライアント領域を0に
                return 0;
            }
            break;
        }

        case WM_ERASEBKGND:
            return 1; // 閭梧勹豸亥悉謚大宛

        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;

        // =============================================================
        // DPI
        // =============================================================
        case WM_DPICHANGED:
        {
            if (Window)
            {
                float newDPI = static_cast<float>(LOWORD(wParam)) / 96.0f;

                m_messageHandler->SignalSystemDPIChanged(Window);

                if (!Window->IsManualManageDPIChanges())
                {
                    Window->SetDPIScaleFactor(newDPI);

                    RECT* prc = reinterpret_cast<RECT*>(lParam);
                    ::SetWindowPos(hwnd,
                                   nullptr,
                                   prc->left,
                                   prc->top,
                                   prc->right - prc->left,
                                   prc->bottom - prc->top,
                                   SWP_NOZORDER | SWP_NOACTIVATE);

                    m_messageHandler->HandleDPIScaleChanged(Window);
                }
                else
                {
                    Window->SetDPIScaleFactor(newDPI);
                }
            }
            return 0;
        }

        case WM_ENTERSIZEMOVE:
        {
            if (Window)
                m_messageHandler->BeginReshapingWindow(Window);
            return 0;
        }
        case WM_EXITSIZEMOVE:
        {
            if (Window)
                m_messageHandler->FinishedReshapingWindow(Window);
            return 0;
        }
        case WM_SIZING:
        {
            if (Window)
                m_messageHandler->OnResizingWindow(Window);
            break;
        }

        case WM_DISPLAYCHANGE:
        {
            DisplayMetrics metrics;
            DisplayMetrics::RebuildDisplayMetrics(metrics);
            BroadcastDisplayMetricsChanged(metrics);
            return 0;
        }

        case WM_SETCURSOR:
        {
            if (Window && !Window->GetDefinition().HasOSWindowBorder)
            {
                m_messageHandler->OnCursorSet();
                return 0;
            }
            break;
        }

        case WM_SYSCOMMAND:
        {
            uint32_t cmd = static_cast<uint32_t>(wParam) & 0xFFF0;
            if (cmd == SC_RESTORE && Window)
            {
                m_messageHandler->OnWindowAction(Window, WindowAction::Restore);
            }
            else if (cmd == SC_MAXIMIZE && Window)
            {
                m_messageHandler->OnWindowAction(Window, WindowAction::Maximize);
            }
            break;
        }

        // =============================================================
        // 13-01: Raw Input (高精度マウス)
        // =============================================================
        case WM_INPUT:
        {
            UINT dwSize = 0;
            ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            constexpr UINT kMaxRawInputSize = sizeof(RAWINPUT) + 64;
            if (dwSize > 0 && dwSize <= kMaxRawInputSize)
            {
                alignas(8) BYTE buffer[kMaxRawInputSize];
                if (::GetRawInputData(
                        reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)) ==
                    dwSize)
                {
                    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
                    if (raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        ProcessRawMouseInput(raw->data.mouse);
                    }
                }
            }
            return 0;
        }

        // =============================================================
        // 11-05: IME メッセージルーティング
        // =============================================================
        case WM_IME_SETCONTEXT:
        case WM_IME_NOTIFY:
        case WM_IME_REQUEST:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_CHAR:
        case WM_INPUTLANGCHANGEREQUEST:
        case WM_INPUTLANGCHANGE:
        {
            int32_t imeResult = 0;
            if (m_textInputMethodSystem.ProcessMessage(hwnd, msg, wParam, lParam, imeResult))
            {
                return imeResult;
            }
            break;
        }

        // =============================================================
        // 追加ウィンドウイベント
        case WM_CAPTURECHANGED:
        {
            // マウスキャプチャが失われた
            return 0;
        }
        case WM_CLIPBOARDUPDATE:
        {
            OnClipboardContentChanged().Broadcast();
            return 0;
        }
        case WM_SETTINGCHANGE:
        {
            // システム設定変更（DPI等の再取得）
            return 0;
        }

#if UE_WINDOWS_USING_UIA
        // =============================================================
        // 14-03: UIA アクセシビリティ (WM_GETOBJECT)
        // =============================================================
        case WM_GETOBJECT:
        {
            if (static_cast<long>(lParam) == UiaRootObjectId && Window)
            {
                // 14-03: UIA アクセシビリティ (WM_GETOBJECT)
            }
            break;
        }
#endif
        case WM_DEVICECHANGE:
        {
            // デバイス追加/除去
            return 0;
        }

        // =============================================================
        // タッチ入力
        // =============================================================
        case WM_TOUCH:
        {
            if (!bShouldProcessUserInput)
            {
                return -1;
            }
            HTOUCHINPUT touchHandle = reinterpret_cast<HTOUCHINPUT>(lParam);
            UINT touchCount = LOWORD(wParam);
            if (touchCount == 0 || touchCount > 256)
            {
                ::CloseTouchInputHandle(touchHandle);
                return 0;
            }

            std::vector<TOUCHINPUT> touches(touchCount);
            if (::GetTouchInputInfo(touchHandle, touchCount, touches.data(), sizeof(TOUCHINPUT)))
            {
                PlatformUserId userId(0);
                InputDeviceId deviceId(0);

                for (UINT i = 0; i < touchCount; ++i)
                {
                    const TOUCHINPUT& ti = touches[i];
                    // TOUCHINPUT uses hundredths of a pixel (CENTIPOINT)
                    POINT pt = {ti.x / 100, ti.y / 100};
                    ::ScreenToClient(hwnd, &pt);
                    Vector2D location{static_cast<float>(pt.x), static_cast<float>(pt.y)};
                    float force = 1.0f;
                    int32_t touchIndex = static_cast<int32_t>(ti.dwID);

                    if (ti.dwFlags & TOUCHEVENTF_DOWN)
                    {
                        m_messageHandler->OnTouchStarted(Window, location, force, touchIndex, userId, deviceId);
                    }
                    else if (ti.dwFlags & TOUCHEVENTF_MOVE)
                    {
                        m_messageHandler->OnTouchMoved(location, force, touchIndex, userId, deviceId);
                    }
                    else if (ti.dwFlags & TOUCHEVENTF_UP)
                    {
                        m_messageHandler->OnTouchEnded(location, touchIndex, userId, deviceId);
                    }
                }
            }

            // WM_TOUCH はこの時点で自前処理し、ハンドルクローズも自前で完結させる。
            ::CloseTouchInputHandle(touchHandle);
            return 0;
        }

        default:
            break;
        }

        return -1; // 未処理 → DefWindowProc
    }

    // =========================================================================
    // 06-06: Deferred Events
    // =========================================================================

    void WindowsApplication::DeferMessage(const std::shared_ptr<WindowsWindow>& Window,
                                          HWND hWnd,
                                          uint32_t Msg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          int32_t X,
                                          int32_t Y,
                                          uint32_t RawInputFlags)
    {
        DeferredWindowsMessage deferred;
        deferred.NativeWindow = Window;
        deferred.hWnd = hWnd;
        deferred.Msg = Msg;
        deferred.wParam = wParam;
        deferred.lParam = lParam;
        deferred.X = X;
        deferred.Y = Y;
        deferred.RawInputFlags = RawInputFlags;
        m_deferredMessages.push_back(deferred);
    }

    void WindowsApplication::ProcessDeferredEvents(float /*TimeDelta*/)
    {
        // swap でバッファを再利用（ヒープ再割当を回避）
        std::vector<DeferredWindowsMessage> messages;
        messages.swap(m_deferredMessages);

        for (const auto& msg : messages)
        {
            ProcessDeferredMessage(msg);
        }
        // m_deferredMessages は messages の旧キャパシティを引き継ぐ
    }

    void WindowsApplication::ProcessDeferredMessage(const DeferredWindowsMessage& DeferMsg)
    {
        auto Window = DeferMsg.NativeWindow.lock();
        if (!Window)
        {
            return; // ウィンドウ破棄済み
        }

        // 遅延メッセージのルーティング（現在は即時処理と同じ）
        ProcessMessage(DeferMsg.hWnd, DeferMsg.Msg, DeferMsg.wParam, DeferMsg.lParam);
    }

    // =========================================================================
    // 06-04: 修飾キー状態

    void WindowsApplication::UpdateModifierKeyState(WPARAM wParam, LPARAM lParam, bool bKeyDown)
    {
        bool bIsExtended = (lParam & (1 << 24)) != 0;

        switch (wParam)
        {
        case VK_SHIFT:
        {
            UINT scanCode = static_cast<UINT>((lParam >> 16) & 0xFF);
            UINT vk = ::MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
            if (vk == VK_LSHIFT)
                m_modifierKeyState[LeftShift] = bKeyDown;
            else if (vk == VK_RSHIFT)
                m_modifierKeyState[RightShift] = bKeyDown;
            break;
        }
        case VK_CONTROL:
            if (bIsExtended)
                m_modifierKeyState[RightControl] = bKeyDown;
            else
                m_modifierKeyState[LeftControl] = bKeyDown;
            break;
        case VK_MENU:
            if (bIsExtended)
                m_modifierKeyState[RightAlt] = bKeyDown;
            else
                m_modifierKeyState[LeftAlt] = bKeyDown;
            break;
        case VK_LWIN:
            m_modifierKeyState[LeftCommand] = bKeyDown;
            break;
        case VK_RWIN:
            m_modifierKeyState[RightCommand] = bKeyDown;
            break;
        case VK_CAPITAL:
            m_modifierKeyState[CapsLock] = !!::GetKeyState(VK_CAPITAL);
            break;
        default:
            break;
        }
    }

    ModifierKeysState WindowsApplication::GetModifierKeys() const
    {
        return ModifierKeysState(m_modifierKeyState[LeftShift] || m_modifierKeyState[RightShift],
                                 m_modifierKeyState[LeftControl] || m_modifierKeyState[RightControl],
                                 m_modifierKeyState[LeftAlt] || m_modifierKeyState[RightAlt],
                                 m_modifierKeyState[LeftCommand] || m_modifierKeyState[RightCommand],
                                 m_modifierKeyState[LeftShift],
                                 m_modifierKeyState[RightShift],
                                 m_modifierKeyState[LeftAlt],
                                 m_modifierKeyState[RightAlt],
                                 m_modifierKeyState[CapsLock]);
    }

    // =========================================================================
    // 06-02: 入力状態
    // =========================================================================

    void WindowsApplication::SetHighPrecisionMouseMode(bool bEnable, const std::shared_ptr<GenericWindow>& /*Window*/)
    {
        RAWINPUTDEVICE rid = {};
        rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        rid.usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags = bEnable ? 0 : RIDEV_REMOVE;
        rid.hwndTarget = nullptr;

        if (::RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
        {
            m_bUsingHighPrecisionMouse = bEnable;
        }
    }

    bool WindowsApplication::IsMouseAttached() const
    {
        return m_bIsMouseAttached;
    }

    bool WindowsApplication::IsGamepadAttached() const
    {
        if (!m_bGamepadEnabled)
            return false;
        for (const auto& state : m_xinputStates)
        {
            if (state.bConnected)
                return true;
        }
        return false;
    }

    bool WindowsApplication::IsUsingHighPrecisionMouseMode() const
    {
        return m_bUsingHighPrecisionMouse;
    }

    // =========================================================================
    // 06-02: ユーティリティ
    // =========================================================================

    void WindowsApplication::DestroyApplication()
    {
        // ウィンドウ破棄は デストラクタ で処理
    }

    PlatformRect WindowsApplication::GetWorkArea(const PlatformRect& /*CurrentWindow*/) const
    {
        RECT workArea = {};
        ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        return {workArea.left, workArea.top, workArea.right, workArea.bottom};
    }

    WindowTransparency WindowsApplication::GetWindowTransparencySupport() const
    {
        return WindowTransparency::PerPixel;
    }

    // =========================================================================
    // 06-07: XInput ゲームパッドポーリング
    // =========================================================================

    void WindowsApplication::PollGameDeviceState(float TimeDelta)
    {
        if (!m_bGamepadEnabled)
            return;

        m_lastTimeDelta = TimeDelta;
        PollXInput();
    }

    // =========================================================================
    // 13-01: Raw Input マウス処理

    void WindowsApplication::ProcessRawMouseInput(const RAWMOUSE& MouseData)
    {
        if (MouseData.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            // RDP/リモートデスクトップ 絶対座標 → 相対デルタ変換
            int32_t screenW = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int32_t screenH = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
            int32_t screenX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
            int32_t screenY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

            int32_t absX = static_cast<int32_t>((MouseData.lLastX / 65535.0f) * screenW + screenX);
            int32_t absY = static_cast<int32_t>((MouseData.lLastY / 65535.0f) * screenH + screenY);

            int32_t deltaX = absX - m_lastRawMouseX;
            int32_t deltaY = absY - m_lastRawMouseY;
            m_lastRawMouseX = absX;
            m_lastRawMouseY = absY;

            if (m_bRawMouseFirstMove)
            {
                m_bRawMouseFirstMove = false;
                return; // ウィンドウ破棄済み
            }

            m_messageHandler->OnRawMouseMove(deltaX, deltaY);
        }
        else
        {
            // 相対移動（通常）
            m_messageHandler->OnRawMouseMove(MouseData.lLastX, MouseData.lLastY);
        }
    }

    // =========================================================================
    // 11-05: テキスト入力メソッドシステム
    // =========================================================================

    ITextInputMethodSystem* WindowsApplication::GetTextInputMethodSystem()
    {
        return &m_textInputMethodSystem;
    }

    // =========================================================================
    // Force Feedback (XInput)
    // =========================================================================

    void WindowsApplication::SetForceFeedbackChannelValue(int32_t ControllerId, int32_t ChannelType, float Value)
    {
        if (ControllerId < 0 || ControllerId >= MaxXInputControllers)
            return;
        if (!m_xinputStates[ControllerId].bConnected)
            return;

        float clamped = std::clamp(Value, 0.0f, 1.0f);
        WORD intensity = static_cast<WORD>(clamped * 65535.0f);

        // 既存の振動状態を保持しつつ該当チャンネルのみ更新
        auto& ctrl = m_xinputStates[ControllerId];
        XINPUT_VIBRATION vib = ctrl.LastVibration;

        // ChannelType: 0=LeftLarge, 1=LeftSmall, 2=RightLarge, 3=RightSmall
        if (ChannelType == 0 || ChannelType == 2)
            vib.wLeftMotorSpeed = intensity;
        else
            vib.wRightMotorSpeed = intensity;

        ctrl.LastVibration = vib;
        ::XInputSetState(static_cast<DWORD>(ControllerId), &vib);
    }

    void WindowsApplication::SetForceFeedbackChannelValues(int32_t ControllerId, const ForceFeedbackValues& Values)
    {
        if (ControllerId < 0 || ControllerId >= MaxXInputControllers)
            return;
        if (!m_xinputStates[ControllerId].bConnected)
            return;

        // 左モーター = max(LeftLarge, RightLarge), 右モーター = max(LeftSmall, RightSmall)
        float leftMotor = std::clamp(std::max(Values.LeftLarge, Values.RightLarge), 0.0f, 1.0f);
        float rightMotor = std::clamp(std::max(Values.LeftSmall, Values.RightSmall), 0.0f, 1.0f);

        XINPUT_VIBRATION vib = {};
        vib.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vib.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

        m_xinputStates[ControllerId].LastVibration = vib;
        ::XInputSetState(static_cast<DWORD>(ControllerId), &vib);
    }

    // =========================================================================
    // 06-07: XInput ゲームパッドポーリング
    // =========================================================================

    void WindowsApplication::PollXInput()
    {
        for (int32_t i = 0; i < MaxXInputControllers; ++i)
        {
            auto& ctrl = m_xinputStates[i];

            // 切断クールダウン
            if (!ctrl.bConnected)
            {
                ctrl.DisconnectedCooldown -= m_lastTimeDelta;
                if (ctrl.DisconnectedCooldown > 0.0f)
                    continue;
            }

            XINPUT_STATE state = {};
            DWORD result = ::XInputGetState(static_cast<DWORD>(i), &state);

            if (result == ERROR_SUCCESS)
            {
                if (!ctrl.bConnected)
                {
                    ctrl.bConnected = true;
                    ctrl.DisconnectedCooldown = 0.0f;
                }

                PlatformUserId userId(i);
                InputDeviceId deviceId(i);
                const XINPUT_GAMEPAD& gp = state.Gamepad;
                const XINPUT_GAMEPAD& prev = ctrl.LastState.Gamepad;

                // XInput デッドゾーン
                float leftX = ApplyDeadZone(gp.sThumbLX / 32767.0f, XInputLeftThumbDeadZone);
                float leftY = ApplyDeadZone(gp.sThumbLY / 32767.0f, XInputLeftThumbDeadZone);
                float rightX = ApplyDeadZone(gp.sThumbRX / 32767.0f, XInputRightThumbDeadZone);
                float rightY = ApplyDeadZone(gp.sThumbRY / 32767.0f, XInputRightThumbDeadZone);

                m_messageHandler->OnControllerAnalog(GamepadKeyNames::LeftAnalogX, userId, deviceId, leftX);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::LeftAnalogY, userId, deviceId, leftY);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::RightAnalogX, userId, deviceId, rightX);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::RightAnalogY, userId, deviceId, rightY);

                // --- トリガー ---
                float leftTrigger = gp.bLeftTrigger / 255.0f;
                float rightTrigger = gp.bRightTrigger / 255.0f;
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::LeftTriggerAnalog, userId, deviceId, leftTrigger);
                m_messageHandler->OnControllerAnalog(
                    GamepadKeyNames::RightTriggerAnalog, userId, deviceId, rightTrigger);

                // --- デジタルボタン ---
                struct ButtonMapping
                {
                    WORD mask;
                    const NS::TCHAR* name;
                };
                static const ButtonMapping buttons[] = {
                    {XINPUT_GAMEPAD_DPAD_UP, GamepadKeyNames::DPadUp},
                    {XINPUT_GAMEPAD_DPAD_DOWN, GamepadKeyNames::DPadDown},
                    {XINPUT_GAMEPAD_DPAD_LEFT, GamepadKeyNames::DPadLeft},
                    {XINPUT_GAMEPAD_DPAD_RIGHT, GamepadKeyNames::DPadRight},
                    {XINPUT_GAMEPAD_START, GamepadKeyNames::SpecialRight},
                    {XINPUT_GAMEPAD_BACK, GamepadKeyNames::SpecialLeft},
                    {XINPUT_GAMEPAD_LEFT_THUMB, GamepadKeyNames::LeftThumb},
                    {XINPUT_GAMEPAD_RIGHT_THUMB, GamepadKeyNames::RightThumb},
                    {XINPUT_GAMEPAD_LEFT_SHOULDER, GamepadKeyNames::LeftShoulder},
                    {XINPUT_GAMEPAD_RIGHT_SHOULDER, GamepadKeyNames::RightShoulder},
                    {XINPUT_GAMEPAD_A, GamepadKeyNames::FaceButtonBottom},
                    {XINPUT_GAMEPAD_B, GamepadKeyNames::FaceButtonRight},
                    {XINPUT_GAMEPAD_X, GamepadKeyNames::FaceButtonLeft},
                    {XINPUT_GAMEPAD_Y, GamepadKeyNames::FaceButtonTop},
                };

                for (const auto& btn : buttons)
                {
                    bool pressed = (gp.wButtons & btn.mask) != 0;
                    bool wasPrevPressed = (prev.wButtons & btn.mask) != 0;

                    if (pressed && !wasPrevPressed)
                    {
                        m_messageHandler->OnControllerButtonPressed(btn.name, userId, deviceId, false);
                    }
                    else if (!pressed && wasPrevPressed)
                    {
                        m_messageHandler->OnControllerButtonReleased(btn.name, userId, deviceId, false);
                    }
                }

                ctrl.LastState = state;
            }
            else
            {
                if (ctrl.bConnected)
                {
                    ctrl.bConnected = false;
                    ctrl.DisconnectedCooldown = 2.0f; // 2秒クールダウン
                }
            }
        }
    }

} // namespace NS

