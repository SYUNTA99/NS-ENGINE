/// @file RHIDeviceLost.cpp
/// @brief Device lost detection/recovery implementation
#include "RHIDeviceLost.h"
#include "IDynamicRHI.h"
#include "IRHIDevice.h"
#include <cstring>
#include <mutex>

namespace NS::RHI
{
    //=========================================================================
    // GetDeviceLostReasonName
    //=========================================================================

    const char* GetDeviceLostReasonName(ERHIDeviceLostReason reason)
    {
        switch (reason)
        {
        case ERHIDeviceLostReason::Unknown:
            return "Unknown";
        case ERHIDeviceLostReason::Hung:
            return "Hung";
        case ERHIDeviceLostReason::Reset:
            return "Reset";
        case ERHIDeviceLostReason::DriverUpgrade:
            return "DriverUpgrade";
        case ERHIDeviceLostReason::DriverInternalError:
            return "DriverInternalError";
        case ERHIDeviceLostReason::InvalidGPUCommand:
            return "InvalidGPUCommand";
        case ERHIDeviceLostReason::PageFault:
            return "PageFault";
        case ERHIDeviceLostReason::PowerEvent:
            return "PowerEvent";
        case ERHIDeviceLostReason::PhysicalRemoval:
            return "PhysicalRemoval";
        case ERHIDeviceLostReason::OutOfMemory:
            return "OutOfMemory";
        default:
            return "Unknown";
        }
    }

    //=========================================================================
    // RHIDeviceLostHandler
    //=========================================================================

    void RHIDeviceLostHandler::Initialize(IRHIDevice* device)
    {
        m_device = device;
        m_deviceLost = false;
        m_callbackCapacity = 8;
        m_callbacks = new CallbackEntry[m_callbackCapacity];
        m_callbackCount = 0;
    }

    void RHIDeviceLostHandler::Shutdown()
    {
        DisableAutoPolling();
        delete[] m_callbacks;
        m_callbacks = nullptr;
        m_callbackCount = 0;
        m_callbackCapacity = 0;
        m_device = nullptr;
    }

    void RHIDeviceLostHandler::AddCallback(RHIDeviceLostHandlerCallback callback, void* userData)
    {
        if (callback == nullptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_callbackMutex);

        if (m_callbackCapacity == 0)
        {
            m_callbackCapacity = 8;
            m_callbacks = new CallbackEntry[m_callbackCapacity];
            m_callbackCount = 0;
        }

        if (m_callbackCount >= m_callbackCapacity)
        {
            uint32 const newCapacity = m_callbackCapacity * 2;
            auto* newCallbacks = new CallbackEntry[newCapacity];
            std::memcpy(newCallbacks, m_callbacks, m_callbackCount * sizeof(CallbackEntry));
            delete[] m_callbacks;
            m_callbacks = newCallbacks;
            m_callbackCapacity = newCapacity;
        }

        m_callbacks[m_callbackCount].callback = callback;
        m_callbacks[m_callbackCount].userData = userData;
        ++m_callbackCount;
    }

    void RHIDeviceLostHandler::RemoveCallback(RHIDeviceLostHandlerCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);

