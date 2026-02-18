/// @file GenericWindow.h
/// @brief ウィンドウ基底クラスインターフェース
#pragma once

#include "GenericPlatform/GenericWindowDefinition.h"
#include <cstdint>
#include <memory>

namespace NS
{
    /// ウィンドウ基底クラス
    class GenericWindow
    {
    public:
        GenericWindow();
        virtual ~GenericWindow() = default;
        NS_DISALLOW_COPY_AND_MOVE(GenericWindow);

    public:
        // ジオメトリ
        virtual void ReshapeWindow(int32_t x, int32_t y, int32_t width, int32_t height);
        virtual void MoveWindowTo(int32_t x, int32_t y);
        virtual bool GetFullScreenInfo(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const;
        virtual bool GetRestoredDimensions(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const;
        virtual void AdjustCachedSize(PlatformRect& size) const;

        // ライフサイクル
        virtual void Destroy();

        // 状態管理
        virtual void SetWindowMode(WindowMode::Type newWindowMode);
        [[nodiscard]] virtual WindowMode::Type GetWindowMode() const;
        virtual void Show();
        virtual void Hide();
        virtual void Minimize();
        virtual void Maximize();
        virtual void Restore();
        virtual void BringToFront(bool bForce = false);
        virtual void HackForceToFront();
        virtual void SetWindowFocus();
        virtual void Enable(bool bEnable);
        [[nodiscard]] virtual bool IsEnabled() const;

        // プロパティ
        virtual void SetOpacity(float inOpacity);
        virtual void SetText(const NS::TCHAR* inText);
        [[nodiscard]] virtual float GetDPIScaleFactor() const;
        virtual void SetDPIScaleFactor(float value);
        [[nodiscard]] virtual bool IsManualManageDPIChanges() const;
        virtual void SetManualManageDPIChanges(bool bAutoHandle);
        [[nodiscard]] virtual int32_t GetWindowBorderSize() const;
        [[nodiscard]] virtual int32_t GetWindowTitleBarSize() const;
        [[nodiscard]] virtual void* GetOSWindowHandle() const;
        virtual void DrawAttention(const WindowDrawAttentionParameters& parameters);
        virtual void SetNativeWindowButtonsVisibility(bool bVisible);

        // クエリ
        [[nodiscard]] virtual bool IsMaximized() const;
        [[nodiscard]] virtual bool IsMinimized() const;
        [[nodiscard]] virtual bool IsVisible() const;
        [[nodiscard]] virtual bool IsForegroundWindow() const;
        [[nodiscard]] virtual bool IsFullscreenSupported() const;
        [[nodiscard]] virtual bool IsPointInWindow(int32_t x, int32_t y) const;
        [[nodiscard]] virtual bool IsDefinitionValid() const;
        [[nodiscard]] virtual const GenericWindowDefinition& GetDefinition() const;

        // ファクトリ
        static std::shared_ptr<GenericWindow> MakeNullWindow();

    protected:
        GenericWindowDefinition m_definition;
        WindowMode::Type m_windowMode = WindowMode::Windowed;
        float m_dpiScaleFactor = 1.0F;
    };

} // namespace NS
