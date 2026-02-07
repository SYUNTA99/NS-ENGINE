/// @file RHIMeshDispatch.h
/// @brief メッシュシェーダーディスパッチヘルパー
/// @details メッシュディスパッチ引数、制限、バッチ管理、パフォーマンス最適化を提供。
/// @see 22-04-mesh-dispatch.md
#pragma once

#include "IRHIBuffer.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <algorithm>
#include <functional>
#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHIDevice;
    class IRHICommandContext;
    class IRHIMeshPipelineState;

    //=========================================================================
    // RHIMeshDispatchArgs (22-04)
    //=========================================================================

    /// メッシュディスパッチ引数（Direct）
    struct RHIMeshDispatchArgs
    {
        uint32 groupCountX = 1;
        uint32 groupCountY = 1;
        uint32 groupCountZ = 1;

        static RHIMeshDispatchArgs FromMeshletCount(uint32 meshletCount, uint32 meshletsPerGroup = 1)
        {
            return {(meshletCount + meshletsPerGroup - 1) / meshletsPerGroup, 1, 1};
        }
    };

    //=========================================================================
    // RHIMeshDispatchLimits (22-04)
    //=========================================================================

    /// メッシュディスパッチ制限
    struct RHIMeshDispatchLimits
    {
        uint32 maxGroupCountX = 65535;
        uint32 maxGroupCountY = 65535;
        uint32 maxGroupCountZ = 65535;
        uint32 maxTotalGroups = 1 << 22; ///< 約400万
    };

    //=========================================================================
    // RHIMeshletBatch (22-04)
    //=========================================================================

    /// メッシュレットバッチ
    struct RHIMeshletBatch
    {
        uint32 meshletOffset; ///< メッシュレット配列内のオフセット
        uint32 meshletCount;  ///< メッシュレット数
        uint32 materialId;    ///< マテリアルID
        uint32 objectId;      ///< オブジェクトID（インスタンスデータ用）
    };

    //=========================================================================
    // RHIMeshletDrawManager (22-04)
    //=========================================================================

    /// メッシュレット描画マネージャー
    /// バッチング、ソート、Indirect引数生成を管理
    class RHI_API RHIMeshletDrawManager
    {
    public:
        explicit RHIMeshletDrawManager(IRHIDevice* device, uint32 maxBatches);

        /// バッチ追加
        void AddBatch(const RHIMeshletBatch& batch);

        /// マテリアルIDでソート
        void SortByMaterial();

        /// フラスタムカリング（GPU）
        /// @param frustumPlanes 6つのフラスタム平面（各4 float = 24 float）
        void CullBatches(const float* frustumPlanes);

        /// Indirect引数バッファを構築
        void BuildIndirectBuffer(IRHICommandContext* context);

        /// すべてのバッチを描画
        void DrawAll(IRHICommandContext* context, IRHIMeshPipelineState* pso);

        /// マテリアル別に描画（PSO切り替え）
        void DrawByMaterial(IRHICommandContext* context,
                            const std::function<IRHIMeshPipelineState*(uint32 materialId)>& psoGetter);

        /// 統計
        uint32 GetTotalMeshlets() const;
        uint32 GetVisibleMeshlets() const;
        uint32 GetBatchCount() const;

        /// クリア
        void Clear();

    private:
        std::vector<RHIMeshletBatch> m_batches;
        RHIBufferRef m_indirectBuffer;
        RHIBufferRef m_countBuffer;
        uint32 m_maxBatches;
        uint32 m_visibleMeshlets = 0;
    };

    //=========================================================================
    // RHIMeshShaderOptimization (22-04)
    //=========================================================================

    /// メッシュシェーダー最適化ヒント
    namespace RHIMeshShaderOptimization
    {
        /// 推奨メッシュレットサイズ
        constexpr uint32 kRecommendedMeshletVertices = 64;
        constexpr uint32 kRecommendedMeshletTriangles = 126; // 128 - 2 for alignment

        /// グループサイズ計算
        /// @param meshletCount 総メッシュレット数
        /// @param meshletsPerGroup グループあたりメッシュレット数
        inline RHIMeshDispatchArgs CalculateDispatchArgs(uint32 meshletCount, uint32 meshletsPerGroup = 1)
        {
            return RHIMeshDispatchArgs::FromMeshletCount(meshletCount, meshletsPerGroup);
        }

        /// 2Dグリッド（大量メッシュレット用）
        inline RHIMeshDispatchArgs CalculateDispatchArgs2D(uint32 meshletCount, uint32 maxGroupX = 65535)
        {
            uint32 groupX = std::min(meshletCount, maxGroupX);
            uint32 groupY = (meshletCount + maxGroupX - 1) / maxGroupX;
            return {groupX, groupY, 1};
        }

        /// Waveサイズに合わせたグループサイズ推奨
        inline uint32 GetRecommendedThreadGroupSize(uint32 waveSize)
        {
            // 通常は128（NVIDIA/AMD両方で効率的）
            return std::max(128u, waveSize);
        }
    } // namespace RHIMeshShaderOptimization

} // namespace NS::RHI
