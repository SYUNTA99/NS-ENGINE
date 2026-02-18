/// @file GenericApplicationMessageHandler.cpp
/// @brief アプリケーションメッセージハンドラ デフォルト実装
#include "GenericPlatform/GenericApplicationMessageHandler.h"

namespace NS
{
    // =================================================================
    // フィルタ
    // =================================================================
    bool GenericApplicationMessageHandler::ShouldProcessUserInputMessages(
        const std::shared_ptr<GenericWindow>& /*Window*/) const
    {
        return true;
    }

    // =================================================================
    // キーボード (04-01)
    // =================================================================
    bool GenericApplicationMessageHandler::OnKeyChar(NS::TCHAR /*Character*/, bool /*IsRepeat*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnKeyDown(int32_t /*KeyCode*/, uint32_t /*CharacterCode*/, bool /*IsRepeat*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnKeyUp(int32_t /*KeyCode*/, uint32_t /*CharacterCode*/, bool /*IsRepeat*/)
    {
        return false;
    }
    void GenericApplicationMessageHandler::OnInputLanguageChanged() {}

    // =================================================================
    // マウスボタン (04-01)
    // =================================================================
    bool GenericApplicationMessageHandler::OnMouseDown(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                       MouseButtons::Type /*Button*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseDown(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                       MouseButtons::Type /*Button*/,
                                                       const Vector2D& /*CursorPos*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseUp(MouseButtons::Type /*Button*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseUp(MouseButtons::Type /*Button*/, const Vector2D& /*CursorPos*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                              MouseButtons::Type /*Button*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseDoubleClick(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                              MouseButtons::Type /*Button*/,
                                                              const Vector2D& /*CursorPos*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseWheel(float /*Delta*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnMouseWheel(float /*Delta*/, const Vector2D& /*CursorPos*/)
    {
        return false;
    }

    // =================================================================
    // マウス移動・カーソル (04-01)
    // =================================================================
    bool GenericApplicationMessageHandler::OnMouseMove(const Vector2D& /*CursorPos*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnRawMouseMove(int32_t /*X*/, int32_t /*Y*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnCursorSet()
    {
        return false;
    }
    void GenericApplicationMessageHandler::SetCursorPos(const Vector2D& /*Position*/) {}

    // =================================================================
    // ゲームパッド (04-02)
    // =================================================================
    bool GenericApplicationMessageHandler::OnControllerAnalog(const NS::TCHAR* /*KeyName*/,
                                                              PlatformUserId /*UserId*/,
                                                              InputDeviceId /*DeviceId*/,
                                                              float /*AnalogValue*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnControllerButtonPressed(const NS::TCHAR* /*KeyName*/,
                                                                     PlatformUserId /*UserId*/,
                                                                     InputDeviceId /*DeviceId*/,
                                                                     bool /*IsRepeat*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnControllerButtonReleased(const NS::TCHAR* /*KeyName*/,
                                                                      PlatformUserId /*UserId*/,
                                                                      InputDeviceId /*DeviceId*/,
                                                                      bool /*IsRepeat*/)
    {
        return false;
    }

    // =================================================================
    // タッチ (04-02)
    // =================================================================
    bool GenericApplicationMessageHandler::OnTouchStarted(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                          const Vector2D& /*Location*/,
                                                          float /*Force*/,
                                                          int32_t /*TouchIndex*/,
                                                          PlatformUserId /*UserId*/,
                                                          InputDeviceId /*DeviceId*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnTouchMoved(const Vector2D& /*Location*/,
                                                        float /*Force*/,
                                                        int32_t /*TouchIndex*/,
                                                        PlatformUserId /*UserId*/,
                                                        InputDeviceId /*DeviceId*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnTouchEnded(const Vector2D& /*Location*/,
                                                        int32_t /*TouchIndex*/,
                                                        PlatformUserId /*UserId*/,
                                                        InputDeviceId /*DeviceId*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnTouchForceChanged(const Vector2D& /*Location*/,
                                                               float /*Force*/,
                                                               int32_t /*TouchIndex*/,
                                                               PlatformUserId /*UserId*/,
                                                               InputDeviceId /*DeviceId*/)
    {
        return false;
    }
    bool GenericApplicationMessageHandler::OnTouchFirstMove(const Vector2D& /*Location*/,
                                                            float /*Force*/,
                                                            int32_t /*TouchIndex*/,
                                                            PlatformUserId /*UserId*/,
                                                            InputDeviceId /*DeviceId*/)
    {
        return false;
    }

