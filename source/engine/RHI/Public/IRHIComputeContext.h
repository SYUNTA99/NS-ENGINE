/// @file IRHIComputeContext.h
/// @brief コンピュートコンテキストインターフェース
/// @details
/// コンピュートシェーダー実行専用のコンテキスト。ディスパッチ、リソースバインディング、UAVクリア、クエリを提供。
/// @see 02-02-compute-context.md
#pragma once

#include "IRHICommandContextBase.h"

namespace NS { namespace RHI {
    //=========================================================================
    // IRHIComputeContext
    //=========================================================================

    /// コンピュートコンテキスト
    /// コンピュートシェーダー実行専用のコンテキスト
    class RHI_API IRHIComputeContext : public IRHICommandContextBase
    {
    public:
        virtual ~IRHIComputeContext() = default;

        //=====================================================================
        // パイプラインステート
        //=====================================================================

        /// コンピュートパイプラインステート設定
        void SetComputePipelineState(IRHIComputePipelineState* pso)
        {
            NS_RHI_DISPATCH(SetComputePipelineState, this, pso);
        }

        /// ルートシグネチャ設定
        void SetComputeRootSignature(IRHIRootSignature* rootSignature)
        {
            NS_RHI_DISPATCH(SetComputeRootSignature, this, rootSignature);
        }

        //=====================================================================
        // 定数バッファ
        //=====================================================================

        /// ルート定数設定（32bit値）
        void SetComputeRoot32BitConstants(uint32 rootParameterIndex,
                                          uint32 num32BitValues,
                                          const void* data,
                                          uint32 destOffset = 0)
        {
            NS_RHI_DISPATCH(SetComputeRoot32BitConstants, this, rootParameterIndex, num32BitValues, data, destOffset);
        }

        /// ルートCBV設定（GPUアドレス）
        void SetComputeRootConstantBufferView(uint32 rootParameterIndex, uint64 bufferAddress)
        {
            NS_RHI_DISPATCH(SetComputeRootCBV, this, rootParameterIndex, bufferAddress);
        }

        //=====================================================================
        // SRV / UAV
        //=====================================================================

        /// ルートSRV設定（GPUアドレス）
        void SetComputeRootShaderResourceView(uint32 rootParameterIndex, uint64 bufferAddress)
        {
            NS_RHI_DISPATCH(SetComputeRootSRV, this, rootParameterIndex, bufferAddress);
        }

        /// ルートUAV設定（GPUアドレス）
        void SetComputeRootUnorderedAccessView(uint32 rootParameterIndex, uint64 bufferAddress)
        {
            NS_RHI_DISPATCH(SetComputeRootUAV, this, rootParameterIndex, bufferAddress);
        }

        //=====================================================================
        // ディスクリプタヒープ (10-02)
        //=====================================================================

        /// ディスクリプタヒープ設定
        /// CBV/SRV/UAVとSamplerの2つまで同時設定可能
        void SetDescriptorHeaps(IRHIDescriptorHeap* cbvSrvUavHeap,
                                 IRHIDescriptorHeap* samplerHeap = nullptr)
        {
            NS_RHI_DISPATCH(SetDescriptorHeaps, this, cbvSrvUavHeap, samplerHeap);
        }

        /// 現在のCBV/SRV/UAVディスクリプタヒープ取得
        virtual IRHIDescriptorHeap* GetCBVSRVUAVHeap() const = 0;

        /// 現在のサンプラーディスクリプタヒープ取得
        virtual IRHIDescriptorHeap* GetSamplerHeap() const = 0;

        //=====================================================================
        // ディスクリプタテーブル
        //=====================================================================

        /// ディスクリプタテーブル設定
        void SetComputeRootDescriptorTable(uint32 rootParameterIndex,
                                           RHIGPUDescriptorHandle baseDescriptor)
        {
            NS_RHI_DISPATCH(SetComputeRootDescriptorTable, this, rootParameterIndex, baseDescriptor);
        }

        //=====================================================================
        // ディスパッチ
        //=====================================================================

