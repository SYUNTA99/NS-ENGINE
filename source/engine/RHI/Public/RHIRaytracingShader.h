/// @file RHIRaytracingShader.h
/// @brief レイトレーシングシェーダー・シェーダーバインディングテーブル
/// @details シェーダー識別子、ヒットグループ、SBT管理のインターフェースを提供。DXR 1.1準拠。
/// @see 19-02-raytracing-shader.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // 定数 (19-02)
        //=========================================================================

        /// シェーダー識別子サイズ（D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES）
        constexpr uint32 kShaderIdentifierSize = 32;

        /// シェーダーレコードアライメント（D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT）
        constexpr uint32 kShaderRecordAlignment = 32;

        /// シェーダーテーブルアライメント（D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT）
        constexpr uint32 kShaderTableAlignment = 64;

        //=========================================================================
        // RHIShaderIdentifier (19-02)
        //=========================================================================

        /// シェーダー識別子（32バイト）
        struct RHI_API RHIShaderIdentifier
        {
            uint8 data[kShaderIdentifierSize] = {};

            bool IsValid() const
            {
                for (uint32 i = 0; i < kShaderIdentifierSize; ++i)
                {
                    if (data[i] != 0)
                        return true;
                }
                return false;
            }

            bool operator==(const RHIShaderIdentifier& other) const
            {
                for (uint32 i = 0; i < kShaderIdentifierSize; ++i)
                {
                    if (data[i] != other.data[i])
                        return false;
                }
                return true;
            }

            bool operator!=(const RHIShaderIdentifier& other) const { return !(*this == other); }
        };

        //=========================================================================
        // RHIShaderRecord (19-02)
        //=========================================================================

        /// シェーダーレコード
        /// シェーダー識別子 + ローカルルート引数
        struct RHI_API RHIShaderRecord
        {
            /// シェーダー識別子
            RHIShaderIdentifier identifier;

            /// ローカルルート引数データ（nullptr = ローカル引数なし）
            const void* localRootArguments = nullptr;

            /// ローカルルート引数サイズ（バイト）
            uint32 localRootArgumentsSize = 0;

            /// レコード全体サイズ取得（アライメント込み）
            uint32 GetRecordSize() const
            {
                uint32 size = kShaderIdentifierSize + localRootArgumentsSize;
                return static_cast<uint32>(
                    AlignUp(static_cast<MemorySize>(size), static_cast<MemorySize>(kShaderRecordAlignment)));
            }
        };

        //=========================================================================
        // RHIHitGroupDesc (19-02)
        //=========================================================================

        /// ヒットグループ記述
        struct RHI_API RHIHitGroupDesc
        {
            /// ヒットグループ名（RTPSO内で一意）
            const char* hitGroupName = nullptr;

            /// 最近接ヒットシェーダーエクスポート名
            const char* closestHitShaderName = nullptr;

            /// 任意ヒットシェーダーエクスポート名（nullptr = なし）
            const char* anyHitShaderName = nullptr;

            /// 交差シェーダーエクスポート名（nullptr = 組み込み三角形交差）
            const char* intersectionShaderName = nullptr;

            /// 有効か
            bool IsValid() const { return hitGroupName != nullptr; }

            /// プロシージャルヒットグループか
            bool IsProceduralHitGroup() const { return intersectionShaderName != nullptr; }
        };

        //=========================================================================
        // RHIShaderTableRegion (19-02)
        //=========================================================================

        /// シェーダーテーブル領域（DispatchRays用）
        struct RHI_API RHIShaderTableRegion
        {
            /// 開始GPUアドレス
            uint64 startAddress = 0;

            /// 領域全体サイズ（バイト）
            uint64 size = 0;

            /// レコードストライド（バイト）
            uint64 stride = 0;

            bool IsValid() const { return startAddress != 0 && size > 0; }
        };

        //=========================================================================
        // RHIShaderBindingTableDesc (19-02)
        //=========================================================================

        /// SBT作成記述
        struct RHI_API RHIShaderBindingTableDesc
        {
            /// レイ生成シェーダーレコード数
            uint32 rayGenRecordCount = 0;

            /// ミスシェーダーレコード数
            uint32 missRecordCount = 0;

            /// ヒットグループレコード数
            uint32 hitGroupRecordCount = 0;

            /// コーラブルシェーダーレコード数
            uint32 callableRecordCount = 0;

            /// 最大ローカルルート引数サイズ（バイト）
            uint32 maxLocalRootArgumentsSize = 0;

            /// デバッグ名
            const char* debugName = nullptr;
        };

        //=========================================================================
        // IRHIShaderBindingTable (19-02)
        //=========================================================================

        /// シェーダーバインディングテーブルインターフェース
        class RHI_API IRHIShaderBindingTable : public IRHIResource
        {
        public:
            DECLARE_RHI_RESOURCE_TYPE(ShaderBindingTable)

            virtual ~IRHIShaderBindingTable() = default;

        protected:
            IRHIShaderBindingTable() : IRHIResource(ERHIResourceType::ShaderBindingTable) {}

        public:
            /// レイ生成シェーダー領域取得
            virtual RHIShaderTableRegion GetRayGenRegion() const = 0;

            /// ミスシェーダー領域取得
            virtual RHIShaderTableRegion GetMissRegion() const = 0;

            /// ヒットグループ領域取得
            virtual RHIShaderTableRegion GetHitGroupRegion() const = 0;

            /// コーラブルシェーダー領域取得
            virtual RHIShaderTableRegion GetCallableRegion() const = 0;

            /// レイ生成シェーダーレコード設定
            /// @param index レコードインデックス
            /// @param record シェーダーレコード
            virtual void SetRayGenRecord(uint32 index, const RHIShaderRecord& record) = 0;

            /// ミスシェーダーレコード設定
            virtual void SetMissRecord(uint32 index, const RHIShaderRecord& record) = 0;

            /// ヒットグループレコード設定
            virtual void SetHitGroupRecord(uint32 index, const RHIShaderRecord& record) = 0;

            /// コーラブルシェーダーレコード設定
            virtual void SetCallableRecord(uint32 index, const RHIShaderRecord& record) = 0;

            /// バッキングバッファ取得
            virtual IRHIBuffer* GetBuffer() const = 0;

            /// SBT全体サイズ取得
            virtual uint64 GetTotalSize() const = 0;
        };

        using RHIShaderBindingTableRef = TRefCountPtr<IRHIShaderBindingTable>;

        //=========================================================================
        // RHIDispatchRaysDesc (19-02)
        //=========================================================================

        /// DispatchRays記述
        struct RHI_API RHIDispatchRaysDesc
        {
            /// レイ生成シェーダーテーブル領域
            RHIShaderTableRegion rayGenShaderTable;

            /// ミスシェーダーテーブル領域
            RHIShaderTableRegion missShaderTable;

            /// ヒットグループテーブル領域
            RHIShaderTableRegion hitGroupTable;

            /// コーラブルシェーダーテーブル領域
            RHIShaderTableRegion callableShaderTable;

            /// ディスパッチ幅
            uint32 width = 1;

            /// ディスパッチ高さ
            uint32 height = 1;

            /// ディスパッチ深度
            uint32 depth = 1;

            /// SBTからの簡易作成
            static RHIDispatchRaysDesc FromSBT(IRHIShaderBindingTable* sbt, uint32 w, uint32 h, uint32 d = 1)
            {
                RHIDispatchRaysDesc desc;
                if (sbt)
                {
                    desc.rayGenShaderTable = sbt->GetRayGenRegion();
                    desc.missShaderTable = sbt->GetMissRegion();
                    desc.hitGroupTable = sbt->GetHitGroupRegion();
                    desc.callableShaderTable = sbt->GetCallableRegion();
                }
                desc.width = w;
                desc.height = h;
                desc.depth = d;
                return desc;
            }
        };

    } // namespace RHI
} // namespace NS
