/// @file GenericApplicationMessageHandler.h
/// @brief アプリケーションメッセージハンドラ
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/InputTypes.h"
#include "ApplicationCore/ModifierKeysState.h"
#include <memory>
#include <string>
#include <vector>

namespace NS
{
    class GenericWindow;

    /// アプリケーションとウィンドウシステム間のイベントブリッジ
    class GenericApplicationMessageHandler
    {
    public:
        virtual ~GenericApplicationMessageHandler() = default;

        virtual bool ShouldProcessUserInputMessages(const std::shared_ptr<GenericWindow>& Window) const;

        // =================================================================
        // キーボード (04-01)
        // =================================================================
        virtual bool OnKeyChar(NS::TCHAR Character, bool IsRepeat);
        virtual bool OnKeyDown(int32_t KeyCode, uint32_t CharacterCode, bool IsRepeat);
        virtual bool OnKeyUp(int32_t KeyCode, uint32_t CharacterCode, bool IsRepeat);
        virtual void OnInputLanguageChanged();

        // =================================================================
        // マウスボタン (04-01)
        // =================================================================
        virtual bool OnMouseDown(const std::shared_ptr<GenericWindow>& Window, MouseButtons::Type Button);
        virtual bool OnMouseDown(const std::shared_ptr<GenericWindow>& Window,
                                 MouseButtons::Type Button,
                                 const Vector2D& CursorPos);
        virtual bool OnMouseUp(MouseButtons::Type Button);
        virtual bool OnMouseUp(MouseButtons::Type Button, const Vector2D& CursorPos);
        virtual bool OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& Window, MouseButtons::Type Button);
        virtual bool OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& Window,
                                        MouseButtons::Type Button,
                                        const Vector2D& CursorPos);
        virtual bool OnMouseWheel(float Delta);
        virtual bool OnMouseWheel(float Delta, const Vector2D& CursorPos);

        // =================================================================
        // マウス移動・カーソル (04-01)
        // =================================================================
        virtual bool OnMouseMove(const Vector2D& CursorPos);
        virtual bool OnRawMouseMove(int32_t X, int32_t Y);
        virtual bool OnCursorSet();
        virtual void SetCursorPos(const Vector2D& Position);

        // =================================================================
        // ゲームパッド (04-02)
        // =================================================================
        virtual bool OnControllerAnalog(const NS::TCHAR* KeyName,
                                        PlatformUserId UserId,
                                        InputDeviceId DeviceId,
                                        float AnalogValue);
        virtual bool OnControllerButtonPressed(const NS::TCHAR* KeyName,
                                               PlatformUserId UserId,
                                               InputDeviceId DeviceId,
                                               bool IsRepeat);
        virtual bool OnControllerButtonReleased(const NS::TCHAR* KeyName,
                                                PlatformUserId UserId,
                                                InputDeviceId DeviceId,
                                                bool IsRepeat);

        // =================================================================
        // タッチ (04-02)
        // =================================================================
        virtual bool OnTouchStarted(const std::shared_ptr<GenericWindow>& Window,
                                    const Vector2D& Location,
                                    float Force,
                                    int32_t TouchIndex,
                                    PlatformUserId UserId,
                                    InputDeviceId DeviceId);
        virtual bool OnTouchMoved(
            const Vector2D& Location, float Force, int32_t TouchIndex, PlatformUserId UserId, InputDeviceId DeviceId);
        virtual bool OnTouchEnded(const Vector2D& Location,
                                  int32_t TouchIndex,
                                  PlatformUserId UserId,
                                  InputDeviceId DeviceId);
        virtual bool OnTouchForceChanged(
            const Vector2D& Location, float Force, int32_t TouchIndex, PlatformUserId UserId, InputDeviceId DeviceId);
        virtual bool OnTouchFirstMove(
            const Vector2D& Location, float Force, int32_t TouchIndex, PlatformUserId UserId, InputDeviceId DeviceId);

        // =================================================================
        // ジェスチャ (04-02)
        // =================================================================
        virtual void OnBeginGesture();
        virtual bool OnTouchGesture(GestureEvent GestureType,
                                    const Vector2D& Delta,
                                    float WheelDelta,
                                    bool bIsDirectionInvertedFromDevice);
        virtual void OnEndGesture();
        virtual bool ShouldSimulateGesture(GestureEvent GestureType, bool bEnable);

        // =================================================================
        // モーション (04-02)
        // =================================================================
        virtual bool OnMotionDetected(const Vector3D& Tilt,
                                      const Vector3D& RotationRate,
                                      const Vector3D& Gravity,
                                      const Vector3D& Acceleration,
                                      PlatformUserId UserId,
                                      InputDeviceId DeviceId);

        // =================================================================
        // ウィンドウイベント (04-03)
        // =================================================================
        virtual void OnSizeChanged(const std::shared_ptr<GenericWindow>& Window,
                                   int32_t Width,
                                   int32_t Height,
                                   bool bWasMinimized);
        virtual void OnMovedWindow(const std::shared_ptr<GenericWindow>& Window, int32_t X, int32_t Y);
        virtual bool OnWindowActivationChanged(const std::shared_ptr<GenericWindow>& Window,
                                               WindowActivation::Type ActivationType);
        virtual void OnApplicationActivationChanged(bool IsActive);
        virtual void OnWindowClose(const std::shared_ptr<GenericWindow>& Window);
        virtual void OnOSPaint(const std::shared_ptr<GenericWindow>& Window);

        // =================================================================
        // リシェイプ・DPI (04-03)
        // =================================================================
        virtual void BeginReshapingWindow(const std::shared_ptr<GenericWindow>& Window);
        virtual void FinishedReshapingWindow(const std::shared_ptr<GenericWindow>& Window);
        virtual void OnResizingWindow(const std::shared_ptr<GenericWindow>& Window);
        virtual void HandleDPIScaleChanged(const std::shared_ptr<GenericWindow>& Window);
        virtual void SignalSystemDPIChanged(const std::shared_ptr<GenericWindow>& Window);

        // =================================================================
        // ゾーン・サイズ・アクション (04-03)
        // =================================================================
        virtual WindowZone::Type GetWindowZoneForPoint(const std::shared_ptr<GenericWindow>& Window,
                                                       int32_t X,
                                                       int32_t Y) const;
        virtual WindowSizeLimits GetSizeLimitsForWindow(const std::shared_ptr<GenericWindow>& Window) const;
        virtual void OnWindowAction(const std::shared_ptr<GenericWindow>& Window, WindowAction ActionType);

        // =================================================================
        // DnD (04-03)
        // =================================================================
        virtual DropEffect::Type OnDragEnterText(const std::shared_ptr<GenericWindow>& Window, const NS::TCHAR* Text);
        virtual DropEffect::Type OnDragEnterFiles(const std::shared_ptr<GenericWindow>& Window,
                                                  const std::vector<std::wstring>& Files);
        virtual DropEffect::Type OnDragEnterExternal(const std::shared_ptr<GenericWindow>& Window,
                                                     const NS::TCHAR* Text,
                                                     const std::vector<std::wstring>& Files);
        virtual DropEffect::Type OnDragOver(const std::shared_ptr<GenericWindow>& Window);
        virtual void OnDragLeave(const std::shared_ptr<GenericWindow>& Window);
        virtual DropEffect::Type OnDragDrop(const std::shared_ptr<GenericWindow>& Window);

        // =================================================================
        // その他 (04-03)
        // =================================================================
        virtual void OnConvertibleLaptopModeChanged();
        virtual bool ShouldUsePlatformUserId();
    };

} // namespace NS
