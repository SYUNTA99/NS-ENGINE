/// @file RHIRaytracingAS.h
/// @brief レイトレーシング加速構造
/// @details BLAS/TLAS構築・更新用の列挙型、記述子、インターフェースを提供。DXR 1.1準拠。
/// @see 19-01-raytracing-as.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // ERHIRaytracingGeometryType (19-01)
        //=========================================================================

        /// レイトレーシングジオメトリタイプ
        enum class ERHIRaytracingGeometryType : uint8
        {
            Triangles,       ///< 三角形メッシュ
            ProceduralAABBs, ///< プロシージャルAABB（カスタム交差テスト）
        };

        //=========================================================================
        // ERHIRaytracingGeometryFlags (19-01)
        //=========================================================================

        /// レイトレーシングジオメトリフラグ
        enum class ERHIRaytracingGeometryFlags : uint32
        {
            None = 0,
            Opaque = 1 << 0,            ///< 不透明（AnyHitシェーダーをスキップ）
            NoDuplicateAnyHit = 1 << 1, ///< AnyHitの重複呼び出しを防止
        };
        RHI_ENUM_CLASS_FLAGS(ERHIRaytracingGeometryFlags)

        //=========================================================================
        // ERHIRaytracingInstanceFlags (19-01)
        //=========================================================================

        /// レイトレーシングインスタンスフラグ
        enum class ERHIRaytracingInstanceFlags : uint32
        {
            None = 0,
            TriangleCullDisable = 1 << 0,           ///< 三角形カリング無効
            TriangleFrontCounterClockwise = 1 << 1, ///< 反時計回りを前面
            ForceOpaque = 1 << 2,                   ///< 強制不透明
            ForceNonOpaque = 1 << 3,                ///< 強制非不透明
        };
        RHI_ENUM_CLASS_FLAGS(ERHIRaytracingInstanceFlags)

        //=========================================================================
        // ERHIRaytracingAccelerationStructureType (19-01)
        //=========================================================================

        /// 加速構造タイプ
        enum class ERHIRaytracingAccelerationStructureType : uint8
        {
            TopLevel,    ///< TLAS（インスタンス参照）
            BottomLevel, ///< BLAS（ジオメトリ実体）
        };

        //=========================================================================
        // ERHIRaytracingBuildFlags (19-01)
        //=========================================================================

        /// 加速構造ビルドフラグ
        enum class ERHIRaytracingBuildFlags : uint32
        {
            None = 0,
            AllowUpdate = 1 << 0,     ///< インプレース更新を許可
            AllowCompaction = 1 << 1, ///< コンパクション（圧縮）を許可
            PreferFastTrace = 1 << 2, ///< トレース速度優先
            PreferFastBuild = 1 << 3, ///< ビルド速度優先
            MinimizeMemory = 1 << 4,  ///< メモリ使用量最小化
            PerformUpdate = 1 << 5,   ///< 更新（リビルドではなく）
        };
        RHI_ENUM_CLASS_FLAGS(ERHIRaytracingBuildFlags)

        //=========================================================================
        // ERHIRaytracingCopyMode (19-01)
        //=========================================================================

        /// 加速構造コピーモード
        enum class ERHIRaytracingCopyMode : uint8
        {
            Clone,                 ///< 完全クローン
            Compact,               ///< コンパクション（圧縮コピー）
            SerializeToBuffer,     ///< バッファへシリアライズ
            DeserializeFromBuffer, ///< バッファからデシリアライズ
        };

        //=========================================================================
        // RHIRaytracingGeometryTrianglesDesc (19-01)
        //=========================================================================

        /// 三角形ジオメトリ記述
        struct RHI_API RHIRaytracingGeometryTrianglesDesc
        {
            /// 頂点バッファGPUアドレス
            uint64 vertexBufferAddress = 0;

            /// 頂点バッファストライド（バイト）
            uint32 vertexStride = 0;

            /// 頂点数
            uint32 vertexCount = 0;

            /// 頂点フォーマット（R32G32B32_Float等）
            ERHIPixelFormat vertexFormat = ERHIPixelFormat::R32G32B32_FLOAT;

            /// インデックスバッファGPUアドレス（0=インデックスなし）
            uint64 indexBufferAddress = 0;

            /// インデックス数
            uint32 indexCount = 0;

            /// インデックスフォーマット
            ERHIIndexFormat indexFormat = ERHIIndexFormat::UInt32;

            /// トランスフォームバッファGPUアドレス（0=なし）
            /// 3x4 float行列
            uint64 transformBufferAddress = 0;
        };

        //=========================================================================
        // RHIRaytracingGeometryAABBsDesc (19-01)
        //=========================================================================

        /// プロシージャルAABBジオメトリ記述
        struct RHI_API RHIRaytracingGeometryAABBsDesc
        {
            /// AABBバッファGPUアドレス
            /// 各AABBは float[6] = { MinX, MinY, MinZ, MaxX, MaxY, MaxZ }
            uint64 aabbBufferAddress = 0;

            /// AABBストライド（バイト、最小24）
            uint32 aabbStride = 24;

            /// AABB数
            uint32 aabbCount = 0;
        };

        //=========================================================================
        // RHIRaytracingGeometryDesc (19-01)
        //=========================================================================

        /// ジオメトリ記述（BLAS構築用）
        struct RHI_API RHIRaytracingGeometryDesc
        {
            ERHIRaytracingGeometryType type = ERHIRaytracingGeometryType::Triangles;
            ERHIRaytracingGeometryFlags flags = ERHIRaytracingGeometryFlags::None;

            union
            {
                /// 三角形記述（type == Triangles時）
                RHIRaytracingGeometryTrianglesDesc triangles;

                /// AABB記述（type == ProceduralAABBs時）
                RHIRaytracingGeometryAABBsDesc aabbs;
            };

            RHIRaytracingGeometryDesc() : triangles{} {}
        };

        //=========================================================================
        // RHIRaytracingInstanceDesc (19-01)
        //=========================================================================

        /// インスタンス記述（TLAS構築用）
        /// D3D12_RAYTRACING_INSTANCE_DESC互換レイアウト
        struct RHI_API RHIRaytracingInstanceDesc
        {
            /// 3x4行列（行優先）
            float transform[3][4] = {};

            /// インスタンスID（24bit）
            uint32 instanceId : 24;

            /// インスタンスマスク（8bit）
            uint32 instanceMask : 8;

            /// SBTオフセット（24bit）
            uint32 instanceContributionToHitGroupIndex : 24;

            /// インスタンスフラグ（8bit）
            uint32 flags : 8;

            /// BLAS GPUアドレス
            uint64 accelerationStructureAddress = 0;

            RHIRaytracingInstanceDesc()
                : instanceId(0), instanceMask(0xFF), instanceContributionToHitGroupIndex(0), flags(0)
            {}

            /// 単位行列設定
            void SetIdentityTransform()
            {
                transform[0][0] = 1.0f;
                transform[0][1] = 0.0f;
                transform[0][2] = 0.0f;
                transform[0][3] = 0.0f;
                transform[1][0] = 0.0f;
                transform[1][1] = 1.0f;
                transform[1][2] = 0.0f;
                transform[1][3] = 0.0f;
                transform[2][0] = 0.0f;
                transform[2][1] = 0.0f;
                transform[2][2] = 1.0f;
                transform[2][3] = 0.0f;
            }
        };

        /// D3D12_RAYTRACING_INSTANCE_DESC互換レイアウト検証
        static_assert(sizeof(RHIRaytracingInstanceDesc) == 64,
                      "RHIRaytracingInstanceDesc must match D3D12_RAYTRACING_INSTANCE_DESC (64 bytes)");

        //=========================================================================
        // RHIRaytracingAccelerationStructureBuildInputs (19-01)
        //=========================================================================

        /// 加速構造ビルド入力
        struct RHI_API RHIRaytracingAccelerationStructureBuildInputs
        {
            ERHIRaytracingAccelerationStructureType type = ERHIRaytracingAccelerationStructureType::BottomLevel;
            ERHIRaytracingBuildFlags flags = ERHIRaytracingBuildFlags::None;

            /// BLAS用: ジオメトリ配列
            const RHIRaytracingGeometryDesc* geometries = nullptr;
            uint32 geometryCount = 0;

            /// TLAS用: インスタンス記述バッファGPUアドレス
            uint64 instanceDescsAddress = 0;

            /// TLAS用: インスタンス数
            uint32 instanceCount = 0;
        };

        //=========================================================================
        // RHIRaytracingAccelerationStructurePrebuildInfo (19-01)
        //=========================================================================

        /// 加速構造プレビルド情報
        struct RHI_API RHIRaytracingAccelerationStructurePrebuildInfo
        {
            /// 結果データサイズ（バイト）
            uint64 resultDataMaxSize = 0;

            /// スクラッチデータサイズ（バイト）
            uint64 scratchDataSize = 0;

            /// 更新用スクラッチデータサイズ（バイト）
            uint64 updateScratchDataSize = 0;
        };

        //=========================================================================
        // RHIRaytracingAccelerationStructureDesc (19-01)
        //=========================================================================

        /// 加速構造作成記述
        struct RHI_API RHIRaytracingAccelerationStructureDesc
        {
            /// 結果データサイズ（PrebuildInfoから取得）
            uint64 resultDataMaxSize = 0;

            /// 結果データを格納するバッファ（AccelerationStructure用途フラグ必須）
            IRHIBuffer* resultBuffer = nullptr;

            /// 結果バッファ内オフセット
            uint64 resultBufferOffset = 0;

            /// GPUマスク（マルチGPU用）
            GPUMask gpuMask;

            /// デバッグ名
            const char* debugName = nullptr;
        };

        //=========================================================================
        // IRHIAccelerationStructure (19-01)
        //=========================================================================

        /// 加速構造インターフェース
        class RHI_API IRHIAccelerationStructure : public IRHIResource
        {
        public:
            DECLARE_RHI_RESOURCE_TYPE(AccelerationStructure)

            virtual ~IRHIAccelerationStructure() = default;

        protected:
            IRHIAccelerationStructure()
                : IRHIResource(ERHIResourceType::AccelerationStructure)
            {
            }

        public:

            /// GPU仮想アドレス取得
            virtual uint64 GetGPUVirtualAddress() const = 0;

            /// 結果バッファ取得
            virtual IRHIBuffer* GetResultBuffer() const = 0;

            /// 結果バッファ内オフセット取得
            virtual uint64 GetResultBufferOffset() const = 0;

            /// 加速構造サイズ取得（バイト）
            virtual uint64 GetSize() const = 0;
        };

        using RHIAccelerationStructureRef = TRefCountPtr<IRHIAccelerationStructure>;

        //=========================================================================
        // RHIRaytracingCapabilities (19-01)
        //=========================================================================

        /// レイトレーシングティア
        enum class ERHIRaytracingTier : uint8
        {
            NotSupported, ///< 未サポート
            Tier1_0,      ///< DXR 1.0
            Tier1_1,      ///< DXR 1.1（インラインレイトレーシング等）
        };

        /// レイトレーシングケイパビリティ
        struct RHI_API RHIRaytracingCapabilities
        {
            ERHIRaytracingTier tier = ERHIRaytracingTier::NotSupported;

            /// 最大インスタンス数（TLAS内）
            uint32 maxInstanceCount = 0;

            /// 最大再帰深度
            uint32 maxRecursionDepth = 0;

            /// 最大ジオメトリ数（BLAS内）
            uint32 maxGeometryCount = 0;

            /// 最大プリミティブ数（BLAS内）
            uint64 maxPrimitiveCount = 0;

            /// インラインレイトレーシングサポート
            bool supportsInlineRaytracing = false;

            /// サポートされているか
            bool IsSupported() const { return tier != ERHIRaytracingTier::NotSupported; }

            /// DXR 1.1以上か
            bool SupportsTier1_1() const { return tier >= ERHIRaytracingTier::Tier1_1; }
        };

        //=========================================================================
        // RHIAccelerationStructureBuildDesc (19-01)
        //=========================================================================

        /// 加速構造ビルドコマンド記述
        struct RHI_API RHIAccelerationStructureBuildDesc
        {
            /// ビルド入力
            RHIRaytracingAccelerationStructureBuildInputs inputs;

            /// 出力先加速構造
            IRHIAccelerationStructure* dest = nullptr;

            /// 更新時のソース加速構造（PerformUpdate時のみ）
            IRHIAccelerationStructure* source = nullptr;

            /// スクラッチバッファGPUアドレス
            uint64 scratchBufferAddress = 0;
        };

    } // namespace RHI
} // namespace NS
