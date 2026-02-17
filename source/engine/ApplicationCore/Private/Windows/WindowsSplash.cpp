/// @file WindowsSplash.cpp
/// @brief Windows スプラッシュスクリーン実装 (12-01)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <windows.h>
#endif

#include "GenericPlatform/GenericPlatformSplash.h"

#include <atomic>
#include <string>
#include <thread>

namespace
{
    constexpr int SplashWidth = 600;
    constexpr int SplashHeight = 300;
    constexpr int ProgressBarHeight = 4;
} // anonymous namespace

namespace NS
{
    /// Windows スプラッシュスクリーン実装
    class WindowsSplash : public GenericPlatformSplash
    {
    public:
        WindowsSplash() = default;

        ~WindowsSplash() override { Hide(); }

        void Show() override
        {
            if (m_bIsShown)
            {
                return;
            }

            m_bIsShown = true;
            m_bShouldClose = false;

            m_splashThread = std::thread([this]() { SplashThreadProc(); });
        }

        void Hide() override
        {
            if (!m_bIsShown)
            {
                return;
            }

            m_bShouldClose = true;

            if (m_hWnd)
            {
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
            }

            if (m_splashThread.joinable())
            {
                m_splashThread.join();
            }

            m_bIsShown = false;
        }

        bool IsShown() const override { return m_bIsShown; }

        void SetSplashText(SplashTextType InType, const wchar_t* InText) override
        {
            ::EnterCriticalSection(&m_cs);
            m_texts[static_cast<int>(InType)] = InText ? InText : L"";
            ::LeaveCriticalSection(&m_cs);

            if (m_hWnd)
            {
                ::InvalidateRect(m_hWnd, nullptr, FALSE);
            }
        }

        void SetProgress(float InProgress) override
        {
            m_progress = InProgress;
            if (m_hWnd)
            {
                ::InvalidateRect(m_hWnd, nullptr, FALSE);
            }
        }

        void SetCustomSplashImage(const wchar_t* /*InImagePath*/) override
        {
            // 将来: WIC で画像をロードして描画
        }

    private:
        void SplashThreadProc()
        {
            ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            ::InitializeCriticalSection(&m_cs);

            // ウィンドウクラス登録
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = SplashWndProc;
            wc.hInstance = ::GetModuleHandle(nullptr);
            wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
            wc.lpszClassName = L"NSSplashWindow";
            wc.cbWndExtra = sizeof(WindowsSplash*);
            ::RegisterClassExW(&wc);

            // スクリーン中央に配置
            int screenW = ::GetSystemMetrics(SM_CXSCREEN);
            int screenH = ::GetSystemMetrics(SM_CYSCREEN);
            int x = (screenW - SplashWidth) / 2;
            int y = (screenH - SplashHeight) / 2;

            m_hWnd = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                       L"NSSplashWindow",
                                       L"",
                                       WS_POPUP | WS_VISIBLE,
                                       x,
                                       y,
                                       SplashWidth,
                                       SplashHeight,
                                       nullptr,
                                       nullptr,
                                       ::GetModuleHandle(nullptr),
                                       this);

            // メッセージループ
            MSG msg;
            while (::GetMessage(&msg, nullptr, 0, 0))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);

                if (m_bShouldClose)
                {
                    break;
                }
            }

            if (m_hWnd)
            {
                ::DestroyWindow(m_hWnd);
                m_hWnd = nullptr;
            }

            ::UnregisterClassW(L"NSSplashWindow", ::GetModuleHandle(nullptr));
            ::DeleteCriticalSection(&m_cs);
            ::CoUninitialize();
        }

        static LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_CREATE)
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                ::SetWindowLongPtrW(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                return 0;
            }

            auto* self = reinterpret_cast<WindowsSplash*>(::GetWindowLongPtrW(hWnd, 0));

            switch (msg)
            {
            case WM_PAINT:
            {
                if (!self)
                    break;
                PAINTSTRUCT ps;
                HDC hdc = ::BeginPaint(hWnd, &ps);
                self->OnPaint(hdc);
                ::EndPaint(hWnd, &ps);
                return 0;
            }
            case WM_CLOSE:
                ::PostQuitMessage(0);
                return 0;
            default:
                break;
            }
            return ::DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        void OnPaint(HDC hdc)
        {
            RECT clientRect;
            ::GetClientRect(m_hWnd, &clientRect);

            // 背景
            HBRUSH bgBrush = ::CreateSolidBrush(RGB(30, 30, 30));
            ::FillRect(hdc, &clientRect, bgBrush);
            ::DeleteObject(bgBrush);

            // テキスト描画
            ::SetBkMode(hdc, TRANSPARENT);
            ::SetTextColor(hdc, RGB(220, 220, 220));

            ::EnterCriticalSection(&m_cs);

            RECT textRect = {20, 20, SplashWidth - 20, SplashHeight - 40};
            for (int i = 0; i < 4; ++i)
            {
                if (!m_texts[i].empty())
                {
                    ::DrawTextW(hdc, m_texts[i].c_str(), -1, &textRect, DT_LEFT | DT_SINGLELINE);
                    textRect.top += 24;
                }
            }

            ::LeaveCriticalSection(&m_cs);

            // プログレスバー
            if (m_progress > 0.0f)
            {
                RECT barBg = {0, SplashHeight - ProgressBarHeight, SplashWidth, SplashHeight};
                HBRUSH barBgBrush = ::CreateSolidBrush(RGB(60, 60, 60));
                ::FillRect(hdc, &barBg, barBgBrush);
                ::DeleteObject(barBgBrush);

                int fillWidth = static_cast<int>(SplashWidth * m_progress);
                RECT barFill = {0, SplashHeight - ProgressBarHeight, fillWidth, SplashHeight};
                HBRUSH barFillBrush = ::CreateSolidBrush(RGB(0, 120, 215));
                ::FillRect(hdc, &barFill, barFillBrush);
                ::DeleteObject(barFillBrush);
            }
        }

        HWND m_hWnd = nullptr;
        std::atomic<bool> m_bIsShown{false};
        std::atomic<bool> m_bShouldClose{false};
        std::thread m_splashThread;
        CRITICAL_SECTION m_cs = {};
        std::wstring m_texts[4];
        std::atomic<float> m_progress{0.0f};
    };

    // =========================================================================
    // Singleton
    // =========================================================================

    GenericPlatformSplash& GenericPlatformSplash::Get()
    {
        static WindowsSplash s_instance;
        return s_instance;
    }

} // namespace NS
