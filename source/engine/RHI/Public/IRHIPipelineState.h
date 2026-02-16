/// @file IRHIPipelineState.h
/// @brief パイプラインステートオブジェクトインターフェース
/// @details グラフィックスPSO、コンピュートPSO、PSOキャッシュ、非同期コンピュートヘルパーを提供。
/// @see 07-05-graphics-pso.md, 07-06-compute-pso.md
#pragma once

#include "IRHIShader.h"
#include "RHIPipelineState.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"

#include <functional>

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIPrimitiveTopologyType (07-05)
    //=========================================================================

    /// プリミティブトポロジータイプ（パイプライン用）
    enum class ERHIPrimitiveTopologyType : uint8
    {
        Undefined,
        Point,
        Line,
        Triangle,
        Patch,
    };

    /// トポロジーからタイプを取得
    inline ERHIPrimitiveTopologyType GetTopologyType(ERHIPrimitiveTopology topology)
    {
        switch (topology)
        {
        case ERHIPrimitiveTopology::PointList:
            return ERHIPrimitiveTopologyType::Point;
        case ERHIPrimitiveTopology::LineList:
        case ERHIPrimitiveTopology::LineStrip:
        case ERHIPrimitiveTopology::LineListAdj:
        case ERHIPrimitiveTopology::LineStripAdj:
            return ERHIPrimitiveTopologyType::Line;
        case ERHIPrimitiveTopology::TriangleList:
        case ERHIPrimitiveTopology::TriangleStrip:
        case ERHIPrimitiveTopology::TriangleListAdj:
        case ERHIPrimitiveTopology::TriangleStripAdj:
            return ERHIPrimitiveTopologyType::Triangle;
        case ERHIPrimitiveTopology::PatchList:
            return ERHIPrimitiveTopologyType::Patch;
        default:
            return ERHIPrimitiveTopologyType::Undefined;
        }
    }

    //=========================================================================
    // RHIRenderTargetFormats (07-05)
    //=========================================================================

    /// レンダーターゲットフォーマット記述
    struct RHI_API RHIRenderTargetFormats
    {
        ERHIPixelFormat formats[kMaxRenderTargets] = {};
        uint32 count = 0;
        ERHIPixelFormat depthStencilFormat = ERHIPixelFormat::Unknown;
        ERHISampleCount sampleCount = ERHISampleCount::Count1;
        uint32 sampleQuality = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIRenderTargetFormats& SetRT(uint32 index, ERHIPixelFormat format)
        {
            if (index < kMaxRenderTargets)
            {
                formats[index] = format;
                if (index >= count)
                    count = index + 1;
            }
            return *this;
        }

        RHIRenderTargetFormats& SetDepthStencil(ERHIPixelFormat format)
        {
            depthStencilFormat = format;
            return *this;
        }

        RHIRenderTargetFormats& SetSampleCount(ERHISampleCount sc, uint32 quality = 0)
        {
            sampleCount = sc;
            sampleQuality = quality;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIRenderTargetFormats SingleRTWithDepth(ERHIPixelFormat rtFormat,
                                                        ERHIPixelFormat dsFormat = ERHIPixelFormat::D32_Float)
        {
            RHIRenderTargetFormats fmt;
            fmt.SetRT(0, rtFormat);
            fmt.SetDepthStencil(dsFormat);
            return fmt;
        }

        static RHIRenderTargetFormats DepthOnly(ERHIPixelFormat dsFormat = ERHIPixelFormat::D32_Float)
        {
            RHIRenderTargetFormats fmt;
            fmt.SetDepthStencil(dsFormat);
            return fmt;
        }
    };

    //=========================================================================
    // RHIGraphicsPipelineStateDesc (07-05)
    //=========================================================================

    /// グラフィックスパイプラインステート記述
    struct RHI_API RHIGraphicsPipelineStateDesc
    {
        //=====================================================================
        // シェーダー
        //=====================================================================

        IRHIShader* vertexShader = nullptr;
        IRHIShader* pixelShader = nullptr;
        IRHIShader* geometryShader = nullptr;
        IRHIShader* hullShader = nullptr;
        IRHIShader* domainShader = nullptr;

        //=====================================================================
        // ルートシグネチャ
        //=====================================================================

        IRHIRootSignature* rootSignature = nullptr;

        //=====================================================================
        // 入力アセンブラー
        //=====================================================================

        RHIInputLayoutDesc inputLayout;
        ERHIPrimitiveTopologyType primitiveTopologyType = ERHIPrimitiveTopologyType::Triangle;

        enum class IndexBufferStripCutValue : uint8
        {
            Disabled,
            MaxUInt16,
            MaxUInt32,
        };
        IndexBufferStripCutValue stripCutValue = IndexBufferStripCutValue::Disabled;

        //=====================================================================
        // レンダーステート
        //=====================================================================

        RHIRasterizerStateDesc rasterizerState;
        RHIBlendStateDesc blendState;
        RHIDepthStencilStateDesc depthStencilState;
        uint32 sampleMask = 0xFFFFFFFF;

        //=====================================================================
        // 出力
        //=====================================================================

        RHIRenderTargetFormats renderTargetFormats;

        //=====================================================================
        // その他
        //=====================================================================

        uint32 nodeMask = 0;

        enum class Flags : uint32
        {
            None = 0,
            ToolDebug = 1 << 0,
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIGraphicsPipelineStateDesc& SetVS(IRHIShader* vs)
        {
            vertexShader = vs;
            return *this;
        }
        RHIGraphicsPipelineStateDesc& SetPS(IRHIShader* ps)
        {
            pixelShader = ps;
            return *this;
        }
        RHIGraphicsPipelineStateDesc& SetGS(IRHIShader* gs)
        {
            geometryShader = gs;
            return *this;
        }
        RHIGraphicsPipelineStateDesc& SetHS(IRHIShader* hs)
        {
            hullShader = hs;
            return *this;
        }
        RHIGraphicsPipelineStateDesc& SetDS(IRHIShader* ds)
        {
            domainShader = ds;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetRootSignature(IRHIRootSignature* rs)
        {
            rootSignature = rs;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetInputLayout(const RHIInputLayoutDesc& il)
        {
            inputLayout = il;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetTopologyType(ERHIPrimitiveTopologyType type)
        {
            primitiveTopologyType = type;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetRasterizerState(const RHIRasterizerStateDesc& rs)
        {
            rasterizerState = rs;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetBlendState(const RHIBlendStateDesc& bs)
        {
            blendState = bs;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetDepthStencilState(const RHIDepthStencilStateDesc& dss)
        {
            depthStencilState = dss;
            return *this;
        }

        RHIGraphicsPipelineStateDesc& SetRenderTargetFormats(const RHIRenderTargetFormats& rtf)
        {
            renderTargetFormats = rtf;
            return *this;
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIGraphicsPipelineStateDesc::Flags)

    //=========================================================================
    // IRHIGraphicsPipelineState (07-05)
    //=========================================================================

    /// グラフィックスパイプラインステート
    class RHI_API IRHIGraphicsPipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(GraphicsPipelineState)

        virtual ~IRHIGraphicsPipelineState() = default;

        virtual IRHIDevice* GetDevice() const = 0;
        virtual IRHIRootSignature* GetRootSignature() const = 0;
        virtual ERHIPrimitiveTopologyType GetPrimitiveTopologyType() const = 0;

        virtual IRHIShader* GetVertexShader() const = 0;
        virtual IRHIShader* GetPixelShader() const = 0;

        bool UsesTessellation() const { return GetPrimitiveTopologyType() == ERHIPrimitiveTopologyType::Patch; }

        /// キャッシュ済みバイナリ取得
        virtual RHIShaderBytecode GetCachedBlob() const = 0;
    };

    using RHIGraphicsPipelineStateRef = TRefCountPtr<IRHIGraphicsPipelineState>;

    //=========================================================================
    // IRHIPipelineStateCache (07-05)
    //=========================================================================

    /// PSOキャッシュ
    /// スレッド安全性:
    /// - FindPipelineState(): 複数スレッドから同時呼び出し可能（読み取りロック）
    /// - AddPipelineState(): 単一スレッドまたは排他ロック下で呼び出すこと
    class RHI_API IRHIPipelineStateCache
    {
    public:
        virtual ~IRHIPipelineStateCache() = default;

        virtual void AddPipelineState(const void* descHash, size_t hashSize, IRHIGraphicsPipelineState* pso) = 0;

        virtual IRHIGraphicsPipelineState* FindPipelineState(const void* descHash, size_t hashSize) = 0;

        virtual bool SaveToFile(const char* path) = 0;
        virtual bool LoadFromFile(const char* path) = 0;
        virtual void Clear() = 0;
        virtual uint32 GetEntryCount() const = 0;
    };

    //=========================================================================
    // RHIComputePipelineStateDesc (07-06)
    //=========================================================================

    /// コンピュートパイプラインステート記述
    struct RHI_API RHIComputePipelineStateDesc
    {
        IRHIShader* computeShader = nullptr;
        IRHIRootSignature* rootSignature = nullptr;
        uint32 nodeMask = 0;

        enum class Flags : uint32
        {
            None = 0,
            ToolDebug = 1 << 0,
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIComputePipelineStateDesc& SetCS(IRHIShader* cs)
        {
            computeShader = cs;
            return *this;
        }

        RHIComputePipelineStateDesc& SetRootSignature(IRHIRootSignature* rs)
        {
            rootSignature = rs;
            return *this;
        }

        RHIComputePipelineStateDesc& SetNodeMask(uint32 mask)
        {
            nodeMask = mask;
            return *this;
        }

        static RHIComputePipelineStateDesc Create(IRHIShader* cs, IRHIRootSignature* rs = nullptr)
        {
            RHIComputePipelineStateDesc desc;
            desc.computeShader = cs;
            desc.rootSignature = rs;
            return desc;
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIComputePipelineStateDesc::Flags)

    //=========================================================================
    // IRHIComputePipelineState (07-06)
    //=========================================================================

    /// コンピュートパイプラインステート
    class RHI_API IRHIComputePipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(ComputePipelineState)

        virtual ~IRHIComputePipelineState() = default;

        virtual IRHIDevice* GetDevice() const = 0;
        virtual IRHIRootSignature* GetRootSignature() const = 0;
        virtual IRHIShader* GetComputeShader() const = 0;

        //=====================================================================
        // スレッドグループ情報
        //=====================================================================

        virtual void GetThreadGroupSize(uint32& outX, uint32& outY, uint32& outZ) const = 0;

        uint32 GetTotalThreadsPerGroup() const
        {
            uint32 x, y, z;
            GetThreadGroupSize(x, y, z);
            return x * y * z;
        }

        //=====================================================================
        // ディスパッチ計算ヘルパー
        //=====================================================================

        void CalculateDispatchGroups(uint32 totalX,
                                     uint32 totalY,
                                     uint32 totalZ,
                                     uint32& outGroupsX,
                                     uint32& outGroupsY,
                                     uint32& outGroupsZ) const
        {
            uint32 threadX, threadY, threadZ;
            GetThreadGroupSize(threadX, threadY, threadZ);
            outGroupsX = (totalX + threadX - 1) / threadX;
            outGroupsY = (totalY + threadY - 1) / threadY;
            outGroupsZ = (totalZ + threadZ - 1) / threadZ;
        }

        uint32 CalculateDispatchGroups1D(uint32 total) const
        {
            uint32 gx, gy, gz;
            CalculateDispatchGroups(total, 1, 1, gx, gy, gz);
            return gx;
        }

        Extent2D CalculateDispatchGroups2D(uint32 width, uint32 height) const
        {
            uint32 gx, gy, gz;
            CalculateDispatchGroups(width, height, 1, gx, gy, gz);
            return Extent2D{gx, gy};
        }

        /// キャッシュ済みバイナリ取得
        virtual RHIShaderBytecode GetCachedBlob() const = 0;
    };

    using RHIComputePipelineStateRef = TRefCountPtr<IRHIComputePipelineState>;

    //=========================================================================
    // RHIAsyncComputeHelper (07-06)
    //=========================================================================

    /// 非同期コンピュートヘルパー
    class RHI_API RHIAsyncComputeHelper
    {
    public:
        RHIAsyncComputeHelper() = default;

        bool Initialize(IRHIDevice* device);
        void Shutdown();

        IRHIQueue* GetComputeQueue() const { return m_computeQueue; }

        //=====================================================================
        // 同期
        //=====================================================================

        uint64 InsertGraphicsToComputeSync(IRHICommandContext* graphicsContext);
        uint64 InsertComputeToGraphicsSync(IRHIComputeContext* computeContext);
        void WaitForComputeOnGraphics(IRHICommandContext* graphicsContext, uint64 computeFenceValue);
        void WaitForGraphicsOnCompute(IRHIComputeContext* computeContext, uint64 graphicsFenceValue);

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        using ComputeSetupFunc = std::function<void(IRHIComputeContext*)>;
        uint64 ExecuteAsync(ComputeSetupFunc setupFunc);

    private:
        IRHIDevice* m_device = nullptr;
        IRHIQueue* m_computeQueue = nullptr;
        TRefCountPtr<IRHIFence> m_computeFence;
        uint64 m_nextFenceValue = 1;
    };

}} // namespace NS::RHI
