/// @file WindowsPlatformApplicationMisc.cpp
/// @brief Windows PlatformApplicationMisc 実装 (10-02, 10-03)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <shellscalingapi.h>
#include <xinput.h>
#endif

#include "Windows/WindowsApplication.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

#include <string>

#pragma comment(lib, "shcore.lib")

namespace NS
{
    // =========================================================================
    // 静的メンバ
    // =========================================================================

    // =========================================================================
    // ライフサイクル
    // =========================================================================

    void WindowsPlatformApplicationMisc::PreInit()
    {
        SetHighDPIMode();
    }

    void WindowsPlatformApplicationMisc::Init() {}

    void WindowsPlatformApplicationMisc::TearDown() {}

    // =========================================================================
    // DPI
    // =========================================================================

    void WindowsPlatformApplicationMisc::SetHighDPIMode()
    {
        // Per-Monitor V2 (Win10 1703+)
        if (::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            return;
        }

        // Fallback: shcore SetProcessDpiAwareness (Win8.1+, shcore.lib 静的リンク済み)
        if (SUCCEEDED(::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)))
        {
            return;
        }

        // Fallback: user32 SetProcessDPIAware (Vista+)
        ::SetProcessDPIAware();
    }

    // =========================================================================
    // アプリケーション生成
    // =========================================================================

    std::shared_ptr<GenericApplication> WindowsPlatformApplicationMisc::CreateApplication()
    {
        HINSTANCE hInstance = ::GetModuleHandle(nullptr);
        HICON hIcon = ::LoadIcon(hInstance, L"AppIcon");

        return WindowsApplication::CreateWindowsApplication(hInstance, hIcon);
    }

    // =========================================================================
    // DPI
    // =========================================================================

    float WindowsPlatformApplicationMisc::GetDPIScaleFactorAtPoint(int32_t X, int32_t Y)
    {
        POINT pt = {X, Y};
        HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(::GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
        {
            return static_cast<float>(dpiX) / 96.0f;
        }
        return 1.0f;
    }

    // =========================================================================
    // スクリーンセーバー
    // =========================================================================

    bool WindowsPlatformApplicationMisc::ControlScreensaver(ScreenSaverAction Action)
    {
        if (Action == ScreenSaverAction::Disable)
        {
            // 0ピクセルマウス移動でスクリーンセーバーリセット
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            ::SendInput(1, &input, sizeof(INPUT));
        }
        return true;
    }

    // =========================================================================
    // クリップボード
    // =========================================================================

    void WindowsPlatformApplicationMisc::ClipboardCopy(const wchar_t* Str)
    {
        if (!Str)
        {
            return;
        }

        if (!::OpenClipboard(nullptr))
        {
            return;
        }

        const size_t len = wcslen(Str);
        const size_t bytes = (len + 1) * sizeof(wchar_t);
        HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!hMem)
        {
            ::CloseClipboard();
            return;
        }

        wchar_t* dest = static_cast<wchar_t*>(::GlobalLock(hMem));
        if (!dest)
        {
            ::GlobalFree(hMem);
            ::CloseClipboard();
            return;
        }

        memcpy(dest, Str, bytes);
        ::GlobalUnlock(hMem);

        ::EmptyClipboard();
        if (!::SetClipboardData(CF_UNICODETEXT, hMem))
        {
            // SetClipboardData 失敗時は所有権が OS に移っていないため解放
            ::GlobalFree(hMem);
        }
        ::CloseClipboard();
    }

    void WindowsPlatformApplicationMisc::ClipboardPaste(std::wstring& Dest)
    {
        Dest.clear();

        if (!::OpenClipboard(nullptr))
        {
            return;
        }

        HGLOBAL hMem = ::GetClipboardData(CF_UNICODETEXT);
        if (hMem)
        {
            const wchar_t* src = static_cast<const wchar_t*>(::GlobalLock(hMem));
            if (src)
            {
                Dest = src;
                ::GlobalUnlock(hMem);
            }
        }

        ::CloseClipboard();
    }

    // =========================================================================
    // ユーティリティ
    // =========================================================================

    bool WindowsPlatformApplicationMisc::IsRemoteSession()
    {
        return ::GetSystemMetrics(SM_REMOTESESSION) != 0;
    }

    // =========================================================================
    // アプリケーション状態
    // =========================================================================

    bool WindowsPlatformApplicationMisc::IsThisApplicationForeground()
    {
        HWND hForeground = ::GetForegroundWindow();
        if (!hForeground)
        {
            return false;
        }
        DWORD foregroundPid = 0;
        ::GetWindowThreadProcessId(hForeground, &foregroundPid);
        return foregroundPid == ::GetCurrentProcessId();
    }

    void WindowsPlatformApplicationMisc::PumpMessages(bool /*bFromMainLoop*/)
    {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    // =========================================================================
    // ゲームパッド
    // =========================================================================

    std::wstring WindowsPlatformApplicationMisc::GetGamepadControllerName(int32_t ControllerId)
    {
        if (ControllerId >= 0 && ControllerId < 4)
        {
            XINPUT_CAPABILITIES caps = {};
            if (XInputGetCapabilities(static_cast<DWORD>(ControllerId), XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS)
            {
                return L"XInput Controller";
            }
        }
        return {};
    }

} // namespace NS
