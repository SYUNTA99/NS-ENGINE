/// @file RHIRaytracingPSO.h
/// @brief レイトレーシングパイプラインステートオブジェクト
/// @details DXILライブラリ、ヒットグループ構成、RTPSO記述子、ビルダーを提供。DXR 1.1準拠。
/// @see 19-03-raytracing-pso.md
#pragma once

#include "IRHIResource.h"
#include "RHIRaytracingShader.h"
#include "RHIRefCountPtr.h"

#include <vector>

namespace NS::RHI
{
    //=========================================================================
    // RHIDXILLibraryDesc (19-03)
    //=========================================================================

    /// DXILライブラリ記述
    struct RHI_API RHIDXILLibraryDesc
    {
        /// ライブラリバイトコード
        RHIShaderBytecode bytecode;

        /// エクスポート名配列（nullptr = 全エクスポート）
        const char* const* exportNames = nullptr;

        /// エクスポート数（0 = 全エクスポート）
        uint32 exportCount = 0;
    };

    //=========================================================================
    // RHIRaytracingShaderConfig (19-03)
    //=========================================================================

    /// レイトレーシングシェーダー構成
    struct RHI_API RHIRaytracingShaderConfig
    {
        /// 最大ペイロードサイズ（バイト）
        uint32 maxPayloadSize = 16;

        /// 最大アトリビュートサイズ（バイト、組み込み三角形=8）
        uint32 maxAttributeSize = 8;
    };

    //=========================================================================
    // RHIRaytracingPipelineConfig (19-03)
    //=========================================================================

    /// レイトレーシングパイプライン構成
    struct RHI_API RHIRaytracingPipelineConfig
    {
        /// 最大トレース再帰深度（1〜31、D3D12制限）
        uint32 maxTraceRecursionDepth = 1;
    };

    //=========================================================================
    // RHILocalRootSignatureAssociation (19-03)
    //=========================================================================

    /// ローカルルートシグネチャとシェーダーの関連付け
    struct RHI_API RHILocalRootSignatureAssociation
    {
        /// ローカルルートシグネチャ
        IRHIRootSignature* localRootSignature = nullptr;

        /// 関連付けるシェーダーエクスポート名配列
        const char* const* associatedExportNames = nullptr;

        /// 関連付けエクスポート数
        uint32 associatedExportCount = 0;
    };

    //=========================================================================
    // RHIRaytracingPipelineStateDesc (19-03)
    //=========================================================================

    /// レイトレーシングPSO記述
    struct RHI_API RHIRaytracingPipelineStateDesc
    {
        //=====================================================================
        // DXILライブラリ
        //=====================================================================

        /// DXILライブラリ配列
        const RHIDXILLibraryDesc* libraries = nullptr;
        uint32 libraryCount = 0;

        //=====================================================================
        // ヒットグループ
        //=====================================================================

        /// ヒットグループ配列
        const RHIHitGroupDesc* hitGroups = nullptr;
        uint32 hitGroupCount = 0;

        //=====================================================================
        // シェーダー構成
        //=====================================================================

        RHIRaytracingShaderConfig shaderConfig;
        RHIRaytracingPipelineConfig pipelineConfig;

        //=====================================================================
        // ルートシグネチャ
        //=====================================================================

        /// グローバルルートシグネチャ
        IRHIRootSignature* globalRootSignature = nullptr;

        /// ローカルルートシグネチャ関連付け
        const RHILocalRootSignatureAssociation* localRootSignatures = nullptr;
        uint32 localRootSignatureCount = 0;

        //=====================================================================
        // デバッグ
        //=====================================================================

        const char* debugName = nullptr;
    };

    //=========================================================================
    // IRHIRaytracingPipelineState (19-03)
    //=========================================================================

    /// レイトレーシングPSOインターフェース
    class RHI_API IRHIRaytracingPipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(RayTracingPSO)

        virtual ~IRHIRaytracingPipelineState() = default;

        /// シェーダー識別子取得
        /// @param exportName シェーダーエクスポート名
        /// @return シェーダー識別子（見つからない場合はゼロ初期化）
        virtual RHIShaderIdentifier GetShaderIdentifier(const char* exportName) const = 0;

