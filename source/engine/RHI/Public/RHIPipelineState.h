/// @file RHIPipelineState.h
/// @brief パイプラインステート記述型
/// @details ブレンド、ラスタライザー、デプスステンシル、入力レイアウトの記述構造体を提供。
/// @see 07-01-blend-state.md, 07-02-rasterizer-state.md, 07-03-depth-stencil-state.md, 07-04-input-layout.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS::RHI
{
    // 前方宣言
    struct RHIShaderBytecode;

    //=========================================================================
    // ERHIConservativeRaster (07-02)
    //=========================================================================

    /// 保守的ラスタライゼーションモード
    enum class ERHIConservativeRaster : uint8
    {
        Off,           // 無効
        On,            // 有効（オーバーエスティメート）
        Underestimate, // アンダーエスティメート（サポート限定）
    };

    //=========================================================================
    // ERHIInputClassification (07-04)
    //=========================================================================

    /// 入力スロット分類
    enum class ERHIInputClassification : uint8
    {
        PerVertex,   // 頂点ごとのデータ
        PerInstance, // インスタンスごとのデータ
    };

    //=========================================================================
    // ERHIVertexFormat (07-04)
    //=========================================================================

    /// 頂点属性フォーマット
    enum class ERHIVertexFormat : uint8
    {
        Unknown,

        // 32ビット浮動小数点
        Float1, // R32_Float
        Float2, // R32G32_Float
        Float3, // R32G32B32_Float
        Float4, // R32G32B32A32_Float

        // 16ビット浮動小数点
        Half2, // R16G16_Float
        Half4, // R16G16B16A16_Float

        // 32ビット整数
        Int1, // R32_Int
        Int2, // R32G32_Int
        Int3, // R32G32B32_Int
        Int4, // R32G32B32A32_Int

        // 32ビット符号なし整数
        UInt1, // R32_UInt
        UInt2, // R32G32_UInt
        UInt3, // R32G32B32_UInt
        UInt4, // R32G32B32A32_UInt

        // 16ビット整数（正規化）
        Short2N,  // R16G16_SNorm
        Short4N,  // R16G16B16A16_SNorm
        UShort2N, // R16G16_UNorm
        UShort4N, // R16G16B16A16_UNorm

        // 16ビット整数
        Short2,  // R16G16_SInt
        Short4,  // R16G16B16A16_SInt
        UShort2, // R16G16_UInt
        UShort4, // R16G16B16A16_UInt

        // 8ビット整数（正規化）
        Byte4N,       // R8G8B8A8_SNorm
        UByte4N,      // R8G8B8A8_UNorm
        UByte4N_BGRA, // B8G8R8A8_UNorm（カラー用）

        // 8ビット整数
        Byte4,  // R8G8B8A8_SInt
        UByte4, // R8G8B8A8_UInt

        // パックフォーマット
        UInt1010102N, // R10G10B10A2_UNorm
    };

    /// 頂点フォーマットサイズ取得（バイト）
    inline uint32 GetVertexFormatSize(ERHIVertexFormat format)
    {
        switch (format)
        {
        case ERHIVertexFormat::Float1:
        case ERHIVertexFormat::Int1:
        case ERHIVertexFormat::UInt1:
        case ERHIVertexFormat::Half2:
        case ERHIVertexFormat::Short2N:
        case ERHIVertexFormat::UShort2N:
        case ERHIVertexFormat::Short2:
        case ERHIVertexFormat::UShort2:
        case ERHIVertexFormat::Byte4N:
        case ERHIVertexFormat::UByte4N:
        case ERHIVertexFormat::UByte4N_BGRA:
        case ERHIVertexFormat::Byte4:
        case ERHIVertexFormat::UByte4:
        case ERHIVertexFormat::UInt1010102N:
            return 4;

        case ERHIVertexFormat::Float2:
        case ERHIVertexFormat::Int2:
        case ERHIVertexFormat::UInt2:
        case ERHIVertexFormat::Half4:
        case ERHIVertexFormat::Short4N:
        case ERHIVertexFormat::UShort4N:
        case ERHIVertexFormat::Short4:
        case ERHIVertexFormat::UShort4:
            return 8;

        case ERHIVertexFormat::Float3:
        case ERHIVertexFormat::Int3:
        case ERHIVertexFormat::UInt3:
            return 12;

        case ERHIVertexFormat::Float4:
        case ERHIVertexFormat::Int4:
        case ERHIVertexFormat::UInt4:
            return 16;

        default:
            return 0;
        }
    }

    //=========================================================================
    // RHIRenderTargetBlendDesc (07-01)
    //=========================================================================

    /// レンダーターゲットブレンド記述
    struct RHI_API RHIRenderTargetBlendDesc
    {
        bool blendEnable = false;
        bool logicOpEnable = false;

        ERHIBlendFactor srcBlend = ERHIBlendFactor::One;
        ERHIBlendFactor dstBlend = ERHIBlendFactor::Zero;
        ERHIBlendOp blendOp = ERHIBlendOp::Add;

        ERHIBlendFactor srcBlendAlpha = ERHIBlendFactor::One;
        ERHIBlendFactor dstBlendAlpha = ERHIBlendFactor::Zero;
        ERHIBlendOp blendOpAlpha = ERHIBlendOp::Add;

        ERHILogicOp logicOp = ERHILogicOp::Noop;
        ERHIColorWriteMask writeMask = ERHIColorWriteMask::All;

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIRenderTargetBlendDesc Disabled() { return RHIRenderTargetBlendDesc{}; }

        static RHIRenderTargetBlendDesc AlphaBlend()
        {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::SrcAlpha;
            desc.dstBlend = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        static RHIRenderTargetBlendDesc Additive()
        {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::SrcAlpha;
            desc.dstBlend = ERHIBlendFactor::One;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::One;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        static RHIRenderTargetBlendDesc Multiply()
        {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::DstColor;
            desc.dstBlend = ERHIBlendFactor::Zero;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::DstAlpha;
            desc.dstBlendAlpha = ERHIBlendFactor::Zero;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        static RHIRenderTargetBlendDesc PremultipliedAlpha()
        {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::One;
            desc.dstBlend = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        static RHIRenderTargetBlendDesc NoWrite()
        {
            RHIRenderTargetBlendDesc desc;
            desc.writeMask = ERHIColorWriteMask::None;
            return desc;
        }
    };

    //=========================================================================
    // RHIBlendStateDesc (07-01)
    //=========================================================================

    /// ブレンドステート記述
    struct RHI_API RHIBlendStateDesc
    {
        bool alphaToCoverageEnable = false;
        bool independentBlendEnable = false;
        RHIRenderTargetBlendDesc renderTarget[kMaxRenderTargets];

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIBlendStateDesc& SetAll(const RHIRenderTargetBlendDesc& desc)
        {
            for (auto& rt : renderTarget)
                rt = desc;
            independentBlendEnable = false;
            return *this;
        }

        RHIBlendStateDesc& SetRT(uint32 index, const RHIRenderTargetBlendDesc& desc)
        {
            if (index < kMaxRenderTargets)
            {
                renderTarget[index] = desc;
                if (index > 0)
                    independentBlendEnable = true;
            }
            return *this;
        }

        RHIBlendStateDesc& SetAlphaToCoverage(bool enable)
        {
            alphaToCoverageEnable = enable;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIBlendStateDesc Default() { return RHIBlendStateDesc{}; }

        static RHIBlendStateDesc AlphaBlend()
        {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::AlphaBlend());
            return desc;
        }

        static RHIBlendStateDesc Additive()
        {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::Additive());
            return desc;
        }

        static RHIBlendStateDesc PremultipliedAlpha()
        {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::PremultipliedAlpha());
            return desc;
        }

        static RHIBlendStateDesc Opaque() { return Default(); }
    };

    //=========================================================================
    // RHIBlendConstants (07-01)
    //=========================================================================

    /// ブレンドファクター定数
    struct RHI_API RHIBlendConstants
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;

        RHIBlendConstants() = default;
        RHIBlendConstants(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}

        static RHIBlendConstants White() { return {1, 1, 1, 1}; }
        static RHIBlendConstants Black() { return {0, 0, 0, 1}; }
        static RHIBlendConstants Transparent() { return {0, 0, 0, 0}; }

        const float* AsArray() const { return &r; }
    };

    //=========================================================================
    // RHISampleMask (07-01)
    //=========================================================================

    /// サンプルマスク
    struct RHI_API RHISampleMask
    {
        uint32 mask = 0xFFFFFFFF;

        RHISampleMask() = default;
        explicit RHISampleMask(uint32 m) : mask(m) {}

        static RHISampleMask All() { return RHISampleMask(0xFFFFFFFF); }
        static RHISampleMask None() { return RHISampleMask(0); }

        static RHISampleMask ForSampleCount(ERHISampleCount count)
        {
            uint32 sampleCount = GetSampleCountValue(count);
            return RHISampleMask((1u << sampleCount) - 1);
        }
    };

    //=========================================================================
    // RHIRasterizerStateDesc (07-02)
    //=========================================================================

    /// ラスタライザーステート記述
    struct RHI_API RHIRasterizerStateDesc
    {
        ERHIFillMode fillMode = ERHIFillMode::Solid;
        ERHICullMode cullMode = ERHICullMode::Back;
        ERHIFrontFace frontFace = ERHIFrontFace::CounterClockwise;

        int32 depthBias = 0;
        float depthBiasClamp = 0.0f;
        float slopeScaledDepthBias = 0.0f;

        bool depthClipEnable = true;
        bool scissorEnable = true;
        bool multisampleEnable = false;
        bool antialiasedLineEnable = false;
        uint32 forcedSampleCount = 0;

        ERHIConservativeRaster conservativeRaster = ERHIConservativeRaster::Off;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIRasterizerStateDesc& SetFillMode(ERHIFillMode mode)
        {
            fillMode = mode;
            return *this;
        }
        RHIRasterizerStateDesc& SetCullMode(ERHICullMode mode)
        {
            cullMode = mode;
            return *this;
        }
        RHIRasterizerStateDesc& SetFrontFace(ERHIFrontFace face)
        {
            frontFace = face;
            return *this;
        }
        RHIRasterizerStateDesc& SetDepthBias(int32 bias, float clamp = 0.0f, float slope = 0.0f)
        {
            depthBias = bias;
            depthBiasClamp = clamp;
            slopeScaledDepthBias = slope;
            return *this;
        }
        RHIRasterizerStateDesc& SetDepthClip(bool enable)
        {
            depthClipEnable = enable;
            return *this;
        }
        RHIRasterizerStateDesc& SetScissor(bool enable)
        {
            scissorEnable = enable;
            return *this;
        }
        RHIRasterizerStateDesc& SetMultisample(bool enable)
        {
            multisampleEnable = enable;
            return *this;
        }
        RHIRasterizerStateDesc& SetConservativeRaster(ERHIConservativeRaster mode)
        {
            conservativeRaster = mode;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIRasterizerStateDesc Default() { return RHIRasterizerStateDesc{}; }

        static RHIRasterizerStateDesc NoCull()
        {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::None;
            return desc;
        }

        static RHIRasterizerStateDesc FrontCull()
        {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            return desc;
        }

        static RHIRasterizerStateDesc Wireframe()
        {
            RHIRasterizerStateDesc desc;
            desc.fillMode = ERHIFillMode::Wireframe;
            desc.cullMode = ERHICullMode::None;
            return desc;
        }

        static RHIRasterizerStateDesc ShadowMap(int32 bias = 100, float slopeScale = 1.0f)
        {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            desc.depthBias = bias;
            desc.slopeScaledDepthBias = slopeScale;
            desc.depthBiasClamp = 0.0f;
            return desc;
        }

        static RHIRasterizerStateDesc Skybox()
        {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            desc.depthClipEnable = false;
            return desc;
        }
    };

    //=========================================================================
    // RHIViewportArray / RHIScissorArray (07-02)
    //=========================================================================

    /// ビューポート配列
    struct RHI_API RHIViewportArray
    {
        RHIViewport viewports[kMaxViewports];
        uint32 count = 0;

        void Set(uint32 index, const RHIViewport& vp)
        {
            if (index < kMaxViewports)
            {
                viewports[index] = vp;
                if (index >= count)
                    count = index + 1;
            }
        }

        void Add(const RHIViewport& vp)
        {
            if (count < kMaxViewports)
                viewports[count++] = vp;
        }
    };

    /// シザー配列
    struct RHI_API RHIScissorArray
    {
        RHIRect rects[kMaxViewports];
        uint32 count = 0;

        void Set(uint32 index, const RHIRect& rect)
        {
            if (index < kMaxViewports)
            {
                rects[index] = rect;
                if (index >= count)
                    count = index + 1;
            }
        }

        void Add(const RHIRect& rect)
        {
            if (count < kMaxViewports)
                rects[count++] = rect;
        }

        /// ビューポート配列からシザー配列を生成
        static RHIScissorArray FromViewports(const RHIViewportArray& viewports)
        {
            RHIScissorArray scissors;
            for (uint32 i = 0; i < viewports.count; ++i)
            {
                const auto& vp = viewports.viewports[i];
                scissors.Add(RHIRect::FromExtent(static_cast<int32>(vp.x),
                                                 static_cast<int32>(vp.y),
                                                 static_cast<uint32>(vp.width),
                                                 static_cast<uint32>(vp.height)));
            }
            return scissors;
        }
    };

    //=========================================================================
    // RHILineState (07-02)
    //=========================================================================

    /// ライン描画ステート
    struct RHI_API RHILineState
    {
        float lineWidth = 1.0f;
        bool stippleEnable = false;
        uint32 stippleFactor = 1;
        uint16 stipplePattern = 0xFFFF;
    };

    //=========================================================================
    // RHIStencilFaceDesc (07-03)
    //=========================================================================

    /// ステンシル面記述
    struct RHI_API RHIStencilFaceDesc
    {
        ERHIStencilOp stencilFailOp = ERHIStencilOp::Keep;
        ERHIStencilOp stencilDepthFailOp = ERHIStencilOp::Keep;
        ERHIStencilOp stencilPassOp = ERHIStencilOp::Keep;
        ERHICompareFunc stencilFunc = ERHICompareFunc::Always;

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIStencilFaceDesc Disabled() { return RHIStencilFaceDesc{}; }

        static RHIStencilFaceDesc Increment()
        {
            RHIStencilFaceDesc desc;
            desc.stencilPassOp = ERHIStencilOp::IncrSat;
            desc.stencilFunc = ERHICompareFunc::Always;
            return desc;
        }

        static RHIStencilFaceDesc Decrement()
        {
            RHIStencilFaceDesc desc;
            desc.stencilPassOp = ERHIStencilOp::DecrSat;
            desc.stencilFunc = ERHICompareFunc::Always;
            return desc;
        }

        static RHIStencilFaceDesc MaskEqual()
        {
            RHIStencilFaceDesc desc;
            desc.stencilFunc = ERHICompareFunc::Equal;
            return desc;
        }

        static RHIStencilFaceDesc MaskWrite()
        {
            RHIStencilFaceDesc desc;
            desc.stencilFunc = ERHICompareFunc::Always;
            desc.stencilFailOp = ERHIStencilOp::Replace;
            desc.stencilDepthFailOp = ERHIStencilOp::Replace;
            desc.stencilPassOp = ERHIStencilOp::Replace;
            return desc;
        }
    };

    //=========================================================================
    // RHIDepthStencilStateDesc (07-03)
    //=========================================================================

    /// デプスステンシルステート記述
    struct RHI_API RHIDepthStencilStateDesc
    {
        bool depthTestEnable = true;
        bool depthWriteEnable = true;
        ERHICompareFunc depthFunc = ERHICompareFunc::Less;

        bool stencilTestEnable = false;
        uint8 stencilReadMask = 0xFF;
        uint8 stencilWriteMask = 0xFF;
        RHIStencilFaceDesc frontFace;
        RHIStencilFaceDesc backFace;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIDepthStencilStateDesc& SetDepthTest(bool enable, ERHICompareFunc func = ERHICompareFunc::Less)
        {
            depthTestEnable = enable;
            depthFunc = func;
            return *this;
        }

        RHIDepthStencilStateDesc& SetDepthWrite(bool enable)
        {
            depthWriteEnable = enable;
            return *this;
        }

        RHIDepthStencilStateDesc& SetStencilTest(bool enable)
        {
            stencilTestEnable = enable;
            return *this;
        }

        RHIDepthStencilStateDesc& SetStencilMasks(uint8 read, uint8 write)
        {
            stencilReadMask = read;
            stencilWriteMask = write;
            return *this;
        }

        RHIDepthStencilStateDesc& SetFrontFace(const RHIStencilFaceDesc& desc)
        {
            frontFace = desc;
            return *this;
        }

        RHIDepthStencilStateDesc& SetBackFace(const RHIStencilFaceDesc& desc)
        {
            backFace = desc;
            return *this;
        }

        RHIDepthStencilStateDesc& SetBothFaces(const RHIStencilFaceDesc& desc)
        {
            frontFace = desc;
            backFace = desc;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIDepthStencilStateDesc Default() { return RHIDepthStencilStateDesc{}; }

        static RHIDepthStencilStateDesc ReversedZ()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::Greater;
            return desc;
        }

        static RHIDepthStencilStateDesc ReadOnly()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthWriteEnable = false;
            return desc;
        }

        static RHIDepthStencilStateDesc NoDepth()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthTestEnable = false;
            desc.depthWriteEnable = false;
            return desc;
        }

        static RHIDepthStencilStateDesc DepthEqual()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::Equal;
            desc.depthWriteEnable = false;
            return desc;
        }

        static RHIDepthStencilStateDesc ReversedZEqual()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::GreaterEqual;
            desc.depthWriteEnable = false;
            return desc;
        }

        static RHIDepthStencilStateDesc Skybox()
        {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::LessEqual;
            desc.depthWriteEnable = false;
            return desc;
        }

        static RHIDepthStencilStateDesc StencilMask()
        {
            RHIDepthStencilStateDesc desc;
            desc.stencilTestEnable = true;
            desc.SetBothFaces(RHIStencilFaceDesc::MaskEqual());
            return desc;
        }
    };

    //=========================================================================
    // RHIDepthBoundsTest (07-03)
    //=========================================================================

    /// デプスバウンドテスト記述
    struct RHI_API RHIDepthBoundsTest
    {
        bool enable = false;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        static RHIDepthBoundsTest Disabled() { return RHIDepthBoundsTest{}; }

        static RHIDepthBoundsTest Range(float min, float max)
        {
            RHIDepthBoundsTest test;
            test.enable = true;
            test.minDepth = min;
            test.maxDepth = max;
            return test;
        }
    };

    //=========================================================================
    // RHIInputElementDesc (07-04)
    //=========================================================================

    /// 最大入力要素数
    constexpr uint32 kMaxInputElements = 32;

    /// 入力要素記述
    struct RHI_API RHIInputElementDesc
    {
        const char* semanticName = nullptr;
        uint32 semanticIndex = 0;
        ERHIVertexFormat format = ERHIVertexFormat::Float3;
        uint32 inputSlot = 0;
        uint32 alignedByteOffset = 0;
        ERHIInputClassification inputClass = ERHIInputClassification::PerVertex;
        uint32 instanceDataStepRate = 0;

        /// オフセット自動計算用定数
        static constexpr uint32 kAppendAligned = ~0u;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIInputElementDesc Position(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"POSITION", 0, ERHIVertexFormat::Float3, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Normal(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"NORMAL", 0, ERHIVertexFormat::Float3, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Tangent(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"TANGENT", 0, ERHIVertexFormat::Float4, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc TexCoord(uint32 index = 0, uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"TEXCOORD", index, ERHIVertexFormat::Float2, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Color(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"COLOR", 0, ERHIVertexFormat::UByte4N, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc BlendIndices(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"BLENDINDICES", 0, ERHIVertexFormat::UByte4, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc BlendWeights(uint32 slot = 0, uint32 offset = kAppendAligned)
        {
            return {"BLENDWEIGHT", 0, ERHIVertexFormat::UByte4N, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc InstanceTransform(uint32 row, uint32 slot = 1)
        {
            return {"INSTANCE_TRANSFORM",
                    row,
                    ERHIVertexFormat::Float4,
                    slot,
                    kAppendAligned,
                    ERHIInputClassification::PerInstance,
                    1};
        }
    };

    //=========================================================================
    // RHIInputLayoutDesc (07-04)
    //=========================================================================

    /// 入力レイアウト記述
    struct RHI_API RHIInputLayoutDesc
    {
        const RHIInputElementDesc* elements = nullptr;
        uint32 elementCount = 0;

        /// 配列から作成
        template <uint32 N> static RHIInputLayoutDesc FromArray(const RHIInputElementDesc (&arr)[N])
        {
            RHIInputLayoutDesc desc;
            desc.elements = arr;
            desc.elementCount = N;
            return desc;
        }

        /// 検証
        bool Validate() const;

        /// 指定スロットのストライド計算
        uint32 CalculateStride(uint32 slot) const;
    };

    //=========================================================================
    // RHIInputLayoutBuilder (07-04)
    //=========================================================================

    /// 入力レイアウトビルダー
    class RHI_API RHIInputLayoutBuilder
    {
    public:
        RHIInputLayoutBuilder() = default;

        RHIInputLayoutBuilder& Add(const RHIInputElementDesc& element)
        {
            if (m_count < kMaxInputElements)
                m_elements[m_count++] = element;
            return *this;
        }

        RHIInputLayoutBuilder& Position(uint32 slot = 0) { return Add(RHIInputElementDesc::Position(slot)); }
        RHIInputLayoutBuilder& Normal(uint32 slot = 0) { return Add(RHIInputElementDesc::Normal(slot)); }
        RHIInputLayoutBuilder& Tangent(uint32 slot = 0) { return Add(RHIInputElementDesc::Tangent(slot)); }
        RHIInputLayoutBuilder& TexCoord(uint32 index = 0, uint32 slot = 0)
        {
            return Add(RHIInputElementDesc::TexCoord(index, slot));
        }
        RHIInputLayoutBuilder& Color(uint32 slot = 0) { return Add(RHIInputElementDesc::Color(slot)); }
        RHIInputLayoutBuilder& BlendIndices(uint32 slot = 0) { return Add(RHIInputElementDesc::BlendIndices(slot)); }
        RHIInputLayoutBuilder& BlendWeights(uint32 slot = 0) { return Add(RHIInputElementDesc::BlendWeights(slot)); }

        RHIInputLayoutDesc Build() const
        {
            RHIInputLayoutDesc desc;
            desc.elements = m_elements;
            desc.elementCount = m_count;
            return desc;
        }

        const RHIInputElementDesc* GetElements() const { return m_elements; }
        uint32 GetElementCount() const { return m_count; }

    private:
        RHIInputElementDesc m_elements[kMaxInputElements];
        uint32 m_count = 0;
    };

    //=========================================================================
    // RHIVertexLayouts プリセット (07-04)
    //=========================================================================

    namespace RHIVertexLayouts
    {
        inline RHIInputLayoutDesc PositionOnly()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        inline RHIInputLayoutDesc PositionTexCoord()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        inline RHIInputLayoutDesc PositionNormalTexCoord()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        inline RHIInputLayoutDesc PositionNormalTangentTexCoord()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::Tangent(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        inline RHIInputLayoutDesc Skinned()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::BlendIndices(),
                RHIInputElementDesc::BlendWeights(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        inline RHIInputLayoutDesc UI()
        {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Color(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }
    } // namespace RHIVertexLayouts

    //=========================================================================
    // IRHIInputLayout (07-04)
    //=========================================================================

    /// 入力レイアウトオブジェクト
    class RHI_API IRHIInputLayout : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(InputLayout)

        virtual ~IRHIInputLayout() = default;

        virtual IRHIDevice* GetDevice() const = 0;
        virtual uint32 GetElementCount() const = 0;
        virtual bool GetElement(uint32 index, RHIInputElementDesc& outElement) const = 0;
        virtual uint32 GetStride(uint32 slot) const = 0;
    };

    using RHIInputLayoutRef = TRefCountPtr<IRHIInputLayout>;

} // namespace NS::RHI
