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
        GenericApplicationMessageHandler() = default;
        virtual ~GenericApplicationMessageHandler() = default;
        NS_DISALLOW_COPY_AND_MOVE(GenericApplicationMessageHandler);

    public:
        [[nodiscard]] virtual bool ShouldProcessUserInputMessages(const std::shared_ptr<GenericWindow>& window) const;

        // =================================================================
        // キーボード (04-01)
        // =================================================================
        virtual bool OnKeyChar(NS::TCHAR character, bool isRepeat);
        virtual bool OnKeyDown(int32_t keyCode, uint32_t characterCode, bool isRepeat);
        virtual bool OnKeyUp(int32_t keyCode, uint32_t characterCode, bool isRepeat);
        virtual void OnInputLanguageChanged();

        // =================================================================
        // マウスボタン (04-01)
        // =================================================================
        virtual bool OnMouseDown(const std::shared_ptr<GenericWindow>& window, MouseButtons::Type button);
        virtual bool OnMouseDown(const std::shared_ptr<GenericWindow>& window,
                                 MouseButtons::Type button,
                                 const Vector2D& cursorPos);
        virtual bool OnMouseUp(MouseButtons::Type button);
        virtual bool OnMouseUp(MouseButtons::Type button, const Vector2D& cursorPos);
        virtual bool OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& window, MouseButtons::Type button);
        virtual bool OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& window,
                                        MouseButtons::Type button,
                                        const Vector2D& cursorPos);
        virtual bool OnMouseWheel(float delta);
        virtual bool OnMouseWheel(float delta, const Vector2D& cursorPos);

        // =================================================================
        // マウス移動・カーソル (04-01)
        // =================================================================
        virtual bool OnMouseMove(const Vector2D& cursorPos);
        virtual bool OnRawMouseMove(int32_t x, int32_t y);
        virtual bool OnCursorSet();
        virtual void SetCursorPos(const Vector2D& position);

        // =================================================================
        // ゲームパッド (04-02)
        // =================================================================
        virtual bool OnControllerAnalog(const NS::TCHAR* keyName,
                                        PlatformUserId userId,
                                        InputDeviceId deviceId,
                                        float analogValue);
        virtual bool OnControllerButtonPressed(const NS::TCHAR* keyName,
                                               PlatformUserId userId,
                                               InputDeviceId deviceId,
                                               bool isRepeat);
        virtual bool OnControllerButtonReleased(const NS::TCHAR* keyName,
                                                PlatformUserId userId,
                                                InputDeviceId deviceId,
                                                bool isRepeat);

        // =================================================================
        // タッチ (04-02)
        // =================================================================
        virtual bool OnTouchStarted(const std::shared_ptr<GenericWindow>& window,
                                    const Vector2D& location,
                                    float force,
                                    int32_t touchIndex,
                                    PlatformUserId userId,
                                    InputDeviceId deviceId);
        virtual bool OnTouchMoved(
            const Vector2D& location, float force, int32_t touchIndex, PlatformUserId userId, InputDeviceId deviceId);
        virtual bool OnTouchEnded(const Vector2D& location,
                                  int32_t touchIndex,
                                  PlatformUserId userId,
                                  InputDeviceId deviceId);
        virtual bool OnTouchForceChanged(
            const Vector2D& location, float force, int32_t touchIndex, PlatformUserId userId, InputDeviceId deviceId);
        virtual bool OnTouchFirstMove(
            const Vector2D& location, float force, int32_t touchIndex, PlatformUserId userId, InputDeviceId deviceId);

        // =================================================================
        // ジェスチャ (04-02)
        // =================================================================
        virtual void OnBeginGesture();
        virtual bool OnTouchGesture(GestureEvent gestureType,
                                    const Vector2D& delta,
                                    float wheelDelta,
                                    bool bIsDirectionInvertedFromDevice);
        virtual void OnEndGesture();
        virtual bool ShouldSimulateGesture(GestureEvent gestureType, bool bEnable);

        // =================================================================
        // モーション (04-02)
        // =================================================================
        virtual bool OnMotionDetected(const Vector3D& tilt,
                                      const Vector3D& rotationRate,
                                      const Vector3D& gravity,
                                      const Vector3D& acceleration,
                                      PlatformUserId userId,
                                      InputDeviceId deviceId);

        // =================================================================
        // ウィンドウイベント (04-03)
        // =================================================================
        virtual void OnSizeChanged(const std::shared_ptr<GenericWindow>& window,
                                   int32_t width,
                                   int32_t height,
                                   bool bWasMinimized);
        virtual void OnMovedWindow(const std::shared_ptr<GenericWindow>& window, int32_t x, int32_t y);
        virtual bool OnWindowActivationChanged(const std::shared_ptr<GenericWindow>& window,
                                               WindowActivation::Type activationType);
        virtual void OnApplicationActivationChanged(bool isActive);
        virtual void OnWindowClose(const std::shared_ptr<GenericWindow>& window);
        virtual void OnOSPaint(const std::shared_ptr<GenericWindow>& window);

        // =================================================================
        // リシェイプ・DPI (04-03)
        // =================================================================
        virtual void BeginReshapingWindow(const std::shared_ptr<GenericWindow>& window);
        virtual void FinishedReshapingWindow(const std::shared_ptr<GenericWindow>& window);
        virtual void OnResizingWindow(const std::shared_ptr<GenericWindow>& window);
        virtual void HandleDPIScaleChanged(const std::shared_ptr<GenericWindow>& window);
        virtual void SignalSystemDPIChanged(const std::shared_ptr<GenericWindow>& window);

        // =================================================================
        // ゾーン・サイズ・アクション (04-03)
        // =================================================================
        [[nodiscard]] virtual WindowZone::Type GetWindowZoneForPoint(const std::shared_ptr<GenericWindow>& window,
                                                                     int32_t x,
                                                                     int32_t y) const;
        [[nodiscard]] virtual WindowSizeLimits GetSizeLimitsForWindow(
            const std::shared_ptr<GenericWindow>& window) const;
        virtual void OnWindowAction(const std::shared_ptr<GenericWindow>& window, WindowAction actionType);

        // =================================================================
        // DnD (04-03)
        // =================================================================
        virtual DropEffect::Type OnDragEnterText(const std::shared_ptr<GenericWindow>& window, const NS::TCHAR* text);
        virtual DropEffect::Type OnDragEnterFiles(const std::shared_ptr<GenericWindow>& window,
                                                  const std::vector<std::wstring>& files);
        virtual DropEffect::Type OnDragEnterExternal(const std::shared_ptr<GenericWindow>& window,
                                                     const NS::TCHAR* text,
                                                     const std::vector<std::wstring>& files);
        virtual DropEffect::Type OnDragOver(const std::shared_ptr<GenericWindow>& window);
        virtual void OnDragLeave(const std::shared_ptr<GenericWindow>& window);
        virtual DropEffect::Type OnDragDrop(const std::shared_ptr<GenericWindow>& window);

        // =================================================================
        // その他 (04-03)
        // =================================================================
        virtual void OnConvertibleLaptopModeChanged();
        virtual bool ShouldUsePlatformUserId();
    };

} // namespace NS
