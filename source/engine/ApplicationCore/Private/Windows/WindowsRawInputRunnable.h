/// @file WindowsRawInputRunnable.h
/// @brief 高ポーリングレートマウス用 Raw Input ワーカースレッド (13-02)
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

#include <array>
#include <atomic>
#include <cstring>
#include <thread>

namespace NS
{
    /// ロックフリーSPSCリングバッファ (RAWMOUSE用)
    /// プロデューサー: ワーカースレッド、コンシューマー: メインスレッド
    class RawMouseSPSCQueue
    {
    public:
        static constexpr uint32_t BlockSize = 64;
        static constexpr uint32_t BlockCount = 16;
        static constexpr uint32_t Capacity = BlockSize * BlockCount; // 1024 entries

        /// プロデューサー側: マウスデータをキューに追加
        void Push(const RAWMOUSE& Data)
        {
            uint32_t writeIdx = m_writeIndex.load(std::memory_order_relaxed);
            m_buffer[writeIdx % Capacity] = Data;
            m_writeIndex.store(writeIdx + 1, std::memory_order_release);
            m_inputCount.fetch_add(1, std::memory_order_release);
        }

        /// コンシューマー側: マウスデータをキューから取得
        bool Pop(RAWMOUSE& OutData)
        {
            uint32_t readIdx = m_readIndex.load(std::memory_order_relaxed);
            uint32_t writeIdx = m_writeIndex.load(std::memory_order_acquire);

            if (readIdx >= writeIdx)
            {
                return false; // 空
            }

            OutData = m_buffer[readIdx % Capacity];
            m_readIndex.store(readIdx + 1, std::memory_order_release);
            return true;
        }

        /// 未読データ数を取得
        uint32_t TakeInputCount() { return m_inputCount.exchange(0, std::memory_order_acquire); }

        /// キューが空か
        bool IsEmpty() const
        {
            return m_readIndex.load(std::memory_order_acquire) >= m_writeIndex.load(std::memory_order_acquire);
        }

    private:
        std::array<RAWMOUSE, Capacity> m_buffer = {};
        alignas(64) std::atomic<uint32_t> m_writeIndex{0};
        alignas(64) std::atomic<uint32_t> m_readIndex{0};
        alignas(64) std::atomic<uint32_t> m_inputCount{0};
    };

    /// Raw Input ワーカースレッド
    /// 高ポーリングレートマウス (1000Hz+) の入力を別スレッドで受信し、
    /// SPSCキューでメインスレッドに転送する
    class WindowsRawInputRunnable
    {
    public:
        /// カスタムメッセージ
        static constexpr UINT WM_UE_RAWINPUT_QUIT = WM_APP;
        static constexpr UINT WM_UE_RAWINPUT_REGISTER = WM_APP + 1;
        static constexpr UINT WM_UE_RAWINPUT_UNREGISTER = WM_APP + 2;

        WindowsRawInputRunnable() = default;

        ~WindowsRawInputRunnable() { Stop(); }

        /// ワーカースレッド起動
        bool Start()
        {
            if (m_bRunning)
                return true;

            m_bRunning = true;
            m_createWindowEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            m_unregisterEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

            m_thread = std::thread([this]() { ThreadProc(); });

            // ワーカースレッドのHWND作成完了を待機
            ::WaitForSingleObject(m_createWindowEvent, 5000);

            if (m_threadHWnd == nullptr)
            {
                // 開始失敗: 状態を復元して再試行可能にする
                m_bRunning = false;
                if (m_thread.joinable())
                {
                    m_thread.join();
                }
                if (m_createWindowEvent)
                {
                    ::CloseHandle(m_createWindowEvent);
                    m_createWindowEvent = nullptr;
                }
                if (m_unregisterEvent)
                {
                    ::CloseHandle(m_unregisterEvent);
                    m_unregisterEvent = nullptr;
                }
                return false;
            }
            return true;
        }

        /// ワーカースレッド停止
        void Stop()
        {
            if (!m_bRunning)
                return;

            m_bRunning = false;

            if (m_threadHWnd)
            {
                ::PostMessage(m_threadHWnd, WM_UE_RAWINPUT_QUIT, 0, 0);
            }

            if (m_thread.joinable())
            {
                m_thread.join();
            }

            if (m_createWindowEvent)
            {
                ::CloseHandle(m_createWindowEvent);
                m_createWindowEvent = nullptr;
            }
            if (m_unregisterEvent)
            {
                ::CloseHandle(m_unregisterEvent);
                m_unregisterEvent = nullptr;
            }
        }

