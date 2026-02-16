/// @file RHIOcclusion.h
/// @brief オクルージョンクエリ・プレディケーション・条件付きレンダリング・HiZバッファ
/// @details オクルージョンクエリ結果、マネージャー、プレディケーション操作、
///          条件付きレンダリング、HiZバッファを提供。
/// @see 14-04-occlusion.md
#pragma once

#include "IRHIViews.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHIQuery.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIOcclusionResult (14-04)
    //=========================================================================

    /// オクルージョンクエリ結果（拡張）
    struct RHI_API RHIOcclusionResult
    {
        uint64 samplesPassed = 0;
        bool valid = false;

        /// 可視か
        bool IsVisible() const { return valid && samplesPassed > 0; }

        /// 可視度（0.0〜1.0、参照サンプル数が必要）
        float GetVisibility(uint64 referenceSamples) const
        {
            if (!valid || referenceSamples == 0)
                return 0.0f;
            return static_cast<float>(samplesPassed) / referenceSamples;
        }
    };

    //=========================================================================
    // RHIOcclusionQueryId (14-04)
    //=========================================================================

    /// オクルージョンクエリID
    struct RHI_API RHIOcclusionQueryId
    {
        uint32 index = ~0u;

        bool IsValid() const { return index != ~0u; }
        static RHIOcclusionQueryId Invalid() { return {}; }
    };

    //=========================================================================
    // RHIOcclusionQueryManager (14-04)
    //=========================================================================

    /// オクルージョンクエリマネージャー
    class RHI_API RHIOcclusionQueryManager
    {
    public:
        RHIOcclusionQueryManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        uint32 maxQueries = 4096,
                        uint32 numBufferedFrames = 3,
                        bool useBinaryOcclusion = true);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // クエリ操作
        //=====================================================================

        /// クエリ開始
        RHIOcclusionQueryId BeginQuery(IRHICommandContext* context);

        /// クエリ終了
        void EndQuery(IRHICommandContext* context, RHIOcclusionQueryId id);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了か
        bool AreResultsReady() const;

        /// 結果取得
        RHIOcclusionResult GetResult(RHIOcclusionQueryId id) const;

        /// 可視か（バイナリ結果）
        bool IsVisible(RHIOcclusionQueryId id) const;

        //=====================================================================
        // 情報
        //=====================================================================

        /// 現在フレームの使用クエリ数
        uint32 GetUsedQueryCount() const { return m_currentQueryCount; }

        /// 最大クエリ数
        uint32 GetMaxQueryCount() const { return m_maxQueries; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        bool m_useBinaryOcclusion = true;
        uint32 m_maxQueries = 0;
        uint32 m_currentQueryCount = 0;
        RHIOcclusionResult* m_results = nullptr;
        uint32 m_resultCount = 0;
    };

    // ERHIPredicationOp は RHIEnums.h で定義

    //=========================================================================
    // RHIConditionalRendering (14-04)
    //=========================================================================

    /// 条件付きレンダリング
    /// オクルージョン結果に基づいて描画をスキップ
    class RHI_API RHIConditionalRendering
    {
    public:
        RHIConditionalRendering() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, RHIOcclusionQueryManager* occlusionManager);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // オブジェクト登録
        //=====================================================================

        /// オブジェクト登録
        bool RegisterObject(uint32 objectId);

        /// オブジェクト登録解除
        void UnregisterObject(uint32 objectId);

        //=====================================================================
        // オクルージョンテスト
        //=====================================================================

        /// オブジェクトのオクルージョンテスト開始
        void BeginOcclusionTest(IRHICommandContext* context, uint32 objectId);

        /// オクルージョンテスト終了
        void EndOcclusionTest(IRHICommandContext* context, uint32 objectId);

        //=====================================================================
        // 条件付き描画
        //=====================================================================

        /// オブジェクトが可視の場合のみ描画開始
        /// @return 描画すべきか（前フレームの結果に基づく）
        bool BeginConditionalDraw(IRHICommandContext* context, uint32 objectId);

        /// 条件付き描画終了
        void EndConditionalDraw(IRHICommandContext* context);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// オブジェクトが可視か
        bool IsObjectVisible(uint32 objectId) const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIOcclusionQueryManager* m_occlusionManager = nullptr;

        struct ObjectData
        {
            RHIOcclusionQueryId queryId;
            bool visible;
            bool tested;
        };
        // objectId -> ObjectData マッピング（実装時にコンテナを選択）
    };

    //=========================================================================
    // RHIHiZBuffer (14-04)
    //=========================================================================

    /// HiZバッファ
    /// 階層的オクルージョンカリング用
    class RHI_API RHIHiZBuffer
    {
    public:
        RHIHiZBuffer() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 width, uint32 height);

        /// シャットダウン
        void Shutdown();

        /// リサイズ
        bool Resize(uint32 width, uint32 height);

        //=====================================================================
        // HiZ生成
        //=====================================================================

        /// デプスバッファからHiZを生成
        void Generate(IRHICommandContext* context, IRHITexture* depthBuffer);

        //=====================================================================
        // テクスチャ取得
        //=====================================================================

        /// HiZテクスチャ取得
        IRHITexture* GetHiZTexture() const { return m_hiZTexture; }

        /// ミップレベル数取得
        uint32 GetMipLevelCount() const { return m_mipCount; }

        /// SRV取得
        IRHIShaderResourceView* GetSRV() const { return m_srv; }

    private:
        IRHIDevice* m_device = nullptr;
        TRefCountPtr<IRHITexture> m_hiZTexture;
        TRefCountPtr<IRHIShaderResourceView> m_srv;
        uint32 m_width = 0;
        uint32 m_height = 0;
        uint32 m_mipCount = 0;
        IRHIComputePipelineState* m_hiZGenPSO = nullptr;
    };

}} // namespace NS::RHI
