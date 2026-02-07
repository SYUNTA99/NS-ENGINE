/// @file RHICommandSignature.h
/// @brief コマンドシグネチャ・GPU駆動レンダリング
/// @details ExecuteIndirect用コマンドシグネチャ、ビルダー、
///          GPU駆動描画バッチ、メッシュレットレンダラーを提供。
/// @see 21-04-command-signature.md, 21-05-execute-indirect.md
#pragma once

#include "IRHIResource.h"
#include "RHIIndirectArguments.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <vector>

namespace NS::RHI
{
    //=========================================================================
    // ERHIIndirectArgumentType (21-04)
    //=========================================================================

    /// Indirect引数タイプ
    enum class ERHIIndirectArgumentType : uint8
    {
        Draw,               ///< DrawInstanced引数
        DrawIndexed,        ///< DrawIndexedInstanced引数
        Dispatch,           ///< Dispatch引数
        DispatchMesh,       ///< DispatchMesh引数
        DispatchRays,       ///< DispatchRays引数
        VertexBufferView,   ///< 頂点バッファビュー変更
        IndexBufferView,    ///< インデックスバッファビュー変更
        Constant,           ///< ルート定数
        ConstantBufferView, ///< CBV変更
        ShaderResourceView, ///< SRV変更
        UnorderedAccessView ///< UAV変更
    };

    //=========================================================================
    // RHIIndirectArgument (21-04)
    //=========================================================================

    /// Indirect引数記述
    struct RHIIndirectArgument
    {
        ERHIIndirectArgumentType type = ERHIIndirectArgumentType::Draw;

        union
        {
            struct
            {
                uint32 rootParameterIndex;
                uint32 destOffsetIn32BitValues;
                uint32 num32BitValues;
            } constant;

            struct
            {
                uint32 rootParameterIndex;
            } cbv;

            struct
            {
                uint32 rootParameterIndex;
            } srv;

            struct
            {
                uint32 rootParameterIndex;
            } uav;

            struct
            {
                uint32 slot;
            } vertexBuffer;
        };

        /// サイズ取得
        uint32 GetByteSize() const;
    };

    //=========================================================================
    // RHICommandSignatureDesc (21-04)
    //=========================================================================

    /// コマンドシグネチャ記述
    struct RHICommandSignatureDesc
    {
        const RHIIndirectArgument* arguments = nullptr;
        uint32 argumentCount = 0;
        uint32 byteStride = 0;                      ///< 0=自動計算
        IRHIRootSignature* rootSignature = nullptr; ///< リソース変更を含む場合必要
        const char* debugName = nullptr;
    };

    //=========================================================================
    // IRHICommandSignature (21-04)
    //=========================================================================

    /// コマンドシグネチャインターフェース
    /// ExecuteIndirectで使用する引数バッファのフォーマットを定義
    class RHI_API IRHICommandSignature : public IRHIResource
    {
    public:
        virtual ~IRHICommandSignature() = default;

        /// 引数のバイトストライド取得
        virtual uint32 GetByteStride() const = 0;

        /// 引数数取得
        virtual uint32 GetArgumentCount() const = 0;

        /// 引数タイプ取得
        virtual ERHIIndirectArgumentType GetArgumentType(uint32 index) const = 0;
    };

    using RHICommandSignatureRef = TRefCountPtr<IRHICommandSignature>;

    //=========================================================================
    // RHICommandSignatureBuilder (21-04)
    //=========================================================================

    /// コマンドシグネチャビルダー
    class RHI_API RHICommandSignatureBuilder
    {
    public:
        RHICommandSignatureBuilder& AddDraw()
        {
            m_arguments.push_back({ERHIIndirectArgumentType::Draw});
            return *this;
        }

        RHICommandSignatureBuilder& AddDrawIndexed()
        {
            m_arguments.push_back({ERHIIndirectArgumentType::DrawIndexed});
            return *this;
        }

        RHICommandSignatureBuilder& AddDispatch()
        {
            m_arguments.push_back({ERHIIndirectArgumentType::Dispatch});
            return *this;
        }

        RHICommandSignatureBuilder& AddDispatchMesh()
        {
            m_arguments.push_back({ERHIIndirectArgumentType::DispatchMesh});
            return *this;
        }

        RHICommandSignatureBuilder& AddConstant(uint32 rootParameterIndex, uint32 destOffset, uint32 numValues)
        {
            RHIIndirectArgument arg{ERHIIndirectArgumentType::Constant};
            arg.constant.rootParameterIndex = rootParameterIndex;
            arg.constant.destOffsetIn32BitValues = destOffset;
            arg.constant.num32BitValues = numValues;
            m_arguments.push_back(arg);
            return *this;
        }

