/// @file WindowsTextInputMethodSystem.h
/// @brief Windows IME (IMM32) 実装ヘッダー (11-03)
#pragma once

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

#include "GenericPlatform/ITextInputMethodSystem.h"

#include <memory>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "imm32.lib")

namespace NS
{
    /// Windows IME API タイプ
    enum class WindowsIMEAPI : uint8_t
    {
        Unknown,
        IMM, // IMM32
        TSF  // Text Services Framework (将来)
    };

    // =========================================================================
    // WindowsTextInputMethodSystem
    // =========================================================================

    /// Windows IMM32 テキスト入力メソッドシステム
    class WindowsTextInputMethodSystem : public ITextInputMethodSystem
    {
    public:
        WindowsTextInputMethodSystem();
        ~WindowsTextInputMethodSystem() override;

        // =================================================================
        // ITextInputMethodSystem overrides
        // =================================================================
        std::shared_ptr<ITextInputMethodChangeNotifier> RegisterContext(
            const std::shared_ptr<ITextInputMethodContext>& Context) override;
        void UnregisterContext(const std::shared_ptr<ITextInputMethodContext>& Context) override;
        void ActivateContext(const std::shared_ptr<ITextInputMethodContext>& Context) override;
        void DeactivateContext(const std::shared_ptr<ITextInputMethodContext>& Context) override;

        // =================================================================
        // Windows メッセージ処理
        // =================================================================

        /// WM_IME_* メッセージをルーティング
        /// @return true: メッセージ処理済み
        bool ProcessMessage(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam, int32_t& OutResult);

    private:
        /// 内部コンテキスト
        struct InternalContext
        {
            std::shared_ptr<ITextInputMethodContext> Owner;
            HWND hWnd = nullptr;
            bool bIsComposing = false;
        };

        /// 変更通知実装
        class ChangeNotifier : public ITextInputMethodChangeNotifier
        {
        public:
            explicit ChangeNotifier(WindowsTextInputMethodSystem& InOwner) : m_owner(InOwner) {}
            void NotifyLayoutChanged() override;
            void NotifySelectionChanged() override;
            void NotifyTextChanged() override;
            void CancelComposition() override;

        private:
            WindowsTextInputMethodSystem& m_owner;
        };

        // IMM32 ハンドラ
        void HandleIMECompositionStart(HWND hWnd);
        void HandleIMEComposition(HWND hWnd, LPARAM lParam);
        void HandleIMECompositionEnd(HWND hWnd);
        void UpdateCandidateWindowPosition(HWND hWnd);

        // アクティブコンテキスト
        InternalContext* FindContextByHWND(HWND hWnd);

        InternalContext* GetActiveContext();
        const InternalContext* GetActiveContext() const;

        std::vector<InternalContext> m_contexts;
        int32_t m_activeContextIndex = -1; // -1 = no active context
        WindowsIMEAPI m_currentAPI = WindowsIMEAPI::IMM;
    };

} // namespace NS
