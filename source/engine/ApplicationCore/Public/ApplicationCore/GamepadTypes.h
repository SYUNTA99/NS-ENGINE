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
        inline const NS::TCHAR* g_leftAnalogX = TEXT("Gamepad_LeftX");
        inline const NS::TCHAR* g_leftAnalogY = TEXT("Gamepad_LeftY");
        inline const NS::TCHAR* g_rightAnalogX = TEXT("Gamepad_RightX");
        inline const NS::TCHAR* g_rightAnalogY = TEXT("Gamepad_RightY");

        // トリガー
        inline const NS::TCHAR* g_leftTriggerAnalog = TEXT("Gamepad_LeftTriggerAxis");
        inline const NS::TCHAR* g_rightTriggerAnalog = TEXT("Gamepad_RightTriggerAxis");

        // フェイスボタン
        inline const NS::TCHAR* g_faceButtonBottom = TEXT("Gamepad_FaceButton_Bottom"); // A / Cross
        inline const NS::TCHAR* g_faceButtonRight = TEXT("Gamepad_FaceButton_Right");   // B / Circle
        inline const NS::TCHAR* g_faceButtonLeft = TEXT("Gamepad_FaceButton_Left");     // X / Square
        inline const NS::TCHAR* g_faceButtonTop = TEXT("Gamepad_FaceButton_Top");       // Y / Triangle

        // ショルダーボタン
        inline const NS::TCHAR* g_leftShoulder = TEXT("Gamepad_LeftShoulder");   // LB / L1
        inline const NS::TCHAR* g_rightShoulder = TEXT("Gamepad_RightShoulder"); // RB / R1

        // サムスティック押下
        inline const NS::TCHAR* g_leftThumb = TEXT("Gamepad_LeftThumbstick");   // L3
        inline const NS::TCHAR* g_rightThumb = TEXT("Gamepad_RightThumbstick"); // R3

        // 特殊ボタン
        inline const NS::TCHAR* g_specialLeft = TEXT("Gamepad_Special_Left");   // Select / Back
        inline const NS::TCHAR* g_specialRight = TEXT("Gamepad_Special_Right"); // Start / Options

        // 十字キー
        inline const NS::TCHAR* g_dPadUp = TEXT("Gamepad_DPad_Up");
        inline const NS::TCHAR* g_dPadDown = TEXT("Gamepad_DPad_Down");
        inline const NS::TCHAR* g_dPadLeft = TEXT("Gamepad_DPad_Left");
        inline const NS::TCHAR* g_dPadRight = TEXT("Gamepad_DPad_Right");
    } // namespace GamepadKeyNames

    // =========================================================================
    // ForceFeedbackChannelType
    // =========================================================================

    /// フォースフィードバックチャンネル
    enum class ForceFeedbackChannelType : uint8_t
    {
        LeftLarge = 0,
        LeftSmall,
        RightLarge,
        RightSmall
    };

    // =========================================================================
    // ForceFeedbackValues
    // =========================================================================

    /// フォースフィードバック値
    struct ForceFeedbackValues
    {
        float leftLarge = 0.0F;
        float leftSmall = 0.0F;
        float rightLarge = 0.0F;
        float rightSmall = 0.0F;

        ForceFeedbackValues() = default;

        ForceFeedbackValues(float inLeftLarge, float inLeftSmall, float inRightLarge, float inRightSmall)
            : leftLarge(inLeftLarge), leftSmall(inLeftSmall), rightLarge(inRightLarge), rightSmall(inRightSmall)
        {}
    };

    // =========================================================================
    // HapticFeedbackBuffer
    // =========================================================================

    /// ハプティクス生データバッファ
    struct HapticFeedbackBuffer
    {
        const uint8_t* rawData = nullptr;
        uint32_t currentPtr = 0;
        int32_t bufferLength = 0;
        int32_t samplesSent = 0;
        bool bFinishedPlaying = false;
        int32_t samplingRate = 0;
        float scaleFactor = 1.0F;
        bool bUseStereo = false;
        uint32_t currentSampleIndex[2] = {0, 0};

        [[nodiscard]] bool NeedsUpdate() const { return !bFinishedPlaying && rawData != nullptr; }
    };

    // =========================================================================
    // HapticFeedbackValues
    // =========================================================================

    /// ハプティクスフィードバック値
    struct HapticFeedbackValues
    {
        float frequency = 0.0F;
        float amplitude = 0.0F;
        HapticFeedbackBuffer* hapticBuffer = nullptr;

        HapticFeedbackValues() = default;

        HapticFeedbackValues(float inFrequency, float inAmplitude) : frequency(inFrequency), amplitude(inAmplitude) {}
    };

} // namespace NS