        /// メインスレッド: 蓄積されたマウス入力を処理
        /// @return 処理した入力数
        uint32_t ProcessWorkerInputs(RAWMOUSE* OutBuffer, uint32_t MaxCount)
        {
            uint32_t count = 0;
            while (count < MaxCount)
            {
                RAWMOUSE mouse = {};
                if (!m_queue.Pop(mouse))
                    break;

                // ボタンスワップ対応
                if (m_bMouseButtonsSwapped)
                {
                    if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
                    {
                        mouse.usButtonFlags &= ~RI_MOUSE_LEFT_BUTTON_DOWN;
                        mouse.usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_DOWN;
                    }
                    else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
                    {
                        mouse.usButtonFlags &= ~RI_MOUSE_RIGHT_BUTTON_DOWN;
                        mouse.usButtonFlags |= RI_MOUSE_LEFT_BUTTON_DOWN;
                    }
                    if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
                    {
                        mouse.usButtonFlags &= ~RI_MOUSE_LEFT_BUTTON_UP;
                        mouse.usButtonFlags |= RI_MOUSE_RIGHT_BUTTON_UP;
                    }
                    else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
                    {
                        mouse.usButtonFlags &= ~RI_MOUSE_RIGHT_BUTTON_UP;
                        mouse.usButtonFlags |= RI_MOUSE_LEFT_BUTTON_UP;
                    }
                }

                OutBuffer[count++] = mouse;
            }
            return count;
        }

        /// 入力数を取得してリセット
        uint32_t TakeMouseInputCount() { return m_queue.TakeInputCount(); }

        /// マウスデータをポップ
        bool PopMouseInput(RAWMOUSE& OutData) { return m_queue.Pop(OutData); }

        /// Raw Input デバイス登録要求
        void RequestRegister()
        {
            if (m_threadHWnd)
            {
                ::ResetEvent(m_unregisterEvent);
                ::PostMessage(m_threadHWnd, WM_UE_RAWINPUT_REGISTER, 0, 0);
            }
        }

        /// Raw Input デバイス登録解除要求
        void RequestUnregister()
        {
            if (m_threadHWnd)
            {
                ::ResetEvent(m_unregisterEvent);
                ::PostMessage(m_threadHWnd, WM_UE_RAWINPUT_UNREGISTER, 0, 0);
                ::WaitForSingleObject(m_unregisterEvent, 1000);
            }
        }

    private:
        void ThreadProc()
        {
            // ボタンスワップ状態
            m_bMouseButtonsSwapped = ::GetSystemMetrics(SM_SWAPBUTTON) != 0;

            // 不可視ウィンドウ作成
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = RawInputWndProc;
            wc.hInstance = ::GetModuleHandleW(nullptr);
            wc.lpszClassName = L"NSRawInputWindow";
            wc.cbWndExtra = sizeof(WindowsRawInputRunnable*);
            ::RegisterClassExW(&wc);

            m_threadHWnd = ::CreateWindowExW(
                0, L"NSRawInputWindow", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), this);

            ::SetEvent(m_createWindowEvent);

            if (!m_threadHWnd)
            {
                return;
            }

            // メッセージループ
            MSG msg;
            while (::GetMessage(&msg, nullptr, 0, 0))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);

                if (msg.message == WM_UE_RAWINPUT_QUIT)
                    break;
            }

            // クリーンアップ
            if (m_threadHWnd)
            {
                ::DestroyWindow(m_threadHWnd);
                m_threadHWnd = nullptr;
            }
            ::UnregisterClassW(L"NSRawInputWindow", ::GetModuleHandleW(nullptr));
        }

        static LRESULT CALLBACK RawInputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (msg == WM_CREATE)
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                ::SetWindowLongPtrW(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                return 0;
            }

            auto* self = reinterpret_cast<WindowsRawInputRunnable*>(::GetWindowLongPtrW(hWnd, 0));

            switch (msg)
            {
            case WM_INPUT:
            {
                if (!self)
                    break;
                UINT dwSize = 0;
                ::GetRawInputData(
                    reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
                if (dwSize > 0 && dwSize <= 64)
                {
                    alignas(8) BYTE buffer[64];
                    if (::GetRawInputData(
                            reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)) ==
                        dwSize)
                    {
                        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
                        if (raw->header.dwType == RIM_TYPEMOUSE)
                        {
                            self->m_queue.Push(raw->data.mouse);
                        }
                    }
                }
                return 0;
            }
            case WM_UE_RAWINPUT_REGISTER:
            {
                if (!self)
                    break;
                RAWINPUTDEVICE rid = {};
                rid.usUsagePage = 0x01;
                rid.usUsage = 0x02;
                rid.dwFlags = RIDEV_NOLEGACY;
                rid.hwndTarget = hWnd;
                ::RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
                return 0;
            }
            case WM_UE_RAWINPUT_UNREGISTER:
            {
                if (!self)
                    break;
                RAWINPUTDEVICE rid = {};
                rid.usUsagePage = 0x01;
                rid.usUsage = 0x02;
                rid.dwFlags = RIDEV_REMOVE;
                rid.hwndTarget = nullptr;
                ::RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
                ::SetEvent(self->m_unregisterEvent);
                return 0;
            }
            default:
                break;
            }

            return ::DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        RawMouseSPSCQueue m_queue;
        std::thread m_thread;
        HWND m_threadHWnd = nullptr;
        HANDLE m_createWindowEvent = nullptr;
        HANDLE m_unregisterEvent = nullptr;
        bool m_bRunning = false;
        bool m_bMouseButtonsSwapped = false;
    };

} // namespace NS
