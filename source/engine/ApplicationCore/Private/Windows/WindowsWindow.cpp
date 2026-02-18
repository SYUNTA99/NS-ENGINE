/// @file WindowsWindow.cpp
/// @brief WindowsWindow 実装（03-02 ~ 03-05）
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dwmapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <windowsx.h>
#endif

#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef CreateDirectory
#undef IsMaximized
#undef IsMinimized

#include "GenericPlatform/GenericApplication.h"
#include "HAL/PlatformMacros.h"
#include "Windows/WindowsWindow.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shcore.lib")

namespace NS
{
    // =========================================================================
    // Static members
    // =========================================================================

    const NS::TCHAR WindowsWindow::kAppWindowClass[] = TEXT("NSEngineWindow");

    static WNDPROC g_sAppWndProc = ::DefWindowProcW;

    void WindowsWindow::SetWndProcCallback(WNDPROC proc)
    {
        g_sAppWndProc = (proc != nullptr) ? proc : ::DefWindowProcW;
    }

    // =========================================================================
    // 03-02: ウィンドウクラス登録
    // =========================================================================

    void WindowsWindow::Initialize(HINSTANCE hInstance, HICON hIcon)
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = g_sAppWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = hIcon;
        wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = kAppWindowClass;
        wc.hIconSm = hIcon;

