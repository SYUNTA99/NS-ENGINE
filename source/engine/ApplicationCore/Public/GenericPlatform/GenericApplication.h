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
        std::wstring Name;
        std::wstring ID;
        int32_t NativeWidth = 0;
        int32_t NativeHeight = 0;
        int32_t MaxResolutionWidth = 0;
        int32_t MaxResolutionHeight = 0;
        PlatformRect DisplayRect;
        PlatformRect WorkArea;
        bool bIsPrimary = false;
        int32_t DPI = 96;
    };

    // =========================================================================
    // DisplayMetrics
    // =========================================================================

    /// ディスプレイメトリクス
    struct DisplayMetrics
    {
        int32_t PrimaryDisplayWidth = 0;
        int32_t PrimaryDisplayHeight = 0;
        std::vector<MonitorInfo> MonitorInfoArray;
        PlatformRect PrimaryDisplayWorkAreaRect;
        PlatformRect VirtualDisplayRect;
        Vector4 TitleSafePaddingSize; // X=left, Y=top, Z=right, W=bottom
        Vector4 ActionSafePaddingSize;

        /// メトリクスを再構築
        static void RebuildDisplayMetrics(DisplayMetrics& OutMetrics);

        /// 座標からモニターワークエリアを取得
        static PlatformRect GetMonitorWorkAreaFromPoint(int32_t X, int32_t Y);

        /// デバッグ用タイトルセーフゾーン比率 (0.0 = 無し, 1.0 = 最大)
        static float GetDebugTitleSafeZoneRatio() { return s_debugTitleSafeZoneRatio; }
        static void SetDebugTitleSafeZoneRatio(float Ratio) { s_debugTitleSafeZoneRatio = Ratio; }

        /// デバッグ用アクションセーフゾーン比率
        static float GetDebugActionSafeZoneRatio() { return s_debugActionSafeZoneRatio; }
        static void SetDebugActionSafeZoneRatio(float Ratio) { s_debugActionSafeZoneRatio = Ratio; }

        /// デフォルトセーフゾーンの適用
        static void ApplyDefaultSafeZones(DisplayMetrics& OutMetrics);

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

        Handle Add(Callback Fn)
        {
            Handle id = m_nextId++;
            m_listeners.push_back({id, std::move(Fn)});
            return id;
        }

        void Remove(Handle InHandle)
        {
            m_listeners.erase(std::remove_if(m_listeners.begin(),
                                             m_listeners.end(),
                                             [InHandle](const Entry& e) { return e.Id == InHandle; }),
                              m_listeners.end());
        }

        void Broadcast(Args... InArgs) const
        {
            // スナップショットで反復（Broadcast中の Add/Remove に対して安全）
            auto snapshot = m_listeners;
            for (const auto& entry : snapshot)
            {
                entry.Fn(InArgs...);
            }
        }

        bool IsBound() const { return !m_listeners.empty(); }

    private:
        struct Entry
        {
            Handle Id;
            Callback Fn;
        };
        std::vector<Entry> m_listeners;
        Handle m_nextId = 1;
    };

    // =========================================================================
    // GenericApplication
    // =========================================================================

    /// アプリケーション基底クラス
    class GenericApplication
    {
    public:
        explicit GenericApplication(const std::shared_ptr<ICursor>& InCursor);
        virtual ~GenericApplication() = default;

        // =================================================================
        // メッセージハンドラ
        // =================================================================
        void SetMessageHandler(const SharedRef<GenericApplicationMessageHandler>& InMessageHandler);
        const SharedRef<GenericApplicationMessageHandler>& GetMessageHandler() const;

        // =================================================================
        // メッセージ処理
        // =================================================================
        virtual void PumpMessages(float TimeDelta);
        virtual void PollGameDeviceState(float TimeDelta);
        virtual void ProcessDeferredEvents(float TimeDelta);
        virtual void Tick(float TimeDelta);

        // =================================================================
        // ウィンドウ管理
        // =================================================================
        virtual std::shared_ptr<GenericWindow> MakeWindow();
        virtual void InitializeWindow(const std::shared_ptr<GenericWindow>& Window,
                                      const GenericWindowDefinition& Definition,
                                      const std::shared_ptr<GenericWindow>& Parent,
                                      bool bShowImmediately);
        virtual void SetCapture(const std::shared_ptr<GenericWindow>& Window);
        virtual void* GetCapture() const;
        virtual std::shared_ptr<GenericWindow> GetWindowUnderCursor() const;

        // =================================================================
        // 入力状態
        // =================================================================
        virtual ModifierKeysState GetModifierKeys() const;
        virtual void SetHighPrecisionMouseMode(bool bEnable, const std::shared_ptr<GenericWindow>& Window);
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
        virtual void GetInitialDisplayMetrics(DisplayMetrics& OutMetrics) const;

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
        virtual PlatformRect GetWorkArea(const PlatformRect& CurrentWindow) const;
        virtual WindowTitleAlignment GetWindowTitleAlignment() const;
        virtual WindowTransparency GetWindowTransparencySupport() const;
        virtual bool TryCalculatePopupWindowPosition(const PlatformRect& InAnchor,
                                                     const Vector2D& InSize,
                                                     const PlatformRect& ProposedPlacement,
                                                     PopUpOrientation Orientation,
                                                     Vector2D& OutCalculatedPosition) const;

        // =================================================================
        // ライフサイクル
        // =================================================================
        virtual void DestroyApplication();
        virtual bool ApplicationLicenseValid() const;
        virtual bool IsAllowedToRender() const;
        virtual bool SupportsSystemHelp() const;
        virtual void ShowSystemHelp();
        virtual void SendAnalytics(IAnalyticsProvider* Provider);

        // =================================================================
        // Console
        // =================================================================
        using ConsoleCommandDelegate = std::function<void(const std::wstring&)>;
        virtual void RegisterConsoleCommandListener(ConsoleCommandDelegate Delegate);
        virtual void AddPendingConsoleCommand(const std::wstring& Command);

        // =================================================================
        // カーソル
        // =================================================================
        const std::shared_ptr<ICursor>& GetCursor() const { return m_cursor; }

    protected:
        void BroadcastDisplayMetricsChanged(const DisplayMetrics& InMetrics);

        SharedRef<GenericApplicationMessageHandler> m_messageHandler;

    private:
        std::shared_ptr<ICursor> m_cursor;
        SimpleEvent<const DisplayMetrics&> m_displayMetricsChangedEvent;
        SimpleEvent<const PlatformRect&> m_virtualKeyboardShownEvent;
        SimpleEvent<> m_virtualKeyboardHiddenEvent;
        SimpleEvent<> m_clipboardContentChangedEvent;
    };

} // namespace NS
