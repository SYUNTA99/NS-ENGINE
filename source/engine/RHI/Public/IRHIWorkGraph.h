/// @file IRHIWorkGraph.h
/// @brief ワークグラフパイプラインインターフェース
/// @details GPU上で動的にワークを生成・実行するワークグラフシステム。
/// @see 02-06-work-graph.md
#pragma once

#include "IRHIResource.h"
#include "RHIWorkGraphTypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // IRHIWorkGraphPipeline
    //=========================================================================

    /// ワークグラフパイプラインステート
    class RHI_API IRHIWorkGraphPipeline : public IRHIResource
    {
    public:
        virtual ~IRHIWorkGraphPipeline() = default;

        /// プログラム識別子取得
        virtual uint64 GetProgramIdentifier() const = 0;

        /// バッキングメモリ要件取得
        virtual uint64 GetBackingMemorySize() const = 0;

        /// ノード数取得
        virtual uint32 GetNodeCount() const = 0;

        /// ノードインデックス取得
        /// @param nodeName ノード名
        /// @return ノードインデックス（見つからない場合は-1）
        virtual int32 GetNodeIndex(const char* nodeName) const = 0;

        /// エントリーポイント数取得
        virtual uint32 GetEntryPointCount() const = 0;
    };

}} // namespace NS::RHI
