/// @file InputDeviceScope.h
/// @brief RAII デバイスコンテキストスコープ + デバイスIDマッピングテンプレート
#pragma once

#include "ApplicationCore/InputTypes.h"
#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace NS
{
    // 前方宣言
    class IInputDevice;

    // =========================================================================
    // InputDeviceScope (RAII)
    // =========================================================================

    /// 入力デバイス処理のスコープを追跡する RAII 型
    class InputDeviceScope
    {
    public:
        InputDeviceScope(IInputDevice* InDevice,
                         const NS::TCHAR* InName,
                         int32_t InHardwareDeviceHandle = -1,
                         const std::wstring& InHardwareDeviceIdentifier = {})
            : InputDevice(InDevice), InputDeviceName(InName), HardwareDeviceHandle(InHardwareDeviceHandle),
              HardwareDeviceIdentifier(InHardwareDeviceIdentifier)
        {
            GetScopeStack().push_back(this);
        }

        ~InputDeviceScope()
        {
            auto& stack = GetScopeStack();
            NS_CHECK(!stack.empty() && stack.back() == this);
            stack.pop_back();
        }

        // コピー・ムーブ禁止
        InputDeviceScope(const InputDeviceScope&) = delete;
        InputDeviceScope& operator=(const InputDeviceScope&) = delete;

        /// 現在のスコープを取得 (nullptr = スコープなし)
        static const InputDeviceScope* GetCurrent()
        {
            const auto& stack = GetScopeStack();
            return stack.empty() ? nullptr : stack.back();
        }

        IInputDevice* InputDevice;
        const NS::TCHAR* InputDeviceName;
        int32_t HardwareDeviceHandle;
        std::wstring HardwareDeviceIdentifier;

    private:
        static std::vector<InputDeviceScope*>& GetScopeStack()
        {
            thread_local std::vector<InputDeviceScope*> s_scopeStack;
            return s_scopeStack;
        }
    };

    // =========================================================================
    // IInputDevice (前方宣言用の最小定義)
    // =========================================================================

    /// 入力デバイス抽象基底 (詳細は将来実装)
    class IInputDevice
    {
    public:
        virtual ~IInputDevice() = default;
    };

    // =========================================================================
    // InputDeviceMap<DeviceKeyType>
    // =========================================================================

    /// プラットフォーム固有デバイスキーと InputDeviceId の双方向マッピング
    template <typename DeviceKeyType> class InputDeviceMap
    {
    public:
        /// キーに対応するデバイスIDを取得、なければ新規割り当て
        InputDeviceId GetOrCreateDeviceId(const DeviceKeyType& Key)
        {
            auto it = m_keyToDevice.find(Key);
            if (it != m_keyToDevice.end())
            {
                return it->second;
            }

            InputDeviceId newId(m_nextDeviceId++);
            m_keyToDevice[Key] = newId;
            m_deviceToKey[newId.GetId()] = Key;
            return newId;
        }

        /// キーからデバイスIDを検索 (見つからなければ NONE)
        InputDeviceId FindDeviceId(const DeviceKeyType& Key) const
        {
            auto it = m_keyToDevice.find(Key);
            return (it != m_keyToDevice.end()) ? it->second : InputDeviceId::NONE;
        }

        /// デバイスIDからキーを検索 (見つからなければ nullptr)
        const DeviceKeyType* FindDeviceKey(InputDeviceId Id) const
        {
            auto it = m_deviceToKey.find(Id.GetId());
            return (it != m_deviceToKey.end()) ? &it->second : nullptr;
        }

        /// キーからデバイスID取得 (見つからなければアサート)
        [[nodiscard]] InputDeviceId FindDeviceIdChecked(const DeviceKeyType& Key) const
        {
            auto it = m_keyToDevice.find(Key);
            NS_CHECK(it != m_keyToDevice.end());
            return it->second;
        }

        /// デバイスIDからキー取得 (見つからなければアサート)
        [[nodiscard]] const DeviceKeyType& GetDeviceKeyChecked(InputDeviceId Id) const
        {
            auto it = m_deviceToKey.find(Id.GetId());
            NS_CHECK(it != m_deviceToKey.end());
            return it->second;
        }

        /// デフォルトデバイスIDにマッピング
        InputDeviceId MapDefaultInputDevice(const DeviceKeyType& Key)
        {
            InputDeviceId defaultId = InputDeviceId::NONE;
            m_keyToDevice[Key] = defaultId;
            m_deviceToKey[defaultId.GetId()] = Key;
            return defaultId;
        }

    private:
        std::unordered_map<DeviceKeyType, InputDeviceId> m_keyToDevice;
        std::unordered_map<int32_t, DeviceKeyType> m_deviceToKey;
        int32_t m_nextDeviceId = 0;
    };

} // namespace NS
