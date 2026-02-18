/// @file D3D12SwapChain.cpp
/// @brief D3D12 SwapChain実装
#include "D3D12SwapChain.h"
#include "D3D12Device.h"
#include "D3D12Queue.h"
#include "D3D12Texture.h"
#include "D3D12View.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // デストラクタ
    //=========================================================================

    D3D12SwapChain::~D3D12SwapChain()
    {
        Shutdown();
    }

    //=========================================================================
    // 初期化
    //=========================================================================

    bool D3D12SwapChain::Init(D3D12Device* device,
                              IDXGIFactory6* factory,
                              D3D12Queue* queue,
                              const NS::RHI::RHISwapChainDesc& desc,
                              const char* debugName)
    {
        if (!device || !factory || !queue || !desc.windowHandle)
        {
            LOG_ERROR("[D3D12RHI] SwapChain::Init - invalid parameters");
            return false;
        }

        device_ = device;
        queue_ = queue;
        width_ = desc.width;
        height_ = desc.height;
        format_ = desc.format;
        bufferCount_ = desc.bufferCount > 0 ? desc.bufferCount : 2;
        presentMode_ = desc.presentMode;
        flags_ = desc.flags;

        // Tearing対応チェック
        tearingSupported_ = CheckTearingSupport(factory);

        // SwapChain記述
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = width_;
        swapChainDesc.Height = height_;
        swapChainDesc.Format = D3D12Texture::ConvertPixelFormat(format_);
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = bufferCount_;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;

        // スケーリングモード
        switch (desc.scalingMode)
        {
        case NS::RHI::ERHIScalingMode::None:
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            break;
        case NS::RHI::ERHIScalingMode::AspectRatioStretch:
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
            break;
        default:
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            break;
        }

        // アルファモード
        switch (desc.alphaMode)
        {
        case NS::RHI::ERHIAlphaMode::Premultiplied:
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            break;
        case NS::RHI::ERHIAlphaMode::Straight:
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_STRAIGHT;
            break;
        default:
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            break;
        }

        // Tearingフラグ
        if (tearingSupported_ && EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::AllowTearing))
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        // フレームレイテンシウェイタブルオブジェクト
        if (EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::FrameLatencyWaitableObject))
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        }

        // モードスイッチ許可
        if (EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::AllowModeSwitch))
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        }

        // SwapChain作成
        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(queue->GetD3DCommandQueue(),
                                                     static_cast<HWND>(desc.windowHandle),
                                                     &swapChainDesc,
                                                     nullptr, // フルスクリーン記述
                                                     nullptr, // 出力制限
                                                     &swapChain1);

        if (FAILED(hr))
        {
            D3D12RHI_CHECK_HR(hr);
            LOG_ERROR("[D3D12RHI] Failed to create swap chain");
            return false;
        }

        // IDXGISwapChain4にアップキャスト
        hr = swapChain1.As(&swapChain_);
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12RHI] Failed to query IDXGISwapChain4");
            return false;
        }

        // ALT+Enterの自動処理を無効化（自分で制御する）
        factory->MakeWindowAssociation(static_cast<HWND>(desc.windowHandle), DXGI_MWA_NO_ALT_ENTER);

        // フレームレイテンシウェイタブルオブジェクト取得
        if (EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::FrameLatencyWaitableObject))
        {
            swapChain_->SetMaximumFrameLatency(maxFrameLatency_);
            frameLatencyHandle_ = swapChain_->GetFrameLatencyWaitableObject();
        }

        // バックバッファ取得
        if (!AcquireBackBuffers())
        {
            LOG_ERROR("[D3D12RHI] Failed to acquire back buffers");
            return false;
        }

        if (debugName)
        {
            LOG_INFO("[D3D12RHI] SwapChain created");
        }

        return true;
    }

    //=========================================================================
    // シャットダウン
    //=========================================================================

    void D3D12SwapChain::Shutdown()
    {
        ReleaseBackBuffers();

        if (frameLatencyHandle_)
        {
            CloseHandle(frameLatencyHandle_);
            frameLatencyHandle_ = nullptr;
        }

        swapChain_.Reset();
        queue_ = nullptr;
        device_ = nullptr;
    }

    //=========================================================================
    // バックバッファ取得
    //=========================================================================

    bool D3D12SwapChain::AcquireBackBuffers()
    {
        backBufferTextures_.resize(bufferCount_);
        backBufferRTVs_.resize(bufferCount_);

        for (uint32 i = 0; i < bufferCount_; ++i)
        {
            // SwapChainからバックバッファリソース取得
            ComPtr<ID3D12Resource> backBufferResource;
            HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBufferResource));
            if (FAILED(hr))
            {
                D3D12RHI_CHECK_HR(hr);
                LOG_ERROR("[D3D12RHI] Failed to get back buffer");
                ReleaseBackBuffers();
                return false;
            }

            // D3D12Textureとしてラップ
            auto* texture = new D3D12Texture();
            if (!texture->InitFromExisting(
                    device_, std::move(backBufferResource), format_, NS::RHI::ERHIResourceState::Present))
            {
                delete texture;
                ReleaseBackBuffers();
                return false;
            }

            backBufferTextures_[i] = texture;

            // RTV作成
            NS::RHI::RHIRenderTargetViewDesc rtvDesc = NS::RHI::RHIRenderTargetViewDesc::Texture2D(texture, 0);

            auto* rtv = new D3D12RenderTargetView();
            if (!rtv->Init(device_, rtvDesc, nullptr))
            {
                delete rtv;
                ReleaseBackBuffers();
                return false;
            }

            backBufferRTVs_[i] = rtv;
        }

        return true;
    }

    //=========================================================================
    // バックバッファ解放
    //=========================================================================

    void D3D12SwapChain::ReleaseBackBuffers()
    {
        for (auto* rtv : backBufferRTVs_)
        {
            delete static_cast<D3D12RenderTargetView*>(rtv);
        }
        backBufferRTVs_.clear();

        for (auto* tex : backBufferTextures_)
        {
            delete static_cast<D3D12Texture*>(tex);
        }
        backBufferTextures_.clear();
    }

    //=========================================================================
    // Tearing対応チェック
    //=========================================================================

    bool D3D12SwapChain::CheckTearingSupport(IDXGIFactory6* factory)
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        return SUCCEEDED(hr) && allowTearing;
    }

    //=========================================================================
    // IRHISwapChain — 基本プロパティ
    //=========================================================================

    NS::RHI::IRHIDevice* D3D12SwapChain::GetDevice() const
    {
        return device_;
    }

    //=========================================================================
    // IRHISwapChain — バックバッファ
    //=========================================================================

    uint32 D3D12SwapChain::GetCurrentBackBufferIndex() const
    {
        if (!swapChain_)
            return 0;
        return swapChain_->GetCurrentBackBufferIndex();
    }

    NS::RHI::IRHITexture* D3D12SwapChain::GetBackBuffer(uint32 index) const
    {
        if (index >= static_cast<uint32>(backBufferTextures_.size()))
            return nullptr;
        return backBufferTextures_[index];
    }

    NS::RHI::IRHIRenderTargetView* D3D12SwapChain::GetBackBufferRTV(uint32 index) const
    {
        if (index >= static_cast<uint32>(backBufferRTVs_.size()))
            return nullptr;
        return backBufferRTVs_[index];
    }

    //=========================================================================
    // IRHISwapChain — フルスクリーン
    //=========================================================================

    bool D3D12SwapChain::IsFullscreen() const
    {
        if (!swapChain_)
            return false;

        BOOL fullscreen = FALSE;
        swapChain_->GetFullscreenState(&fullscreen, nullptr);
        return fullscreen != FALSE;
    }

    bool D3D12SwapChain::SetFullscreen(bool fullscreen, const NS::RHI::RHIFullscreenDesc* /*desc*/)
    {
        if (!swapChain_)
            return false;

        HRESULT hr = swapChain_->SetFullscreenState(fullscreen ? TRUE : FALSE, nullptr);
        if (FAILED(hr))
        {
            D3D12RHI_CHECK_HR(hr);
            LOG_ERROR("[D3D12RHI] SetFullscreenState failed");
            return false;
        }

        return true;
    }

    //=========================================================================
    // IRHISwapChain — リサイズ
    //=========================================================================

    bool D3D12SwapChain::Resize(const NS::RHI::RHISwapChainResizeDesc& desc)
    {
        if (!swapChain_ || !device_)
            return false;

        // GPU完了待機
        device_->WaitIdle();

        // 既存バックバッファ解放
        ReleaseBackBuffers();

        // 新しいサイズ決定
        uint32 newWidth = desc.width > 0 ? desc.width : width_;
        uint32 newHeight = desc.height > 0 ? desc.height : height_;
        uint32 newBufferCount = desc.bufferCount > 0 ? desc.bufferCount : bufferCount_;
        DXGI_FORMAT newFormat = desc.format != NS::RHI::ERHIPixelFormat::Unknown
                                    ? D3D12Texture::ConvertPixelFormat(desc.format)
                                    : D3D12Texture::ConvertPixelFormat(format_);

        UINT resizeFlags = 0;
        if (tearingSupported_ && EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::AllowTearing))
        {
            resizeFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
        if (EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::FrameLatencyWaitableObject))
        {
            resizeFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        }

        HRESULT hr = swapChain_->ResizeBuffers(newBufferCount, newWidth, newHeight, newFormat, resizeFlags);

        if (FAILED(hr))
        {
            D3D12RHI_CHECK_HR(hr);
            LOG_ERROR("[D3D12RHI] ResizeBuffers failed");
            return false;
        }

        // 状態更新
        width_ = newWidth;
        height_ = newHeight;
        bufferCount_ = newBufferCount;
        if (desc.format != NS::RHI::ERHIPixelFormat::Unknown)
        {
            format_ = desc.format;
        }

        // バックバッファ再取得
        if (!AcquireBackBuffers())
        {
            LOG_ERROR("[D3D12RHI] Failed to re-acquire back buffers after resize");
            return false;
        }

        LOG_INFO("[D3D12RHI] SwapChain resized");
        return true;
    }

    //=========================================================================
    // IRHISwapChain — イベント
    //=========================================================================

    void D3D12SwapChain::SetEventCallback(NS::RHI::RHISwapChainEventCallback callback, void* userData)
    {
        eventCallback_ = callback;
        eventUserData_ = userData;
    }

    bool D3D12SwapChain::ProcessWindowMessage(void* /*hwnd*/, uint32 message, uint64 /*wParam*/, int64 /*lParam*/)
    {
        // WM_SIZE処理
        constexpr uint32 WM_SIZE_MSG = 0x0005;
        if (message == WM_SIZE_MSG && eventCallback_)
        {
            eventCallback_(this, NS::RHI::ERHISwapChainEvent::ResizeNeeded, eventUserData_);
            return true;
        }
        return false;
    }

    //=========================================================================
    // IRHISwapChain — フレームレイテンシ
    //=========================================================================

    void D3D12SwapChain::SetMaximumFrameLatency(uint32 maxLatency)
    {
        maxFrameLatency_ = maxLatency;
        if (swapChain_ && EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::FrameLatencyWaitableObject))
        {
            swapChain_->SetMaximumFrameLatency(maxLatency);
        }
    }

    bool D3D12SwapChain::WaitForNextFrame(uint64 timeoutMs)
    {
        if (!frameLatencyHandle_)
            return true;

        DWORD waitMs = (timeoutMs == UINT64_MAX) ? INFINITE : static_cast<DWORD>(timeoutMs);
        DWORD result = WaitForSingleObject(frameLatencyHandle_, waitMs);
        return result == WAIT_OBJECT_0;
    }

    //=========================================================================
    // IRHISwapChain — Present
    //=========================================================================

    NS::RHI::ERHIPresentResult D3D12SwapChain::Present(const NS::RHI::RHIPresentParams& params)
    {
        if (!swapChain_)
            return NS::RHI::ERHIPresentResult::Error;

        UINT syncInterval = params.syncInterval;
        UINT presentFlags = 0;

        // Tearing対応: syncInterval=0 + ALLOW_TEARING
        if (tearingSupported_ && syncInterval == 0 &&
            EnumHasAnyFlags(flags_, NS::RHI::ERHISwapChainFlags::AllowTearing) && !IsFullscreen())
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        // その他のフラグ
        if (EnumHasAnyFlags(params.flags, NS::RHI::ERHIPresentFlags::Test))
        {
            presentFlags |= DXGI_PRESENT_TEST;
        }
        if (EnumHasAnyFlags(params.flags, NS::RHI::ERHIPresentFlags::DoNotWait))
        {
            presentFlags |= DXGI_PRESENT_DO_NOT_WAIT;
        }
        if (EnumHasAnyFlags(params.flags, NS::RHI::ERHIPresentFlags::RestartFrame))
        {
            presentFlags |= DXGI_PRESENT_RESTART;
        }

        // Dirty rects
        DXGI_PRESENT_PARAMETERS presentParams{};
        if (params.dirtyRects && params.dirtyRectCount > 0 &&
            EnumHasAnyFlags(params.flags, NS::RHI::ERHIPresentFlags::UseDirtyRects))
        {
            presentParams.DirtyRectsCount = params.dirtyRectCount;
            presentParams.pDirtyRects = reinterpret_cast<RECT*>(const_cast<NS::RHI::RHIDirtyRect*>(params.dirtyRects));
        }
        if (params.scrollRect && EnumHasAnyFlags(params.flags, NS::RHI::ERHIPresentFlags::UseScrollRect))
        {
            presentParams.pScrollRect =
                reinterpret_cast<RECT*>(const_cast<NS::RHI::RHIDirtyRect*>(&params.scrollRect->source));
            presentParams.pScrollOffset = reinterpret_cast<POINT*>(const_cast<int32*>(&params.scrollRect->offsetX));
        }

        // Present実行（リトライ対応: E_INVALIDARG/DXGI_ERROR_INVALID_CALL は最大5回）
        constexpr uint32 kMaxRetries = 5;
        HRESULT hr = E_FAIL;

        for (uint32 retry = 0; retry <= kMaxRetries; ++retry)
        {
            if (presentParams.DirtyRectsCount > 0 || presentParams.pScrollRect)
            {
                hr = swapChain_->Present1(syncInterval, presentFlags, &presentParams);
            }
            else
            {
                hr = swapChain_->Present(syncInterval, presentFlags);
            }

            // リトライ可能なエラーでなければ抜ける
            if (hr != E_INVALIDARG && hr != DXGI_ERROR_INVALID_CALL)
                break;

            if (retry < kMaxRetries)
            {
                LOG_ERROR("[D3D12RHI] Present failed, retrying");
            }
        }

        ++presentCount_;

        // 結果変換
        switch (hr)
        {
        case S_OK:
            occluded_ = false;
            return NS::RHI::ERHIPresentResult::Success;
        case DXGI_STATUS_OCCLUDED:
            occluded_ = true;
            return NS::RHI::ERHIPresentResult::Occluded;
        case DXGI_ERROR_DEVICE_RESET:
            return NS::RHI::ERHIPresentResult::DeviceReset;
        case DXGI_ERROR_DEVICE_REMOVED:
            return NS::RHI::ERHIPresentResult::DeviceLost;
        case DXGI_ERROR_WAS_STILL_DRAWING:
            return NS::RHI::ERHIPresentResult::FrameSkipped;
        default:
            D3D12RHI_CHECK_HR(hr);
            LOG_ERROR("[D3D12RHI] Present failed after retries");
            return NS::RHI::ERHIPresentResult::Error;
        }
    }

    //=========================================================================
    // IRHISwapChain — フレーム統計
    //=========================================================================

    bool D3D12SwapChain::GetFrameStatistics(NS::RHI::RHIFrameStatistics& outStats) const
    {
        if (!swapChain_)
            return false;

        DXGI_FRAME_STATISTICS stats{};
        HRESULT hr = swapChain_->GetFrameStatistics(&stats);
        if (FAILED(hr))
            return false;

        outStats.presentCount = stats.PresentCount;
        outStats.presentRefreshCount = stats.PresentRefreshCount;
        outStats.syncRefreshCount = stats.SyncRefreshCount;
        outStats.syncQPCTime = stats.SyncQPCTime.QuadPart;
        outStats.syncGPUTime = stats.SyncGPUTime.QuadPart;
        outStats.frameNumber = presentCount_;

        return true;
    }

    bool D3D12SwapChain::WaitForPresentCompletion(uint64 /*presentId*/, uint64 /*timeoutMs*/)
    {
        // DXGI 1.5+ Present1でのWait — 基本実装
        return true;
    }

    //=========================================================================
    // IRHISwapChain — Resize + Present
    //=========================================================================

    NS::RHI::ERHIPresentResult D3D12SwapChain::PresentAndResize(uint32 width,
                                                                uint32 height,
                                                                NS::RHI::ERHIPixelFormat format,
                                                                NS::RHI::ERHISwapChainFlags flags)
    {
        // まずPresent
        NS::RHI::RHIPresentParams params;
        params.syncInterval = (presentMode_ == NS::RHI::ERHIPresentMode::Immediate) ? 0 : 1;
        auto result = Present(params);

        // Resize
        NS::RHI::RHISwapChainResizeDesc resizeDesc;
        resizeDesc.width = width;
        resizeDesc.height = height;
        resizeDesc.format = format;
        resizeDesc.flags = flags;
        Resize(resizeDesc);

        return result;
    }

    //=========================================================================
    // IRHISwapChain — HDR
    //=========================================================================

    bool D3D12SwapChain::SetColorSpace(uint8 colorSpace)
    {
        if (!swapChain_)
            return false;

        DXGI_COLOR_SPACE_TYPE dxgiColorSpace;
        switch (colorSpace)
        {
        case 0: // SDR
            dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
            break;
        case 1: // HDR ST2084
            dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;
        case 2: // HDR scRGB
            dxgiColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;
        default:
            return false;
        }

        UINT colorSpaceSupport = 0;
        HRESULT hr = swapChain_->CheckColorSpaceSupport(dxgiColorSpace, &colorSpaceSupport);
        if (FAILED(hr) || !(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
            return false;
        }

        hr = swapChain_->SetColorSpace1(dxgiColorSpace);
        if (FAILED(hr))
        {
            D3D12RHI_CHECK_HR(hr);
            LOG_ERROR("[D3D12RHI] SetColorSpace1 failed");
            return false;
        }

        colorSpace_ = colorSpace;
        return true;
    }

    bool D3D12SwapChain::SetHDREnabled(bool enabled)
    {
        if (!swapChain_)
            return false;

        if (enabled)
        {
            // HDR ST2084を試行
            if (SetColorSpace(1))
            {
                hdrEnabled_ = true;
                return true;
            }
            return false;
        }
        else
        {
            // SDRに戻す
            SetColorSpace(0);
            hdrEnabled_ = false;
            return true;
        }
    }

} // namespace NS::D3D12RHI
