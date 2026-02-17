/// @file GamepadUtils.h
/// @brief ゲームパッドユーティリティ (DynamicReleaseDeadZone)
#pragma once

#include "ApplicationCore/CVarStub.h"
#include <algorithm>
#include <cstdint>

namespace NS
{
    // =========================================================================
    // CVar 定義
    // =========================================================================

    namespace GamepadCVars
    {
        NS_CVAR_FLOAT(TriggerDynamicReleaseDeadZone, 1.0f, "Default trigger dynamic release dead zone (1.0 = disabled)")
        NS_CVAR_BOOL(AllowTriggerDynamicReleaseDeadZoneCustomization, true, "Allow per-trigger dead zone customization")
        NS_CVAR_FLOAT(TriggerDynamicReleaseDeadZoneRePressFactor, 0.1f, "Re-press threshold = DeadZone * this factor")
        NS_CVAR_FLOAT(TriggerDynamicReleaseDeadZoneMinimumRePress, 0.05f, "Minimum re-press threshold")
    } // namespace GamepadCVars

    // =========================================================================
    // DynamicReleaseDeadZone
    // =========================================================================

    /// トリガー入力の動的リリースデッドゾーン
    /// トリガーのラチェット挙動により、押下→リリースの閾値を動的に調整する。
    struct DynamicReleaseDeadZone
    {
        float DeadZone = 1.0f;
        uint8_t TriggerThreshold = 0;
        bool bHasOverride : 1;
        bool bWasSimplePressed : 1;
        bool bWasDynamicPressed : 1;

        DynamicReleaseDeadZone() : bHasOverride(false), bWasSimplePressed(false), bWasDynamicPressed(false) {}

        /// 基本判定: 閾値超過で true
        bool IsPressed(float AnalogValue) const { return IsPressed(AnalogValue, GetEffectiveDeadZone()); }

        /// ラチェットロジック付き判定
        /// @param AnalogValue 0.0-1.0 のアナログ値
        /// @param bOutWasPressed 前フレームの押下状態 (in/out)
        bool IsPressed(float AnalogValue, bool& bOutWasPressed) const
        {
            float effectiveDeadZone = GetEffectiveDeadZone();

            if (bOutWasPressed)
            {
                // リリース判定: デッドゾーン * リプレスファクター
                float releaseThreshold =
                    std::max(effectiveDeadZone * GamepadCVars::TriggerDynamicReleaseDeadZoneRePressFactor,
                             GamepadCVars::TriggerDynamicReleaseDeadZoneMinimumRePress);

                if (AnalogValue < releaseThreshold)
                {
                    bOutWasPressed = false;
                }
            }
            else
            {
                // プレス判定
                if (AnalogValue >= effectiveDeadZone)
                {
                    bOutWasPressed = true;
                }
            }
            return bOutWasPressed;
        }

        /// カスタム閾値付き判定
        bool IsPressed(float AnalogValue, float CustomDeadZone) const { return AnalogValue >= CustomDeadZone; }

    private:
        float GetEffectiveDeadZone() const
        {
            if (bHasOverride && GamepadCVars::AllowTriggerDynamicReleaseDeadZoneCustomization)
            {
                return DeadZone;
            }
            return GamepadCVars::TriggerDynamicReleaseDeadZone;
        }
    };

} // namespace NS
