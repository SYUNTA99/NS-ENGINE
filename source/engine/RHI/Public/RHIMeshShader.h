/// @file RHIMeshShader.h
/// @brief メッシュシェーダー機能検出・メッシュレット構造体
/// @details メッシュシェーダーのケイパビリティ情報とメッシュレットデータ構造を提供。
/// @see 22-01-mesh-shader.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

#include <vector>

namespace NS::RHI
{

    //=========================================================================
    // RHIMeshShaderCapabilities (22-01)
    //=========================================================================

    /// メッシュシェーダー機能フラグ
    struct RHIMeshShaderCapabilities
    {
        bool supported = false;                    ///< メッシュシェーダー対応
        bool amplificationShaderSupported = false; ///< 増幅シェーダー対応

        uint32 maxOutputVertices = 0;    ///< メッシュシェーダー最大出力頂点数
        uint32 maxOutputPrimitives = 0;  ///< メッシュシェーダー最大出力プリミティブ数
        uint32 maxMeshWorkGroupSize = 0; ///< メッシュシェーダーワークグループサイズ
        uint32 maxTaskWorkGroupSize = 0; ///< 増幅シェーダーワークグループサイズ

        uint32 maxMeshOutputMemorySize = 0; ///< 出力メモリサイズ上限
        uint32 maxMeshSharedMemorySize = 0; ///< 共有メモリサイズ上限
        uint32 maxTaskOutputCount = 0;      ///< タスクシェーダー出力数上限
        uint32 maxTaskPayloadSize = 0;      ///< ペイロードサイズ上限

        bool prefersMeshShaderForLOD = false;              ///< LOD処理をMS推奨
        bool prefersMeshShaderForOcclusionCulling = false; ///< オクルージョンカリングにMS推奨
    };

    //=========================================================================
    // メッシュレット構造体 (22-01)
    //=========================================================================

    /// メッシュレット定義
    /// GPU側でも同じレイアウトを使用
    struct RHIMeshlet
    {
        uint32 vertexOffset;   ///< 頂点インデックス配列内のオフセット
        uint32 triangleOffset; ///< プリミティブインデックス配列内のオフセット
        uint32 vertexCount;    ///< 頂点数（最大64または128）
        uint32 triangleCount;  ///< 三角形数（最大64または128）
    };
    static_assert(sizeof(RHIMeshlet) == 16, "Meshlet must be 16 bytes for GPU alignment");

    /// メッシュレットバウンディング情報
    struct RHIMeshletBounds
    {
        float centerX, centerY, centerZ; ///< バウンディング球の中心
        float radius;                    ///< バウンディング球の半径

        float coneAxisX, coneAxisY, coneAxisZ; ///< 法線コーン軸
        float coneCutoff;                      ///< 法線コーンカットオフ
    };
    static_assert(sizeof(RHIMeshletBounds) == 32, "MeshletBounds must be 32 bytes");

    /// メッシュレットデータセット
    struct RHIMeshletData
    {
        std::vector<RHIMeshlet> meshlets;     ///< メッシュレット配列
        std::vector<RHIMeshletBounds> bounds; ///< バウンディング配列
        std::vector<uint32> vertexIndices;    ///< 頂点インデックス
        std::vector<uint8> primitiveIndices;  ///< プリミティブインデックス（バイト三角形）

        uint32 GetMeshletCount() const { return static_cast<uint32>(meshlets.size()); }
    };

} // namespace NS::RHI
