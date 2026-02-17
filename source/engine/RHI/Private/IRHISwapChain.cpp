/// @file IRHISwapChain.cpp
/// @brief マルチスワップチェーンPresenter実装
#include "IRHISwapChain.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIMultiSwapChainPresenter
    //=========================================================================

    bool RHIMultiSwapChainPresenter::Initialize(IRHIDevice* device)
    {
        m_device = device;

        m_swapChainCapacity = 4;
        m_swapChains = new IRHISwapChain*[m_swapChainCapacity];
        std::memset(m_swapChains, 0, sizeof(IRHISwapChain*) * m_swapChainCapacity);
        m_swapChainCount = 0;

        return true;
    }

    void RHIMultiSwapChainPresenter::Shutdown()
    {
        delete[] m_swapChains;
        m_swapChains = nullptr;
        m_swapChainCount = 0;
        m_swapChainCapacity = 0;
        m_device = nullptr;
    }

    void RHIMultiSwapChainPresenter::AddSwapChain(IRHISwapChain* swapChain)
    {
        if (swapChain == nullptr) {
            return;
}

        // 重複チェック
        for (uint32 i = 0; i < m_swapChainCount; ++i)
        {
            if (m_swapChains[i] == swapChain) {
                return;
}
        }

        // 容量拡張
        if (m_swapChainCount >= m_swapChainCapacity)
        {
            uint32 const newCapacity = m_swapChainCapacity * 2;
            auto** newArray = new IRHISwapChain*[newCapacity];
            std::memcpy(newArray, m_swapChains, sizeof(IRHISwapChain*) * m_swapChainCount);
            std::memset(newArray + m_swapChainCount, 0, sizeof(IRHISwapChain*) * (newCapacity - m_swapChainCount));
            delete[] m_swapChains;
            m_swapChains = newArray;
            m_swapChainCapacity = newCapacity;
        }

        m_swapChains[m_swapChainCount++] = swapChain;
    }

    void RHIMultiSwapChainPresenter::RemoveSwapChain(IRHISwapChain* swapChain)
    {
        for (uint32 i = 0; i < m_swapChainCount; ++i)
        {
            if (m_swapChains[i] == swapChain)
            {
                // 末尾と入れ替えて削除
                m_swapChains[i] = m_swapChains[m_swapChainCount - 1];
                m_swapChains[m_swapChainCount - 1] = nullptr;
                --m_swapChainCount;
                return;
            }
        }
    }

    void RHIMultiSwapChainPresenter::ClearSwapChains()
    {
        for (uint32 i = 0; i < m_swapChainCount; ++i)
        {
            m_swapChains[i] = nullptr;
        }
        m_swapChainCount = 0;
    }

    void RHIMultiSwapChainPresenter::PresentAll(uint32 syncInterval)
    {
        for (uint32 i = 0; i < m_swapChainCount; ++i)
        {
            if (m_swapChains[i] != nullptr)
            {
                m_swapChains[i]->Present(syncInterval);
            }
        }
    }

    void RHIMultiSwapChainPresenter::Present(IRHISwapChain* swapChain, uint32 syncInterval)
    {
        if (swapChain != nullptr)
        {
            swapChain->Present(syncInterval);
        }
    }

} // namespace NS::RHI
