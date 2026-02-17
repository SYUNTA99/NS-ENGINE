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

        // ジオメトリ
        virtual void ReshapeWindow(int32_t X, int32_t Y, int32_t Width, int32_t Height);
        virtual void MoveWindowTo(int32_t X, int32_t Y);
        virtual bool GetFullScreenInfo(int32_t& X, int32_t& Y, int32_t& Width, int32_t& Height) const;
        virtual bool GetRestoredDimensions(int32_t& X, int32_t& Y, int32_t& Width, int32_t& Height) const;
        virtual void AdjustCachedSize(PlatformRect& Size) const;

        // ライフサイクル
        virtual void Destroy();

        // 状態管理
        virtual void SetWindowMode(WindowMode::Type NewWindowMode);
        virtual WindowMode::Type GetWindowMode() const;
        virtual void Show();
        virtual void Hide();
        virtual void Minimize();
        virtual void Maximize();
        virtual void Restore();
        virtual void BringToFront(bool bForce = false);
        virtual void HACK_ForceToFront();
        virtual void SetWindowFocus();
        virtual void Enable(bool bEnable);
        virtual bool IsEnabled() const;

        // プロパティ
        virtual void SetOpacity(float InOpacity);
        virtual void SetText(const NS::TCHAR* InText);
        virtual float GetDPIScaleFactor() const;
        virtual void SetDPIScaleFactor(float Value);
        virtual bool IsManualManageDPIChanges() const;
        virtual void SetManualManageDPIChanges(bool bAutoHandle);
        virtual int32_t GetWindowBorderSize() const;
        virtual int32_t GetWindowTitleBarSize() const;
        virtual void* GetOSWindowHandle() const;
        virtual void DrawAttention(const WindowDrawAttentionParameters& Parameters);
        virtual void SetNativeWindowButtonsVisibility(bool bVisible);

        // クエリ
        virtual bool IsMaximized() const;
        virtual bool IsMinimized() const;
        virtual bool IsVisible() const;
        virtual bool IsForegroundWindow() const;
        virtual bool IsFullscreenSupported() const;
        virtual bool IsPointInWindow(int32_t X, int32_t Y) const;
        virtual bool IsDefinitionValid() const;
        virtual const GenericWindowDefinition& GetDefinition() const;

        // ファクトリ
        static std::shared_ptr<GenericWindow> MakeNullWindow();

    protected:
        GenericWindowDefinition m_definition;
        WindowMode::Type m_windowMode = WindowMode::Windowed;
        float m_dpiScaleFactor = 1.0f;
    };

} // namespace NS
