/// @file WindowsTextInputMethodSystem.cpp
/// @brief Windows IMM32 テキスト入力メソッドシステム実装 (11-04, 11-05)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <imm.h>
#endif

#include "Windows/WindowsTextInputMethodSystem.h"
#include "GenericPlatform/GenericWindow.h"

#include <algorithm>
#include <string>
#include <vector>

namespace NS
{
    // =========================================================================
    // ChangeNotifier
    // =========================================================================

    WindowsTextInputMethodSystem::InternalContext* WindowsTextInputMethodSystem::GetActiveContext()
    {
        if (m_activeContextIndex >= 0 && m_activeContextIndex < static_cast<int32_t>(m_contexts.size()))
        {
            return &m_contexts[m_activeContextIndex];
        }
        return nullptr;
    }

    const WindowsTextInputMethodSystem::InternalContext* WindowsTextInputMethodSystem::GetActiveContext() const
    {
        if (m_activeContextIndex >= 0 && m_activeContextIndex < static_cast<int32_t>(m_contexts.size()))
        {
            return &m_contexts[m_activeContextIndex];
        }
        return nullptr;
    }

    void WindowsTextInputMethodSystem::ChangeNotifier::NotifyLayoutChanged()
    {
        auto* ctx = m_owner.GetActiveContext();
        if (ctx && ctx->hWnd)
        {
            m_owner.UpdateCandidateWindowPosition(ctx->hWnd);
        }
    }

    void WindowsTextInputMethodSystem::ChangeNotifier::NotifySelectionChanged() {}

    void WindowsTextInputMethodSystem::ChangeNotifier::NotifyTextChanged() {}

    void WindowsTextInputMethodSystem::ChangeNotifier::CancelComposition()
    {
        auto* ctx = m_owner.GetActiveContext();
        if (ctx && ctx->hWnd)
        {
            HIMC hImc = ::ImmGetContext(ctx->hWnd);
            if (hImc)
            {
                ::ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                ::ImmReleaseContext(ctx->hWnd, hImc);
            }
        }
    }

    // =========================================================================
    // コンストラクタ・デストラクタ
    // =========================================================================

    WindowsTextInputMethodSystem::WindowsTextInputMethodSystem() = default;
    WindowsTextInputMethodSystem::~WindowsTextInputMethodSystem() = default;

    // =========================================================================
    // ITextInputMethodSystem overrides
    // =========================================================================

    std::shared_ptr<ITextInputMethodChangeNotifier> WindowsTextInputMethodSystem::RegisterContext(
        const std::shared_ptr<ITextInputMethodContext>& Context)
    {
        if (!Context)
        {
            // Keep API contract stable: callers can keep using the returned notifier without null checks.
            return std::make_shared<ChangeNotifier>(*this);
        }

        InternalContext ctx;
        ctx.Owner = Context;

        // コンテキストからウィンドウを取得
        auto window = Context->GetWindow();
        // HWND は WindowsWindow から取得 (Phase 6 で m_hwnd を公開済み)
        // ここでは nullptr を設定し、ActivateContext で取得する
        ctx.hWnd = window ? static_cast<HWND>(window->GetOSWindowHandle()) : nullptr;

        m_contexts.push_back(std::move(ctx));

        return std::make_shared<ChangeNotifier>(*this);
    }

    void WindowsTextInputMethodSystem::UnregisterContext(const std::shared_ptr<ITextInputMethodContext>& Context)
    {
        auto it = std::find_if(
            m_contexts.begin(), m_contexts.end(), [&](const InternalContext& c) { return c.Owner == Context; });
        if (it != m_contexts.end())
        {
            int32_t erasedIndex = static_cast<int32_t>(std::distance(m_contexts.begin(), it));
            if (m_activeContextIndex == erasedIndex)
            {
                m_activeContextIndex = -1;
            }
            else if (m_activeContextIndex > erasedIndex)
            {
                --m_activeContextIndex; // 削除によるインデックスずれ補正
            }
            m_contexts.erase(it);
        }
    }

    void WindowsTextInputMethodSystem::ActivateContext(const std::shared_ptr<ITextInputMethodContext>& Context)
    {
        auto it = std::find_if(
            m_contexts.begin(), m_contexts.end(), [&](const InternalContext& c) { return c.Owner == Context; });
        if (it != m_contexts.end())
        {
            m_activeContextIndex = static_cast<int32_t>(std::distance(m_contexts.begin(), it));
            auto* activeCtx = GetActiveContext();

            if (activeCtx && activeCtx->hWnd)
            {
                HIMC hImc = ::ImmGetContext(activeCtx->hWnd);
                if (hImc)
                {
                    ::ImmSetOpenStatus(hImc, TRUE);
                    ::ImmReleaseContext(activeCtx->hWnd, hImc);
                }
            }
        }
    }

    void WindowsTextInputMethodSystem::DeactivateContext(const std::shared_ptr<ITextInputMethodContext>& Context)
    {
        auto* activeCtx = GetActiveContext();
        if (activeCtx && activeCtx->Owner == Context)
        {
            // コンポジション中なら確定させる
            if (activeCtx->bIsComposing && activeCtx->hWnd)
            {
                HIMC hImc = ::ImmGetContext(activeCtx->hWnd);
                if (hImc)
                {
                    ::ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
                    ::ImmReleaseContext(activeCtx->hWnd, hImc);
                }
            }
            m_activeContextIndex = -1;
        }
    }

    // =========================================================================
    // Windows メッセージ処理 (11-05)
    // =========================================================================

