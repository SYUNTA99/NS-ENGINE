/// @file IRHIPipelineState.cpp
/// @brief RHIAsyncComputeHelper 実装
#include "IRHIPipelineState.h"
#include "IRHICommandContext.h"
#include "IRHIComputeContext.h"
#include "IRHIDevice.h"
#include "IRHIFence.h"
#include "IRHIQueue.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIAsyncComputeHelper
    //=========================================================================

    bool RHIAsyncComputeHelper::Initialize(IRHIDevice* device)
    {
        m_device = device;

        m_computeQueue = device->GetComputeQueue();
        if (m_computeQueue == nullptr) {
            return false;
}

        m_computeFence = device->CreateFence(0, "AsyncComputeFence");
        if (!m_computeFence) {
            return false;
}

        m_nextFenceValue = 1;
        return true;
    }

    void RHIAsyncComputeHelper::Shutdown()
    {
        if (m_computeFence)
        {
            // 全コンピュート完了待ち
            m_computeFence->Wait(m_nextFenceValue - 1);
        }

        m_computeFence.Reset();
        m_computeQueue = nullptr;
        m_device = nullptr;
    }

    uint64 RHIAsyncComputeHelper::InsertGraphicsToComputeSync(IRHICommandContext* graphicsContext)
    {
        (void)graphicsContext;
        // グラフィックスキューのフェンスをシグナルし、
        // コンピュートキューで待機するポイントを返す
        uint64 const value = m_nextFenceValue++;
        IRHIQueue* graphicsQueue = m_device->GetGraphicsQueue();
        graphicsQueue->Signal(m_computeFence.Get(), value);
        return value;
    }

    uint64 RHIAsyncComputeHelper::InsertComputeToGraphicsSync(IRHIComputeContext* computeContext)
    {
        (void)computeContext;
        // コンピュートキューのフェンスをシグナルし、
        // グラフィックスキューで待機するポイントを返す
        uint64 const value = m_nextFenceValue++;
        m_computeQueue->Signal(m_computeFence.Get(), value);
        return value;
    }

    void RHIAsyncComputeHelper::WaitForComputeOnGraphics(IRHICommandContext* graphicsContext, uint64 computeFenceValue)
    {
        (void)graphicsContext;
        // グラフィックスキューでコンピュートフェンスを待機
        IRHIQueue* graphicsQueue = m_device->GetGraphicsQueue();
        graphicsQueue->Wait(m_computeFence.Get(), computeFenceValue);
    }

    void RHIAsyncComputeHelper::WaitForGraphicsOnCompute(IRHIComputeContext* computeContext, uint64 graphicsFenceValue)
    {
        (void)computeContext;
        // コンピュートキューでグラフィックスフェンスを待機
        m_computeQueue->Wait(m_computeFence.Get(), graphicsFenceValue);
    }

    uint64 RHIAsyncComputeHelper::ExecuteAsync(ComputeSetupFunc setupFunc)
    {
        if ((m_device == nullptr) || (m_computeQueue == nullptr)) {
            return 0;
}

        IRHIComputeContext* ctx = m_device->ObtainComputeContext();
        if (ctx == nullptr) {
            return 0;
}

        setupFunc(ctx);

        uint64 const fenceValue = m_nextFenceValue++;
        m_computeQueue->ExecuteContext(reinterpret_cast<IRHICommandContext*>(ctx));
        m_computeQueue->Signal(m_computeFence.Get(), fenceValue);

        m_device->ReleaseContext(ctx);
        return fenceValue;
    }

} // namespace NS::RHI
