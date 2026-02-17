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
        virtual ~IPlatformInputDeviceMapper() = default;

        /// シングルトン取得
        static IPlatformInputDeviceMapper& Get();

        // =================================================================
        // プライマリ・デフォルト
        // =================================================================

        /// プライマリプラットフォームユーザーを取得
        virtual PlatformUserId GetPrimaryPlatformUser() const { return PlatformUserId(0); }

        /// デフォルト入力デバイスを取得
        virtual InputDeviceId GetDefaultInputDevice() const { return InputDeviceId(0); }

        // =================================================================
        // マッピングクエリ
        // =================================================================

        /// デバイスIDからユーザーIDを取得
        virtual PlatformUserId GetUserForInputDevice(InputDeviceId DeviceId) const = 0;

        /// ユーザーのプライマリ入力デバイスを取得
        virtual InputDeviceId GetPrimaryInputDeviceForUser(PlatformUserId UserId) const = 0;

        /// ユーザーの全入力デバイスを取得
        virtual bool GetAllInputDevicesForUser(PlatformUserId UserId, std::vector<InputDeviceId>& OutDevices) const = 0;

        /// 全入力デバイスを取得
        virtual void GetAllInputDevices(std::vector<InputDeviceId>& OutDevices) const = 0;

        /// 全接続中入力デバイスを取得
        virtual void GetAllConnectedInputDevices(std::vector<InputDeviceId>& OutDevices) const = 0;

        /// 全アクティブユーザーを取得
        virtual void GetAllActiveUsers(std::vector<PlatformUserId>& OutUsers) const = 0;

        // =================================================================
        // 状態クエリ
        // =================================================================

        /// デバイスの接続状態を取得
        virtual InputDeviceConnectionState GetInputDeviceConnectionState(InputDeviceId DeviceId) const = 0;

        /// デバイスIDが有効か
        virtual bool IsValidInputDevice(InputDeviceId DeviceId) const = 0;

        /// ペアリングされていないデバイスの割り当て先ユーザー
        virtual PlatformUserId GetUserForUnpairedInputDevices() const { return PlatformUserId(0); }

        /// ユーザーの全接続デバイスを取得
        virtual void GetAllConnectedInputDevicesForUser(PlatformUserId UserId,
                                                        std::vector<InputDeviceId>& OutDevices) const
        {
            std::vector<InputDeviceId> allDevices;
            GetAllInputDevicesForUser(UserId, allDevices);
            OutDevices.clear();
            for (const auto& dev : allDevices)
            {
                if (GetInputDeviceConnectionState(dev) == InputDeviceConnectionState::Connected)
                {
                    OutDevices.push_back(dev);
                }
            }
        }

        /// 入力デバイスを持たない最初のユーザー
        virtual PlatformUserId GetFirstPlatformUserWithNoInputDevice() const { return PlatformUserId::NONE; }

        /// 未ペアリングユーザーか
        virtual bool IsUnpairedUserId(PlatformUserId UserId) const
        {
            return UserId == GetUserForUnpairedInputDevices();
        }

        /// デバイスが未ペアリングユーザーに割り当てられているか
        virtual bool IsInputDeviceMappedToUnpairedUser(InputDeviceId DeviceId) const
        {
            return GetUserForInputDevice(DeviceId) == GetUserForUnpairedInputDevices();
        }

        // =================================================================
        // Legacy 互換
        // =================================================================

        /// ControllerId からユーザーID/デバイスIDへ変換
        virtual bool RemapControllerIdToPlatformUserAndDevice(int32_t ControllerId,
                                                              PlatformUserId& OutUserId,
                                                              InputDeviceId& OutDeviceId) const
        {
            OutUserId = PlatformUserId(ControllerId);
            OutDeviceId = InputDeviceId(ControllerId);
            return true;
        }

        /// ユーザーID/デバイスID から ControllerId へ変換
        virtual bool RemapUserAndDeviceToControllerId(PlatformUserId UserId,
                                                      int32_t& OutControllerId,
                                                      InputDeviceId /*OptionalDeviceId*/ = InputDeviceId::NONE) const
        {
            OutControllerId = UserId.GetId();
            return true;
        }

        /// ユーザーインデックス取得
        virtual int32_t GetUserIndexForPlatformUser(PlatformUserId UserId) const { return UserId.GetId(); }

        /// インデックスからユーザーID取得
        virtual PlatformUserId GetPlatformUserForUserIndex(int32_t LocalUserIndex) const
        {
            return PlatformUserId(LocalUserIndex);
        }

        // =================================================================
        // 設定
        // =================================================================

        /// 最大プラットフォームユーザー数
        virtual int32_t GetMaxPlatformUserCount() const { return 8; }

        /// 現在のデバイスマッピングポリシー
        virtual InputDeviceMappingPolicy GetCurrentDeviceMappingPolicy() const
        {
            return InputDeviceMappingPolicy::MapAllDevicesToPrimaryUser;
        }

        // =================================================================
        // デリゲート
        // =================================================================

        using ConnectionChangeDelegate = std::function<void(InputDeviceConnectionState, PlatformUserId, InputDeviceId)>;
        using PairingChangeDelegate =
            std::function<void(InputDeviceId, PlatformUserId /*NewUser*/, PlatformUserId /*OldUser*/)>;

        void OnInputDeviceConnectionChange(ConnectionChangeDelegate Fn)
        {
            m_connectionChangeDelegates.push_back(std::move(Fn));
        }
        void OnInputDevicePairingChange(PairingChangeDelegate Fn) { m_pairingChangeDelegates.push_back(std::move(Fn)); }

    protected:
        // =================================================================
        // 内部管理 (派生クラス用)
        // =================================================================

        virtual void Internal_MapInputDeviceToUser(InputDeviceId DeviceId, PlatformUserId UserId) = 0;
        virtual void Internal_ChangeInputDeviceUserMapping(InputDeviceId DeviceId,
                                                           PlatformUserId NewUserId,
                                                           PlatformUserId OldUserId) = 0;
        virtual void Internal_SetInputDeviceConnectionState(InputDeviceId DeviceId,
                                                            InputDeviceConnectionState State) = 0;
        virtual PlatformUserId AllocateNewUserId() = 0;
        virtual InputDeviceId AllocateNewInputDeviceId() = 0;

        void BroadcastConnectionChange(InputDeviceConnectionState State, PlatformUserId UserId, InputDeviceId DeviceId)
        {
            for (const auto& fn : m_connectionChangeDelegates)
            {
                fn(State, UserId, DeviceId);
            }
        }

        void BroadcastPairingChange(InputDeviceId DeviceId, PlatformUserId NewUser, PlatformUserId OldUser)
        {
            for (const auto& fn : m_pairingChangeDelegates)
            {
                fn(DeviceId, NewUser, OldUser);
            }
        }

    private:
        std::vector<ConnectionChangeDelegate> m_connectionChangeDelegates;
        std::vector<PairingChangeDelegate> m_pairingChangeDelegates;
    };

} // namespace NS
