/// @file RHIWorkGraphTypes.h
/// @brief ワークグラフ型定義
/// @details ワークグラフシステムの列挙型、記述子、メモリ要件を定義。
/// @see 02-06-work-graph.md
#pragma once

#include "Common/Utility/Types.h"
#include "RHIFwd.h"
#include "RHIMacros.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHIWorkGraphNodeType
    //=========================================================================

    /// ワークグラフノードタイプ
    enum class ERHIWorkGraphNodeType : uint8
    {
        Broadcasting, ///< ブロードキャストノード（スレッドグループ単位）
        Coalescing,   ///< コアレッシングノード（スレッド単位、グループ化）
        Thread,       ///< スレッドノード（単一スレッド）
    };

    //=========================================================================
    // ERHIWorkGraphLaunchMode
    //=========================================================================

    /// ノード起動モード
    enum class ERHIWorkGraphLaunchMode : uint8
    {
        Normal,    ///< 通常起動（スレッドグループ単位）
        PerThread, ///< スレッド単位起動
    };

    //=========================================================================
    // ERHIWorkGraphDispatchMode
    //=========================================================================

    /// ディスパッチモード
    enum class ERHIWorkGraphDispatchMode : uint8
    {
        Initialize, ///< CPUから初期入力を提供
        Continue,   ///< 前回の実行からの残りワークを継続
    };

    //=========================================================================
    // RHIWorkGraphInputRecord
    //=========================================================================

    /// ワークグラフノードの入力レコード
    struct RHIWorkGraphInputRecord
    {
        const void* data = nullptr; ///< 入力データポインタ
        uint32 sizeInBytes = 0;     ///< レコードサイズ
        uint32 count = 0;           ///< レコード数
    };

    //=========================================================================
    // RHIWorkGraphNodeDesc
    //=========================================================================

    /// ワークグラフノード定義
    struct RHIWorkGraphNodeDesc
    {
        const char* name = nullptr;             ///< ノード名
        const char* shaderEntryPoint = nullptr; ///< シェーダーエントリーポイント
        ERHIWorkGraphNodeType nodeType = ERHIWorkGraphNodeType::Broadcasting;
        uint32 maxRecursionDepth = 0;          ///< 最大再帰深度
        bool isEntryPoint = false;             ///< エントリーポイントか
        uint32 maxDispatchGrid[3] = {0, 0, 0}; ///< 最大ディスパッチグリッド
    };

    //=========================================================================
    // RHIWorkGraphEdge
    //=========================================================================

    /// ノード間接続
    struct RHIWorkGraphEdge
    {
        uint32 sourceNodeIndex = 0; ///< 出力元ノード
        uint32 destNodeIndex = 0;   ///< 入力先ノード
        uint32 outputSlot = 0;      ///< 出力スロット
    };

    //=========================================================================
    // RHIWorkGraphPipelineDesc
    //=========================================================================

    /// ワークグラフパイプライン記述
    struct RHIWorkGraphPipelineDesc
    {
        IRHIShaderLibrary* shaderLibrary = nullptr;  ///< シェーダーライブラリ
        const RHIWorkGraphNodeDesc* nodes = nullptr; ///< ノード定義配列
        uint32 nodeCount = 0;
        const RHIWorkGraphEdge* edges = nullptr; ///< エッジ定義配列
        uint32 edgeCount = 0;
        IRHIRootSignature* globalRootSignature = nullptr; ///< グローバルルートシグネチャ
        const char* programName = nullptr;                ///< プログラム名
        const char* debugName = nullptr;                  ///< デバッグ名
    };

    //=========================================================================
    // RHIWorkGraphBackingMemory
    //=========================================================================

    /// ワークグラフ実行用バッキングメモリ
    struct RHIWorkGraphBackingMemory
    {
        IRHIBuffer* buffer = nullptr; ///< バッキングバッファ
        uint64 offset = 0;            ///< オフセット
        uint64 size = 0;              ///< サイズ
    };

    //=========================================================================
    // RHIWorkGraphMemoryRequirements
    //=========================================================================

    /// バッキングメモリ要件
    struct RHIWorkGraphMemoryRequirements
    {
        uint64 minSize = 0;          ///< 最小サイズ
        uint64 maxSize = UINT64_MAX; ///< 最大サイズ
        uint64 sizeGranularity = 0;  ///< サイズ粒度
    };

    //=========================================================================
    // RHIWorkGraphDispatchDesc
    //=========================================================================

    /// ワークグラフディスパッチ記述
    struct RHIWorkGraphDispatchDesc
    {
        IRHIWorkGraphPipeline* pipeline = nullptr; ///< パイプライン
        RHIWorkGraphBackingMemory backingMemory;   ///< バッキングメモリ
        ERHIWorkGraphDispatchMode mode = ERHIWorkGraphDispatchMode::Initialize;
        const char* entryPointName = nullptr; ///< エントリーポイントノード名
        RHIWorkGraphInputRecord inputRecords; ///< 入力レコード
    };

} // namespace NS::RHI