        for (uint32 i = 0; i < m_callbackCount; ++i)
        {
            if (m_callbacks[i].callback == callback)
            {
                m_callbacks[i] = m_callbacks[m_callbackCount - 1];
                --m_callbackCount;
                return;
            }
        }
    }

    bool RHIDeviceLostHandler::Poll()
    {
        if (m_deviceLost || (m_device == nullptr))
        {
            return m_deviceLost;
        }

        // Backend-specific device-lost detection is not wired yet.
        bool const isLost = false;

        if (isLost)
        {
            m_deviceLost = true;

            RHIDeviceLostInfo info;
            info.reason = ERHIDeviceLostReason::Unknown;

            CallbackEntry* callbacksSnapshot = nullptr;
            uint32 callbackCountSnapshot = 0;
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                callbackCountSnapshot = m_callbackCount;
                if (callbackCountSnapshot > 0)
                {
                    callbacksSnapshot = new CallbackEntry[callbackCountSnapshot];
                    std::memcpy(callbacksSnapshot, m_callbacks, callbackCountSnapshot * sizeof(CallbackEntry));
                }
            }

            for (uint32 i = 0; i < callbackCountSnapshot; ++i)
            {
                if (callbacksSnapshot[i].callback)
                {
                    callbacksSnapshot[i].callback(m_device, info, callbacksSnapshot[i].userData);
                }
            }

            delete[] callbacksSnapshot;
        }

        return m_deviceLost;
    }

    void RHIDeviceLostHandler::EnableAutoPolling(uint32 intervalMs)
    {
        (void)intervalMs;
        m_autoPolling = true;
    }

    void RHIDeviceLostHandler::DisableAutoPolling()
    {
        m_autoPolling = false;
    }

    //=========================================================================
    // RHIDeviceRecoveryManager
    //=========================================================================

    void RHIDeviceRecoveryManager::Initialize(IDynamicRHI* rhi, const RHIDeviceRecoveryOptions& options)
    {
        m_rhi = rhi;
        m_options = options;
        m_recovering = false;
        m_recoveryAttempts = 0;
        m_recreateCallbackCount = 0;
        m_recreateCallbackCapacity = 16;
        m_recreateCallbacks = new RecreateCallbackEntry[m_recreateCallbackCapacity];
    }

    void RHIDeviceRecoveryManager::Shutdown()
    {
        delete[] m_recreateCallbacks;
        m_recreateCallbacks = nullptr;
        m_recreateCallbackCount = 0;
        m_recreateCallbackCapacity = 0;
        m_rhi = nullptr;
    }

    bool RHIDeviceRecoveryManager::AttemptRecovery()
    {
        if ((m_rhi == nullptr) || !m_options.autoRecreate)
        {
            return false;
        }

        if (m_recoveryAttempts >= m_options.maxRetries)
        {
            return false;
        }

        m_recovering = true;
        ++m_recoveryAttempts;

        // Resource recreation is backend-specific and not wired yet.
        // m_recoveredDevice = m_rhi->RecreateDevice(...);

        if ((m_recoveredDevice != nullptr) && m_options.recreateResources)
        {
            for (uint32 i = 0; i < m_recreateCallbackCount; ++i)
            {
                m_recreateCallbacks[i].callback(m_recoveredDevice, m_recreateCallbacks[i].userData);
            }
        }

        m_recovering = false;
        return m_recoveredDevice != nullptr;
    }

    void RHIDeviceRecoveryManager::AddResourceRecreateCallback(ResourceRecreateCallback callback, void* userData)
    {
        if (callback == nullptr)
        {
            return;
        }

        if (m_recreateCallbackCount >= m_recreateCallbackCapacity)
        {
            uint32 const newCapacity = (m_recreateCallbackCapacity > 0) ? (m_recreateCallbackCapacity * 2) : 16;
            auto* newEntries = new RecreateCallbackEntry[newCapacity];
            std::memcpy(newEntries, m_recreateCallbacks, m_recreateCallbackCount * sizeof(RecreateCallbackEntry));
            delete[] m_recreateCallbacks;
            m_recreateCallbacks = newEntries;
            m_recreateCallbackCapacity = newCapacity;
        }

        m_recreateCallbacks[m_recreateCallbackCount].callback = callback;
        m_recreateCallbacks[m_recreateCallbackCount].userData = userData;
        ++m_recreateCallbackCount;
    }

    void RHIDeviceRecoveryManager::RemoveResourceRecreateCallback(ResourceRecreateCallback callback)
    {
        for (uint32 i = 0; i < m_recreateCallbackCount; ++i)
        {
            if (m_recreateCallbacks[i].callback == callback)
            {
                m_recreateCallbacks[i] = m_recreateCallbacks[m_recreateCallbackCount - 1];
                --m_recreateCallbackCount;
                return;
            }
        }
    }

} // namespace NS::RHI