        /// コンピュートシェーダーディスパッチ
        /// @param threadGroupCountX X方向スレッドグループ数
        /// @param threadGroupCountY Y方向スレッドグループ数
        /// @param threadGroupCountZ Z方向スレッドグループ数
        void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
        {
            NS_RHI_DISPATCH(Dispatch, this, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        /// 間接ディスパッチ
        /// @param argsBuffer 引数バッファ
        /// @param argsOffset バッファ内のオフセット
        void DispatchIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0)
        {
            NS_RHI_DISPATCH(DispatchIndirect, this, argsBuffer, argsOffset);
        }

        /// 複数回間接ディスパッチ (21-03)
        /// @param argsBuffer 引数バッファ
        /// @param argsOffset バッファ内のオフセット
        /// @param dispatchCount ディスパッチ回数
        /// @param stride 引数間のストライド（0=sizeof(RHIDispatchArguments)）
        void DispatchIndirectMulti(IRHIBuffer* argsBuffer,
                                   uint64 argsOffset,
                                   uint32 dispatchCount,
                                   uint32 stride = 0)
        {
            NS_RHI_DISPATCH(DispatchIndirectMulti, this, argsBuffer, argsOffset, dispatchCount, stride);
        }

        //=====================================================================
        // UAVクリア
        //=====================================================================

        /// UAVをuint値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのuint値
        void ClearUnorderedAccessViewUint(IRHIUnorderedAccessView* uav, const uint32 values[4])
        {
            NS_RHI_DISPATCH(ClearUnorderedAccessViewUint, this, uav, values);
        }

        /// UAVをfloat値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのfloat値
        void ClearUnorderedAccessViewFloat(IRHIUnorderedAccessView* uav, const float values[4])
        {
            NS_RHI_DISPATCH(ClearUnorderedAccessViewFloat, this, uav, values);
        }

        //=====================================================================
        // タイムスタンプ
        //=====================================================================

        /// タイムスタンプ書き込み
        /// @param queryHeap クエリプール
        /// @param queryIndex クエリインデックス
        void WriteTimestamp(IRHIQueryHeap* queryHeap, uint32 queryIndex)
        {
            NS_RHI_DISPATCH(WriteTimestamp, this, queryHeap, queryIndex);
        }

        //=====================================================================
        // パイプライン統計
        //=====================================================================

        /// クエリ開始
        void BeginQuery(IRHIQueryHeap* queryHeap, uint32 queryIndex)
        {
            NS_RHI_DISPATCH(BeginQuery, this, queryHeap, queryIndex);
        }

        /// クエリ終了
        void EndQuery(IRHIQueryHeap* queryHeap, uint32 queryIndex)
        {
            NS_RHI_DISPATCH(EndQuery, this, queryHeap, queryIndex);
        }

        /// クエリ結果をバッファに書き込み
        void ResolveQueryData(IRHIQueryHeap* queryHeap,
                              uint32 startIndex,
                              uint32 numQueries,
                              IRHIBuffer* destinationBuffer,
                              uint64 destinationOffset)
        {
            NS_RHI_DISPATCH(ResolveQueryData, this, queryHeap, startIndex, numQueries, destinationBuffer, destinationOffset);
        }

        //=====================================================================
        // ルート定数便利関数 (05-05)
        //=====================================================================

        /// 型付きコンピュートルート定数設定
        template <typename T>
        void SetComputeRootConstants(uint32 rootIndex, const T& value)
        {
            static_assert(sizeof(T) % 4 == 0, "Size must be multiple of 4 bytes");
            SetComputeRoot32BitConstants(rootIndex, sizeof(T) / 4, &value);
        }

        //=====================================================================
        // ディスパッチ便利関数 (07-06)
        //=====================================================================

        /// 1Dディスパッチ
        void Dispatch1D(uint32 groupCountX) { Dispatch(groupCountX, 1, 1); }

        /// 2Dディスパッチ
        void Dispatch2D(uint32 groupCountX, uint32 groupCountY) { Dispatch(groupCountX, groupCountY, 1); }
    };

}} // namespace NS::RHI
