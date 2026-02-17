/// @file RHIOcclusion.cpp
/// @brief オクルージョンクエリマネージャー・条件付きレンダリング・HiZバッファ実装
#include "RHIOcclusion.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIOcclusionQueryManager
    //=========================================================================

    bool RHIOcclusionQueryManager::Initialize(IRHIDevice* device,
                                              uint32 maxQueries,
                                              uint32 numBufferedFrames,
                                              bool useBinaryOcclusion)
    {
        m_device = device;
        m_maxQueries = maxQueries;
        m_useBinaryOcclusion = useBinaryOcclusion;

        ERHIQueryType const queryType = useBinaryOcclusion ? ERHIQueryType::BinaryOcclusion : ERHIQueryType::Occlusion;
        if (!m_queryAllocator.Initialize(device, queryType, maxQueries, numBufferedFrames))
        {
            return false;
        }

        m_results = new RHIOcclusionResult[maxQueries];
        m_resultCount = 0;
        m_currentQueryCount = 0;

        return true;
    }

    void RHIOcclusionQueryManager::Shutdown()
    {
        m_queryAllocator.Shutdown();
        delete[] m_results;
        m_results = nullptr;
        m_device = nullptr;
    }

    void RHIOcclusionQueryManager::BeginFrame()
    {
        m_queryAllocator.BeginFrame(0);
        m_currentQueryCount = 0;
    }

    void RHIOcclusionQueryManager::EndFrame(IRHICommandContext* context)
    {
        (void)context;
        m_queryAllocator.EndFrame();

        // 前フレーム結果の読み取り
        // 結果バッファのマッピングはバックエンド依存
    }

    RHIOcclusionQueryId RHIOcclusionQueryManager::BeginQuery(IRHICommandContext* context)
    {
        auto alloc = m_queryAllocator.Allocate(1);
        if (!alloc.IsValid())
        {
            return RHIOcclusionQueryId::Invalid();
        }

        if (context != nullptr)
        {
            context->BeginQuery(alloc.heap, alloc.startIndex);
        }

        RHIOcclusionQueryId id;
        id.index = m_currentQueryCount++;
        return id;
    }

    void RHIOcclusionQueryManager::EndQuery(IRHICommandContext* context, RHIOcclusionQueryId id)
    {
        if (!id.IsValid() || (context == nullptr))
        {
            return;
        }
        // EndQueryはheap + indexを必要とする
        // 実際にはallocated queryのheap/indexをトラッキングする必要がある
        (void)id;
    }

    bool RHIOcclusionQueryManager::AreResultsReady() const
    {
        return m_queryAllocator.AreResultsReady(0);
    }

    RHIOcclusionResult RHIOcclusionQueryManager::GetResult(RHIOcclusionQueryId id) const
    {
        if (!id.IsValid() || id.index >= m_resultCount)
        {
            return {};
        }
        return m_results[id.index];
    }

    bool RHIOcclusionQueryManager::IsVisible(RHIOcclusionQueryId id) const
    {
        return GetResult(id).IsVisible();
    }

    //=========================================================================
    // RHIConditionalRendering
    //=========================================================================

    bool RHIConditionalRendering::Initialize(IRHIDevice* device, RHIOcclusionQueryManager* occlusionManager)
    {
        m_device = device;
        m_occlusionManager = occlusionManager;
        return device != nullptr && occlusionManager != nullptr;
    }

    void RHIConditionalRendering::Shutdown()
    {
        m_occlusionManager = nullptr;
        m_device = nullptr;
    }

    void RHIConditionalRendering::BeginFrame()
    {
        // フレーム開始: 前フレームの結果を取得可能にする
    }

    void RHIConditionalRendering::EndFrame(IRHICommandContext* context)
    {
        (void)context;
    }

    bool RHIConditionalRendering::RegisterObject(uint32 objectId)
    {
        (void)objectId;
        // objectId -> ObjectData マッピングに登録
        return true;
    }

    void RHIConditionalRendering::UnregisterObject(uint32 objectId)
    {
        (void)objectId;
    }

    void RHIConditionalRendering::BeginOcclusionTest(IRHICommandContext* context, uint32 objectId)
    {
        if (m_occlusionManager == nullptr)
        {
            return;
        }

        (void)objectId;
        // オクルージョンクエリ開始
        // objectIdに対応するqueryIdを保存
        m_occlusionManager->BeginQuery(context);
    }

    void RHIConditionalRendering::EndOcclusionTest(IRHICommandContext* context, uint32 objectId)
    {
        (void)context;
        (void)objectId;
    }

    bool RHIConditionalRendering::BeginConditionalDraw(IRHICommandContext* context, uint32 objectId) const
    {
        (void)context;
        return IsObjectVisible(objectId);
    }

    void RHIConditionalRendering::EndConditionalDraw(IRHICommandContext* context)
    {
        (void)context;
    }

    bool RHIConditionalRendering::IsObjectVisible(uint32 objectId) const
    {
        (void)objectId;
        // デフォルトは可視（データがなければ描画する）
        return true;
    }

    //=========================================================================
    // RHIHiZBuffer
    //=========================================================================

    bool RHIHiZBuffer::Initialize(IRHIDevice* device, uint32 width, uint32 height)
    {
        m_device = device;
        m_width = width;
        m_height = height;

        // ミップレベル計算
        uint32 maxDim = (width > height) ? width : height;
        m_mipCount = 1;
        while (maxDim > 1)
        {
            maxDim >>= 1;
            ++m_mipCount;
        }

        // HiZテクスチャ作成
        RHITextureDesc texDesc;
        texDesc.width = width;
        texDesc.height = height;
        texDesc.format = ERHIPixelFormat::R32_FLOAT;
        texDesc.mipLevels = m_mipCount;
        texDesc.usage = ERHITextureUsage::ShaderResource | ERHITextureUsage::UnorderedAccess;
        m_hiZTexture = device->CreateTexture(texDesc, "HiZBuffer");

        if (!m_hiZTexture)
        {
            return false;
        }

        // SRV作成
        RHITextureSRVDesc srvDesc;
        srvDesc.texture = m_hiZTexture.Get();
        srvDesc.format = ERHIPixelFormat::R32_FLOAT;
        srvDesc.mipLevels = m_mipCount;
        m_srv = device->CreateShaderResourceView(srvDesc, "HiZBuffer_SRV");

        return m_srv != nullptr;
    }

    void RHIHiZBuffer::Shutdown()
    {
        m_srv = nullptr;
        m_hiZTexture = nullptr;
        m_hiZGenPSO = nullptr;
        m_device = nullptr;
    }

    bool RHIHiZBuffer::Resize(uint32 width, uint32 height)
    {
        if (width == m_width && height == m_height)
        {
            return true;
        }

        IRHIDevice* device = m_device;
        Shutdown();
        return Initialize(device, width, height);
    }

    void RHIHiZBuffer::Generate(IRHICommandContext* context, IRHITexture* depthBuffer)
    {
        // HiZ生成パス
        // 1. デプスバッファをMip 0にコピー/ダウンサンプル
        // 2. 各ミップレベルでコンピュートシェーダーによる最小値ダウンサンプル
        // バックエンド依存（PSOとシェーダーが必要）
        (void)context;
        (void)depthBuffer;
    }

} // namespace NS::RHI
