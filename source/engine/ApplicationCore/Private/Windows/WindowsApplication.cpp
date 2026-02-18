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
#include <ranges>
#include <utility> // std::swap

#pragma comment(lib, "xinput.lib")

namespace
{
    // XInput デッドゾーン

    // XInput デッドゾーン
    constexpr float kXInputLeftThumbDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0F;
    constexpr float kXInputRightThumbDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0F;
    constexpr float kXInputTriggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0F;

    float ApplyDeadZone(float value, float deadZone)
    {
        if (value > deadZone)
        {
            return (value - deadZone) / (1.0F - deadZone);
        }
        if (value < -deadZone)
        {
            return (value + deadZone) / (1.0F - deadZone);
        }
        return 0.0F;
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

        // AppWndProc をウィンドウクラス登録前に設定
        WindowsWindow::SetWndProcCallback(WindowsApplication::AppWndProc);

        // ウィンドウクラス登録
        WindowsWindow::Initialize(hInstance, hIcon);

        auto app = std::shared_ptr<WindowsApplication>(new WindowsApplication(hInstance, hIcon));
        g_windowsApp = app.get();

        return app;
    }

    WindowsApplication::WindowsApplication(HINSTANCE hInstance, HICON /*hIcon*/)
        : GenericApplication(nullptr) // カーソルは後で設定 (Phase 7)
          ,
          m_hInstance(hInstance)
    {
        // OLE 初期化
        HRESULT const hr = ::OleInitialize(nullptr);
        m_bOleInitialized = SUCCEEDED(hr);
        if (!m_bOleInitialized)
        {
            // RPC_E_CHANGED_MODE: COM already initialized with different threading model
            // OLE D&D is disabled in this case
        }

        // アクセシビリティキーの保存
        m_savedStickyKeys.cbSize = sizeof(STICKYKEYS);
        ::SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &m_savedStickyKeys, 0);

