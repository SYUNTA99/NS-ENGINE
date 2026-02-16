/// @file RHIIndirectArguments.h
/// @brief Indirect描画/ディスパッチ引数構造体
/// @details D3D12/Vulkan/Metal共通のIndirect引数レイアウト、
///          ヘルパー関数、コンパイル時レイアウト検証を提供。
/// @see 21-01-indirect-arguments.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

#include <cstddef>
#include <cstring>

namespace NS { namespace RHI {
    //=========================================================================
    // Draw Indirect 引数 (21-01)
    //=========================================================================

    /// DrawInstanced引数
    /// D3D12_DRAW_ARGUMENTS / VkDrawIndirectCommand互換
    struct RHIDrawArguments
    {
        uint32 vertexCountPerInstance = 0; ///< 頂点数
        uint32 instanceCount = 1;          ///< インスタンス数
        uint32 startVertexLocation = 0;    ///< 開始頂点
        uint32 startInstanceLocation = 0;  ///< 開始インスタンス
    };
    static_assert(sizeof(RHIDrawArguments) == 16, "Layout must match D3D12/Vulkan");

    /// DrawIndexedInstanced引数
    /// D3D12_DRAW_INDEXED_ARGUMENTS / VkDrawIndexedIndirectCommand互換
    struct RHIDrawIndexedArguments
    {
        uint32 indexCountPerInstance = 0; ///< インデックス数
        uint32 instanceCount = 1;         ///< インスタンス数
        uint32 startIndexLocation = 0;    ///< 開始インデックス
        int32 baseVertexLocation = 0;     ///< ベース頂点オフセット
        uint32 startInstanceLocation = 0; ///< 開始インスタンス
    };
    static_assert(sizeof(RHIDrawIndexedArguments) == 20, "Layout must match D3D12/Vulkan");

    //=========================================================================
    // Dispatch Indirect 引数 (21-01)
    //=========================================================================

    /// Dispatch引数
    /// D3D12_DISPATCH_ARGUMENTS / VkDispatchIndirectCommand互換
    struct RHIDispatchArguments
    {
        uint32 threadGroupCountX = 1;
        uint32 threadGroupCountY = 1;
        uint32 threadGroupCountZ = 1;
    };
    static_assert(sizeof(RHIDispatchArguments) == 12, "Layout must match D3D12/Vulkan");

    //=========================================================================
    // Mesh Shader Indirect 引数 (21-01)
    //=========================================================================

    /// DispatchMesh引数
    /// D3D12_DISPATCH_MESH_ARGUMENTS互換
    struct RHIDispatchMeshArguments
    {
        uint32 threadGroupCountX = 1;
        uint32 threadGroupCountY = 1;
        uint32 threadGroupCountZ = 1;
    };
    static_assert(sizeof(RHIDispatchMeshArguments) == 12, "Layout must match D3D12");

    //=========================================================================
    // Multi-Draw Indirect 引数 (21-01)
    //=========================================================================

    /// カウント付きIndirect描画引数
    struct RHIMultiDrawArguments
    {
        uint32 maxDrawCount = 0; ///< 最大描画コール数
        uint32 stride = 0;       ///< 引数間のストライド
    };

    //=========================================================================
    // Ray Tracing Indirect 引数 (21-01)
    //=========================================================================

    /// DispatchRays引数（簡略版）
    struct RHIDispatchRaysArguments
    {
        uint32 width = 1;
        uint32 height = 1;
        uint32 depth = 1;
    };
    static_assert(sizeof(RHIDispatchRaysArguments) == 12, "Layout must match");

    //=========================================================================
    // 引数バッファヘルパー (21-01)
    //=========================================================================

    /// Indirect引数バッファの要件
    struct RHIIndirectArgumentsBufferRequirements
    {
        uint64 alignment = 4; ///< アライメント要件
        uint64 minSize = 0;   ///< 最小サイズ

        /// 必要サイズ計算
        template <typename T> static constexpr uint64 CalculateSize(uint32 count) { return sizeof(T) * count; }
    };

    /// 引数バッファ生成ヘルパー
    class RHIIndirectArgumentsHelper
    {
    public:
        /// Draw引数配列をバッファに書き込み
        static void WriteDrawArguments(void* dest, const RHIDrawArguments* args, uint32 count)
        {
            std::memcpy(dest, args, sizeof(RHIDrawArguments) * count);
        }

        /// DrawIndexed引数配列をバッファに書き込み
        static void WriteDrawIndexedArguments(void* dest, const RHIDrawIndexedArguments* args, uint32 count)
        {
            std::memcpy(dest, args, sizeof(RHIDrawIndexedArguments) * count);
        }

        /// Dispatch引数配列をバッファに書き込み
        static void WriteDispatchArguments(void* dest, const RHIDispatchArguments* args, uint32 count)
        {
            std::memcpy(dest, args, sizeof(RHIDispatchArguments) * count);
        }

        /// 単一のDraw引数を生成
        static RHIDrawArguments MakeDrawArguments(uint32 vertexCount,
                                                  uint32 instanceCount = 1,
                                                  uint32 startVertex = 0,
                                                  uint32 startInstance = 0)
        {
            return {vertexCount, instanceCount, startVertex, startInstance};
        }

        /// 単一のDrawIndexed引数を生成
        static RHIDrawIndexedArguments MakeDrawIndexedArguments(uint32 indexCount,
                                                                uint32 instanceCount = 1,
                                                                uint32 startIndex = 0,
                                                                int32 baseVertex = 0,
                                                                uint32 startInstance = 0)
        {
            return {indexCount, instanceCount, startIndex, baseVertex, startInstance};
        }

        /// 単一のDispatch引数を生成
        static RHIDispatchArguments MakeDispatchArguments(uint32 x, uint32 y = 1, uint32 z = 1) { return {x, y, z}; }
    };

    //=========================================================================
    // コンパイル時検証 (21-01)
    //=========================================================================

    namespace Private
    {
        static_assert(offsetof(RHIDrawArguments, vertexCountPerInstance) == 0);
        static_assert(offsetof(RHIDrawArguments, instanceCount) == 4);
        static_assert(offsetof(RHIDrawArguments, startVertexLocation) == 8);
        static_assert(offsetof(RHIDrawArguments, startInstanceLocation) == 12);

        static_assert(offsetof(RHIDrawIndexedArguments, indexCountPerInstance) == 0);
        static_assert(offsetof(RHIDrawIndexedArguments, instanceCount) == 4);
        static_assert(offsetof(RHIDrawIndexedArguments, startIndexLocation) == 8);
        static_assert(offsetof(RHIDrawIndexedArguments, baseVertexLocation) == 12);
        static_assert(offsetof(RHIDrawIndexedArguments, startInstanceLocation) == 16);

        static_assert(offsetof(RHIDispatchArguments, threadGroupCountX) == 0);
        static_assert(offsetof(RHIDispatchArguments, threadGroupCountY) == 4);
        static_assert(offsetof(RHIDispatchArguments, threadGroupCountZ) == 8);
    } // namespace Private

}} // namespace NS::RHI
