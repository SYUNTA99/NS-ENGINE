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

        [[nodiscard]] PlatformUserId GetUserForInputDevice(InputDeviceId deviceId) const override
        {
            auto it = m_deviceToUser.find(deviceId.GetId());
            return (it != m_deviceToUser.end()) ? it->second : PlatformUserId(0);
        }

        [[nodiscard]] InputDeviceId GetPrimaryInputDeviceForUser(PlatformUserId userId) const override
        {
            for (const auto& [devId, mappedUserId] : m_deviceToUser)
            {
                if (mappedUserId == userId)
                {
                    return InputDeviceId(devId);
                }
            }
            return InputDeviceId::kNone;
        }

        bool GetAllInputDevicesForUser(PlatformUserId userId, std::vector<InputDeviceId>& outDevices) const override
        {
            outDevices.clear();
            for (const auto& [devId, mappedUserId] : m_deviceToUser)
            {
                if (mappedUserId == userId)
                {
                    outDevices.emplace_back(devId);
                }
            }
            return !outDevices.empty();
        }

        void GetAllInputDevices(std::vector<InputDeviceId>& outDevices) const override
        {
            outDevices.clear();
            for (const auto& [devId, userId] : m_deviceToUser)
            {
                (void)userId;
                outDevices.emplace_back(devId);
            }
        }

        void GetAllConnectedInputDevices(std::vector<InputDeviceId>& outDevices) const override
        {
            outDevices.clear();
            for (const auto& [devId, state] : m_deviceStates)
            {
                if (state == InputDeviceConnectionState::Connected)
                {
                    outDevices.emplace_back(devId);
                }
            }
        }

        void GetAllActiveUsers(std::vector<PlatformUserId>& outUsers) const override
        {
            outUsers.clear();
            outUsers.emplace_back(0);
        }

        [[nodiscard]] InputDeviceConnectionState GetInputDeviceConnectionState(InputDeviceId deviceId) const override
        {
            auto it = m_deviceStates.find(deviceId.GetId());
            return (it != m_deviceStates.end()) ? it->second : InputDeviceConnectionState::Unknown;
        }

        [[nodiscard]] bool IsValidInputDevice(InputDeviceId deviceId) const override
        {
            return m_deviceToUser.find(deviceId.GetId()) != m_deviceToUser.end();
        }

    protected:
        void InternalMapInputDeviceToUser(InputDeviceId deviceId, PlatformUserId userId) override
        {
            m_deviceToUser[deviceId.GetId()] = userId;
        }

        void InternalChangeInputDeviceUserMapping(InputDeviceId deviceId,
                                                  PlatformUserId newUserId,
                                                  PlatformUserId oldUserId) override
        {
            m_deviceToUser[deviceId.GetId()] = newUserId;
            BroadcastPairingChange(deviceId, newUserId, oldUserId);
        }

        void InternalSetInputDeviceConnectionState(InputDeviceId deviceId, InputDeviceConnectionState state) override
        {
            m_deviceStates[deviceId.GetId()] = state;
            PlatformUserId const userId = GetUserForInputDevice(deviceId);
            BroadcastConnectionChange(state, userId, deviceId);
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
        static GenericPlatformInputDeviceMapper sInstance;
        return sInstance;
    }

} // namespace NS