    bool WindowsTextInputMethodSystem::ProcessMessage(
        HWND hWnd, uint32_t Msg, WPARAM /*wParam*/, LPARAM lParam, int32_t& OutResult)
    {
        switch (Msg)
        {
        case WM_IME_SETCONTEXT:
            // デフォルト処理に任せる
            return false;

        case WM_IME_STARTCOMPOSITION:
            HandleIMECompositionStart(hWnd);
            OutResult = 0;
            return true;

        case WM_IME_COMPOSITION:
            HandleIMEComposition(hWnd, lParam);
            OutResult = 0;
            return true;

        case WM_IME_ENDCOMPOSITION:
            HandleIMECompositionEnd(hWnd);
            OutResult = 0;
            return true;

        case WM_IME_NOTIFY:
        case WM_IME_REQUEST:
        case WM_IME_CHAR:
        case WM_INPUTLANGCHANGE:
        case WM_INPUTLANGCHANGEREQUEST:
            // デフォルト処理
            return false;

        default:
            return false;
        }
    }

    // =========================================================================
    // IMM32 ハンドラ
    // =========================================================================

    void WindowsTextInputMethodSystem::HandleIMECompositionStart(HWND hWnd)
    {
        auto* ctx = GetActiveContext();
        if (ctx)
        {
            ctx->hWnd = hWnd;
            ctx->bIsComposing = true;
            ctx->Owner->BeginComposition();
        }
    }

    void WindowsTextInputMethodSystem::HandleIMEComposition(HWND hWnd, LPARAM lParam)
    {
        auto* ctx = GetActiveContext();
        if (!ctx)
        {
            return;
        }

        HIMC hImc = ::ImmGetContext(hWnd);
        if (!hImc)
        {
            return;
        }

        if (lParam & GCS_COMPSTR)
        {
            // 2-call パターン: 最初にサイズ取得、次にデータ取得
            LONG bytes = ::ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
            if (bytes > 0)
            {
                std::vector<wchar_t> buf(bytes / sizeof(wchar_t) + 1, L'\0');
                ::ImmGetCompositionStringW(hImc, GCS_COMPSTR, buf.data(), bytes);

                // コンポジション範囲を更新
                int32_t len = static_cast<int32_t>(wcslen(buf.data()));
                ctx->Owner->UpdateCompositionRange(0, len);
            }
        }

        if (lParam & GCS_RESULTSTR)
        {
            // 確定文字列
            LONG bytes = ::ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
            if (bytes > 0)
            {
                std::vector<wchar_t> buf(bytes / sizeof(wchar_t) + 1, L'\0');
                ::ImmGetCompositionStringW(hImc, GCS_RESULTSTR, buf.data(), bytes);

                ctx->Owner->InsertTextAtCursor(buf.data());
            }
        }

        // 候補ウィンドウ位置更新
        UpdateCandidateWindowPosition(hWnd);

        ::ImmReleaseContext(hWnd, hImc);
    }

    void WindowsTextInputMethodSystem::HandleIMECompositionEnd(HWND /*hWnd*/)
    {
        auto* ctx = GetActiveContext();
        if (ctx)
        {
            ctx->bIsComposing = false;
            ctx->Owner->EndComposition();
        }
    }

    void WindowsTextInputMethodSystem::UpdateCandidateWindowPosition(HWND hWnd)
    {
        auto* ctx = GetActiveContext();
        if (!ctx)
        {
            return;
        }

        // テキスト境界からIME候補ウィンドウ位置を計算
        PlatformRect bounds = {};
        int32_t selBegin = 0, selLen = 0;
        CaretPosition caret = CaretPosition::Ending;
        ctx->Owner->GetSelectionRange(selBegin, selLen, caret);
        ctx->Owner->GetTextBounds(selBegin, selLen > 0 ? selLen : 1, bounds);

        HIMC hImc = ::ImmGetContext(hWnd);
        if (hImc)
        {
            // CFS_EXCLUDE: 候補ウィンドウを指定矩形外に配置
            CANDIDATEFORM cf = {};
            cf.dwIndex = 0;
            cf.dwStyle = CFS_EXCLUDE;
            cf.ptCurrentPos.x = bounds.Left;
            cf.ptCurrentPos.y = bounds.Bottom;
            cf.rcArea.left = bounds.Left;
            cf.rcArea.top = bounds.Top;
            cf.rcArea.right = bounds.Right;
            cf.rcArea.bottom = bounds.Bottom;

            // スクリーン座標→クライアント座標変換
            ::ScreenToClient(hWnd, &cf.ptCurrentPos);
            POINT topLeft = {cf.rcArea.left, cf.rcArea.top};
            POINT bottomRight = {cf.rcArea.right, cf.rcArea.bottom};
            ::ScreenToClient(hWnd, &topLeft);
            ::ScreenToClient(hWnd, &bottomRight);
            cf.rcArea.left = topLeft.x;
            cf.rcArea.top = topLeft.y;
            cf.rcArea.right = bottomRight.x;
            cf.rcArea.bottom = bottomRight.y;

            ::ImmSetCandidateWindow(hImc, &cf);
            ::ImmReleaseContext(hWnd, hImc);
        }
    }

    WindowsTextInputMethodSystem::InternalContext* WindowsTextInputMethodSystem::FindContextByHWND(HWND hWnd)
    {
        for (auto& ctx : m_contexts)
        {
            if (ctx.hWnd == hWnd)
            {
                return &ctx;
            }
        }
        return nullptr;
    }

} // namespace NS
