/// @file D3D12Query.cpp
/// @brief D3D12 クエリヒープ実装
#include "D3D12Query.h"
#include "D3D12Device.h"
#include "RHI/Public/IRHICommandContext.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12QueryHeap
    //=========================================================================

    bool D3D12QueryHeap::Init(D3D12Device* device, const NS::RHI::RHIQueryHeapDesc& desc, const char* debugName)
    {
        if (!device || desc.count == 0)
            return false;

        device_ = device;
        type_ = desc.type;
        count_ = desc.count;
        statsFlags_ = desc.pipelineStatisticsFlags;
        d3dQueryType_ = ConvertQueryType(desc.type);

        auto* d3dDevice = device_->GetD3DDevice();

        // クエリヒープ作成
        D3D12_QUERY_HEAP_DESC heapDesc{};
        heapDesc.Type = ConvertQueryHeapType(desc.type);
        heapDesc.Count = desc.count;
        heapDesc.NodeMask = desc.nodeMask;

        HRESULT hr = d3dDevice->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create query heap");
            return false;
        }

        // Readbackバッファ作成（永続Map）
        uint32 resultSize = GetQueryResultSize();
        uint64 bufferSize = static_cast<uint64>(resultSize) * desc.count;

        D3D12_RESOURCE_DESC bufferDesc{};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = bufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        hr = d3dDevice->CreateCommittedResource(&heapProps,
                                                D3D12_HEAP_FLAG_NONE,
                                                &bufferDesc,
                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                nullptr,
                                                IID_PPV_ARGS(&readbackBuffer_));
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create query readback buffer");
            return false;
        }

        // 永続Map（READBACKヒープはUnmap不要）
        hr = readbackBuffer_->Map(0, nullptr, &mappedPtr_);
        if (FAILED(hr))
        {
            LOG_HRESULT(hr, "[D3D12RHI] Failed to map query readback buffer");
            return false;
        }

        // デバッグ名設定
        if (debugName && debugName[0] != '\0')
        {
            wchar_t wName[256]{};
            MultiByteToWideChar(CP_UTF8, 0, debugName, -1, wName, 256);
            queryHeap_->SetName(wName);

            wchar_t wBufName[256]{};
            wsprintfW(wBufName, L"%s_Readback", wName);
            readbackBuffer_->SetName(wBufName);
        }

        return true;
    }

    NS::RHI::IRHIDevice* D3D12QueryHeap::GetDevice() const
    {
        return static_cast<NS::RHI::IRHIDevice*>(device_);
    }

    uint32 D3D12QueryHeap::GetQueryResultSize() const
    {
        switch (type_)
        {
        case NS::RHI::ERHIQueryType::Timestamp:
        case NS::RHI::ERHIQueryType::Occlusion:
        case NS::RHI::ERHIQueryType::BinaryOcclusion:
        case NS::RHI::ERHIQueryType::Predication:
            return sizeof(uint64); // 8 bytes
        case NS::RHI::ERHIQueryType::PipelineStatistics:
            return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
        case NS::RHI::ERHIQueryType::StreamOutputStatistics:
        case NS::RHI::ERHIQueryType::StreamOutputOverflow:
            return sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
        default:
            return sizeof(uint64);
        }
    }

    //=========================================================================
    // D3D12GPUProfiler
    //=========================================================================

    D3D12GPUProfiler::~D3D12GPUProfiler()
    {
        Shutdown();
    }

    bool D3D12GPUProfiler::Init(D3D12Device* device)
    {
        if (!device)
            return false;

        device_ = device;

        // Timestamp周波数取得
        timestampFrequency_ = device->GetTimestampFrequency();

        // Timestamp QueryHeap作成（開始/終了ペア × kMaxScopesPerFrame × kMaxBufferedFrames + フレーム全体ペア）
        uint32 totalQueries = (kMaxScopesPerFrame * 2 + 2) * kMaxBufferedFrames;
        auto heapDesc = NS::RHI::RHIQueryHeapDesc::Timestamp(totalQueries);
        timestampHeap_ = static_cast<D3D12QueryHeap*>(device->CreateQueryHeap(heapDesc, "GPUProfiler_Timestamps"));
        if (!timestampHeap_)
        {
            LOG_ERROR("[D3D12GPUProfiler] Failed to create timestamp heap");
            return false;
        }

        return true;
    }

    void D3D12GPUProfiler::Shutdown()
    {
        if (timestampHeap_)
        {
            delete timestampHeap_;
            timestampHeap_ = nullptr;
        }
        device_ = nullptr;
        profiling_ = false;
    }

    void D3D12GPUProfiler::BeginProfiling()
    {
        profiling_ = true;
    }

    void D3D12GPUProfiler::EndProfiling()
    {
        profiling_ = false;
    }

    void D3D12GPUProfiler::BeginFrame()
    {
        if (!profiling_) return;

        auto& frame = frames_[currentFrameIdx_];
        frame.frameNumber = frameNumber_;
        frame.scopes.clear();
        frame.resolved = false;

        // フレーム開始クエリインデックス（フレーム全体 + スコープ用）
        uint32 queriesPerFrame = kMaxScopesPerFrame * 2 + 2;
        currentQueryIndex_ = currentFrameIdx_ * queriesPerFrame;
        currentParentScope_ = -1;
        currentDepth_ = 0;
    }

    void D3D12GPUProfiler::EndFrame(NS::RHI::IRHICommandContext* context)
    {
        if (!profiling_ || !context || !timestampHeap_) return;

        auto& frame = frames_[currentFrameIdx_];
        uint32 queriesPerFrame = kMaxScopesPerFrame * 2 + 2;
        uint32 baseIndex = currentFrameIdx_ * queriesPerFrame;
        uint32 usedQueries = currentQueryIndex_ - baseIndex;

        // ResolveQueryData: ヒープ内蔵Readbackバッファへ解決
        if (usedQueries > 0)
        {
            context->ResolveQueryData(timestampHeap_, baseIndex, usedQueries, nullptr, 0);
        }

        frame.resolved = true;
        frameNumber_++;
        currentFrameIdx_ = (currentFrameIdx_ + 1) % kMaxBufferedFrames;
    }

    uint32 D3D12GPUProfiler::BeginScope(NS::RHI::IRHICommandContext* context,
                                         const char* name,
                                         NS::RHI::ERHIGPUProfileEventType type,
                                         NS::RHI::ERHIGPUProfileFlags /*flags*/)
    {
        if (!profiling_ || !context || !timestampHeap_) return UINT32_MAX;

        auto& frame = frames_[currentFrameIdx_];
        if (frame.scopes.size() >= kMaxScopesPerFrame) return UINT32_MAX;

        uint32 scopeId = static_cast<uint32>(frame.scopes.size());
        ScopeData scope;
        scope.name = name;
        scope.type = type;
        scope.startQueryIndex = currentQueryIndex_++;
        scope.endQueryIndex = 0; // EndScopeで設定
        scope.parentIndex = currentParentScope_;
        scope.depth = currentDepth_;
        frame.scopes.push_back(scope);

        // Timestampクエリ発行
        context->WriteTimestamp(timestampHeap_, scope.startQueryIndex);

        currentParentScope_ = static_cast<int32>(scopeId);
        currentDepth_++;
        return scopeId;
    }

    void D3D12GPUProfiler::EndScope(NS::RHI::IRHICommandContext* context, uint32 scopeId)
    {
        if (!profiling_ || !context || !timestampHeap_) return;

        auto& frame = frames_[currentFrameIdx_];
        if (scopeId >= frame.scopes.size()) return;

        auto& scope = frame.scopes[scopeId];
        scope.endQueryIndex = currentQueryIndex_++;

        // Timestampクエリ発行
        context->WriteTimestamp(timestampHeap_, scope.endQueryIndex);

        currentParentScope_ = scope.parentIndex;
        if (currentDepth_ > 0) currentDepth_--;
    }

    bool D3D12GPUProfiler::IsFrameReady(uint64 frameNumber) const
    {
        for (uint32 i = 0; i < kMaxBufferedFrames; ++i)
        {
            if (frames_[i].frameNumber == frameNumber && frames_[i].resolved)
                return true;
        }
        return false;
    }

    bool D3D12GPUProfiler::GetFrameResults(uint64 frameNumber,
                                            std::vector<NS::RHI::RHIGPUProfileEvent>& outEvents)
    {
        const FrameData* frame = nullptr;
        for (uint32 i = 0; i < kMaxBufferedFrames; ++i)
        {
            if (frames_[i].frameNumber == frameNumber && frames_[i].resolved)
            {
                frame = &frames_[i];
                break;
            }
        }
        if (!frame || !timestampHeap_) return false;

        const uint64* timestamps = static_cast<const uint64*>(timestampHeap_->GetMappedPtr());
        if (!timestamps) return false;

        double ticksToMicro = (timestampFrequency_ > 0)
                                  ? (1000000.0 / static_cast<double>(timestampFrequency_))
                                  : 0.0;

        outEvents.clear();
        outEvents.reserve(frame->scopes.size());

        for (const auto& scope : frame->scopes)
        {
            NS::RHI::RHIGPUProfileEvent event{};
            event.name = scope.name;
            event.type = scope.type;
            event.startTimestamp = timestamps[scope.startQueryIndex];
            event.endTimestamp = timestamps[scope.endQueryIndex];
            event.parentIndex = scope.parentIndex;
            event.depth = scope.depth;
            event.frameNumber = frameNumber;

            if (event.endTimestamp >= event.startTimestamp)
                event.elapsedMicroseconds = static_cast<double>(event.endTimestamp - event.startTimestamp) * ticksToMicro;

            outEvents.push_back(event);
        }
        return true;
    }

    uint64 D3D12GPUProfiler::GetLatestCompletedFrame() const
    {
        uint64 latest = 0;
        for (uint32 i = 0; i < kMaxBufferedFrames; ++i)
        {
            if (frames_[i].resolved && frames_[i].frameNumber > latest)
                latest = frames_[i].frameNumber;
        }
        return latest;
    }

    uint64 D3D12GPUProfiler::GetTimestampFrequency() const
    {
        return timestampFrequency_;
    }

    double D3D12GPUProfiler::GetFrameGPUTime(uint64 frameNumber) const
    {
        std::vector<NS::RHI::RHIGPUProfileEvent> events;
        // const_castは避け、単純にReadbackから計算
        const FrameData* frame = nullptr;
        for (uint32 i = 0; i < kMaxBufferedFrames; ++i)
        {
            if (frames_[i].frameNumber == frameNumber && frames_[i].resolved)
            {
                frame = &frames_[i];
                break;
            }
        }
        if (!frame || frame->scopes.empty() || !timestampHeap_)
            return 0.0;

        const uint64* timestamps = static_cast<const uint64*>(timestampHeap_->GetMappedPtr());
        if (!timestamps) return 0.0;

        // ルートスコープの合計時間
        double total = 0.0;
        double ticksToMicro = (timestampFrequency_ > 0)
                                  ? (1000000.0 / static_cast<double>(timestampFrequency_))
                                  : 0.0;

        for (const auto& scope : frame->scopes)
        {
            if (scope.parentIndex == -1) // ルートスコープのみ
            {
                uint64 start = timestamps[scope.startQueryIndex];
                uint64 end = timestamps[scope.endQueryIndex];
                if (end >= start)
                    total += static_cast<double>(end - start) * ticksToMicro;
            }
        }
        return total;
    }

} // namespace NS::D3D12RHI