        RHICommandSignatureBuilder& AddVertexBufferView(uint32 slot)
        {
            RHIIndirectArgument arg{ERHIIndirectArgumentType::VertexBufferView};
            arg.vertexBuffer.slot = slot;
            m_arguments.push_back(arg);
            return *this;
        }

        RHICommandSignatureBuilder& AddCBV(uint32 rootParameterIndex)
        {
            RHIIndirectArgument arg{ERHIIndirectArgumentType::ConstantBufferView};
            arg.cbv.rootParameterIndex = rootParameterIndex;
            m_arguments.push_back(arg);
            return *this;
        }

        RHICommandSignatureBuilder& SetRootSignature(IRHIRootSignature* rootSig)
        {
            m_rootSignature = rootSig;
            return *this;
        }

        RHICommandSignatureBuilder& SetDebugName(const char* name)
        {
            m_debugName = name;
            return *this;
        }

        RHICommandSignatureDesc Build() const
        {
            return {m_arguments.data(), static_cast<uint32>(m_arguments.size()), 0, m_rootSignature, m_debugName};
        }

    private:
        std::vector<RHIIndirectArgument> m_arguments;
        IRHIRootSignature* m_rootSignature = nullptr;
        const char* m_debugName = nullptr;
    };

    //=========================================================================
    // RHIStandardCommandSignatures (21-04)
    //=========================================================================

    /// 標準のコマンドシグネチャを取得
    namespace RHIStandardCommandSignatures
    {
        /// 単純なDrawIndexed用
        RHI_API IRHICommandSignature* GetDrawIndexed(IRHIDevice* device);

        /// 単純なDispatch用
        RHI_API IRHICommandSignature* GetDispatch(IRHIDevice* device);

        /// 単純なDispatchMesh用
        RHI_API IRHICommandSignature* GetDispatchMesh(IRHIDevice* device);
    } // namespace RHIStandardCommandSignatures

    //=========================================================================
    // RHIGPUDrivenBatch (21-05)
    //=========================================================================

    /// GPU駆動描画バッチ
    /// カリング結果に基づく間接描画を管理
    class RHI_API RHIGPUDrivenBatch
    {
    public:
        struct PerDrawData
        {
            uint32 objectId;
            uint32 materialId;
            uint32 meshletOffset;
            uint32 meshletCount;
        };

        RHIGPUDrivenBatch(IRHIDevice* device, uint32 maxDraws);

        /// 描画データをGPUバッファにアップロード
        void UploadDrawData(const PerDrawData* data, uint32 count);

        /// カリングコンピュートシェーダー実行
        void ExecuteCulling(IRHIComputeContext* context, IRHIBuffer* visibilityBuffer, IRHIBuffer* instanceBuffer);

        /// 間接描画実行
        void ExecuteDraws(IRHICommandContext* context, IRHICommandSignature* signature);

        /// 引数バッファ取得
        IRHIBuffer* GetArgumentBuffer() const { return m_argumentBuffer.Get(); }

        /// カウントバッファ取得
        IRHIBuffer* GetCountBuffer() const { return m_countBuffer.Get(); }

    private:
        RHIBufferRef m_drawDataBuffer;
        RHIBufferRef m_argumentBuffer;
        RHIBufferRef m_countBuffer;
        uint32 m_maxDraws;
    };

    //=========================================================================
    // RHIMeshletGPURenderer (21-05)
    //=========================================================================

    /// メッシュレットベースGPU駆動レンダリング
    class RHI_API RHIMeshletGPURenderer
    {
    public:
        struct MeshletBatch
        {
            uint32 meshletBufferOffset;
            uint32 meshletCount;
            uint32 materialId;
        };

        RHIMeshletGPURenderer(IRHIDevice* device, uint32 maxMeshlets);

        /// フラスタムカリング + オクルージョンカリング
        void ExecuteTwoPassCulling(IRHIComputeContext* context,
                                   const float* viewProjMatrix,
                                   IRHITexture* hierarchicalZ);

        /// 可視メッシュレットを描画
        void ExecuteDraws(IRHICommandContext* context);

        /// 統計取得
        uint32 GetVisibleMeshletCount() const;

    private:
        RHIBufferRef m_meshletBuffer;
        RHIBufferRef m_visibleMeshletBuffer;
        RHIBufferRef m_indirectArgsBuffer;
        RHIBufferRef m_statsBuffer;
        RHICommandSignatureRef m_dispatchMeshSignature;
        uint32 m_maxMeshlets;
    };

} // namespace NS::RHI
