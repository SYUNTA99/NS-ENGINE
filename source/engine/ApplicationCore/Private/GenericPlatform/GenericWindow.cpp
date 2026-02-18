/// @file GenericWindow.cpp
/// @brief GenericWindowデフォルト実装
#include "GenericPlatform/GenericWindow.h"

namespace NS
{
    GenericWindow::GenericWindow()  {}

    // ジオメトリ
    void GenericWindow::ReshapeWindow(int32_t /*unused*/, int32_t /*unused*/, int32_t /*unused*/, int32_t /*unused*/) {}
    void GenericWindow::MoveWindowTo(int32_t /*unused*/, int32_t /*unused*/) {}
    bool GenericWindow::GetFullScreenInfo(int32_t& /*unused*/, int32_t& /*unused*/, int32_t& /*unused*/, int32_t& /*unused*/) const
    {
        return false;
    }
    bool GenericWindow::GetRestoredDimensions(int32_t& /*unused*/, int32_t& /*unused*/, int32_t& /*unused*/, int32_t& /*unused*/) const
    {
        return false;
    }
    void GenericWindow::AdjustCachedSize(PlatformRect& /*unused*/) const {}

    // ライフサイクル
    void GenericWindow::Destroy() {}

    // 状態管理
    void GenericWindow::SetWindowMode(WindowMode::Type newWindowMode)
    {
        m_windowMode = newWindowMode;
    }
    WindowMode::Type GenericWindow::GetWindowMode() const
    {
        return m_windowMode;
    }
    void GenericWindow::Show() {}
    void GenericWindow::Hide() {}
    void GenericWindow::Minimize() {}
    void GenericWindow::Maximize() {}
    void GenericWindow::Restore() {}
    void GenericWindow::BringToFront(bool /*unused*/) {}
    void GenericWindow::HackForceToFront() {}
    void GenericWindow::SetWindowFocus() {}
    void GenericWindow::Enable(bool /*unused*/) {}
    bool GenericWindow::IsEnabled() const
    {
        return false;
    }

    // プロパティ
    void GenericWindow::SetOpacity(float /*unused*/) {}
    void GenericWindow::SetText(const NS::TCHAR* /*unused*/) {}
    float GenericWindow::GetDPIScaleFactor() const
    {
        return m_dpiScaleFactor;
    }
    void GenericWindow::SetDPIScaleFactor(float value)
    {
        m_dpiScaleFactor = value;
    }
    bool GenericWindow::IsManualManageDPIChanges() const
    {
        return m_definition.bManualDPI;
    }
    void GenericWindow::SetManualManageDPIChanges(bool bAutoHandle)
    {
        m_definition.bManualDPI = bAutoHandle;
    }
    int32_t GenericWindow::GetWindowBorderSize() const
    {
        return 0;
    }
    int32_t GenericWindow::GetWindowTitleBarSize() const
    {
        return 0;
    }
    void* GenericWindow::GetOSWindowHandle() const
    {
        return nullptr;
    }
    void GenericWindow::DrawAttention(const WindowDrawAttentionParameters& /*unused*/) {}
    void GenericWindow::SetNativeWindowButtonsVisibility(bool /*unused*/) {}

    // クエリ
    bool GenericWindow::IsMaximized() const
    {
        return false;
    }
    bool GenericWindow::IsMinimized() const
    {
        return false;
    }
    bool GenericWindow::IsVisible() const
    {
        return false;
    }
    bool GenericWindow::IsForegroundWindow() const
    {
        return false;
    }
    bool GenericWindow::IsFullscreenSupported() const
    {
        return false;
    }
    bool GenericWindow::IsPointInWindow(int32_t /*unused*/, int32_t /*unused*/) const
    {
        return false;
    }
    bool GenericWindow::IsDefinitionValid() const
    {
        return false;
    }
    const GenericWindowDefinition& GenericWindow::GetDefinition() const
    {
        return m_definition;
    }

    // ファクトリ
    std::shared_ptr<GenericWindow> GenericWindow::MakeNullWindow()
    {
        return std::make_shared<GenericWindow>();
    }

} // namespace NS
