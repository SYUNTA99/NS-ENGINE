/// @file RHIQuery.h
/// @brief GPUクエリタイプ・ヒープ・アロケーター
/// @details クエリタイプ列挙型、ヒープ記述、結果構造体、クエリヒープインターフェース、
///          フレームベースのクエリアロケーターを提供。
/// @see 14-01-query-types.md, 14-02-query-pool.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // ERHIQueryType (14-01)
        //=========================================================================

        /// クエリタイプ
        enum class ERHIQueryType : uint8
        {
            Occlusion,              ///< 可視ピクセル数をカウント
            BinaryOcclusion,        ///< 1ピクセルでも可視ならtrue
            Timestamp,              ///< GPUタイムスタンプ値
            PipelineStatistics,     ///< 全パイプラインステージの統計
            StreamOutputStatistics, ///< ストリームアウトの統計
            StreamOutputOverflow,   ///< ストリームアウトバッファオーバーフロー検出
            Predication,            ///< 条件付きレンダリング用
        };

        //=========================================================================
        // ERHIPipelineStatisticsFlags (14-01)
        //=========================================================================

        /// パイプライン統計フラグ
        enum class ERHIPipelineStatisticsFlags : uint32
        {
            None = 0,
            IAVertices = 1 << 0,     ///< 入力アセンブラ頂点数
            IAPrimitives = 1 << 1,   ///< 入力アセンブラプリミティブ数
            VSInvocations = 1 << 2,  ///< 頂点シェーダー呼び出し数
            GSInvocations = 1 << 3,  ///< ジオメトリシェーダー呼び出し数
            GSPrimitives = 1 << 4,   ///< ジオメトリシェーダープリミティブ数
            CInvocations = 1 << 5,   ///< ラスタライザ呼び出し数
            CPrimitives = 1 << 6,    ///< ラスタライザプリミティブ数
            PSInvocations = 1 << 7,  ///< ピクセルシェーダー呼び出し数
            HSInvocations = 1 << 8,  ///< ハルシェーダー呼び出し数
            DSInvocations = 1 << 9,  ///< ドメインシェーダー呼び出し数
            CSInvocations = 1 << 10, ///< コンピュートシェーダー呼び出し数
            ASInvocations = 1 << 11, ///< アンプリフィケーションシェーダー呼び出し数
            MSInvocations = 1 << 12, ///< メッシュシェーダー呼び出し数

            All = 0x1FFF,
        };
        RHI_ENUM_CLASS_FLAGS(ERHIPipelineStatisticsFlags)

        // ERHIQueryFlags: RHIEnums.h で定義済み

        //=========================================================================
        // RHIQueryHeapDesc (14-01)
        //=========================================================================

        /// クエリヒープ記述
        struct RHI_API RHIQueryHeapDesc
        {
            ERHIQueryType type = ERHIQueryType::Timestamp;
            uint32 count = 0;
            ERHIPipelineStatisticsFlags pipelineStatisticsFlags = ERHIPipelineStatisticsFlags::None;
            uint32 nodeMask = 0; ///< マルチGPU用

            //=====================================================================
            // ビルダー
            //=====================================================================

            static RHIQueryHeapDesc Timestamp(uint32 queryCount)
            {
                RHIQueryHeapDesc desc;
                desc.type = ERHIQueryType::Timestamp;
                desc.count = queryCount;
                return desc;
            }

            static RHIQueryHeapDesc Occlusion(uint32 queryCount)
            {
                RHIQueryHeapDesc desc;
                desc.type = ERHIQueryType::Occlusion;
                desc.count = queryCount;
                return desc;
            }

            static RHIQueryHeapDesc BinaryOcclusion(uint32 queryCount)
            {
                RHIQueryHeapDesc desc;
                desc.type = ERHIQueryType::BinaryOcclusion;
                desc.count = queryCount;
                return desc;
            }

            static RHIQueryHeapDesc PipelineStatistics(
                uint32 queryCount, ERHIPipelineStatisticsFlags flags = ERHIPipelineStatisticsFlags::All)
            {
                RHIQueryHeapDesc desc;
                desc.type = ERHIQueryType::PipelineStatistics;
                desc.count = queryCount;
                desc.pipelineStatisticsFlags = flags;
                return desc;
            }
        };

        //=========================================================================
        // クエリ結果構造体 (14-01)
        //=========================================================================

        /// オクルージョンクエリ結果
        struct RHI_API RHIOcclusionQueryResult
        {
            uint64 visibleSamples = 0;

            bool IsVisible() const { return visibleSamples > 0; }
        };

        /// パイプライン統計クエリ結果
        struct RHI_API RHIPipelineStatisticsResult
        {
            uint64 iaVertices = 0;
            uint64 iaPrimitives = 0;
            uint64 vsInvocations = 0;
            uint64 gsInvocations = 0;
            uint64 gsPrimitives = 0;
            uint64 cInvocations = 0;
            uint64 cPrimitives = 0;
            uint64 psInvocations = 0;
            uint64 hsInvocations = 0;
            uint64 dsInvocations = 0;
            uint64 csInvocations = 0;
        };

        /// ストリームアウト統計クエリ結果
        struct RHI_API RHIStreamOutputStatisticsResult
        {
            uint64 primitivesWritten = 0;
            uint64 primitivesStorageNeeded = 0;

            bool HasOverflow() const { return primitivesStorageNeeded > primitivesWritten; }
        };

        //=========================================================================
        // IRHIQueryHeap (14-02)
        //=========================================================================

        /// クエリヒープインターフェース
        class RHI_API IRHIQueryHeap : public IRHIResource
        {
        public:
            DECLARE_RHI_RESOURCE_TYPE(QueryHeap)

            virtual ~IRHIQueryHeap() = default;

            //=====================================================================
            // 基本プロパティ
            //=====================================================================

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// クエリタイプ取得
            virtual ERHIQueryType GetQueryType() const = 0;

            /// クエリ数取得
            virtual uint32 GetQueryCount() const = 0;

            /// パイプライン統計フラグ取得
            virtual ERHIPipelineStatisticsFlags GetPipelineStatisticsFlags() const = 0;

            //=====================================================================
            // 結果サイズ
            //=====================================================================

            /// 1クエリあたりの結果サイズ（バイト）
            virtual uint32 GetQueryResultSize() const = 0;

            /// アライメント取得
            virtual uint32 GetQueryResultAlignment() const = 0;

        protected:
            IRHIQueryHeap() : IRHIResource(ERHIResourceType::QueryHeap) {}
        };

        using RHIQueryHeapRef = TRefCountPtr<IRHIQueryHeap>;

        //=========================================================================
        // RHIQueryAllocation (14-02)
        //=========================================================================

        /// クエリアロケーション
        struct RHI_API RHIQueryAllocation
        {
            IRHIQueryHeap* heap = nullptr;
            uint32 startIndex = 0;
            uint32 count = 0;

            bool IsValid() const { return heap != nullptr && count > 0; }
        };

        //=========================================================================
        // RHIQueryAllocator (14-02)
        //=========================================================================

        /// クエリアロケーター
        /// フレームごとのクエリ割り当てを管理
        class RHI_API RHIQueryAllocator
        {
        public:
            RHIQueryAllocator() = default;

            /// 初期化
            bool Initialize(IRHIDevice* device,
                            ERHIQueryType type,
                            uint32 queriesPerFrame,
                            uint32 numBufferedFrames = 3);

            /// シャットダウン
            void Shutdown();

            //=====================================================================
            // フレーム操作
            //=====================================================================

            void BeginFrame(uint32 frameIndex);
            void EndFrame();

            //=====================================================================
            // クエリ割り当て
            //=====================================================================

            /// クエリ割り当て
            RHIQueryAllocation Allocate(uint32 count = 1);

            /// 利用可能クエリ数
            uint32 GetAvailableCount() const;

            //=====================================================================
            // 結果読み取り
            //=====================================================================

            /// 前フレームの結果準備完了か
            bool AreResultsReady(uint32 frameIndex) const;

            /// 結果バッファ取得
            IRHIBuffer* GetResultBuffer(uint32 frameIndex) const;

        private:
            IRHIDevice* m_device = nullptr;
            ERHIQueryType m_type = ERHIQueryType::Timestamp;

            struct FrameData
            {
                RHIQueryHeapRef heap;
                TRefCountPtr<IRHIBuffer> resultBuffer;
                uint32 allocatedCount;
                bool resolved;
            };
            FrameData* m_frameData = nullptr;
            uint32 m_numFrames = 0;
            uint32 m_currentFrame = 0;
            uint32 m_queriesPerFrame = 0;
        };

    } // namespace RHI
} // namespace NS
