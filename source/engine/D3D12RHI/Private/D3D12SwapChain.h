/// @file D3D12SwapChain.h
/// @brief D3D12 SwapChain実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHISwapChain.h"

#include <vector>

namespace NS::D3D12RHI
{
    class D3D12Device;
    class D3D12Queue;

    //=========================================================================
    // D3D12SwapChain
    //=========================================================================

    class D3D12SwapChain final : public NS::RHI::IRHISwapChain
    {
    public:
        D3D12SwapChain() = default;
        ~D3D12SwapChain() override;

        /// 初期化
        bool Init(D3D12Device* device,
                  IDXGIFactory6* factory,
                  D3D12Queue* queue,
                  const NS::RHI::RHISwapChainDesc& desc,
                  const char* debugName);

        /// シャットダウン
        void Shutdown();

        /// ネイティブSwapChain取得
        IDXGISwapChain4* GetDXGISwapChain() const { return swapChain_.Get(); }

        //=====================================================================
        // IRHISwapChain — 基本プロパティ
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        uint32 GetWidth() const override { return width_; }
        uint32 GetHeight() const override { return height_; }
        NS::RHI::ERHIPixelFormat GetFormat() const override { return format_; }
        uint32 GetBufferCount() const override { return bufferCount_; }
        NS::RHI::ERHIPresentMode GetPresentMode() const override { return presentMode_; }
        NS::RHI::ERHISwapChainFlags GetFlags() const override { return flags_; }

        //=====================================================================
        // IRHISwapChain — バックバッファ
        //=====================================================================

        uint32 GetCurrentBackBufferIndex() const override;
        NS::RHI::IRHITexture* GetBackBuffer(uint32 index) const override;
        NS::RHI::IRHIRenderTargetView* GetBackBufferRTV(uint32 index) const override;

        //=====================================================================
        // IRHISwapChain — フルスクリーン
        //=====================================================================

        bool IsFullscreen() const override;
        bool SetFullscreen(bool fullscreen, const NS::RHI::RHIFullscreenDesc* desc) override;

        //=====================================================================
        // IRHISwapChain — 状態
        //=====================================================================

        bool IsOccluded() const override { return occluded_; }
        bool IsHDREnabled() const override { return hdrEnabled_; }
        bool IsVariableRefreshRateEnabled() const override { return tearingSupported_; }

        //=====================================================================
        // IRHISwapChain — リサイズ
        //=====================================================================

        bool Resize(const NS::RHI::RHISwapChainResizeDesc& desc) override;

        //=====================================================================
        // IRHISwapChain — イベント
        //=====================================================================

        void SetEventCallback(NS::RHI::RHISwapChainEventCallback callback, void* userData) override;
        bool ProcessWindowMessage(void* hwnd, uint32 message, uint64 wParam, int64 lParam) override;

        //=====================================================================
        // IRHISwapChain — フレームレイテンシ
        //=====================================================================

        void* GetFrameLatencyWaitableObject() const override { return frameLatencyHandle_; }
        void SetMaximumFrameLatency(uint32 maxLatency) override;
        uint32 GetCurrentFrameLatency() const override { return maxFrameLatency_; }
        bool WaitForNextFrame(uint64 timeoutMs) override;

        //=====================================================================
        // IRHISwapChain — Present
        //=====================================================================

        NS::RHI::ERHIPresentResult Present(const NS::RHI::RHIPresentParams& params) override;

        //=====================================================================
        // IRHISwapChain — フレーム統計
        //=====================================================================

        bool GetFrameStatistics(NS::RHI::RHIFrameStatistics& outStats) const override;
        uint64 GetLastPresentId() const override { return presentCount_; }
        bool WaitForPresentCompletion(uint64 presentId, uint64 timeoutMs) override;

        //=====================================================================
        // IRHISwapChain — Resize + Present
        //=====================================================================

        NS::RHI::ERHIPresentResult PresentAndResize(uint32 width,
                                                    uint32 height,
                                                    NS::RHI::ERHIPixelFormat format,
                                                    NS::RHI::ERHISwapChainFlags flags) override;

        //=====================================================================
        // IRHISwapChain — HDR
        //=====================================================================

        bool SetColorSpace(uint8 colorSpace) override;
        uint8 GetColorSpace() const override { return colorSpace_; }
        bool SetHDREnabled(bool enabled) override;
        void SetHDRAutoSwitch(bool enabled) override { hdrAutoSwitch_ = enabled; }
        bool SupportsAutoHDR() const override { return false; }

    private:
        /// バックバッファ取得・RTV作成
        bool AcquireBackBuffers();

        /// バックバッファ解放
        void ReleaseBackBuffers();

        /// Tearing対応チェック
        static bool CheckTearingSupport(IDXGIFactory6* factory);

        D3D12Device* device_ = nullptr;
        D3D12Queue* queue_ = nullptr;
        ComPtr<IDXGISwapChain4> swapChain_;

        // バックバッファ
        std::vector<NS::RHI::IRHITexture*> backBufferTextures_;
        std::vector<NS::RHI::IRHIRenderTargetView*> backBufferRTVs_;

        // 設定
        uint32 width_ = 0;
        uint32 height_ = 0;
        NS::RHI::ERHIPixelFormat format_ = NS::RHI::ERHIPixelFormat::R8G8B8A8_UNORM;
        uint32 bufferCount_ = 2;
        NS::RHI::ERHIPresentMode presentMode_ = NS::RHI::ERHIPresentMode::VSync;
        NS::RHI::ERHISwapChainFlags flags_ = NS::RHI::ERHISwapChainFlags::None;

        // 状態
        bool tearingSupported_ = false;
        bool occluded_ = false;
        bool hdrEnabled_ = false;
        bool hdrAutoSwitch_ = false;
        uint8 colorSpace_ = 0;
        uint64 presentCount_ = 0;
        uint32 maxFrameLatency_ = 2;
        HANDLE frameLatencyHandle_ = nullptr;

        // イベント
        NS::RHI::RHISwapChainEventCallback eventCallback_ = nullptr;
        void* eventUserData_ = nullptr;
    };

} // namespace NS::D3D12RHI
