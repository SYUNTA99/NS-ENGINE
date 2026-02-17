/// @file IInputInterface.h
/// @brief フォースフィードバック・ハプティクス・ライトカラー制御インターフェース
#pragma once

#include "ApplicationCore/GamepadTypes.h"
#include "ApplicationCore/InputTypes.h"
#include <cstdint>

namespace NS
{
    // 前方宣言
    class InputDeviceProperty;

    /// 入力デバイス出力制御インターフェース
    class IInputInterface
    {
    public:
        virtual ~IInputInterface() = default;

        // =================================================================
        // フォースフィードバック
        // =================================================================

        /// 単一チャンネル値設定
        virtual void SetForceFeedbackChannelValue(int32_t ControllerId,
                                                  ForceFeedbackChannelType ChannelType,
                                                  float Value)
        {
            (void)ControllerId;
            (void)ChannelType;
            (void)Value;
        }

        /// 全チャンネル一括設定
        virtual void SetForceFeedbackChannelValues(int32_t ControllerId, const ForceFeedbackValues& Values)
        {
            (void)ControllerId;
            (void)Values;
        }

        // =================================================================
        // ハプティクス
        // =================================================================

        /// ハプティクスフィードバック値設定
        /// @param Hand 0=左, 1=右
        virtual void SetHapticFeedbackValues(int32_t ControllerId, int32_t Hand, const HapticFeedbackValues& Values)
        {
            (void)ControllerId;
            (void)Hand;
            (void)Values;
        }

        // =================================================================
        // ライトカラー
        // =================================================================

        /// コントローラーLEDカラー設定 (R,G,B 0-255)
        virtual void SetLightColor(int32_t ControllerId, uint8_t R, uint8_t G, uint8_t B)
        {
            (void)ControllerId;
            (void)R;
            (void)G;
            (void)B;
        }

        /// コントローラーLEDカラーをデフォルトにリセット
        virtual void ResetLightColor(int32_t ControllerId) { (void)ControllerId; }

        // =================================================================
        // デバイスプロパティ
        // =================================================================

        /// プラットフォーム固有デバイスプロパティ設定
        virtual void SetDeviceProperty(int32_t ControllerId, const InputDeviceProperty* Property)
        {
            (void)ControllerId;
            (void)Property;
        }

        // =================================================================
        // クエリ
        // =================================================================

        /// ゲームパッドが接続されているか
        virtual bool IsGamepadAttached() const { return false; }
    };

} // namespace NS
