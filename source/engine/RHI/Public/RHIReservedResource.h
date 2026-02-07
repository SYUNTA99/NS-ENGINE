/// @file RHIReservedResource.h
/// @brief Reserved/Sparse Resourceシステム
/// @details 物理メモリの部分コミットによるリソース管理を提供。
/// @see 11-06-reserved-resources.md
#pragma once

#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIReservedResourceCapabilities (11-06)
    //=========================================================================

    /// Reserved Resourceケイパビリティ
    struct RHI_API RHIReservedResourceCapabilities
    {
        bool supportsBuffers = false;   ///< バッファでのサポート
        bool supportsTexture2D = false; ///< 2Dテクスチャでのサポート
        bool supportsTexture3D = false; ///< 3Dテクスチャでのサポート
        bool supportsMipmaps = false;   ///< ミップマップ付きテクスチャのサポート
        uint64 maxVirtualSize = 0;      ///< 最大仮想サイズ

        /// タイルサイズ（バイト）
        static constexpr uint32 kTileSizeInBytes = 65536; // 64KB
    };

    //=========================================================================
    // RHICommitResourceInfo (11-06)
    //=========================================================================

    /// コミット操作記述
    struct RHI_API RHICommitResourceInfo
    {
        uint64 sizeInBytes; ///< コミットサイズ（タイル境界に切り上げ）

        explicit RHICommitResourceInfo(uint64 inSize) : sizeInBytes(inSize) {}
    };

    //=========================================================================
    // RHITextureCommitRegion (11-06)
    //=========================================================================

    /// テクスチャコミット領域（タイル単位）
    struct RHI_API RHITextureCommitRegion
    {
        uint32 mipLevel = 0;    ///< ミップレベル
        uint32 arraySlice = 0;  ///< 配列スライス
        uint32 tileOffsetX = 0; ///< タイルオフセットX
        uint32 tileOffsetY = 0; ///< タイルオフセットY
        uint32 tileOffsetZ = 0; ///< タイルオフセットZ
        uint32 tileSizeX = 0;   ///< タイル数X
        uint32 tileSizeY = 0;   ///< タイル数Y
        uint32 tileSizeZ = 0;   ///< タイル数Z
    };

    //=========================================================================
    // RHITextureTileInfo (11-06)
    //=========================================================================

    /// テクスチャタイル情報
    struct RHI_API RHITextureTileInfo
    {
        /// 各MIPレベルのタイル情報
        struct MipInfo
        {
            uint32 tilesX;
            uint32 tilesY;
            uint32 tilesZ;
            uint32 totalTiles;
        };

        MipInfo* mipInfos = nullptr; ///< ミップ情報配列
        uint32 mipCount = 0;         ///< ミップ数

        /// パックドミップ（タイル未満のミップ群）
        struct PackedMips
        {
            uint32 startMip = 0;  ///< パックドミップ開始レベル
            uint32 tileCount = 0; ///< 必要タイル数
        };
        PackedMips packedMips;

        uint32 totalTiles = 0;       ///< 総タイル数
        uint64 totalVirtualSize = 0; ///< 総仮想サイズ（バイト）
    };

} // namespace NS::RHI
