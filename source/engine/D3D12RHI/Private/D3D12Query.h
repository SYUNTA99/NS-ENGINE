/// @file D3D12Query.h
/// @brief D3D12 クエリヒープ + GPUプロファイラー
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/RHIGPUProfiler.h"
#include "RHI/Public/RHIQuery.h"

#include <vector>

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12QueryHeap — IRHIQueryHeap実装
    //=========================================================================

    class D3D12QueryHeap final : public NS::RHI::IRHIQueryHeap
    {
    public:
        D3D12QueryHeap() = default;
        ~D3D12QueryHeap() override = default;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIQueryHeapDesc& desc, const char* debugName);

        // --- IRHIQueryHeap ---
        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::ERHIQueryType GetQueryType() const override { return type_; }
        uint32 GetQueryCount() const override { return count_; }
        NS::RHI::ERHIPipelineStatisticsFlags GetPipelineStatisticsFlags() const override { return statsFlags_; }
        uint32 GetQueryResultSize() const override;
        uint32 GetQueryResultAlignment() const override { return 8; }

        /// ネイティブヒープ取得
        ID3D12QueryHeap* GetD3DQueryHeap() const { return queryHeap_.Get(); }

        /// D3D12クエリタイプ取得
        D3D12_QUERY_TYPE GetD3DQueryType() const { return d3dQueryType_; }

        /// Readbackバッファ取得（ResolveQueryData先）
        ID3D12Resource* GetReadbackBuffer() const { return readbackBuffer_.Get(); }

        /// マップ済みReadbackポインタ取得（永続Map）
        const void* GetMappedPtr() const { return mappedPtr_; }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12QueryHeap> queryHeap_;
        ComPtr<ID3D12Resource> readbackBuffer_;
        void* mappedPtr_ = nullptr;

        NS::RHI::ERHIQueryType type_ = NS::RHI::ERHIQueryType::Timestamp;
        D3D12_QUERY_TYPE d3dQueryType_ = D3D12_QUERY_TYPE_TIMESTAMP;
        uint32 count_ = 0;
        NS::RHI::ERHIPipelineStatisticsFlags statsFlags_ = NS::RHI::ERHIPipelineStatisticsFlags::None;
    };

    //=========================================================================
    // ヘルパー: ERHIQueryType → D3D12型変換
    //=========================================================================

    /// ERHIQueryType → D3D12_QUERY_HEAP_TYPE
    inline D3D12_QUERY_HEAP_TYPE ConvertQueryHeapType(NS::RHI::ERHIQueryType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIQueryType::Occlusion:
        case NS::RHI::ERHIQueryType::BinaryOcclusion:
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        case NS::RHI::ERHIQueryType::Timestamp:
            return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        case NS::RHI::ERHIQueryType::PipelineStatistics:
            return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        case NS::RHI::ERHIQueryType::StreamOutputStatistics:
        case NS::RHI::ERHIQueryType::StreamOutputOverflow:
            return D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
        case NS::RHI::ERHIQueryType::Predication:
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        default:
            return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        }
    }

    /// ERHIQueryType → D3D12_QUERY_TYPE
    inline D3D12_QUERY_TYPE ConvertQueryType(NS::RHI::ERHIQueryType type)
    {
        switch (type)
        {
        case NS::RHI::ERHIQueryType::Occlusion:
            return D3D12_QUERY_TYPE_OCCLUSION;
        case NS::RHI::ERHIQueryType::BinaryOcclusion:
            return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
        case NS::RHI::ERHIQueryType::Timestamp:
            return D3D12_QUERY_TYPE_TIMESTAMP;
        case NS::RHI::ERHIQueryType::PipelineStatistics:
            return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
        case NS::RHI::ERHIQueryType::StreamOutputStatistics:
            return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
        case NS::RHI::ERHIQueryType::StreamOutputOverflow:
            return D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
        case NS::RHI::ERHIQueryType::Predication:
            return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
        default:
            return D3D12_QUERY_TYPE_TIMESTAMP;
        }
    }

    //=========================================================================
    // D3D12GPUProfiler — IRHIGPUProfiler実装
    //=========================================================================

    class D3D12GPUProfiler final : public NS::RHI::IRHIGPUProfiler
    {
    public:
        static constexpr uint32 kMaxScopesPerFrame = 256;
        static constexpr uint32 kMaxBufferedFrames = 3;

        D3D12GPUProfiler() = default;
        ~D3D12GPUProfiler() override;

        bool Init(D3D12Device* device);
        void Shutdown();

        // --- IRHIGPUProfiler ---
        void BeginProfiling() override;
        void EndProfiling() override;
        bool IsProfiling() const override { return profiling_; }

        uint32 BeginScope(NS::RHI::IRHICommandContext* context,
                          const char* name,
                          NS::RHI::ERHIGPUProfileEventType type,
                          NS::RHI::ERHIGPUProfileFlags flags) override;
        void EndScope(NS::RHI::IRHICommandContext* context, uint32 scopeId) override;

        bool IsFrameReady(uint64 frameNumber) const override;
        bool GetFrameResults(uint64 frameNumber,
                             std::vector<NS::RHI::RHIGPUProfileEvent>& outEvents) override;
        uint64 GetLatestCompletedFrame() const override;

        uint64 GetTimestampFrequency() const override;
        double GetFrameGPUTime(uint64 frameNumber) const override;

        /// フレーム操作
        void BeginFrame();
        void EndFrame(NS::RHI::IRHICommandContext* context);

    private:
        struct ScopeData
        {
            const char* name = nullptr;
            NS::RHI::ERHIGPUProfileEventType type = NS::RHI::ERHIGPUProfileEventType::Custom;
            uint32 startQueryIndex = 0;
            uint32 endQueryIndex = 0;
            int32 parentIndex = -1;
            uint32 depth = 0;
        };

        struct FrameData
        {
            uint64 frameNumber = 0;
            std::vector<ScopeData> scopes;
            bool resolved = false;
        };

        D3D12Device* device_ = nullptr;
        D3D12QueryHeap* timestampHeap_ = nullptr;
        bool profiling_ = false;
        uint64 timestampFrequency_ = 0;

        FrameData frames_[kMaxBufferedFrames];
        uint32 currentFrameIdx_ = 0;
        uint32 currentQueryIndex_ = 0;
        int32 currentParentScope_ = -1;
        uint32 currentDepth_ = 0;
        uint64 frameNumber_ = 0;
    };

} // namespace NS::D3D12RHI
