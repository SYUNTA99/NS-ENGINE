/// @file IPlatformInputDeviceMapper.h
/// @brief 入力デバイスとプラットフォームユーザーのマッピングインターフェース
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/InputTypes.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace NS
{
    // =========================================================================
    // InputDeviceMappingPolicy
    // =========================================================================

    /// デバイスマッピングポリシー
    enum class InputDeviceMappingPolicy : uint8_t
    {
        Invalid = 0,
        /// プラットフォームのログインシステムを使用
        UseManagedPlatformLogin,
        /// プライマリユーザーがキーボードと最初のゲームパッドを共有
        PrimaryUserSharesKeyboardAndFirstGamepad,
        /// 各デバイスにユニークユーザーを割り当て
        CreateUniquePlatformUserForEachDevice,
        /// 全デバイスをプライマリユーザーに割り当て
        MapAllDevicesToPrimaryUser
    };

    // =========================================================================
    // IPlatformInputDeviceMapper
    // =========================================================================

    /// 入力デバイス-ユーザーマッピングインターフェース
    class IPlatformInputDeviceMapper
    {
    public:
        IPlatformInputDeviceMapper() = default;
        virtual ~IPlatformInputDeviceMapper() = default;
        NS_DISALLOW_COPY_AND_MOVE(IPlatformInputDeviceMapper);

    public:
        /// シングルトン取得
        static IPlatformInputDeviceMapper& Get();

        // =================================================================
        // プライマリ・デフォルト
        // =================================================================

        /// プライマリプラットフォームユーザーを取得
        [[nodiscard]] virtual PlatformUserId GetPrimaryPlatformUser() const { return PlatformUserId(0); }

        /// デフォルト入力デバイスを取得
        [[nodiscard]] virtual InputDeviceId GetDefaultInputDevice() const { return InputDeviceId(0); }

        // =================================================================
        // マッピングクエリ
        // =================================================================

        /// デバイスIDからユーザーIDを取得
        [[nodiscard]] virtual PlatformUserId GetUserForInputDevice(InputDeviceId deviceId) const = 0;

        /// ユーザーのプライマリ入力デバイスを取得
        [[nodiscard]] virtual InputDeviceId GetPrimaryInputDeviceForUser(PlatformUserId userId) const = 0;

        /// ユーザーの全入力デバイスを取得
        virtual bool GetAllInputDevicesForUser(PlatformUserId userId, std::vector<InputDeviceId>& outDevices) const = 0;

        /// 全入力デバイスを取得
        virtual void GetAllInputDevices(std::vector<InputDeviceId>& outDevices) const = 0;

        /// 全接続中入力デバイスを取得
        virtual void GetAllConnectedInputDevices(std::vector<InputDeviceId>& outDevices) const = 0;

        /// 全アクティブユーザーを取得
        virtual void GetAllActiveUsers(std::vector<PlatformUserId>& outUsers) const = 0;

        // =================================================================
        // 状態クエリ
        // =================================================================

        /// デバイスの接続状態を取得
        [[nodiscard]] virtual InputDeviceConnectionState GetInputDeviceConnectionState(
            InputDeviceId deviceId) const = 0;

        /// デバイスIDが有効か
        [[nodiscard]] virtual bool IsValidInputDevice(InputDeviceId deviceId) const = 0;

        /// ペアリングされていないデバイスの割り当て先ユーザー
        [[nodiscard]] virtual PlatformUserId GetUserForUnpairedInputDevices() const { return PlatformUserId(0); }

        /// ユーザーの全接続デバイスを取得
        virtual void GetAllConnectedInputDevicesForUser(PlatformUserId userId,
                                                        std::vector<InputDeviceId>& outDevices) const
        {
            std::vector<InputDeviceId> allDevices;
            GetAllInputDevicesForUser(userId, allDevices);
            outDevices.clear();
            for (const auto& dev : allDevices)
            {
                if (GetInputDeviceConnectionState(dev) == InputDeviceConnectionState::Connected)
                {
                    outDevices.push_back(dev);
                }
            }
        }

        /// 入力デバイスを持たない最初のユーザー
        [[nodiscard]] virtual PlatformUserId GetFirstPlatformUserWithNoInputDevice() const
        {
            return PlatformUserId::kNone;
        }

        /// 未ペアリングユーザーか
        [[nodiscard]] virtual bool IsUnpairedUserId(PlatformUserId userId) const
        {
            return userId == GetUserForUnpairedInputDevices();
        }

        /// デバイスが未ペアリングユーザーに割り当てられているか
        [[nodiscard]] virtual bool IsInputDeviceMappedToUnpairedUser(InputDeviceId deviceId) const
        {
            return GetUserForInputDevice(deviceId) == GetUserForUnpairedInputDevices();
        }

        // =================================================================
        // Legacy 互換
        // =================================================================

        /// ControllerId からユーザーID/デバイスIDへ変換
        virtual bool RemapControllerIdToPlatformUserAndDevice(int32_t controllerId,
                                                              PlatformUserId& outUserId,
                                                              InputDeviceId& outDeviceId) const
        {
            outUserId = PlatformUserId(controllerId);
            outDeviceId = InputDeviceId(controllerId);
            return true;
        }

        /// ユーザーID/デバイスID から ControllerId へ変換
        virtual bool RemapUserAndDeviceToControllerId(PlatformUserId userId,
                                                      int32_t& outControllerId,
                                                      InputDeviceId /*OptionalDeviceId*/ = InputDeviceId::kNone) const
        {
            outControllerId = userId.GetId();
            return true;
        }

        /// ユーザーインデックス取得
        [[nodiscard]] virtual int32_t GetUserIndexForPlatformUser(PlatformUserId userId) const
        {
            return userId.GetId();
        }

        /// インデックスからユーザーID取得
        [[nodiscard]] virtual PlatformUserId GetPlatformUserForUserIndex(int32_t localUserIndex) const
        {
            return PlatformUserId(localUserIndex);
        }

        // =================================================================
        // 設定
        // =================================================================

        /// 最大プラットフォームユーザー数
        [[nodiscard]] virtual int32_t GetMaxPlatformUserCount() const { return 8; }

        /// 現在のデバイスマッピングポリシー
        [[nodiscard]] virtual InputDeviceMappingPolicy GetCurrentDeviceMappingPolicy() const
        {
            return InputDeviceMappingPolicy::MapAllDevicesToPrimaryUser;
        }

        // =================================================================
        // デリゲート
        // =================================================================

        using ConnectionChangeDelegate = std::function<void(InputDeviceConnectionState, PlatformUserId, InputDeviceId)>;
        using PairingChangeDelegate =
            std::function<void(InputDeviceId, PlatformUserId /*NewUser*/, PlatformUserId /*OldUser*/)>;

        void OnInputDeviceConnectionChange(ConnectionChangeDelegate fn)
        {
            m_connectionChangeDelegates.push_back(std::move(fn));
        }
        void OnInputDevicePairingChange(PairingChangeDelegate fn) { m_pairingChangeDelegates.push_back(std::move(fn)); }

    protected:
        // =================================================================
        // 内部管理 (派生クラス用)
        // =================================================================

        virtual void InternalMapInputDeviceToUser(InputDeviceId deviceId, PlatformUserId userId) = 0;
        virtual void InternalChangeInputDeviceUserMapping(InputDeviceId deviceId,
                                                          PlatformUserId newUserId,
                                                          PlatformUserId oldUserId) = 0;
        virtual void InternalSetInputDeviceConnectionState(InputDeviceId deviceId,
                                                           InputDeviceConnectionState state) = 0;
        virtual PlatformUserId AllocateNewUserId() = 0;
        virtual InputDeviceId AllocateNewInputDeviceId() = 0;

        void BroadcastConnectionChange(InputDeviceConnectionState state, PlatformUserId userId, InputDeviceId deviceId)
        {
            for (const auto& fn : m_connectionChangeDelegates)
            {
                fn(state, userId, deviceId);
            }
        }

        void BroadcastPairingChange(InputDeviceId deviceId, PlatformUserId newUser, PlatformUserId oldUser)
        {
            for (const auto& fn : m_pairingChangeDelegates)
            {
                fn(deviceId, newUser, oldUser);
            }
        }

    private:
        std::vector<ConnectionChangeDelegate> m_connectionChangeDelegates;
        std::vector<PairingChangeDelegate> m_pairingChangeDelegates;
    };

} // namespace NS