        m_savedToggleKeys.cbSize = sizeof(TOGGLEKEYS);
        ::SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &m_savedToggleKeys, 0);

        m_savedFilterKeys.cbSize = sizeof(FILTERKEYS);
        ::SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &m_savedFilterKeys, 0);

        // ゴースト化防止
        ::DisableProcessWindowsGhosting();

        m_deferredMessages.reserve(64);

        // ITaskbarList3 初期化 (ComPtr RAII)
        HRESULT const hrTb =
            ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_taskbarList));
        if (SUCCEEDED(hrTb) && (m_taskbarList != nullptr))
        {
            m_taskbarList->HrInit();
        }
    }

    WindowsApplication::~WindowsApplication()
    {
        // グローバル WindowsApplication インスタンス（AppWndProc で使用）
        g_windowsApp = nullptr;

        // ウィンドウを逆順で破棄
        for (auto& m_window : std::ranges::reverse_view(m_windows))
        {
            if (m_window)
            {
                m_window->Destroy();
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
        ::UnregisterClass(WindowsWindow::kAppWindowClass, m_hInstance);
    }

    // =========================================================================
    // 06-02: ウィンドウ管理

    std::shared_ptr<GenericWindow> WindowsApplication::MakeWindow()
    {
        auto window = WindowsWindow::MakeWindow();
        m_windows.push_back(window);
        return window;
    }

    void WindowsApplication::InitializeWindow(const std::shared_ptr<GenericWindow>& window,
                                              const GenericWindowDefinition& definition,
                                              const std::shared_ptr<GenericWindow>& parent,
                                              bool bShowImmediately)
    {
        auto winWindow = std::static_pointer_cast<WindowsWindow>(window);
        auto winParent = parent ? std::static_pointer_cast<WindowsWindow>(parent) : nullptr;

        winWindow->Initialize(this, definition, m_hInstance, winParent, bShowImmediately);

        // Initialize 失敗時は管理リストから除去（無効エントリ蓄積防止）
        if (winWindow->GetHWnd() == nullptr)
        {
            m_windows.erase(std::remove(m_windows.begin(), m_windows.end(), winWindow), m_windows.end());
        }
    }

    void WindowsApplication::SetCapture(const std::shared_ptr<GenericWindow>& window)
    {
        if (window)
        {
            auto winWindow = std::static_pointer_cast<WindowsWindow>(window);
            ::SetCapture(winWindow->GetHWnd());
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
        for (const auto& window : m_windows)
        {
            if (window && window->GetHWnd() == hwnd)
            {
                return window;
            }
        }
        return nullptr;
    }

    // =========================================================================
    // 06-02: メッセージハンドラ管理

    void WindowsApplication::AddMessageHandler(IWindowsMessageHandler& handler)
    {
        m_messageHandlers.push_back(&handler);
    }

    void WindowsApplication::RemoveMessageHandler(IWindowsMessageHandler& handler)
    {
        m_messageHandlers.erase(std::remove(m_messageHandlers.begin(), m_messageHandlers.end(), &handler),
                                m_messageHandlers.end());
    }

    // =========================================================================
    // 06-03: メッセージポンプ

    void WindowsApplication::PumpMessages(float /*TimeDelta*/)
    {
        MSG msg = {};
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    void WindowsApplication::Tick(float timeDelta)
    {
        PollGameDeviceState(timeDelta);
    }

    // =========================================================================
    // 06-03: AppWndProc
    // =========================================================================

    LRESULT CALLBACK WindowsApplication::AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (g_windowsApp != nullptr)
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

            int32_t const result = g_windowsApp->ProcessMessage(hwnd, msg, wParam, lParam);
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
        if (m_messageHandler.Get() == nullptr)
        {
            return -1;
        }

        auto window = FindWindowByHWND(hwnd);
        const bool kBShouldProcessUserInput = m_messageHandler->ShouldProcessUserInputMessages(window);
        auto handledResult = [](bool bHandled) -> int32_t { return bHandled ? 0 : -1; };

        switch (msg)
        {
        // =============================================================
        // 06-04: キーボード
        case WM_CHAR:
        case WM_SYSCHAR:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            bool const bIsRepeat = (lParam & (1 << 30)) != 0;
            return handledResult(m_messageHandler->OnKeyChar(static_cast<NS::TCHAR>(wParam), bIsRepeat));
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            auto const keyCode = static_cast<int32_t>(wParam);
            bool const bIsRepeat = (lParam & (1 << 30)) != 0;
            uint32_t const charCode = ::MapVirtualKey(keyCode, MAPVK_VK_TO_CHAR);
            UpdateModifierKeyState(wParam, lParam, true);
            return handledResult(m_messageHandler->OnKeyDown(keyCode, charCode, bIsRepeat));
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            auto const keyCode = static_cast<int32_t>(wParam);
            uint32_t const charCode = ::MapVirtualKey(keyCode, MAPVK_VK_TO_CHAR);
            UpdateModifierKeyState(wParam, lParam, false);
            return handledResult(m_messageHandler->OnKeyUp(keyCode, charCode, false));
        }

        // =============================================================
        // 06-05: マウスボタン
        // =============================================================
        case WM_LBUTTONDOWN:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDown(window,
                                              MouseButtons::Left,
                                              Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                       .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONDOWN:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDown(window,
                                              MouseButtons::Middle,
                                              Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                       .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONDOWN:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDown(window,
                                              MouseButtons::Right,
                                              Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                       .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_XBUTTONDOWN:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            {
                const bool kBHandled =
                    m_messageHandler->OnMouseDown(window,
                                                  XButtonToMouseButton(wParam),
                                                  Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                           .y = static_cast<float>(GET_Y_LPARAM(lParam))});
                return kBHandled ? TRUE : -1;
            }

        case WM_LBUTTONUP:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(m_messageHandler->OnMouseUp(MouseButtons::Left,
                                                             Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                                      .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONUP:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(m_messageHandler->OnMouseUp(MouseButtons::Middle,
                                                             Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                                      .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONUP:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(m_messageHandler->OnMouseUp(MouseButtons::Right,
                                                             Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                                      .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_XBUTTONUP:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            {
                const bool kBHandled =
                    m_messageHandler->OnMouseUp(XButtonToMouseButton(wParam),
                                                Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                         .y = static_cast<float>(GET_Y_LPARAM(lParam))});
                return kBHandled ? TRUE : -1;
            }

        case WM_LBUTTONDBLCLK:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDoubleClick(window,
                                                     MouseButtons::Left,
                                                     Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                              .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_MBUTTONDBLCLK:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDoubleClick(window,
                                                     MouseButtons::Middle,
                                                     Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                              .y = static_cast<float>(GET_Y_LPARAM(lParam))}));
        case WM_RBUTTONDBLCLK:
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            return handledResult(
                m_messageHandler->OnMouseDoubleClick(window,
                                                     MouseButtons::Right,
                                                     Vector2D{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                                              .y = static_cast<float>(GET_Y_LPARAM(lParam))}));

        // =============================================================
        // 06-05: マウス移動・ホイール
        // =============================================================
        case WM_MOUSEMOVE:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            // WM_MOUSEMOVE の lParam はクライアント座標
            Vector2D const cursorPos{.x = static_cast<float>(GET_X_LPARAM(lParam)),
                                     .y = static_cast<float>(GET_Y_LPARAM(lParam))};
            return handledResult(m_messageHandler->OnMouseMove(cursorPos));
        }
        case WM_NCMOUSEMOVE:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            // WM_NCMOUSEMOVE の lParam はスクリーン座標 -> クライアント座標に変換
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ScreenToClient(hwnd, &pt);
            return handledResult(
                m_messageHandler->OnMouseMove(Vector2D{.x = static_cast<float>(pt.x), .y = static_cast<float>(pt.y)}));
        }

        case WM_MOUSEWHEEL:
        {
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            float const delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            // WM_MOUSEWHEEL の lParam はスクリーン座標なので、クライアント座標に変換
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ::ScreenToClient(hwnd, &pt);
            return handledResult(m_messageHandler->OnMouseWheel(
                delta, Vector2D{.x = static_cast<float>(pt.x), .y = static_cast<float>(pt.y)}));
        }

        // =============================================================
        // 06-06: ウィンドウイベント
        case WM_SIZE:
        {
            if (window)
            {
                int32_t const w = LOWORD(lParam);
                int32_t const h = HIWORD(lParam);
                bool const bWasMinimized = (wParam == SIZE_MINIMIZED);
                m_messageHandler->OnSizeChanged(window, w, h, bWasMinimized);

                if (wParam == SIZE_MAXIMIZED)
                {
                    m_messageHandler->OnWindowAction(window, WindowAction::Maximize);
                }
                else if (wParam == SIZE_RESTORED)
                {
                    m_messageHandler->OnWindowAction(window, WindowAction::Restore);
                }
            }
            return 0;
        }
        case WM_MOVE:
        {
            if (window)
            {
                int32_t const x = static_cast<int16_t>(LOWORD(lParam));
                int32_t const y = static_cast<int16_t>(HIWORD(lParam));
                m_messageHandler->OnMovedWindow(window, x, y);
            }
            return 0;
        }
        case WM_ACTIVATE:
        {
            if (window)
            {
                WindowActivation::Type activation =
                    (LOWORD(wParam) != WA_INACTIVE) ? WindowActivation::Activate : WindowActivation::Deactivate;
                if (LOWORD(wParam) == WA_CLICKACTIVE)
                {
                    activation = WindowActivation::ActivateByMouse;
                }
                m_messageHandler->OnWindowActivationChanged(window, activation);
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
            if (window)
            {
                m_messageHandler->OnWindowClose(window);
            }
            return 0;
        }
        case WM_PAINT:
        {
            if (window)
            {
                m_messageHandler->OnOSPaint(window);
            }
            break; // DefWindowProc handles BeginPaint/EndPaint
        }

        // =============================================================
        // 06-04/06-06: 非クライアント領域
        // =============================================================
        case WM_NCHITTEST:
        {
            if (window && !window->GetDefinition().hasOsWindowBorder)
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ::ScreenToClient(hwnd, &pt);

                WindowZone::Type const zone = m_messageHandler->GetWindowZoneForPoint(window, pt.x, pt.y);
                return WindowsWindow::WindowZoneToHitTest(zone);
            }
            break;
        }
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        {
            if (window)
            {
                m_messageHandler->OnWindowAction(window, WindowAction::ClickedNonClientArea);
            }
            break;
        }

        case WM_GETMINMAXINFO:
        {
            if (window)
            {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                WindowSizeLimits const limits = m_messageHandler->GetSizeLimitsForWindow(window);
                float const dpi = window->GetDPIScaleFactor();

                if (auto minW = limits.GetMinWidth())
                {
                    mmi->ptMinTrackSize.x = static_cast<LONG>(*minW * dpi);
                }
                if (auto minH = limits.GetMinHeight())
                {
                    mmi->ptMinTrackSize.y = static_cast<LONG>(*minH * dpi);
                }
                if (auto maxW = limits.GetMaxWidth())
                {
                    mmi->ptMaxTrackSize.x = static_cast<LONG>(*maxW * dpi);
                }
                if (auto maxH = limits.GetMaxHeight())
                {
                    mmi->ptMaxTrackSize.y = static_cast<LONG>(*maxH * dpi);
                }
            }
            return 0;
        }

        case WM_NCCALCSIZE:
        {
            if (wParam == TRUE && window && !window->GetDefinition().hasOsWindowBorder)
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
            if (window)
            {
                float const newDPI = static_cast<float>(LOWORD(wParam)) / 96.0F;

                m_messageHandler->SignalSystemDPIChanged(window);

                if (!window->IsManualManageDPIChanges())
                {
                    window->SetDPIScaleFactor(newDPI);

                    RECT* prc = reinterpret_cast<RECT*>(lParam);
                    ::SetWindowPos(hwnd,
                                   nullptr,
                                   prc->left,
                                   prc->top,
                                   prc->right - prc->left,
                                   prc->bottom - prc->top,
                                   SWP_NOZORDER | SWP_NOACTIVATE);

                    m_messageHandler->HandleDPIScaleChanged(window);
                }
                else
                {
                    window->SetDPIScaleFactor(newDPI);
                }
            }
            return 0;
        }

        case WM_ENTERSIZEMOVE:
        {
            if (window)
            {
                m_messageHandler->BeginReshapingWindow(window);
            }
            return 0;
        }
        case WM_EXITSIZEMOVE:
        {
            if (window)
            {
                m_messageHandler->FinishedReshapingWindow(window);
            }
            return 0;
        }
        case WM_SIZING:
        {
            if (window)
            {
                m_messageHandler->OnResizingWindow(window);
            }
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
            if (window && !window->GetDefinition().hasOsWindowBorder)
            {
                m_messageHandler->OnCursorSet();
                return 0;
            }
            break;
        }

        case WM_SYSCOMMAND:
        {
            uint32_t const cmd = static_cast<uint32_t>(wParam) & 0xFFF0;
            if (cmd == SC_RESTORE && window)
            {
                m_messageHandler->OnWindowAction(window, WindowAction::Restore);
            }
            else if (cmd == SC_MAXIMIZE && window)
            {
                m_messageHandler->OnWindowAction(window, WindowAction::Maximize);
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
                    auto* raw = reinterpret_cast<RAWINPUT*>(buffer);
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
        case WM_CLIPBOARDUPDATE:
        {
            OnClipboardContentChanged().Broadcast();
            return 0;
        }
        case WM_CAPTURECHANGED: // マウスキャプチャが失われた
        case WM_SETTINGCHANGE:  // システム設定変更（DPI等の再取得）
            return 0;

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
            if (!kBShouldProcessUserInput)
            {
                return -1;
            }
            auto touchHandle = reinterpret_cast<HTOUCHINPUT>(lParam);
            UINT const touchCount = LOWORD(wParam);
            if (touchCount == 0 || touchCount > 256)
            {
                ::CloseTouchInputHandle(touchHandle);
                return 0;
            }

            std::vector<TOUCHINPUT> touches(touchCount);
            if (::GetTouchInputInfo(touchHandle, touchCount, touches.data(), sizeof(TOUCHINPUT)) != 0)
            {
                PlatformUserId const userId(0);
                InputDeviceId const deviceId(0);

                for (UINT i = 0; i < touchCount; ++i)
                {
                    const TOUCHINPUT& ti = touches[i];
                    // TOUCHINPUT uses hundredths of a pixel (CENTIPOINT)
                    POINT pt = {ti.x / 100, ti.y / 100};
                    ::ScreenToClient(hwnd, &pt);
                    Vector2D const location{.x = static_cast<float>(pt.x), .y = static_cast<float>(pt.y)};
                    float const force = 1.0F;
                    auto const touchIndex = static_cast<int32_t>(ti.dwID);

                    if ((ti.dwFlags & TOUCHEVENTF_DOWN) != 0U)
                    {
                        m_messageHandler->OnTouchStarted(window, location, force, touchIndex, userId, deviceId);
                    }
                    else if ((ti.dwFlags & TOUCHEVENTF_MOVE) != 0U)
                    {
                        m_messageHandler->OnTouchMoved(location, force, touchIndex, userId, deviceId);
                    }
                    else if ((ti.dwFlags & TOUCHEVENTF_UP) != 0U)
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

    void WindowsApplication::DeferMessage(const std::shared_ptr<WindowsWindow>& window,
                                          HWND hWnd,
                                          uint32_t msg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          int32_t x,
                                          int32_t y,
                                          uint32_t rawInputFlags)
    {
        DeferredWindowsMessage deferred;
        deferred.nativeWindow = window;
        deferred.hWnd = hWnd;
        deferred.msg = msg;
        deferred.wParam = wParam;
        deferred.lParam = lParam;
        deferred.x = x;
        deferred.y = y;
        deferred.rawInputFlags = rawInputFlags;
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

    void WindowsApplication::ProcessDeferredMessage(const DeferredWindowsMessage& deferMsg)
    {
        auto window = deferMsg.nativeWindow.lock();
        if (!window)
        {
            return; // ウィンドウ破棄済み
        }

        // 遅延メッセージのルーティング（現在は即時処理と同じ）
        ProcessMessage(deferMsg.hWnd, deferMsg.msg, deferMsg.wParam, deferMsg.lParam);
    }

    // =========================================================================
    // 06-04: 修飾キー状態

    void WindowsApplication::UpdateModifierKeyState(WPARAM wParam, LPARAM lParam, bool bKeyDown)
    {
        bool const bIsExtended = (lParam & (1 << 24)) != 0;

        switch (wParam)
        {
        case VK_SHIFT:
        {
            UINT const scanCode = static_cast<UINT>((lParam >> 16) & 0xFF);
            UINT const vk = ::MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
            if (vk == VK_LSHIFT)
            {
                m_modifierKeyState[LeftShift] = bKeyDown;
            }
            else if (vk == VK_RSHIFT)
            {
                m_modifierKeyState[RightShift] = bKeyDown;
            }
            break;
        }
        case VK_CONTROL:
            if (bIsExtended)
            {
                m_modifierKeyState[RightControl] = bKeyDown;
            }
            else
            {
                m_modifierKeyState[LeftControl] = bKeyDown;
            }
            break;
        case VK_MENU:
            if (bIsExtended)
            {
                m_modifierKeyState[RightAlt] = bKeyDown;
            }
            else
            {
                m_modifierKeyState[LeftAlt] = bKeyDown;
            }
            break;
        case VK_LWIN:
            m_modifierKeyState[LeftCommand] = bKeyDown;
            break;
        case VK_RWIN:
            m_modifierKeyState[RightCommand] = bKeyDown;
            break;
        case VK_CAPITAL:
            m_modifierKeyState[CapsLock] = !(::GetKeyState(VK_CAPITAL) == 0);
            break;
        default:
            break;
        }
    }

    ModifierKeysState WindowsApplication::GetModifierKeys() const
    {
        return {m_modifierKeyState[LeftShift] || m_modifierKeyState[RightShift],
                m_modifierKeyState[LeftControl] || m_modifierKeyState[RightControl],
                m_modifierKeyState[LeftAlt] || m_modifierKeyState[RightAlt],
                m_modifierKeyState[LeftCommand] || m_modifierKeyState[RightCommand],
                m_modifierKeyState[LeftShift],
                m_modifierKeyState[RightShift],
                m_modifierKeyState[LeftAlt],
                m_modifierKeyState[RightAlt],
                m_modifierKeyState[CapsLock]};
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

        if (::RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)) != 0)
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
        {
            return false;
        }
        for (const auto& state : m_xinputStates)
        {
            if (state.bConnected)
            {
                return true;
            }
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
        return {.left = workArea.left, .top = workArea.top, .right = workArea.right, .bottom = workArea.bottom};
    }

    WindowTransparency WindowsApplication::GetWindowTransparencySupport() const
    {
        return WindowTransparency::PerPixel;
    }

    // =========================================================================
    // 06-07: XInput ゲームパッドポーリング
    // =========================================================================

    void WindowsApplication::PollGameDeviceState(float timeDelta)
    {
        if (!m_bGamepadEnabled)
        {
            return;
        }

        m_lastTimeDelta = timeDelta;
        PollXInput();
    }

    // =========================================================================
    // 13-01: Raw Input マウス処理

    void WindowsApplication::ProcessRawMouseInput(const RAWMOUSE& mouseData)
    {
        if ((mouseData.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
            // RDP/リモートデスクトップ 絶対座標 → 相対デルタ変換
            int32_t const screenW = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int32_t const screenH = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
            int32_t const screenX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
            int32_t const screenY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

            auto const absX =
                static_cast<int32_t>((static_cast<float>(mouseData.lLastX) / 65535.0F) * static_cast<float>(screenW) +
                                     static_cast<float>(screenX));
            auto const absY =
                static_cast<int32_t>((static_cast<float>(mouseData.lLastY) / 65535.0F) * static_cast<float>(screenH) +
                                     static_cast<float>(screenY));

            int32_t const deltaX = absX - m_lastRawMouseX;
            int32_t const deltaY = absY - m_lastRawMouseY;
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
            m_messageHandler->OnRawMouseMove(mouseData.lLastX, mouseData.lLastY);
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

    void WindowsApplication::SetForceFeedbackChannelValue(int32_t controllerId, int32_t channelType, float value)
    {
        if (controllerId < 0 || controllerId >= kMaxXInputControllers)
        {
            return;
        }
        if (!m_xinputStates[controllerId].bConnected)
        {
            return;
        }

        float const clamped = std::clamp(value, 0.0F, 1.0F);
        WORD const intensity = static_cast<WORD>(clamped * 65535.0F);

        // 既存の振動状態を保持しつつ該当チャンネルのみ更新
        auto& ctrl = m_xinputStates[controllerId];
        XINPUT_VIBRATION vib = ctrl.lastVibration;

        // ChannelType: 0=LeftLarge, 1=LeftSmall, 2=RightLarge, 3=RightSmall
        if (channelType == 0 || channelType == 2)
        {
            vib.wLeftMotorSpeed = intensity;
        }
        else
        {
            vib.wRightMotorSpeed = intensity;
        }

        ctrl.lastVibration = vib;
        ::XInputSetState(static_cast<DWORD>(controllerId), &vib);
    }

    void WindowsApplication::SetForceFeedbackChannelValues(int32_t controllerId, const ForceFeedbackValues& values)
    {
        if (controllerId < 0 || controllerId >= kMaxXInputControllers)
        {
            return;
        }
        if (!m_xinputStates[controllerId].bConnected)
        {
            return;
        }

        // 左モーター = max(LeftLarge, RightLarge), 右モーター = max(LeftSmall, RightSmall)
        float const leftMotor = std::clamp(std::max(values.leftLarge, values.rightLarge), 0.0F, 1.0F);
        float const rightMotor = std::clamp(std::max(values.leftSmall, values.rightSmall), 0.0F, 1.0F);

        XINPUT_VIBRATION vib = {};
        vib.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0F);
        vib.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0F);

        m_xinputStates[controllerId].lastVibration = vib;
        ::XInputSetState(static_cast<DWORD>(controllerId), &vib);
    }

    // =========================================================================
    // 06-07: XInput ゲームパッドポーリング
    // =========================================================================

    void WindowsApplication::PollXInput()
    {
        for (int32_t i = 0; i < kMaxXInputControllers; ++i)
        {
            auto& ctrl = m_xinputStates[i];

            // 切断クールダウン
            if (!ctrl.bConnected)
            {
                ctrl.disconnectedCooldown -= m_lastTimeDelta;
                if (ctrl.disconnectedCooldown > 0.0F)
                {
                    continue;
                }
            }

            XINPUT_STATE state = {};
            DWORD const result = ::XInputGetState(static_cast<DWORD>(i), &state);

            if (result == ERROR_SUCCESS)
            {
                if (!ctrl.bConnected)
                {
                    ctrl.bConnected = true;
                    ctrl.disconnectedCooldown = 0.0F;
                }

                PlatformUserId const userId(i);
                InputDeviceId const deviceId(i);
                const XINPUT_GAMEPAD& gp = state.Gamepad;
                const XINPUT_GAMEPAD& prev = ctrl.lastState.Gamepad;

                // XInput デッドゾーン
                float const leftX = ApplyDeadZone(static_cast<float>(gp.sThumbLX) / 32767.0F, kXInputLeftThumbDeadZone);
                float const leftY = ApplyDeadZone(static_cast<float>(gp.sThumbLY) / 32767.0F, kXInputLeftThumbDeadZone);
                float const rightX =
                    ApplyDeadZone(static_cast<float>(gp.sThumbRX) / 32767.0F, kXInputRightThumbDeadZone);
                float const rightY =
                    ApplyDeadZone(static_cast<float>(gp.sThumbRY) / 32767.0F, kXInputRightThumbDeadZone);

                m_messageHandler->OnControllerAnalog(GamepadKeyNames::g_leftAnalogX, userId, deviceId, leftX);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::g_leftAnalogY, userId, deviceId, leftY);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::g_rightAnalogX, userId, deviceId, rightX);
                m_messageHandler->OnControllerAnalog(GamepadKeyNames::g_rightAnalogY, userId, deviceId, rightY);

                // --- トリガー ---
                float const leftTrigger = static_cast<float>(gp.bLeftTrigger) / 255.0F;
                float const rightTrigger = static_cast<float>(gp.bRightTrigger) / 255.0F;
                m_messageHandler->OnControllerAnalog(
                    GamepadKeyNames::g_leftTriggerAnalog, userId, deviceId, leftTrigger);
                m_messageHandler->OnControllerAnalog(
                    GamepadKeyNames::g_rightTriggerAnalog, userId, deviceId, rightTrigger);

                // --- デジタルボタン ---
                struct ButtonMapping
                {
                    WORD mask;
                    const NS::TCHAR* name;
                };
                static const ButtonMapping kButtons[] = {
                    {.mask = XINPUT_GAMEPAD_DPAD_UP, .name = GamepadKeyNames::g_dPadUp},
                    {.mask = XINPUT_GAMEPAD_DPAD_DOWN, .name = GamepadKeyNames::g_dPadDown},
                    {.mask = XINPUT_GAMEPAD_DPAD_LEFT, .name = GamepadKeyNames::g_dPadLeft},
                    {.mask = XINPUT_GAMEPAD_DPAD_RIGHT, .name = GamepadKeyNames::g_dPadRight},
                    {.mask = XINPUT_GAMEPAD_START, .name = GamepadKeyNames::g_specialRight},
                    {.mask = XINPUT_GAMEPAD_BACK, .name = GamepadKeyNames::g_specialLeft},
                    {.mask = XINPUT_GAMEPAD_LEFT_THUMB, .name = GamepadKeyNames::g_leftThumb},
                    {.mask = XINPUT_GAMEPAD_RIGHT_THUMB, .name = GamepadKeyNames::g_rightThumb},
                    {.mask = XINPUT_GAMEPAD_LEFT_SHOULDER, .name = GamepadKeyNames::g_leftShoulder},
                    {.mask = XINPUT_GAMEPAD_RIGHT_SHOULDER, .name = GamepadKeyNames::g_rightShoulder},
                    {.mask = XINPUT_GAMEPAD_A, .name = GamepadKeyNames::g_faceButtonBottom},
                    {.mask = XINPUT_GAMEPAD_B, .name = GamepadKeyNames::g_faceButtonRight},
                    {.mask = XINPUT_GAMEPAD_X, .name = GamepadKeyNames::g_faceButtonLeft},
                    {.mask = XINPUT_GAMEPAD_Y, .name = GamepadKeyNames::g_faceButtonTop},
                };

                for (const auto& btn : kButtons)
                {
                    bool const pressed = (gp.wButtons & btn.mask) != 0;
                    bool const wasPrevPressed = (prev.wButtons & btn.mask) != 0;

                    if (pressed && !wasPrevPressed)
                    {
                        m_messageHandler->OnControllerButtonPressed(btn.name, userId, deviceId, false);
                    }
                    else if (!pressed && wasPrevPressed)
                    {
                        m_messageHandler->OnControllerButtonReleased(btn.name, userId, deviceId, false);
                    }
                }

                ctrl.lastState = state;
            }
            else
            {
                if (ctrl.bConnected)
                {
                    ctrl.bConnected = false;
                    ctrl.disconnectedCooldown = 2.0F; // 2秒クールダウン
                }
            }
        }
    }

} // namespace NS