        /// 最大ペイロードサイズ取得
        virtual uint32 GetMaxPayloadSize() const = 0;

        /// 最大アトリビュートサイズ取得
        virtual uint32 GetMaxAttributeSize() const = 0;

        /// 最大再帰深度取得
        virtual uint32 GetMaxRecursionDepth() const = 0;

        /// グローバルルートシグネチャ取得
        virtual IRHIRootSignature* GetGlobalRootSignature() const = 0;
    };

    using RHIRaytracingPipelineStateRef = TRefCountPtr<IRHIRaytracingPipelineState>;

    //=========================================================================
    // RHIRaytracingPipelineStateBuilder (19-03)
    //=========================================================================

    /// レイトレーシングPSOビルダー
    class RHI_API RHIRaytracingPipelineStateBuilder
    {
    public:
        RHIRaytracingPipelineStateBuilder& AddLibrary(const RHIShaderBytecode& bytecode,
                                                      const char* const* exportNames = nullptr,
                                                      uint32 exportCount = 0)
        {
            RHIDXILLibraryDesc lib;
            lib.bytecode = bytecode;
            lib.exportNames = exportNames;
            lib.exportCount = exportCount;
            m_libraries.push_back(lib);
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& AddHitGroup(const char* hitGroupName,
                                                       const char* closestHitName,
                                                       const char* anyHitName = nullptr,
                                                       const char* intersectionName = nullptr)
        {
            RHIHitGroupDesc hg;
            hg.hitGroupName = hitGroupName;
            hg.closestHitShaderName = closestHitName;
            hg.anyHitShaderName = anyHitName;
            hg.intersectionShaderName = intersectionName;
            m_hitGroups.push_back(hg);
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& SetShaderConfig(uint32 maxPayloadSize, uint32 maxAttributeSize = 8)
        {
            m_shaderConfig.maxPayloadSize = maxPayloadSize;
            m_shaderConfig.maxAttributeSize = maxAttributeSize;
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& SetMaxRecursionDepth(uint32 depth)
        {
            m_pipelineConfig.maxTraceRecursionDepth = depth;
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& SetGlobalRootSignature(IRHIRootSignature* rootSig)
        {
            m_globalRootSignature = rootSig;
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& AddLocalRootSignature(IRHIRootSignature* rootSig,
                                                                 const char* const* exportNames,
                                                                 uint32 exportCount)
        {
            RHILocalRootSignatureAssociation assoc;
            assoc.localRootSignature = rootSig;
            assoc.associatedExportNames = exportNames;
            assoc.associatedExportCount = exportCount;
            m_localRootSignatures.push_back(assoc);
            return *this;
        }

        RHIRaytracingPipelineStateBuilder& SetDebugName(const char* name)
        {
            m_debugName = name;
            return *this;
        }

        const RHIRaytracingPipelineStateDesc& Build() const
        {
            m_desc.libraries = m_libraries.data();
            m_desc.libraryCount = static_cast<uint32>(m_libraries.size());
            m_desc.hitGroups = m_hitGroups.data();
            m_desc.hitGroupCount = static_cast<uint32>(m_hitGroups.size());
            m_desc.shaderConfig = m_shaderConfig;
            m_desc.pipelineConfig = m_pipelineConfig;
            m_desc.globalRootSignature = m_globalRootSignature;
            m_desc.localRootSignatures = m_localRootSignatures.data();
            m_desc.localRootSignatureCount = static_cast<uint32>(m_localRootSignatures.size());
            m_desc.debugName = m_debugName;
            return m_desc;
        }

    private:
        std::vector<RHIDXILLibraryDesc> m_libraries;
        std::vector<RHIHitGroupDesc> m_hitGroups;
        RHIRaytracingShaderConfig m_shaderConfig;
        RHIRaytracingPipelineConfig m_pipelineConfig;
        IRHIRootSignature* m_globalRootSignature = nullptr;
        std::vector<RHILocalRootSignatureAssociation> m_localRootSignatures;
        const char* m_debugName = nullptr;
        mutable RHIRaytracingPipelineStateDesc m_desc;
    };

} // namespace NS::RHI