        if (::RegisterClassEx(&wc) == 0U)
        {
            // すでに登録済みの場合は無視（2回目の呼び出し対応）
            if (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                wchar_t buf[128];
                _snwprintf_s(buf, _TRUNCATE, L"NSEngine: RegisterClassEx failed: %lu\n", ::GetLastError());
                ::OutputDebugStringW(buf);
            }
        }
    }

    // =========================================================================
    // ファクトリ・コンストラクタ・デストラクタ
    // =========================================================================

    std::shared_ptr<WindowsWindow> WindowsWindow::MakeWindow()
    {
        return std::make_shared<WindowsWindow>();
    }

    WindowsWindow::WindowsWindow()
    {
        m_preFullscreenWindowPlacement.length = sizeof(WINDOWPLACEMENT);
    }

    WindowsWindow::~WindowsWindow()
    {
        Destroy();
    }

    // =========================================================================
    // 03-02: インスタンス初期化
    // =========================================================================

    void WindowsWindow::Initialize(GenericApplication* const kApplication,
                                   const GenericWindowDefinition& inDefinition,
                                   HINSTANCE hInstance,
                                   const std::shared_ptr<WindowsWindow>& inParent,
                                   bool bShowImmediately)
    {
        m_owningApplication = kApplication;
        m_definition = inDefinition;

        // --- ウィンドウスタイル設定 ---
        DWORD style = 0;
        DWORD exStyle = 0;

        if (inDefinition.hasOsWindowBorder)
        {
            style = WS_OVERLAPPED;

            if (inDefinition.isRegularWindow)
            {
                if (inDefinition.hasCloseButton)
                {
                    style |= WS_SYSMENU;
                }
                if (inDefinition.supportsMinimize)
                {
                    style |= WS_MINIMIZEBOX;
                }
                if (inDefinition.supportsMaximize)
                {
                    style |= WS_MAXIMIZEBOX;
                }
                if (inDefinition.hasSizingFrame)
                {
                    style |= WS_THICKFRAME;
                }
                else
                {
                    style |= WS_BORDER;
                }

                style |= WS_CAPTION;
            }
            else
            {
                style |= WS_POPUP | WS_BORDER;
            }
        }
        else
        {
            style = WS_POPUP;

            if (inDefinition.transparencySupport == WindowTransparency::PerPixel)
            {
                exStyle |= WS_EX_COMPOSITED;
            }
        }

        if (!inDefinition.acceptsInput)
        {
            exStyle |= WS_EX_TRANSPARENT;
        }

        if (inDefinition.isTopmostWindow)
        {
            exStyle |= WS_EX_TOPMOST;
        }

        if (!inDefinition.appearsInTaskbar)
        {
            exStyle |= WS_EX_TOOLWINDOW;
        }

        // --- クライアント領域サイズ → ウィンドウ全体サイズ ---
        auto const clientW = static_cast<int32_t>(inDefinition.widthDesiredOnScreen);
        auto const clientH = static_cast<int32_t>(inDefinition.heightDesiredOnScreen);
        auto windowX = static_cast<int32_t>(inDefinition.xDesiredPositionOnScreen);
        auto windowY = static_cast<int32_t>(inDefinition.yDesiredPositionOnScreen);

        RECT borderRect = {0, 0, clientW, clientH};
        ::AdjustWindowRectEx(&borderRect, style, FALSE, exStyle);

        int32_t const windowW = borderRect.right - borderRect.left;
        int32_t const windowH = borderRect.bottom - borderRect.top;

        if (windowX < 0)
        {
            windowX = CW_USEDEFAULT;
        }
        if (windowY < 0)
        {
            windowY = CW_USEDEFAULT;
        }

        HWND parentHWnd = inParent ? inParent->GetHWnd() : nullptr;

        // --- CreateWindowEx ---
        m_hwnd = ::CreateWindowEx(exStyle,
                                  kAppWindowClass,
                                  inDefinition.title.c_str(),
                                  style,
                                  windowX,
                                  windowY,
                                  windowW,
                                  windowH,
                                  parentHWnd,
                                  nullptr,
                                  hInstance,
                                  nullptr);

        if (m_hwnd == nullptr)
        {
            wchar_t buf[128];
            _snwprintf_s(buf, _TRUNCATE, L"NSEngine: CreateWindowEx failed: %lu\n", ::GetLastError());
            ::OutputDebugStringW(buf);
            return;
        }

        // this ポインタを GWLP_USERDATA に格納
        ::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // --- DWM 讒区・ ---
        if (!inDefinition.hasOsWindowBorder)
        {
            DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
            ::DwmSetWindowAttribute(m_hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        }

        if (inDefinition.transparencySupport == WindowTransparency::PerPixel)
        {
            MARGINS const margins = {-1, -1, -1, -1};
            ::DwmExtendFrameIntoClientArea(m_hwnd, &margins);

            // 角丸
            if (inDefinition.cornerRadius > 0.0F)
            {
                auto const cr = static_cast<int32_t>(inDefinition.cornerRadius);
                HRGN rgn = ::CreateRoundRectRgn(0, 0, clientW + 1, clientH + 1, cr, cr);
                if (::SetWindowRgn(m_hwnd, rgn, FALSE) == 0)
                {
                    ::DeleteObject(rgn);
                }
            }
        }
        else if (inDefinition.transparencySupport == WindowTransparency::PerWindow)
        {
            // PerWindow 透過：SetLayeredWindowAttributes は SetOpacity で処理
            LONG_PTR const curExStyle = ::GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
            ::SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, curExStyle | WS_EX_LAYERED);
            ::SetLayeredWindowAttributes(
                m_hwnd, 0, static_cast<BYTE>(std::clamp(inDefinition.opacity, 0.0F, 1.0F) * 255.0F), LWA_ALPHA);
        }

        // --- タッチ入力登録 ---
        ::RegisterTouchWindow(m_hwnd, 0);

        // --- OLE ドラッグ＆ドロップ登録 ---
        ::RegisterDragDrop(m_hwnd, static_cast<IDropTarget*>(this));

        // --- クリップボード監視 ---
        ::AddClipboardFormatListener(m_hwnd);

        // --- 初期表示 ---
        if (bShowImmediately)
        {
            Show();
        }
    }

    // =========================================================================
    // 03-02: Destroy
    // =========================================================================

    void WindowsWindow::Destroy()
    {
        if (m_hwnd != nullptr)
        {
            ::RevokeDragDrop(m_hwnd);
            ::RemoveClipboardFormatListener(m_hwnd);
            ::DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_waitableTimer != nullptr)
        {
            ::CloseHandle(m_waitableTimer);
            m_waitableTimer = nullptr;
        }

        m_bIsVisible = false;
    }

    // =========================================================================
    // 03-03: Show / Hide
    // =========================================================================

    void WindowsWindow::Show()
    {
        if (m_bIsFirstTimeVisible)
        {
            m_bIsFirstTimeVisible = false;

            if (m_definition.focusWhenFirstShown && m_definition.activationPolicy != WindowActivationPolicy::Never)
            {
                ::ShowWindow(m_hwnd, SW_SHOW);
            }
            else
            {
                ::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
            }
        }
        else
        {
            ::ShowWindow(m_hwnd, SW_SHOW);
        }

        m_bIsVisible = true;
    }

    void WindowsWindow::Hide()
    {
        ::ShowWindow(m_hwnd, SW_HIDE);
        m_bIsVisible = false;
    }

    // =========================================================================
    // 03-03: Minimize / Maximize / Restore
    // =========================================================================

    void WindowsWindow::Minimize()
    {
        ::ShowWindow(m_hwnd, SW_MINIMIZE);
    }

    void WindowsWindow::Maximize()
    {
        ::ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    void WindowsWindow::Restore()
    {
        ::ShowWindow(m_hwnd, SW_RESTORE);
    }

    bool WindowsWindow::IsMaximized() const
    {
        return !(::IsZoomed(m_hwnd) == 0);
    }

    bool WindowsWindow::IsMinimized() const
    {
        return !(::IsIconic(m_hwnd) == 0);
    }

    bool WindowsWindow::IsVisible() const
    {
        return m_bIsVisible;
    }

    // =========================================================================
    // 03-03: SetWindowMode
    // =========================================================================

    void WindowsWindow::SetWindowMode(WindowMode::Type newWindowMode)
    {
        if (newWindowMode == m_windowMode)
        {
            return;
        }

        WindowMode::Type const previousMode = m_windowMode;
        m_windowMode = newWindowMode;

        // Windowed → Fullscreen/WindowedFullscreen: 座標を保存
        if (previousMode == WindowMode::Windowed)
        {
            ::GetWindowPlacement(m_hwnd, &m_preFullscreenWindowPlacement);
        }

        HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(MONITORINFO);
        ::GetMonitorInfo(hMonitor, &mi);

        switch (newWindowMode)
        {
        case WindowMode::Fullscreen:
        case WindowMode::WindowedFullscreen:
        {
            ::SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            HWND insertAfter = (newWindowMode == WindowMode::WindowedFullscreen) ? HWND_TOPMOST : HWND_TOP;
            RECT const& rc = mi.rcMonitor;
            ::SetWindowPos(m_hwnd,
                           insertAfter,
                           rc.left,
                           rc.top,
                           rc.right - rc.left,
                           rc.bottom - rc.top,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE);
            break;
        }
        case WindowMode::Windowed:
        {
            ::SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            ::SetWindowPlacement(m_hwnd, &m_preFullscreenWindowPlacement);
            ::SetWindowPos(
                m_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
            break;
        }
        default:
            break;
        }
    }

    WindowMode::Type WindowsWindow::GetWindowMode() const
    {
        return m_windowMode;
    }

    // =========================================================================
    // 03-03: BringToFront / Focus / Enable
    // =========================================================================

    void WindowsWindow::BringToFront(bool bForce)
    {
        if (bForce)
        {
            ::SetForegroundWindow(m_hwnd);
        }
        else
        {
            ::SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    void WindowsWindow::HackForceToFront()
    {
        ::AllowSetForegroundWindow(::GetCurrentProcessId());
        ::SetForegroundWindow(m_hwnd);
    }

    void WindowsWindow::SetWindowFocus()
    {
        ::SetFocus(m_hwnd);
    }

    void WindowsWindow::Enable(bool bEnable)
    {
        ::EnableWindow(m_hwnd, bEnable ? TRUE : FALSE);
    }

    bool WindowsWindow::IsEnabled() const
    {
        return !(::IsWindowEnabled(m_hwnd) == 0);
    }

    // =========================================================================
    // 03-03: Opacity / Text / DrawAttention
    // =========================================================================

    void WindowsWindow::SetOpacity(float inOpacity)
    {
        LONG_PTR const exStyle = ::GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);

        if (inOpacity < 1.0F)
        {
            ::SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
            ::SetLayeredWindowAttributes(
                m_hwnd, 0, static_cast<BYTE>(std::clamp(inOpacity, 0.0F, 1.0F) * 255.0F), LWA_ALPHA);
        }
        else
        {
            // 完全不透明 → レイヤードスタイル除去
            ::SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        }
    }

    void WindowsWindow::SetText(const NS::TCHAR* inText)
    {
        ::SetWindowText(m_hwnd, inText);
    }

    void WindowsWindow::DrawAttention(const WindowDrawAttentionParameters& params)
    {
        FLASHWINFO fi = {};
        fi.cbSize = sizeof(FLASHWINFO);
        fi.hwnd = m_hwnd;

        if (params.requestType == WindowDrawAttentionRequestType::UntilActivated)
        {
            fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        }
        else
        {
            fi.dwFlags = FLASHW_STOP;
        }

        ::FlashWindowEx(&fi);
    }

    // =========================================================================
    // 03-03: ジオメトリ
    // =========================================================================

    void WindowsWindow::ReshapeWindow(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        AdjustWindowRegion(width, height);
        ::SetWindowPos(m_hwnd, nullptr, x, y, m_virtualWidth, m_virtualHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void WindowsWindow::MoveWindowTo(int32_t x, int32_t y)
    {
        ::SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    bool WindowsWindow::GetFullScreenInfo(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const
    {
        HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(MONITORINFO);
        if (::GetMonitorInfo(hMonitor, &mi) != 0)
        {
            x = mi.rcMonitor.left;
            y = mi.rcMonitor.top;
            width = mi.rcMonitor.right - mi.rcMonitor.left;
            height = mi.rcMonitor.bottom - mi.rcMonitor.top;
            return true;
        }
        return false;
    }

    bool WindowsWindow::GetRestoredDimensions(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const
    {
        WINDOWPLACEMENT wp = {};
        wp.length = sizeof(WINDOWPLACEMENT);
        if (::GetWindowPlacement(m_hwnd, &wp) != 0)
        {
            x = wp.rcNormalPosition.left;
            y = wp.rcNormalPosition.top;
            width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
            height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
            return true;
        }
        return false;
    }

    void WindowsWindow::AdjustCachedSize(PlatformRect& size) const
    {
        // 仮想サイズ最適化
        // キャッシュサイズを仮想サイズに矯正
        if (m_definition.sizeWillChangeOften && m_virtualWidth > 0 && m_virtualHeight > 0)
        {
            size.right = size.left + m_virtualWidth;
            size.bottom = size.top + m_virtualHeight;
        }
    }

    bool WindowsWindow::IsForegroundWindow() const
    {
        return ::GetForegroundWindow() == m_hwnd;
    }

    bool WindowsWindow::IsPointInWindow(int32_t x, int32_t y) const
    {
        RECT rc = {};
        ::GetClientRect(m_hwnd, &rc);
        POINT const pt = {x, y};
        return !(::PtInRect(&rc, pt) == 0);
    }

    // =========================================================================
    // 03-03: 親ウィンドウ状態
    // =========================================================================

    void WindowsWindow::OnParentWindowMinimized()
    {
        ::GetWindowPlacement(m_hwnd, &m_preFullscreenWindowPlacement);
    }

    void WindowsWindow::OnParentWindowRestored()
    {
        ::SetWindowPlacement(m_hwnd, &m_preFullscreenWindowPlacement);
    }

    void WindowsWindow::OnTransparencySupportChanged(WindowTransparency newTransparency)
    {
        m_definition.transparencySupport = newTransparency;

        LONG_PTR exStyle = ::GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);

        if (newTransparency == WindowTransparency::PerPixel)
        {
            exStyle |= WS_EX_COMPOSITED;
            ::SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);

            MARGINS const margins = {-1, -1, -1, -1};
            ::DwmExtendFrameIntoClientArea(m_hwnd, &margins);
        }
        else
        {
            exStyle &= ~WS_EX_COMPOSITED;
            ::SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);
        }
    }

    // =========================================================================
    // 03-03: ウィンドウ領域調整 (仮想サイズ最適化)
    // =========================================================================

    void WindowsWindow::AdjustWindowRegion(int32_t width, int32_t height)
    {
        if (!m_definition.sizeWillChangeOften)
        {
            m_virtualWidth = width;
            m_virtualHeight = height;
            return;
        }

        // VirtualSize = Max(NewSize, Min(OldSize, ExpectedMaxSize))
        int32_t const expectedMaxW = m_definition.expectedMaxWidth;
        int32_t const expectedMaxH = m_definition.expectedMaxHeight;

        int32_t const minRetainedW = (expectedMaxW >= 0) ? (std::min)(m_virtualWidth, expectedMaxW) : m_virtualWidth;
        int32_t const minRetainedH = (expectedMaxH >= 0) ? (std::min)(m_virtualHeight, expectedMaxH) : m_virtualHeight;

        m_virtualWidth = (std::max)(width, minRetainedW);
        m_virtualHeight = (std::max)(height, minRetainedH);

        // クリッピング領域設定
        HRGN rgn = ::CreateRectRgn(0, 0, width, height);
        if (::SetWindowRgn(m_hwnd, rgn, FALSE) == 0)
        {
            ::DeleteObject(rgn);
        }
    }

    // =========================================================================
    // 03-04: WindowZone → Win32 ヒットテストコード
    // =========================================================================

    int32_t WindowsWindow::WindowZoneToHitTest(WindowZone::Type zone)
    {
        switch (zone)
        {
        case WindowZone::TitleBar:
            return HTCAPTION;
        case WindowZone::TopLeftBorder:
            return HTTOPLEFT;
        case WindowZone::TopBorder:
            return HTTOP;
        case WindowZone::TopRightBorder:
            return HTTOPRIGHT;
        case WindowZone::LeftBorder:
            return HTLEFT;
        case WindowZone::RightBorder:
            return HTRIGHT;
        case WindowZone::BottomLeftBorder:
            return HTBOTTOMLEFT;
        case WindowZone::BottomBorder:
            return HTBOTTOM;
        case WindowZone::BottomRightBorder:
            return HTBOTTOMRIGHT;
        case WindowZone::MinimizeButton:
            return HTMINBUTTON;
        case WindowZone::MaximizeButton:
            return HTMAXBUTTON;
        case WindowZone::CloseButton:
            return HTCLOSE;
        case WindowZone::SysMenu:
            return HTSYSMENU;
        case WindowZone::ClientArea:
            return HTCLIENT;
        case WindowZone::NotInWindow:
            return HTNOWHERE;
        default:
            return HTCLIENT;
        }
    }

    // =========================================================================
    // 03-05: DPI
    // =========================================================================

    float WindowsWindow::GetDPIScaleFactor() const
    {
        return m_dpiScaleFactor;
    }

    void WindowsWindow::SetDPIScaleFactor(float value)
    {
        m_dpiScaleFactor = value;
    }

    bool WindowsWindow::IsManualManageDPIChanges() const
    {
        return m_definition.bManualDPI;
    }

    void WindowsWindow::SetManualManageDPIChanges(bool bManual)
    {
        m_definition.bManualDPI = bManual;
    }

    // =========================================================================
    // 03-03: プロパティアクセサ
    // =========================================================================

    int32_t WindowsWindow::GetWindowBorderSize() const
    {
        if (m_definition.hasOsWindowBorder)
        {
            return ::GetSystemMetrics(SM_CXSIZEFRAME);
        }
        return 0;
    }

    int32_t WindowsWindow::GetWindowTitleBarSize() const
    {
        if (m_definition.hasOsWindowBorder)
        {
            return ::GetSystemMetrics(SM_CYCAPTION);
        }
        return 0;
    }

    void* WindowsWindow::GetOSWindowHandle() const
    {
        return m_hwnd;
    }

    // =========================================================================
    // 13-04: OLE ドラッグ＆ドロップ
    // =========================================================================

    /// OLE ドラッグデータ
    struct DragDropOLEData
    {
        std::wstring text;
        std::vector<std::wstring> files;
        bool bHasText = false;
        bool bHasFiles = false;
    };

    namespace
    {
        /// OLE ドラッグデータ
        DragDropOLEData ParseOLEData(IDataObject* pDataObj)
        {
            DragDropOLEData data;

            // CF_UNICODETEXT
            FORMATETC fmtText = {};
            fmtText.cfFormat = CF_UNICODETEXT;
            fmtText.dwAspect = DVASPECT_CONTENT;
            fmtText.lindex = -1;
            fmtText.tymed = TYMED_HGLOBAL;

            STGMEDIUM stgText = {};
            if (SUCCEEDED(pDataObj->GetData(&fmtText, &stgText)))
            {
                const auto* text = static_cast<const wchar_t*>(::GlobalLock(stgText.hGlobal));
                if (text != nullptr)
                {
                    data.text = text;
                    data.bHasText = true;
                    ::GlobalUnlock(stgText.hGlobal);
                }
                ::ReleaseStgMedium(&stgText);
            }

            // CF_HDROP (ファイルリスト)
            FORMATETC fmtDrop = {};
            fmtDrop.cfFormat = CF_HDROP;
            fmtDrop.dwAspect = DVASPECT_CONTENT;
            fmtDrop.lindex = -1;
            fmtDrop.tymed = TYMED_HGLOBAL;

            STGMEDIUM stgDrop = {};
            if (SUCCEEDED(pDataObj->GetData(&fmtDrop, &stgDrop)))
            {
                auto* hDrop = static_cast<HDROP>(::GlobalLock(stgDrop.hGlobal));
                if (hDrop != nullptr)
                {
                    UINT const fileCount = ::DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                    data.files.reserve(fileCount);
                    for (UINT i = 0; i < fileCount; ++i)
                    {
                        UINT const len = ::DragQueryFileW(hDrop, i, nullptr, 0);
                        std::wstring path(len + 1, L'\0');
                        ::DragQueryFileW(hDrop, i, path.data(), len + 1);
                        path.resize(len);
                        data.files.push_back(std::move(path));
                    }
                    data.bHasFiles = !data.files.empty();
                    ::GlobalUnlock(stgDrop.hGlobal);
                }
                ::ReleaseStgMedium(&stgDrop);
            }

            return data;
        }

        /// DropEffect 螟画鋤
        DWORD DropEffectToDWORD(DropEffect::Type effect)
        {
            switch (effect)
            {
            case DropEffect::Copy:
                return DROPEFFECT_COPY;
            case DropEffect::Move:
                return DROPEFFECT_MOVE;
            case DropEffect::Link:
                return DROPEFFECT_LINK;
            default:
                return DROPEFFECT_NONE;
            }
        }
    } // anonymous namespace

    HRESULT STDMETHODCALLTYPE WindowsWindow::DragEnter(IDataObject* pDataObj,
                                                       DWORD /*grfKeyState*/,
                                                       POINTL /*pt*/,
                                                       DWORD* pdwEffect)
    {
        if ((pDataObj == nullptr) || (pdwEffect == nullptr))
        {
            if (pdwEffect != nullptr)
            {
                *pdwEffect = DROPEFFECT_NONE;
            }
            return S_OK;
        }

        m_dragDropData = std::make_unique<DragDropOLEData>(ParseOLEData(pDataObj));

        DropEffect::Type effect = DropEffect::None;
        if (m_owningApplication != nullptr)
        {
            const auto& handler = m_owningApplication->GetMessageHandler();
            auto self = std::static_pointer_cast<GenericWindow>(shared_from_this());

            if (m_dragDropData->bHasText && m_dragDropData->bHasFiles)
            {
                effect = handler->OnDragEnterExternal(self, m_dragDropData->text.c_str(), m_dragDropData->files);
            }
            else if (m_dragDropData->bHasFiles)
            {
                effect = handler->OnDragEnterFiles(self, m_dragDropData->files);
            }
            else if (m_dragDropData->bHasText)
            {
                effect = handler->OnDragEnterText(self, m_dragDropData->text.c_str());
            }
        }

        *pdwEffect = DropEffectToDWORD(effect);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsWindow::DragOver(DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
    {
        DropEffect::Type effect = DropEffect::None;
        if (m_owningApplication != nullptr)
        {
            const auto& handler = m_owningApplication->GetMessageHandler();
            auto self = std::static_pointer_cast<GenericWindow>(shared_from_this());
            effect = handler->OnDragOver(self);
        }

        if (pdwEffect != nullptr)
        {
            *pdwEffect = DropEffectToDWORD(effect);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsWindow::DragLeave()
    {
        if (m_owningApplication != nullptr)
        {
            const auto& handler = m_owningApplication->GetMessageHandler();
            auto self = std::static_pointer_cast<GenericWindow>(shared_from_this());
            handler->OnDragLeave(self);
        }
        m_dragDropData.reset();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsWindow::Drop(IDataObject* /*pDataObj*/,
                                                  DWORD /*grfKeyState*/,
                                                  POINTL /*pt*/,
                                                  DWORD* pdwEffect)
    {
        DropEffect::Type effect = DropEffect::None;
        if (m_owningApplication != nullptr)
        {
            const auto& handler = m_owningApplication->GetMessageHandler();
            auto self = std::static_pointer_cast<GenericWindow>(shared_from_this());
            effect = handler->OnDragDrop(self);
        }
        m_dragDropData.reset();

        if (pdwEffect != nullptr)
        {
            *pdwEffect = DropEffectToDWORD(effect);
        }
        return S_OK;
    }

    // =========================================================================
    // IUnknown
    // =========================================================================

    HRESULT STDMETHODCALLTYPE WindowsWindow::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (ppvObject == nullptr)
        {
            return E_INVALIDARG;
        }

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
        {
            *ppvObject = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE WindowsWindow::AddRef()
    {
        return ::InterlockedIncrement(&m_oleRefCount);
    }

    ULONG STDMETHODCALLTYPE WindowsWindow::Release()
    {
        LONG const count = ::InterlockedDecrement(&m_oleRefCount);
        if (count < 0)
        {
            ::InterlockedExchange(&m_oleRefCount, 0);
            return 0;
        }
        // WindowsWindow lifetime is managed by shared_ptr; do not delete here.
        return static_cast<ULONG>(count);
    }

} // namespace NS
