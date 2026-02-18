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
        if ((ctx != nullptr) && (ctx->hWnd != nullptr))
        {
            m_owner.UpdateCandidateWindowPosition(ctx->hWnd);
        }
    }

    void WindowsTextInputMethodSystem::ChangeNotifier::NotifySelectionChanged() {}

    void WindowsTextInputMethodSystem::ChangeNotifier::NotifyTextChanged() {}

    void WindowsTextInputMethodSystem::ChangeNotifier::CancelComposition()
    {
        auto* ctx = m_owner.GetActiveContext();
        if ((ctx != nullptr) && (ctx->hWnd != nullptr))
        {
            HIMC hImc = ::ImmGetContext(ctx->hWnd);
            if (hImc != nullptr)
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
        const std::shared_ptr<ITextInputMethodContext>& context)
    {
        if (!context)
        {
            // Keep API contract stable: callers can keep using the returned notifier without null checks.
            return std::make_shared<ChangeNotifier>(*this);
        }

        InternalContext ctx;
        ctx.owner = context;

        // コンテキストからウィンドウを取得
        auto window = context->GetWindow();
        // HWND は WindowsWindow から取得 (Phase 6 で m_hwnd を公開済み)
        // ここでは nullptr を設定し、ActivateContext で取得する
        ctx.hWnd = window ? static_cast<HWND>(window->GetOSWindowHandle()) : nullptr;

        m_contexts.push_back(std::move(ctx));

        return std::make_shared<ChangeNotifier>(*this);
    }

    void WindowsTextInputMethodSystem::UnregisterContext(const std::shared_ptr<ITextInputMethodContext>& context)
    {
        auto it = std::find_if(
            m_contexts.begin(), m_contexts.end(), [&](const InternalContext& c) { return c.owner == context; });
        if (it != m_contexts.end())
        {
            auto const erasedIndex = static_cast<int32_t>(std::distance(m_contexts.begin(), it));
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

    void WindowsTextInputMethodSystem::ActivateContext(const std::shared_ptr<ITextInputMethodContext>& context)
    {
        auto it = std::find_if(
            m_contexts.begin(), m_contexts.end(), [&](const InternalContext& c) { return c.owner == context; });
        if (it != m_contexts.end())
        {
            m_activeContextIndex = static_cast<int32_t>(std::distance(m_contexts.begin(), it));
            auto* activeCtx = GetActiveContext();

            if ((activeCtx != nullptr) && (activeCtx->hWnd != nullptr))
            {
                HIMC hImc = ::ImmGetContext(activeCtx->hWnd);
                if (hImc != nullptr)
                {
                    ::ImmSetOpenStatus(hImc, TRUE);
                    ::ImmReleaseContext(activeCtx->hWnd, hImc);
                }
            }
        }
    }

    void WindowsTextInputMethodSystem::DeactivateContext(const std::shared_ptr<ITextInputMethodContext>& context)
    {
        auto* activeCtx = GetActiveContext();
        if ((activeCtx != nullptr) && activeCtx->owner == context)
        {
            // コンポジション中なら確定させる
            if (activeCtx->bIsComposing && (activeCtx->hWnd != nullptr))
            {
                HIMC hImc = ::ImmGetContext(activeCtx->hWnd);
                if (hImc != nullptr)
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
        HWND hWnd, uint32_t msg, WPARAM /*wParam*/, LPARAM lParam, int32_t& outResult)
    {
        switch (msg)
        {
        case WM_IME_SETCONTEXT:
            // デフォルト処理に任せる
            return false;

        case WM_IME_STARTCOMPOSITION:
            HandleIMECompositionStart(hWnd);
            outResult = 0;
            return true;

        case WM_IME_COMPOSITION:
            HandleIMEComposition(hWnd, lParam);
            outResult = 0;
            return true;

        case WM_IME_ENDCOMPOSITION:
            HandleIMECompositionEnd(hWnd);
            outResult = 0;
            return true;

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
        if (ctx != nullptr)
        {
            ctx->hWnd = hWnd;
            ctx->bIsComposing = true;
            ctx->owner->BeginComposition();
        }
    }

    void WindowsTextInputMethodSystem::HandleIMEComposition(HWND hWnd, LPARAM lParam)
    {
        auto* ctx = GetActiveContext();
        if (ctx == nullptr)
        {
            return;
        }

        HIMC hImc = ::ImmGetContext(hWnd);
        if (hImc == nullptr)
        {
            return;
        }

        if ((lParam & GCS_COMPSTR) != 0)
        {
            // 2-call パターン: 最初にサイズ取得、次にデータ取得
            LONG const bytes = ::ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
            if (bytes > 0)
            {
                std::vector<wchar_t> buf((bytes / sizeof(wchar_t)) + 1, L'\0');
                ::ImmGetCompositionStringW(hImc, GCS_COMPSTR, buf.data(), bytes);

                // コンポジション範囲を更新
                auto const len = static_cast<int32_t>(wcslen(buf.data()));
                ctx->owner->UpdateCompositionRange(0, len);
            }
        }

        if ((lParam & GCS_RESULTSTR) != 0)
        {
            // 確定文字列
            LONG const bytes = ::ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
            if (bytes > 0)
            {
                std::vector<wchar_t> buf((bytes / sizeof(wchar_t)) + 1, L'\0');
                ::ImmGetCompositionStringW(hImc, GCS_RESULTSTR, buf.data(), bytes);

                ctx->owner->InsertTextAtCursor(buf.data());
            }
        }

        // 候補ウィンドウ位置更新
        UpdateCandidateWindowPosition(hWnd);

        ::ImmReleaseContext(hWnd, hImc);
    }

    void WindowsTextInputMethodSystem::HandleIMECompositionEnd(HWND /*hWnd*/)
    {
        auto* ctx = GetActiveContext();
        if (ctx != nullptr)
        {
            ctx->bIsComposing = false;
            ctx->owner->EndComposition();
        }
    }

    void WindowsTextInputMethodSystem::UpdateCandidateWindowPosition(HWND hWnd)
    {
        auto* ctx = GetActiveContext();
        if (ctx == nullptr)
        {
            return;
        }

        // テキスト境界からIME候補ウィンドウ位置を計算
        PlatformRect bounds = {};
        int32_t selBegin = 0;
        int32_t selLen = 0;
        CaretPosition caret = CaretPosition::Ending;
        ctx->owner->GetSelectionRange(selBegin, selLen, caret);
        ctx->owner->GetTextBounds(selBegin, selLen > 0 ? selLen : 1, bounds);

        HIMC hImc = ::ImmGetContext(hWnd);
        if (hImc != nullptr)
        {
            // CFS_EXCLUDE: 候補ウィンドウを指定矩形外に配置
            CANDIDATEFORM cf = {};
            cf.dwIndex = 0;
            cf.dwStyle = CFS_EXCLUDE;
            cf.ptCurrentPos.x = bounds.left;
            cf.ptCurrentPos.y = bounds.bottom;
            cf.rcArea.left = bounds.left;
            cf.rcArea.top = bounds.top;
            cf.rcArea.right = bounds.right;
            cf.rcArea.bottom = bounds.bottom;

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
