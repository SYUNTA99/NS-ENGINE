/// @file GenericWindow.cpp
/// @brief GenericWindowデフォルト実装
#include "GenericPlatform/GenericWindow.h"

namespace NS
{
    GenericWindow::GenericWindow() : m_windowMode(WindowMode::Windowed), m_dpiScaleFactor(1.0f) {}

    // ジオメトリ
    void GenericWindow::ReshapeWindow(int32_t, int32_t, int32_t, int32_t) {}
    void GenericWindow::MoveWindowTo(int32_t, int32_t) {}
    bool GenericWindow::GetFullScreenInfo(int32_t&, int32_t&, int32_t&, int32_t&) const
    {
        return false;
    }
    bool GenericWindow::GetRestoredDimensions(int32_t&, int32_t&, int32_t&, int32_t&) const
    {
        return false;
    }
    void GenericWindow::AdjustCachedSize(PlatformRect&) const {}

    // ライフサイクル
    void GenericWindow::Destroy() {}

    // 状態管理
    void GenericWindow::SetWindowMode(WindowMode::Type NewWindowMode)
    {
        m_windowMode = NewWindowMode;
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
    void GenericWindow::BringToFront(bool) {}
    void GenericWindow::HACK_ForceToFront() {}
    void GenericWindow::SetWindowFocus() {}
    void GenericWindow::Enable(bool) {}
    bool GenericWindow::IsEnabled() const
    {
        return false;
    }

    // プロパティ
    void GenericWindow::SetOpacity(float) {}
    void GenericWindow::SetText(const NS::TCHAR*) {}
    float GenericWindow::GetDPIScaleFactor() const
    {
        return m_dpiScaleFactor;
    }
    void GenericWindow::SetDPIScaleFactor(float Value)
    {
        m_dpiScaleFactor = Value;
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
    void GenericWindow::DrawAttention(const WindowDrawAttentionParameters&) {}
    void GenericWindow::SetNativeWindowButtonsVisibility(bool) {}

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
    bool GenericWindow::IsPointInWindow(int32_t, int32_t) const
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
