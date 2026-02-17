/// @file GenericPlatformInputDeviceMapper.cpp
/// @brief IPlatformInputDeviceMapper のデフォルト実装 (単一ユーザー・単一デバイス)
#include "GenericPlatform/IPlatformInputDeviceMapper.h"

#include <algorithm>
#include <unordered_map>

namespace NS
{
    // =========================================================================
    // GenericPlatformInputDeviceMapper
    // =========================================================================

    /// 単一ユーザー・複数デバイスの簡易マッパー
    class GenericPlatformInputDeviceMapper : public IPlatformInputDeviceMapper
    {
    public:
        GenericPlatformInputDeviceMapper()
        {
            // プライマリユーザーとデフォルトデバイスを初期化
            m_deviceToUser[0] = PlatformUserId(0);
            m_deviceStates[0] = InputDeviceConnectionState::Connected;
        }

        PlatformUserId GetUserForInputDevice(InputDeviceId DeviceId) const override
        {
            auto it = m_deviceToUser.find(DeviceId.GetId());
            return (it != m_deviceToUser.end()) ? it->second : PlatformUserId(0);
        }

        InputDeviceId GetPrimaryInputDeviceForUser(PlatformUserId UserId) const override
        {
            for (const auto& [devId, userId] : m_deviceToUser)
            {
                if (userId == UserId)
                {
                    return InputDeviceId(devId);
                }
            }
            return InputDeviceId::NONE;
        }

        bool GetAllInputDevicesForUser(PlatformUserId UserId, std::vector<InputDeviceId>& OutDevices) const override
        {
            OutDevices.clear();
            for (const auto& [devId, userId] : m_deviceToUser)
            {
                if (userId == UserId)
                {
                    OutDevices.push_back(InputDeviceId(devId));
                }
            }
            return !OutDevices.empty();
        }

        void GetAllInputDevices(std::vector<InputDeviceId>& OutDevices) const override
        {
            OutDevices.clear();
            for (const auto& [devId, userId] : m_deviceToUser)
            {
                (void)userId;
                OutDevices.push_back(InputDeviceId(devId));
            }
        }

        void GetAllConnectedInputDevices(std::vector<InputDeviceId>& OutDevices) const override
        {
            OutDevices.clear();
            for (const auto& [devId, state] : m_deviceStates)
            {
                if (state == InputDeviceConnectionState::Connected)
                {
                    OutDevices.push_back(InputDeviceId(devId));
                }
            }
        }

        void GetAllActiveUsers(std::vector<PlatformUserId>& OutUsers) const override
        {
            OutUsers.clear();
            OutUsers.push_back(PlatformUserId(0));
        }

        InputDeviceConnectionState GetInputDeviceConnectionState(InputDeviceId DeviceId) const override
        {
            auto it = m_deviceStates.find(DeviceId.GetId());
            return (it != m_deviceStates.end()) ? it->second : InputDeviceConnectionState::Unknown;
        }

        bool IsValidInputDevice(InputDeviceId DeviceId) const override
        {
            return m_deviceToUser.find(DeviceId.GetId()) != m_deviceToUser.end();
        }

    protected:
        void Internal_MapInputDeviceToUser(InputDeviceId DeviceId, PlatformUserId UserId) override
        {
            m_deviceToUser[DeviceId.GetId()] = UserId;
        }

        void Internal_ChangeInputDeviceUserMapping(InputDeviceId DeviceId,
                                                   PlatformUserId NewUserId,
                                                   PlatformUserId OldUserId) override
        {
            m_deviceToUser[DeviceId.GetId()] = NewUserId;
            BroadcastPairingChange(DeviceId, NewUserId, OldUserId);
        }

        void Internal_SetInputDeviceConnectionState(InputDeviceId DeviceId, InputDeviceConnectionState State) override
        {
            m_deviceStates[DeviceId.GetId()] = State;
            PlatformUserId userId = GetUserForInputDevice(DeviceId);
            BroadcastConnectionChange(State, userId, DeviceId);
        }

        PlatformUserId AllocateNewUserId() override { return PlatformUserId(m_nextUserId++); }

        InputDeviceId AllocateNewInputDeviceId() override { return InputDeviceId(m_nextDeviceId++); }

    private:
        std::unordered_map<int32_t, PlatformUserId> m_deviceToUser;
        std::unordered_map<int32_t, InputDeviceConnectionState> m_deviceStates;
        int32_t m_nextUserId = 1;
        int32_t m_nextDeviceId = 1;
    };

    // =========================================================================
    // Singleton
    // =========================================================================

    IPlatformInputDeviceMapper& IPlatformInputDeviceMapper::Get()
    {
        static GenericPlatformInputDeviceMapper s_instance;
        return s_instance;
    }

} // namespace NS
