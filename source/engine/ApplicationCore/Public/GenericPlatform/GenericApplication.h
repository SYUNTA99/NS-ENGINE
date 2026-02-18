/// @file GenericApplication.h
/// @brief アプリケーション基底クラス
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/ModifierKeysState.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/GenericWindow.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace NS
{
    // 前方宣言（未実装のサブシステム）
    class ICursor;
    class IInputInterface;
    class ITextInputMethodSystem;
    class IAnalyticsProvider;

    // =========================================================================
    // MonitorInfo
    // =========================================================================

    /// モニター情報
    struct MonitorInfo
    {
        std::wstring name;
        std::wstring id;
        int32_t nativeWidth = 0;
        int32_t nativeHeight = 0;
        int32_t maxResolutionWidth = 0;
        int32_t maxResolutionHeight = 0;
        PlatformRect displayRect;
        PlatformRect workArea;
        bool bIsPrimary = false;
        int32_t dpi = 96;
    };

    // =========================================================================
    // DisplayMetrics
    // =========================================================================

    /// ディスプレイメトリクス
    struct DisplayMetrics
    {
        int32_t primaryDisplayWidth = 0;
        int32_t primaryDisplayHeight = 0;
        std::vector<MonitorInfo> monitorInfoArray;
        PlatformRect primaryDisplayWorkAreaRect;
        PlatformRect virtualDisplayRect;
        Vector4 titleSafePaddingSize; // X=left, Y=top, Z=right, W=bottom
        Vector4 actionSafePaddingSize;

        /// メトリクスを再構築
        static void RebuildDisplayMetrics(DisplayMetrics& outMetrics);

        /// 座標からモニターワークエリアを取得
        static PlatformRect GetMonitorWorkAreaFromPoint(int32_t x, int32_t y);

        /// デバッグ用タイトルセーフゾーン比率 (0.0 = 無し, 1.0 = 最大)
        static float GetDebugTitleSafeZoneRatio() { return s_debugTitleSafeZoneRatio; }
        static void SetDebugTitleSafeZoneRatio(float ratio) { s_debugTitleSafeZoneRatio = ratio; }

        /// デバッグ用アクションセーフゾーン比率
        static float GetDebugActionSafeZoneRatio() { return s_debugActionSafeZoneRatio; }
        static void SetDebugActionSafeZoneRatio(float ratio) { s_debugActionSafeZoneRatio = ratio; }

        /// デフォルトセーフゾーンの適用
        static void ApplyDefaultSafeZones(DisplayMetrics& outMetrics);

    private:
        static float s_debugTitleSafeZoneRatio;
        static float s_debugActionSafeZoneRatio;
    };

    // =========================================================================
    // SimpleEvent — 軽量マルチキャストデリゲート
    // =========================================================================

    template <typename... Args> class SimpleEvent
    {
    public:
        using Callback = std::function<void(Args...)>;
        using Handle = uint64_t;

        Handle Add(Callback fn)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            Handle id = m_nextId++;
            m_listeners.push_back({id, std::move(fn)});
            return id;
        }

        void Remove(Handle inHandle)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_listeners.erase(std::remove_if(m_listeners.begin(),
                                             m_listeners.end(),
                                             [inHandle](const Entry& e) { return e.id == inHandle; }),
                              m_listeners.end());
        }

        void Broadcast(Args... inArgs) const
        {
            std::vector<Entry> snapshot;
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                snapshot = m_listeners;
            }
            for (const auto& entry : snapshot)
            {
                entry.fn(inArgs...);
            }
        }

        bool IsBound() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return !m_listeners.empty();
        }

    private:
        struct Entry
        {
            Handle id;
            Callback fn;
        };
        std::vector<Entry> m_listeners;
        Handle m_nextId = 1;
        mutable std::mutex m_mutex;
    };

    // =========================================================================
    // GenericApplication
    // =========================================================================

    /// アプリケーション基底クラス
    class GenericApplication
    {
    public:
        explicit GenericApplication(const std::shared_ptr<ICursor>& inCursor);
        virtual ~GenericApplication() = default;
        NS_DISALLOW_COPY_AND_MOVE(GenericApplication);

    public:
        // =================================================================
        // メッセージハンドラ
        // =================================================================
        void SetMessageHandler(const SharedRef<GenericApplicationMessageHandler>& inMessageHandler);
        const SharedRef<GenericApplicationMessageHandler>& GetMessageHandler() const;

        // =================================================================
        // メッセージ処理
        // =================================================================
        virtual void PumpMessages(float timeDelta);
        virtual void PollGameDeviceState(float timeDelta);
        virtual void ProcessDeferredEvents(float timeDelta);
        virtual void Tick(float timeDelta);

        // =================================================================
        // ウィンドウ管理
        // =================================================================
        virtual std::shared_ptr<GenericWindow> MakeWindow();
        virtual void InitializeWindow(const std::shared_ptr<GenericWindow>& window,
                                      const GenericWindowDefinition& definition,
                                      const std::shared_ptr<GenericWindow>& parent,
                                      bool bShowImmediately);
        virtual void SetCapture(const std::shared_ptr<GenericWindow>& window);
        virtual void* GetCapture() const;
        virtual std::shared_ptr<GenericWindow> GetWindowUnderCursor() const;

        // =================================================================
        // 入力状態
        // =================================================================
        virtual ModifierKeysState GetModifierKeys() const;
        virtual void SetHighPrecisionMouseMode(bool bEnable, const std::shared_ptr<GenericWindow>& window);
        virtual bool IsMouseAttached() const;
        virtual bool IsGamepadAttached() const;
        virtual bool IsCursorDirectlyOverSlateWindow() const;
        virtual void FinishedInputThisFrame();
        virtual bool IsUsingHighPrecisionMouseMode() const;
        virtual bool IsUsingTrackpad() const;
        virtual bool IsMinimized() const;

        // =================================================================
        // サブシステム
        // =================================================================
        virtual IInputInterface* GetInputInterface();
        virtual ITextInputMethodSystem* GetTextInputMethodSystem();
        virtual void GetInitialDisplayMetrics(DisplayMetrics& outMetrics) const;

        // =================================================================
        // イベント
        // =================================================================
        SimpleEvent<const DisplayMetrics&>& OnDisplayMetricsChanged() { return m_displayMetricsChangedEvent; }
        SimpleEvent<const PlatformRect&>& OnVirtualKeyboardShown() { return m_virtualKeyboardShownEvent; }
        SimpleEvent<>& OnVirtualKeyboardHidden() { return m_virtualKeyboardHiddenEvent; }
        SimpleEvent<>& OnClipboardContentChanged() { return m_clipboardContentChangedEvent; }

        // =================================================================
        // ユーティリティ・クエリ
        // =================================================================
        virtual PlatformRect GetWorkArea(const PlatformRect& currentWindow) const;
        virtual WindowTitleAlignment GetWindowTitleAlignment() const;
        virtual WindowTransparency GetWindowTransparencySupport() const;
        virtual bool TryCalculatePopupWindowPosition(const PlatformRect& inAnchor,
                                                     const Vector2D& inSize,
                                                     const PlatformRect& proposedPlacement,
                                                     PopUpOrientation orientation,
                                                     Vector2D& outCalculatedPosition) const;

        // =================================================================
        // ライフサイクル
        // =================================================================
        virtual void DestroyApplication();
        virtual bool ApplicationLicenseValid() const;
        virtual bool IsAllowedToRender() const;
        virtual bool SupportsSystemHelp() const;
        virtual void ShowSystemHelp();
        virtual void SendAnalytics(IAnalyticsProvider* provider);

        // =================================================================
        // Console
        // =================================================================
        using ConsoleCommandDelegate = std::function<void(const std::wstring&)>;
        virtual void RegisterConsoleCommandListener(ConsoleCommandDelegate delegate);
        virtual void AddPendingConsoleCommand(const std::wstring& command);

        // =================================================================
        // カーソル
        // =================================================================
        const std::shared_ptr<ICursor>& GetCursor() const { return m_cursor; }

    protected:
        void BroadcastDisplayMetricsChanged(const DisplayMetrics& inMetrics);

        SharedRef<GenericApplicationMessageHandler> m_messageHandler;

    private:
        std::shared_ptr<ICursor> m_cursor;
        SimpleEvent<const DisplayMetrics&> m_displayMetricsChangedEvent;
        SimpleEvent<const PlatformRect&> m_virtualKeyboardShownEvent;
        SimpleEvent<> m_virtualKeyboardHiddenEvent;
        SimpleEvent<> m_clipboardContentChangedEvent;
    };

} // namespace NS