    // =================================================================
    // ジェスチャ (04-02)
    // =================================================================
    void GenericApplicationMessageHandler::OnBeginGesture() {}
    bool GenericApplicationMessageHandler::OnTouchGesture(GestureEvent /*GestureType*/,
                                                          const Vector2D& /*Delta*/,
                                                          float /*WheelDelta*/,
                                                          bool /*bIsDirectionInvertedFromDevice*/)
    {
        return false;
    }
    void GenericApplicationMessageHandler::OnEndGesture() {}
    bool GenericApplicationMessageHandler::ShouldSimulateGesture(GestureEvent /*GestureType*/, bool /*bEnable*/)
    {
        return false;
    }

    // =================================================================
    // モーション (04-02)
    // =================================================================
    bool GenericApplicationMessageHandler::OnMotionDetected(const Vector3D& /*Tilt*/,
                                                            const Vector3D& /*RotationRate*/,
                                                            const Vector3D& /*Gravity*/,
                                                            const Vector3D& /*Acceleration*/,
                                                            PlatformUserId /*UserId*/,
                                                            InputDeviceId /*DeviceId*/)
    {
        return false;
    }

    // =================================================================
    // ウィンドウイベント (04-03)
    // =================================================================
    void GenericApplicationMessageHandler::OnSizeChanged(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                         int32_t /*Width*/,
                                                         int32_t /*Height*/,
                                                         bool /*bWasMinimized*/)
    {}
    void GenericApplicationMessageHandler::OnMovedWindow(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                         int32_t /*X*/,
                                                         int32_t /*Y*/)
    {}
    bool GenericApplicationMessageHandler::OnWindowActivationChanged(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                                     WindowActivation::Type /*ActivationType*/)
    {
        return false;
    }
    void GenericApplicationMessageHandler::OnApplicationActivationChanged(bool /*IsActive*/) {}
    void GenericApplicationMessageHandler::OnWindowClose(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    void GenericApplicationMessageHandler::OnOSPaint(const std::shared_ptr<GenericWindow>& /*Window*/) {}

    // =================================================================
    // リシェイプ・DPI (04-03)
    // =================================================================
    void GenericApplicationMessageHandler::BeginReshapingWindow(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    void GenericApplicationMessageHandler::FinishedReshapingWindow(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    void GenericApplicationMessageHandler::OnResizingWindow(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    void GenericApplicationMessageHandler::HandleDPIScaleChanged(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    void GenericApplicationMessageHandler::SignalSystemDPIChanged(const std::shared_ptr<GenericWindow>& /*Window*/) {}

    // =================================================================
    // ゾーン・サイズ・アクション (04-03)
    // =================================================================
    WindowZone::Type GenericApplicationMessageHandler::GetWindowZoneForPoint(
        const std::shared_ptr<GenericWindow>& /*Window*/, int32_t /*X*/, int32_t /*Y*/) const
    {
        return WindowZone::NotInWindow;
    }
    WindowSizeLimits GenericApplicationMessageHandler::GetSizeLimitsForWindow(
        const std::shared_ptr<GenericWindow>& /*Window*/) const
    {
        return {};
    }
    void GenericApplicationMessageHandler::OnWindowAction(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                          WindowAction /*ActionType*/)
    {}

    // =================================================================
    // DnD (04-03)
    // =================================================================
    DropEffect::Type GenericApplicationMessageHandler::OnDragEnterText(const std::shared_ptr<GenericWindow>& /*Window*/,
                                                                       const NS::TCHAR* /*Text*/)
    {
        return DropEffect::None;
    }
    DropEffect::Type GenericApplicationMessageHandler::OnDragEnterFiles(
        const std::shared_ptr<GenericWindow>& /*Window*/, const std::vector<std::wstring>& /*Files*/)
    {
        return DropEffect::None;
    }
    DropEffect::Type GenericApplicationMessageHandler::OnDragEnterExternal(
        const std::shared_ptr<GenericWindow>& /*Window*/,
        const NS::TCHAR* /*Text*/,
        const std::vector<std::wstring>& /*Files*/)
    {
        return DropEffect::None;
    }
    DropEffect::Type GenericApplicationMessageHandler::OnDragOver(const std::shared_ptr<GenericWindow>& /*Window*/)
    {
        return DropEffect::None;
    }
    void GenericApplicationMessageHandler::OnDragLeave(const std::shared_ptr<GenericWindow>& /*Window*/) {}
    DropEffect::Type GenericApplicationMessageHandler::OnDragDrop(const std::shared_ptr<GenericWindow>& /*Window*/)
    {
        return DropEffect::None;
    }

    // =================================================================
    // その他 (04-03)
    // =================================================================
    void GenericApplicationMessageHandler::OnConvertibleLaptopModeChanged() {}
    bool GenericApplicationMessageHandler::ShouldUsePlatformUserId()
    {
        return false;
    }

} // namespace NS
