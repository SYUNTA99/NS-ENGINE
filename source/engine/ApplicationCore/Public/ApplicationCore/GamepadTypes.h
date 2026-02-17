/// @file GamepadTypes.h
/// @brief ゲームパッド・フォースフィードバック・ハプティクス型定義
#pragma once

#include "HAL/PlatformTypes.h"
#include <cstdint>

namespace NS
{
    // =========================================================================
    // GamepadKeyNames
    // =========================================================================

    /// ゲームパッドキー名定数
    namespace GamepadKeyNames
    {
        // アナログスティック
        inline const NS::TCHAR* LeftAnalogX = TEXT("Gamepad_LeftX");
        inline const NS::TCHAR* LeftAnalogY = TEXT("Gamepad_LeftY");
        inline const NS::TCHAR* RightAnalogX = TEXT("Gamepad_RightX");
        inline const NS::TCHAR* RightAnalogY = TEXT("Gamepad_RightY");

        // トリガー
        inline const NS::TCHAR* LeftTriggerAnalog = TEXT("Gamepad_LeftTriggerAxis");
        inline const NS::TCHAR* RightTriggerAnalog = TEXT("Gamepad_RightTriggerAxis");

        // フェイスボタン
        inline const NS::TCHAR* FaceButtonBottom = TEXT("Gamepad_FaceButton_Bottom"); // A / Cross
        inline const NS::TCHAR* FaceButtonRight = TEXT("Gamepad_FaceButton_Right");   // B / Circle
        inline const NS::TCHAR* FaceButtonLeft = TEXT("Gamepad_FaceButton_Left");     // X / Square
        inline const NS::TCHAR* FaceButtonTop = TEXT("Gamepad_FaceButton_Top");       // Y / Triangle

        // ショルダーボタン
        inline const NS::TCHAR* LeftShoulder = TEXT("Gamepad_LeftShoulder");   // LB / L1
        inline const NS::TCHAR* RightShoulder = TEXT("Gamepad_RightShoulder"); // RB / R1

        // サムスティック押下
        inline const NS::TCHAR* LeftThumb = TEXT("Gamepad_LeftThumbstick");   // L3
        inline const NS::TCHAR* RightThumb = TEXT("Gamepad_RightThumbstick"); // R3

        // 特殊ボタン
        inline const NS::TCHAR* SpecialLeft = TEXT("Gamepad_Special_Left");   // Select / Back
        inline const NS::TCHAR* SpecialRight = TEXT("Gamepad_Special_Right"); // Start / Options

        // 十字キー
        inline const NS::TCHAR* DPadUp = TEXT("Gamepad_DPad_Up");
        inline const NS::TCHAR* DPadDown = TEXT("Gamepad_DPad_Down");
        inline const NS::TCHAR* DPadLeft = TEXT("Gamepad_DPad_Left");
        inline const NS::TCHAR* DPadRight = TEXT("Gamepad_DPad_Right");
    } // namespace GamepadKeyNames

    // =========================================================================
    // ForceFeedbackChannelType
    // =========================================================================

    /// フォースフィードバックチャンネル
    enum class ForceFeedbackChannelType : uint8_t
    {
        LEFT_LARGE = 0,
        LEFT_SMALL,
        RIGHT_LARGE,
        RIGHT_SMALL
    };

    // =========================================================================
    // ForceFeedbackValues
    // =========================================================================

    /// フォースフィードバック値
    struct ForceFeedbackValues
    {
        float LeftLarge = 0.0f;
        float LeftSmall = 0.0f;
        float RightLarge = 0.0f;
        float RightSmall = 0.0f;

        ForceFeedbackValues() = default;

        ForceFeedbackValues(float InLeftLarge, float InLeftSmall, float InRightLarge, float InRightSmall)
            : LeftLarge(InLeftLarge), LeftSmall(InLeftSmall), RightLarge(InRightLarge), RightSmall(InRightSmall)
        {}
    };

    // =========================================================================
    // HapticFeedbackBuffer
    // =========================================================================

    /// ハプティクス生データバッファ
    struct HapticFeedbackBuffer
    {
        const uint8_t* RawData = nullptr;
        uint32_t CurrentPtr = 0;
        int32_t BufferLength = 0;
        int32_t SamplesSent = 0;
        bool bFinishedPlaying = false;
        int32_t SamplingRate = 0;
        float ScaleFactor = 1.0f;
        bool bUseStereo = false;
        uint32_t CurrentSampleIndex[2] = {0, 0};

        bool NeedsUpdate() const { return !bFinishedPlaying && RawData != nullptr; }
    };

    // =========================================================================
    // HapticFeedbackValues
    // =========================================================================

    /// ハプティクスフィードバック値
    struct HapticFeedbackValues
    {
        float Frequency = 0.0f;
        float Amplitude = 0.0f;
        HapticFeedbackBuffer* HapticBuffer = nullptr;

        HapticFeedbackValues() = default;

        HapticFeedbackValues(float InFrequency, float InAmplitude) : Frequency(InFrequency), Amplitude(InAmplitude) {}
    };

} // namespace NS
