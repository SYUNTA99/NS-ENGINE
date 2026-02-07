/// @file IRHIComputeContext.h
/// @brief コンピュートコンテキストインターフェース
/// @details
/// コンピュートシェーダー実行専用のコンテキスト。ディスパッチ、リソースバインディング、UAVクリア、クエリを提供。
/// @see 02-02-compute-context.md
#pragma once

#include "IRHICommandContextBase.h"

namespace NS::RHI
{
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
        virtual void SetComputePipelineState(IRHIComputePipelineState* pso) = 0;

        /// ルートシグネチャ設定
        virtual void SetComputeRootSignature(IRHIRootSignature* rootSignature) = 0;

        //=====================================================================
        // 定数バッファ
        //=====================================================================

        /// ルート定数設定（32bit値）
        virtual void SetComputeRoot32BitConstants(uint32 rootParameterIndex,
                                                  uint32 num32BitValues,
                                                  const void* data,
                                                  uint32 destOffset = 0) = 0;

        /// ルートCBV設定（GPUアドレス）
        virtual void SetComputeRootConstantBufferView(uint32 rootParameterIndex, uint64 bufferAddress) = 0;

        //=====================================================================
        // SRV / UAV
        //=====================================================================

        /// ルートSRV設定（GPUアドレス）
        virtual void SetComputeRootShaderResourceView(uint32 rootParameterIndex, uint64 bufferAddress) = 0;

        /// ルートUAV設定（GPUアドレス）
        virtual void SetComputeRootUnorderedAccessView(uint32 rootParameterIndex, uint64 bufferAddress) = 0;

        //=====================================================================
        // ディスクリプタヒープ (10-02)
        //=====================================================================

        /// ディスクリプタヒープ設定
        /// CBV/SRV/UAVとSamplerの2つまで同時設定可能
        virtual void SetDescriptorHeaps(IRHIDescriptorHeap* cbvSrvUavHeap,
                                         IRHIDescriptorHeap* samplerHeap = nullptr) = 0;

        /// 現在のCBV/SRV/UAVディスクリプタヒープ取得
        virtual IRHIDescriptorHeap* GetCBVSRVUAVHeap() const = 0;

        /// 現在のサンプラーディスクリプタヒープ取得
        virtual IRHIDescriptorHeap* GetSamplerHeap() const = 0;

        //=====================================================================
        // ディスクリプタテーブル
        //=====================================================================

        /// ディスクリプタテーブル設定
        virtual void SetComputeRootDescriptorTable(uint32 rootParameterIndex,
                                                   RHIGPUDescriptorHandle baseDescriptor) = 0;

        //=====================================================================
        // ディスパッチ
        //=====================================================================

        /// コンピュートシェーダーディスパッチ
        /// @param threadGroupCountX X方向スレッドグループ数
        /// @param threadGroupCountY Y方向スレッドグループ数
        /// @param threadGroupCountZ Z方向スレッドグループ数
        virtual void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) = 0;

        /// 間接ディスパッチ
        /// @param argsBuffer 引数バッファ
        /// @param argsOffset バッファ内のオフセット
        virtual void DispatchIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0) = 0;

        /// 複数回間接ディスパッチ (21-03)
        /// @param argsBuffer 引数バッファ
        /// @param argsOffset バッファ内のオフセット
        /// @param dispatchCount ディスパッチ回数
        /// @param stride 引数間のストライド（0=sizeof(RHIDispatchArguments)）
        virtual void DispatchIndirectMulti(IRHIBuffer* argsBuffer,
                                           uint64 argsOffset,
                                           uint32 dispatchCount,
                                           uint32 stride = 0) = 0;

        //=====================================================================
        // UAVクリア
        //=====================================================================

        /// UAVをuint値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのuint値
        virtual void ClearUnorderedAccessViewUint(IRHIUnorderedAccessView* uav, const uint32 values[4]) = 0;

        /// UAVをfloat値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのfloat値
        virtual void ClearUnorderedAccessViewFloat(IRHIUnorderedAccessView* uav, const float values[4]) = 0;

        //=====================================================================
        // タイムスタンプ
        //=====================================================================

        /// タイムスタンプ書き込み
        /// @param queryHeap クエリプール
        /// @param queryIndex クエリインデックス
        virtual void WriteTimestamp(IRHIQueryHeap* queryHeap, uint32 queryIndex) = 0;

        //=====================================================================
        // パイプライン統計
        //=====================================================================

        /// クエリ開始
        virtual void BeginQuery(IRHIQueryHeap* queryHeap, uint32 queryIndex) = 0;

        /// クエリ終了
        virtual void EndQuery(IRHIQueryHeap* queryHeap, uint32 queryIndex) = 0;

        /// クエリ結果をバッファに書き込み
        virtual void ResolveQueryData(IRHIQueryHeap* queryHeap,
                                      uint32 startIndex,
                                      uint32 numQueries,
                                      IRHIBuffer* destinationBuffer,
                                      uint64 destinationOffset) = 0;

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

} // namespace NS::RHI
