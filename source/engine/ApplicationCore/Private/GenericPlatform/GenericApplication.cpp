/// @file GenericApplication.cpp
/// @brief GenericApplication デフォルト実装
#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/NullApplication.h"

namespace NS
{
    // =========================================================================
    // DisplayMetrics static members
    // =========================================================================

    float DisplayMetrics::s_debugTitleSafeZoneRatio = 0.0f;
    float DisplayMetrics::s_debugActionSafeZoneRatio = 0.0f;

    // =========================================================================
    // コンストラクタ
    // =========================================================================

    GenericApplication::GenericApplication(const std::shared_ptr<ICursor>& InCursor)
        : m_messageHandler(
              SharedRef<GenericApplicationMessageHandler>(std::make_shared<GenericApplicationMessageHandler>())),
          m_cursor(InCursor ? InCursor : std::make_shared<NullCursor>())
    {}

    // =========================================================================
    // メッセージハンドラ
    // =========================================================================

    void GenericApplication::SetMessageHandler(const SharedRef<GenericApplicationMessageHandler>& InMessageHandler)
    {
        m_messageHandler = InMessageHandler;
    }

    const SharedRef<GenericApplicationMessageHandler>& GenericApplication::GetMessageHandler() const
    {
        return m_messageHandler;
    }

    // =========================================================================
    // メッセージ処理
    // =========================================================================

    void GenericApplication::PumpMessages(float /*TimeDelta*/) {}
    void GenericApplication::PollGameDeviceState(float /*TimeDelta*/) {}
    void GenericApplication::ProcessDeferredEvents(float /*TimeDelta*/) {}
    void GenericApplication::Tick(float /*TimeDelta*/) {}

    // =========================================================================
    // ウィンドウ管理
    // =========================================================================

    std::shared_ptr<GenericWindow> GenericApplication::MakeWindow()
    {
        return GenericWindow::MakeNullWindow();
    }

    void GenericApplication::InitializeWindow(const std::shared_ptr<GenericWindow>& /*Window*/,
                                              const GenericWindowDefinition& /*Definition*/,
                                              const std::shared_ptr<GenericWindow>& /*Parent*/,
                                              bool /*bShowImmediately*/)
    {}

    void GenericApplication::SetCapture(const std::shared_ptr<GenericWindow>& /*Window*/) {}

    void* GenericApplication::GetCapture() const
    {
        return nullptr;
    }

    std::shared_ptr<GenericWindow> GenericApplication::GetWindowUnderCursor() const
    {
        return nullptr;
    }

    // =========================================================================
    // 入力状態
    // =========================================================================

    ModifierKeysState GenericApplication::GetModifierKeys() const
    {
        return ModifierKeysState();
    }

    void GenericApplication::SetHighPrecisionMouseMode(bool /*bEnable*/,
                                                       const std::shared_ptr<GenericWindow>& /*Window*/)
    {}

    bool GenericApplication::IsMouseAttached() const
    {
        return true;
    }
    bool GenericApplication::IsGamepadAttached() const
    {
        return false;
    }
    bool GenericApplication::IsCursorDirectlyOverSlateWindow() const
    {
        return false;
    }
    void GenericApplication::FinishedInputThisFrame() {}
    bool GenericApplication::IsUsingHighPrecisionMouseMode() const
    {
        return false;
    }
    bool GenericApplication::IsUsingTrackpad() const
    {
        return false;
    }
    bool GenericApplication::IsMinimized() const
    {
        return false;
    }

    // =========================================================================
    // サブシステム
    // =========================================================================

    IInputInterface* GenericApplication::GetInputInterface()
    {
        return nullptr;
    }

    ITextInputMethodSystem* GenericApplication::GetTextInputMethodSystem()
    {
        return nullptr;
    }

    void GenericApplication::GetInitialDisplayMetrics(DisplayMetrics& OutMetrics) const
    {
        DisplayMetrics::RebuildDisplayMetrics(OutMetrics);
    }

    // =========================================================================
    // ユーティリティ
    // =========================================================================

    PlatformRect GenericApplication::GetWorkArea(const PlatformRect& /*CurrentWindow*/) const
    {
        return PlatformRect();
    }

    WindowTitleAlignment GenericApplication::GetWindowTitleAlignment() const
    {
        return WindowTitleAlignment::Left;
    }

    WindowTransparency GenericApplication::GetWindowTransparencySupport() const
    {
        return WindowTransparency::None;
    }

    bool GenericApplication::TryCalculatePopupWindowPosition(const PlatformRect& /*InAnchor*/,
                                                             const Vector2D& /*InSize*/,
                                                             const PlatformRect& /*ProposedPlacement*/,
                                                             PopUpOrientation /*Orientation*/,
                                                             Vector2D& /*OutCalculatedPosition*/) const
    {
        return false;
    }

    // =========================================================================
    // ライフサイクル
    // =========================================================================

    void GenericApplication::DestroyApplication() {}
    bool GenericApplication::ApplicationLicenseValid() const
    {
        return true;
    }
    bool GenericApplication::IsAllowedToRender() const
    {
        return true;
    }
    bool GenericApplication::SupportsSystemHelp() const
    {
        return false;
    }
    void GenericApplication::ShowSystemHelp() {}
    void GenericApplication::SendAnalytics(IAnalyticsProvider* /*Provider*/) {}

    // =========================================================================
    // Console
    // =========================================================================

    void GenericApplication::RegisterConsoleCommandListener(ConsoleCommandDelegate /*Delegate*/) {}
    void GenericApplication::AddPendingConsoleCommand(const std::wstring& /*Command*/) {}

    // =========================================================================
    // Protected
    // =========================================================================

    void GenericApplication::BroadcastDisplayMetricsChanged(const DisplayMetrics& InMetrics)
    {
        m_displayMetricsChangedEvent.Broadcast(InMetrics);
    }

} // namespace NS
