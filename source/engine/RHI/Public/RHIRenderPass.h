/// @file RHIRenderPass.h
/// @brief レンダーパス記述・ロードストアアクション・スコープレンダーパス
/// @details レンダーパスの記述構造、ロードストアアクション列挙型、
///          サブパス、統計、スコープレンダーパスRAIIクラスを提供。
/// @see 13-01-render-pass.md, 13-02-load-store.md, 13-03-render-pass.md
#pragma once

#include "IRHITexture.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHILoadAction (13-02)
    //=========================================================================

    /// ロードアクション
    /// レンダーパス開始時のアタッチメント処理
    enum class ERHILoadAction : uint8
    {
        Load,     ///< 既存の内容を保持
        Clear,    ///< 指定値でクリア
        DontCare, ///< 前の内容は不定（最適化ヒント）
        NoAccess, ///< このパスでは読み書きしない
    };

    //=========================================================================
    // ERHIStoreAction (13-02)
    //=========================================================================

    /// ストアアクション
    /// レンダーパス終了時のアタッチメント処理
    enum class ERHIStoreAction : uint8
    {
        Store,           ///< レンダリング結果を保存
        DontCare,        ///< 結果は破棄（最適化ヒント）
        Resolve,         ///< MSAAをシングルサンプルにリゾルブ
        StoreAndResolve, ///< MSAA結果を保存し、同時にリゾルブ
        NoAccess,        ///< このパスでは書き込まない
    };

    //=========================================================================
    // ERHIClearFlags (13-03)
    //=========================================================================

    /// クリアフラグ
    enum class ERHIClearFlags : uint8
    {
        None = 0,
        Depth = 1 << 0,
        Stencil = 1 << 1,
        DepthStencil = Depth | Stencil,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIClearFlags)

    //=========================================================================
    // ERHIPipelineStageFlags (13-03)
    //=========================================================================

    /// パイプラインステージフラグ（サブパス依存関係用）
    enum class ERHIPipelineStageFlags : uint32
    {
        None = 0,
        TopOfPipe = 1 << 0,
        VertexInput = 1 << 1,
        VertexShader = 1 << 2,
        HullShader = 1 << 3,
        DomainShader = 1 << 4,
        GeometryShader = 1 << 5,
        PixelShader = 1 << 6,
        EarlyDepthStencil = 1 << 7,
        LateDepthStencil = 1 << 8,
        RenderTarget = 1 << 9,
        ComputeShader = 1 << 10,
        Copy = 1 << 11,
        Resolve = 1 << 12,
        BottomOfPipe = 1 << 13,

        AllGraphics = VertexInput | VertexShader | HullShader | DomainShader | GeometryShader | PixelShader |
                      EarlyDepthStencil | LateDepthStencil | RenderTarget,
        AllCommands = AllGraphics | ComputeShader | Copy | Resolve,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIPipelineStageFlags)

    //=========================================================================
    // ERHITileMemoryAction (13-02)
    //=========================================================================

    /// タイルメモリアクション（タイルベースGPU用）
    enum class ERHITileMemoryAction : uint8
    {
        KeepInTile,    ///< タイルメモリに保持
        FlushToShared, ///< 共有メモリにフラッシュ
        DiscardTile,   ///< タイルメモリを破棄
    };

    //=========================================================================
    // RHIRenderTargetAttachment (13-01)
    //=========================================================================

    /// レンダーターゲットアタッチメント記述
    struct RHI_API RHIRenderTargetAttachment
    {
        IRHIRenderTargetView* rtv = nullptr;           ///< レンダーターゲットビュー
        IRHIRenderTargetView* resolveTarget = nullptr; ///< リゾルブターゲット（MSAA時）
        ERHILoadAction loadAction = ERHILoadAction::Load;
        ERHIStoreAction storeAction = ERHIStoreAction::Store;
        RHIClearValue clearValue = {};

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ロードして保存
        static RHIRenderTargetAttachment LoadStore(IRHIRenderTargetView* view)
        {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::Load;
            att.storeAction = ERHIStoreAction::Store;
            return att;
        }

        /// クリアして保存
        static RHIRenderTargetAttachment ClearStore(
            IRHIRenderTargetView* view, float r = 0, float g = 0, float b = 0, float a = 1)
        {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::Clear;
            att.storeAction = ERHIStoreAction::Store;
            att.clearValue = RHIClearValue::Color(r, g, b, a);
            return att;
        }

        /// ドントケアで保存
        static RHIRenderTargetAttachment DontCareStore(IRHIRenderTargetView* view)
        {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::DontCare;
            att.storeAction = ERHIStoreAction::Store;
            return att;
        }

        /// クリアしてリゾルブ（MSAA）
        static RHIRenderTargetAttachment ClearResolve(IRHIRenderTargetView* msaaView,
                                                      IRHIRenderTargetView* resolveView,
                                                      float r = 0,
                                                      float g = 0,
                                                      float b = 0,
                                                      float a = 1)
        {
            RHIRenderTargetAttachment att;
            att.rtv = msaaView;
            att.resolveTarget = resolveView;
            att.loadAction = ERHILoadAction::Clear;
            att.storeAction = ERHIStoreAction::Resolve;
            att.clearValue = RHIClearValue::Color(r, g, b, a);
            return att;
        }
    };

    //=========================================================================
    // RHIDepthStencilAttachment (13-01)
    //=========================================================================

    /// デプスステンシルアタッチメント記述
    struct RHI_API RHIDepthStencilAttachment
    {
        IRHIDepthStencilView* dsv = nullptr;
        ERHILoadAction depthLoadAction = ERHILoadAction::Load;
        ERHIStoreAction depthStoreAction = ERHIStoreAction::Store;
        ERHILoadAction stencilLoadAction = ERHILoadAction::Load;
        ERHIStoreAction stencilStoreAction = ERHIStoreAction::Store;
        float clearDepth = 1.0f;
        uint8 clearStencil = 0;
        bool depthWriteEnabled = true;
        bool stencilWriteEnabled = true;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ロードして保存
        static RHIDepthStencilAttachment LoadStore(IRHIDepthStencilView* view)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            return att;
        }

        /// クリアして保存
        static RHIDepthStencilAttachment ClearStore(IRHIDepthStencilView* view, float depth = 1.0f, uint8 stencil = 0)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthLoadAction = ERHILoadAction::Clear;
            att.stencilLoadAction = ERHILoadAction::Clear;
            att.clearDepth = depth;
            att.clearStencil = stencil;
            return att;
        }

        /// 読み取り専用
        static RHIDepthStencilAttachment ReadOnly(IRHIDepthStencilView* view)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthWriteEnabled = false;
            att.stencilWriteEnabled = false;
            att.depthStoreAction = ERHIStoreAction::DontCare;
            att.stencilStoreAction = ERHIStoreAction::DontCare;
            return att;
        }

        /// デプスのみクリア
        static RHIDepthStencilAttachment ClearDepthOnly(IRHIDepthStencilView* view, float depth = 1.0f)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthLoadAction = ERHILoadAction::Clear;
            att.stencilLoadAction = ERHILoadAction::DontCare;
            att.stencilStoreAction = ERHIStoreAction::DontCare;
            att.clearDepth = depth;
            return att;
        }
    };

    //=========================================================================
    // RHIRenderPassDesc (13-01)
    //=========================================================================

    /// レンダーパス記述
    struct RHI_API RHIRenderPassDesc
    {
        RHIRenderTargetAttachment renderTargets[kMaxRenderTargets];
        uint32 renderTargetCount = 0;
        RHIDepthStencilAttachment depthStencil;
        bool hasDepthStencil = false;

        uint32 renderAreaX = 0;
        uint32 renderAreaY = 0;
        uint32 renderAreaWidth = 0;  ///< 0 = 自動（RT全体）
        uint32 renderAreaHeight = 0; ///< 0 = 自動（RT全体）

        IRHITexture* shadingRateImage = nullptr; ///< VRS用

        /// レンダーパスフラグ
        enum class Flags : uint32
        {
            None = 0,
            SuspendingPass = 1 << 0,         ///< UAVを継続して使用
            ResumingPass = 1 << 1,           ///< 前のパスからUAVを継続
            TileBasedRenderingHint = 1 << 2, ///< タイルベースレンダリングヒント
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// レンダーターゲット追加
        RHIRenderPassDesc& AddRenderTarget(const RHIRenderTargetAttachment& rt)
        {
            if (renderTargetCount < kMaxRenderTargets)
            {
                renderTargets[renderTargetCount++] = rt;
            }
            return *this;
        }

        /// デプスステンシル設定
        RHIRenderPassDesc& SetDepthStencil(const RHIDepthStencilAttachment& ds)
        {
            depthStencil = ds;
            hasDepthStencil = true;
            return *this;
        }

        /// レンダー領域設定
        RHIRenderPassDesc& SetRenderArea(uint32 x, uint32 y, uint32 w, uint32 h)
        {
            renderAreaX = x;
            renderAreaY = y;
            renderAreaWidth = w;
            renderAreaHeight = h;
            return *this;
        }

        /// フルスクリーンパス（RTサイズ全体）
        RHIRenderPassDesc& SetFullRenderArea()
        {
            renderAreaX = 0;
            renderAreaY = 0;
            renderAreaWidth = 0;
            renderAreaHeight = 0;
            return *this;
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIRenderPassDesc::Flags)

    //=========================================================================
    // RHIRenderPassPresets (13-01)
    //=========================================================================

    /// レンダーパスプリセット
    namespace RHIRenderPassPresets
    {
        /// 単一RTクリア
        inline RHIRenderPassDesc SingleRTClear(
            IRHIRenderTargetView* rtv, float r = 0, float g = 0, float b = 0, float a = 1)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(rtv, r, g, b, a));
            return desc;
        }

        /// 単一RT + デプスクリア
        inline RHIRenderPassDesc SingleRTWithDepthClear(IRHIRenderTargetView* rtv,
                                                        IRHIDepthStencilView* dsv,
                                                        float r = 0,
                                                        float g = 0,
                                                        float b = 0,
                                                        float a = 1,
                                                        float depth = 1.0f)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(rtv, r, g, b, a));
            desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(dsv, depth));
            return desc;
        }

        /// デプスオンリーパス
        inline RHIRenderPassDesc DepthOnly(IRHIDepthStencilView* dsv, bool clear = true, float depth = 1.0f)
        {
            RHIRenderPassDesc desc;
            if (clear)
                desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(dsv, depth));
            else
                desc.SetDepthStencil(RHIDepthStencilAttachment::LoadStore(dsv));
            return desc;
        }

        /// GBufferパス
        inline RHIRenderPassDesc GBuffer(IRHIRenderTargetView* albedo,
                                         IRHIRenderTargetView* normal,
                                         IRHIRenderTargetView* material,
                                         IRHIDepthStencilView* depth)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(albedo));
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(normal));
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(material));
            desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(depth));
            return desc;
        }

        /// ポストプロセス（デプスなし）
        inline RHIRenderPassDesc PostProcess(IRHIRenderTargetView* output)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::DontCareStore(output));
            return desc;
        }
    } // namespace RHIRenderPassPresets

    //=========================================================================
    // RHILoadStoreOptimization (13-02)
    //=========================================================================

    /// ロードストアアクション最適化ヘルパー
    namespace RHILoadStoreOptimization
    {
        /// ロードアクション最適化
        inline ERHILoadAction OptimizeLoad(ERHILoadAction action, bool willReadContent)
        {
            if (action == ERHILoadAction::DontCare && willReadContent)
                return ERHILoadAction::Load;
            if (action == ERHILoadAction::Load && !willReadContent)
                return ERHILoadAction::DontCare;
            return action;
        }

        /// ストアアクション最適化
        inline ERHIStoreAction OptimizeStore(ERHIStoreAction action, bool willBeReadLater)
        {
            if (action == ERHIStoreAction::Store && !willBeReadLater)
                return ERHIStoreAction::DontCare;
            if (action == ERHIStoreAction::DontCare && willBeReadLater)
                return ERHIStoreAction::Store;
            return action;
        }

        /// タイルベースGPU向け最適化判定
        inline bool CanBenefitFromDontCare(ERHILoadAction load, ERHIStoreAction store)
        {
            return load == ERHILoadAction::DontCare || store == ERHIStoreAction::DontCare;
        }
    } // namespace RHILoadStoreOptimization

    //=========================================================================
    // RHIExtendedLoadStoreDesc (13-02)
    //=========================================================================

    /// 拡張ロードストア記述（プラットフォーム拡張）
    struct RHI_API RHIExtendedLoadStoreDesc
    {
        ERHILoadAction loadAction = ERHILoadAction::Load;
        ERHIStoreAction storeAction = ERHIStoreAction::Store;
        ERHITileMemoryAction tileAction = ERHITileMemoryAction::KeepInTile;
        bool preserveCompression = true; ///< 圧縮状態を維持（DCC/CMask等）
        bool preserveFMask = true;       ///< FMASK保持（MSAA用）
    };

    //=========================================================================
    // RHISubpassDesc (13-03)
    //=========================================================================

    /// サブパス記述
    struct RHI_API RHISubpassDesc
    {
        const uint32* inputAttachments = nullptr;
        uint32 inputAttachmentCount = 0;
        const uint32* colorAttachments = nullptr;
        uint32 colorAttachmentCount = 0;
        const uint32* resolveAttachments = nullptr;
        const uint32* preserveAttachments = nullptr;
        uint32 preserveAttachmentCount = 0;
        int32 depthStencilAttachment = -1; ///< -1で未使用
    };

    //=========================================================================
    // RHISubpassDependency (13-03)
    //=========================================================================

    /// サブパス依存関係
    struct RHI_API RHISubpassDependency
    {
        uint32 srcSubpass = 0;
        uint32 dstSubpass = 0;
        ERHIPipelineStageFlags srcStageMask = ERHIPipelineStageFlags::AllCommands;
        ERHIPipelineStageFlags dstStageMask = ERHIPipelineStageFlags::AllCommands;
        ERHIAccess srcAccessMask = ERHIAccess::Unknown;
        ERHIAccess dstAccessMask = ERHIAccess::Unknown;

        /// 外部サブパス定数
        static constexpr uint32 kExternalSubpass = ~0u;
    };

    //=========================================================================
    // RHIRenderPassStatistics (13-03)
    //=========================================================================

    /// レンダーパス統計
    struct RHI_API RHIRenderPassStatistics
    {
        uint32 drawCallCount = 0;
        uint64 primitiveCount = 0;
        uint64 vertexCount = 0;
        uint64 instanceCount = 0;
        uint32 dispatchCount = 0;
        uint32 stateChangeCount = 0;
        uint32 barrierCount = 0;
    };

    //=========================================================================
    // RHIScopedRenderPass (13-03)
    //=========================================================================

    /// スコープレンダーパス（RAII）
    /// コンストラクタ/デストラクタの実装はIRHICommandContext.hに定義
    class RHIScopedRenderPass
    {
    public:
        RHIScopedRenderPass(IRHICommandContext* context, const RHIRenderPassDesc& desc);
        ~RHIScopedRenderPass();

        NS_DISALLOW_COPY(RHIScopedRenderPass);

        RHIScopedRenderPass(RHIScopedRenderPass&& other) noexcept : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

    private:
        IRHICommandContext* m_context;
    };

}} // namespace NS::RHI

/// スコープレンダーパスマクロ
#define RHI_SCOPED_RENDER_PASS(context, desc)                                                                          \
    ::NS::RHI::RHIScopedRenderPass NS_MACRO_CONCATENATE(_rhiRenderPass, __LINE__)(context, desc)
