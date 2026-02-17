/// @file InputDeviceProperty.h
/// @brief 入力デバイスプロパティ階層
#pragma once

#include "HAL/PlatformTypes.h"
#include <cstdint>

namespace NS
{
    // =========================================================================
    // InputDeviceProperty 基底
    // =========================================================================

    /// 入力デバイスプロパティ基底クラス
    class InputDeviceProperty
    {
    public:
        explicit InputDeviceProperty(const NS::TCHAR* InName) : m_name(InName) {}
        virtual ~InputDeviceProperty() = default;

        const NS::TCHAR* GetName() const { return m_name; }

    private:
        const NS::TCHAR* m_name;
    };

    // =========================================================================
    // ジャイロ・ライト
    // =========================================================================

    /// ジャイロ自動キャリブレーション設定
    class InputDeviceGyroAutoCalibrationProperty : public InputDeviceProperty
    {
    public:
        InputDeviceGyroAutoCalibrationProperty() : InputDeviceProperty(TEXT("GyroAutoCalibration")) {}

        bool bEnable = true;
    };

    /// コントローラーLEDカラー設定
    class InputDeviceLightColorProperty : public InputDeviceProperty
    {
    public:
        InputDeviceLightColorProperty() : InputDeviceProperty(TEXT("LightColor")) {}

        bool bEnable = true;
        uint8_t R = 0;
        uint8_t G = 0;
        uint8_t B = 0;
    };

    // =========================================================================
    // トリガー系
    // =========================================================================

    /// トリガーマスク
    enum class InputDeviceTriggerMask : uint8_t
    {
        None = 0,
        Left = 1 << 0,
        Right = 1 << 1,
        All = Left | Right
    };

    /// トリガープロパティ基底
    class InputDeviceTriggerProperty : public InputDeviceProperty
    {
    public:
        explicit InputDeviceTriggerProperty(const NS::TCHAR* InName) : InputDeviceProperty(InName) {}

        InputDeviceTriggerMask AffectedTriggers = InputDeviceTriggerMask::All;
    };

    /// トリガーリセット
    class InputDeviceTriggerResetProperty : public InputDeviceTriggerProperty
    {
    public:
        InputDeviceTriggerResetProperty() : InputDeviceTriggerProperty(TEXT("TriggerReset")) {}
    };

    /// トリガーフィードバック (Position + Strengh ※UE5タイポ保存)
    class InputDeviceTriggerFeedbackProperty : public InputDeviceTriggerProperty
    {
    public:
        InputDeviceTriggerFeedbackProperty() : InputDeviceTriggerProperty(TEXT("TriggerFeedback")) {}

        float Position = 0.0f;
        float Strengh = 0.0f; // ※UE5タイポ保存: Strength → Strengh
    };

    /// トリガー抵抗 (StartStrengh/EndStrengh ※UE5タイポ保存)
    class InputDeviceTriggerResistanceProperty : public InputDeviceTriggerProperty
    {
    public:
        InputDeviceTriggerResistanceProperty() : InputDeviceTriggerProperty(TEXT("TriggerResistance")) {}

        float StartPosition = 0.0f;
        float StartStrengh = 0.0f; // ※UE5タイポ保存
        float EndPosition = 1.0f;
        float EndStrengh = 1.0f; // ※UE5タイポ保存
    };

    /// トリガー振動
    class InputDeviceTriggerVibrationProperty : public InputDeviceTriggerProperty
    {
    public:
        InputDeviceTriggerVibrationProperty() : InputDeviceTriggerProperty(TEXT("TriggerVibration")) {}

        float Position = 0.0f;
        float Frequency = 0.0f;
        float Amplitude = 0.0f;
    };

    /// トリガー動的リリースデッドゾーン
    class InputDeviceTriggerDynamicReleaseDeadZoneProperty : public InputDeviceTriggerProperty
    {
    public:
        InputDeviceTriggerDynamicReleaseDeadZoneProperty()
            : InputDeviceTriggerProperty(TEXT("TriggerDynamicReleaseDeadZone"))
        {}

        float DeadZone = 1.0f;
    };

    // =========================================================================
    // アナログスティック系
    // =========================================================================

    /// アナログスティックマスク
    enum class InputDeviceAnalogStickMask : uint8_t
    {
        None = 0,
        Left = 1 << 0,
        Right = 1 << 1
    };

    /// アナログスティックプロパティ基底
    class InputDeviceAnalogStickProperty : public InputDeviceProperty
    {
    public:
        explicit InputDeviceAnalogStickProperty(const NS::TCHAR* InName) : InputDeviceProperty(InName) {}

        InputDeviceAnalogStickMask AffectedSticks = InputDeviceAnalogStickMask::Left;
    };

    /// アナログスティックデッドゾーン
    class InputDeviceAnalogStickDeadZoneProperty : public InputDeviceAnalogStickProperty
    {
    public:
        InputDeviceAnalogStickDeadZoneProperty() : InputDeviceAnalogStickProperty(TEXT("AnalogStickDeadZone")) {}

        float DeadZone = 0.0f;
    };

} // namespace NS
