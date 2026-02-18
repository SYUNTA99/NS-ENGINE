/// @file IRHITexture.h
/// @brief テクスチャ記述・インターフェース・サブリソース・データ転送
/// @details テクスチャ作成記述、IRHITextureインターフェース、サブリソース計算、
///          Map/Unmap、アップロード/リードバック/コピーヘルパーを提供。
/// @see 04-01-texture-desc.md, 04-02-texture-interface.md,
///      04-03-texture-subresource.md, 04-04-texture-data.md
#pragma once

#include "IRHIResource.h"
#include "RHICheck.h"
#include "RHIEnums.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"
#include <algorithm>
#include <cstdio>

namespace NS { namespace RHI {
    /// 前方宣言
    class IRHIDevice;
    class IRHISwapChain;
    class IRHICommandContext;
    class IRHIBuffer;
    using RHIBufferRef = TRefCountPtr<IRHIBuffer>;

    //=========================================================================
    // ピクセルフォーマットヘルパー前方宣言（15-xxで実装）
    //=========================================================================

    /// ピクセルあたりのビット数取得
    RHI_API uint32 GetPixelFormatBitsPerPixel(ERHIPixelFormat format);

    /// 圧縮フォーマットか
    RHI_API bool IsCompressedFormat(ERHIPixelFormat format);

    /// 圧縮ブロック高さ取得（非圧縮は1）
    RHI_API uint32 GetPixelFormatBlockHeight(ERHIPixelFormat format);

    /// 平面数取得（デプス/ステンシル分離フォーマットの場合2）
    RHI_API uint32 GetPixelFormatPlaneCount(ERHIPixelFormat format);

    //=========================================================================
    // RHIClearValue (04-01)
    //=========================================================================

    /// テクスチャクリア値
    struct RHI_API RHIClearValue
    {
        /// 色クリア値（RGBA）
        float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        /// デプスクリア値
        float depth = 1.0f;

        /// ステンシルクリア値
        uint8 stencil = 0;

        /// 色用クリア値作成
        static RHIClearValue Color(float r, float g, float b, float a = 1.0f)
        {
            RHIClearValue v;
            v.color[0] = r;
            v.color[1] = g;
            v.color[2] = b;
            v.color[3] = a;
            return v;
        }

        /// 黒でクリア
        static RHIClearValue Black() { return Color(0, 0, 0, 1); }

        /// 白でクリア
        static RHIClearValue White() { return Color(1, 1, 1, 1); }

        /// 透明でクリア
        static RHIClearValue Transparent() { return Color(0, 0, 0, 0); }

        /// デプス用クリア値作成
        static RHIClearValue Depth(float d = 1.0f, uint8 s = 0)
        {
            RHIClearValue v;
            v.depth = d;
            v.stencil = s;
            return v;
        }

        /// デプスリバース（Reversed-Z）
        static RHIClearValue DepthReversed() { return Depth(0.0f, 0); }
    };

    //=========================================================================
    // RHITextureDesc (04-01)
    //=========================================================================

    /// テクスチャ作成記述
    struct RHI_API RHITextureDesc
    {
        //=====================================================================
        // 次元・サイズ
        //=====================================================================

        /// テクスチャ次元
        ERHITextureDimension dimension = ERHITextureDimension::Texture2D;

        /// 幅（ピクセル）
        uint32 width = 1;

        /// 高さ（ピクセル、1Dの場合は1）
        uint32 height = 1;

        /// 深度（3Dの場合）または配列サイズ
        uint32 depthOrArraySize = 1;

        //=====================================================================
        // フォーマット
        //=====================================================================

        /// ピクセルフォーマット
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;

        //=====================================================================
        // MIPマップ
        //=====================================================================

        /// MIPレベル数（0 = 自動計算）
        uint32 mipLevels = 1;

        //=====================================================================
        // マルチサンプル
        //=====================================================================

        /// サンプル数
        ERHISampleCount sampleCount = ERHISampleCount::Count1;

        /// サンプル品質
        uint32 sampleQuality = 0;

        //=====================================================================
        // 使用フラグ
        //=====================================================================

        /// 使用フラグ
        ERHITextureUsage usage = ERHITextureUsage::Default;

        //=====================================================================
        // レイアウト
        //=====================================================================

        /// 初期レイアウト
        ERHITextureLayout initialLayout = ERHITextureLayout::Unknown;

        //=====================================================================
        // マルチGPU
        //=====================================================================

        /// 対象GPU
        GPUMask gpuMask = GPUMask::GPU0();

        //=====================================================================
        // 最適化ヒント
        //=====================================================================

        /// クリア値（レンダーターゲット/デプス用）
        RHIClearValue clearValue;

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名
        const char* debugName = nullptr;

        //=====================================================================
        // ビルダーパターン
        //=====================================================================

        RHITextureDesc& SetDimension(ERHITextureDimension d)
        {
            dimension = d;
            return *this;
        }
        RHITextureDesc& SetWidth(uint32 w)
        {
            width = w;
            return *this;
        }
        RHITextureDesc& SetHeight(uint32 h)
        {
            height = h;
            return *this;
        }
        RHITextureDesc& SetDepth(uint32 d)
        {
            depthOrArraySize = d;
            return *this;
        }
        RHITextureDesc& SetArraySize(uint32 s)
        {
            depthOrArraySize = s;
            return *this;
        }
        RHITextureDesc& SetFormat(ERHIPixelFormat f)
        {
            format = f;
            return *this;
        }
        RHITextureDesc& SetMipLevels(uint32 m)
        {
            mipLevels = m;
            return *this;
        }
        RHITextureDesc& SetSampleCount(ERHISampleCount c)
        {
            sampleCount = c;
            return *this;
        }
        RHITextureDesc& SetUsage(ERHITextureUsage u)
        {
            usage = u;
            return *this;
        }
        RHITextureDesc& SetGPUMask(GPUMask m)
        {
            gpuMask = m;
            return *this;
        }
        RHITextureDesc& SetDebugName(const char* name)
        {
            debugName = name;
            return *this;
        }
        RHITextureDesc& SetClearValue(const RHIClearValue& cv)
        {
            clearValue = cv;
            return *this;
        }

        /// サイズ一括設定（2D）
        RHITextureDesc& SetSize(uint32 w, uint32 h)
        {
            width = w;
            height = h;
            return *this;
        }

        /// サイズ一括設定（3D）
        RHITextureDesc& SetSize3D(uint32 w, uint32 h, uint32 d)
        {
            width = w;
            height = h;
            depthOrArraySize = d;
            return *this;
        }
    };

    //=========================================================================
    // テクスチャ作成ヘルパー (04-01)
    //=========================================================================

    /// 2Dテクスチャ作成Desc
    inline RHITextureDesc CreateTexture2DDesc(uint32 width,
                                              uint32 height,
                                              ERHIPixelFormat format,
                                              ERHITextureUsage usage = ERHITextureUsage::Default,
                                              uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = 1;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// レンダーターゲット作成Desc
    inline RHITextureDesc CreateRenderTargetDesc(uint32 width,
                                                 uint32 height,
                                                 ERHIPixelFormat format,
                                                 ERHISampleCount sampleCount = ERHISampleCount::Count1,
                                                 const RHIClearValue& clearValue = RHIClearValue::Black())
    {
        RHITextureDesc desc;
        desc.dimension = (sampleCount != ERHISampleCount::Count1) ? ERHITextureDimension::Texture2DMS
                                                                  : ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.sampleCount = sampleCount;
        desc.usage = ERHITextureUsage::RenderTargetShaderResource;
        desc.clearValue = clearValue;
        return desc;
    }

    /// デプスバッファ作成Desc
    inline RHITextureDesc CreateDepthStencilDesc(uint32 width,
                                                 uint32 height,
                                                 ERHIPixelFormat format = ERHIPixelFormat::D32_FLOAT,
                                                 ERHISampleCount sampleCount = ERHISampleCount::Count1,
                                                 bool shaderResource = true,
                                                 const RHIClearValue& clearValue = RHIClearValue::Depth())
    {
        RHITextureDesc desc;
        desc.dimension = (sampleCount != ERHISampleCount::Count1) ? ERHITextureDimension::Texture2DMS
                                                                  : ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.sampleCount = sampleCount;
        desc.usage = shaderResource ? ERHITextureUsage::DepthShaderResource : ERHITextureUsage::DepthStencil;
        desc.clearValue = clearValue;
        return desc;
    }

    /// 3Dテクスチャ作成Desc
    inline RHITextureDesc CreateTexture3DDesc(uint32 width,
                                              uint32 height,
                                              uint32 depth,
                                              ERHIPixelFormat format,
                                              ERHITextureUsage usage = ERHITextureUsage::Default,
                                              uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture3D;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = depth;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// テクスチャ配列作成Desc
    inline RHITextureDesc CreateTexture2DArrayDesc(uint32 width,
                                                   uint32 height,
                                                   uint32 arraySize,
                                                   ERHIPixelFormat format,
                                                   ERHITextureUsage usage = ERHITextureUsage::Default,
                                                   uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2DArray;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = arraySize;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// キューブマップ作成Desc
    inline RHITextureDesc CreateTextureCubeDesc(uint32 size,
                                                ERHIPixelFormat format,
                                                ERHITextureUsage usage = ERHITextureUsage::Default,
                                                uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::TextureCube;
        desc.width = size;
        desc.height = size;
        desc.depthOrArraySize = 6;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// UAVテクスチャ作成Desc
    inline RHITextureDesc CreateUAVTextureDesc(uint32 width, uint32 height, ERHIPixelFormat format)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.usage = ERHITextureUsage::UnorderedShaderResource;
        return desc;
    }

    //=========================================================================
    // MIPレベル計算 (04-01)
    //=========================================================================

    /// 最大MIPレベル数を計算
    inline uint32 CalculateMaxMipLevels(uint32 width, uint32 height = 1, uint32 depth = 1)
    {
        uint32 maxDim = std::max({width, height, depth});
        uint32 levels = 1;
        while (maxDim > 1)
        {
            maxDim >>= 1;
            ++levels;
        }
        return levels;
    }

    /// 指定MIPレベルのサイズを計算
    inline uint32 CalculateMipSize(uint32 baseSize, uint32 mipLevel)
    {
        return std::max(1u, baseSize >> mipLevel);
    }

    /// Descの自動MIPレベル数を解決
    inline void ResolveMipLevels(RHITextureDesc& desc)
    {
        if (desc.mipLevels == 0)
        {
            desc.mipLevels =
                CalculateMaxMipLevels(desc.width, desc.height, Is3DTexture(desc.dimension) ? desc.depthOrArraySize : 1);
        }
    }

    /// 指定MIPレベルの3Dサイズ取得
    inline Extent3D GetMipLevelExtent(const RHITextureDesc& desc, uint32 mipLevel)
    {
        return Extent3D{CalculateMipSize(desc.width, mipLevel),
                        CalculateMipSize(desc.height, mipLevel),
                        Is3DTexture(desc.dimension) ? CalculateMipSize(desc.depthOrArraySize, mipLevel) : 1};
    }

    //=========================================================================
    // テクスチャメモリサイズ推定 (04-01)
    //=========================================================================

    /// テクスチャのメモリサイズを推定（概算）
    /// @note 実際の割り当てはアライメント等で異なる場合あり
    inline MemorySize EstimateTextureMemorySize(const RHITextureDesc& desc)
    {
        uint32 bpp = GetPixelFormatBitsPerPixel(desc.format);
        if (bpp == 0)
            return 0;

        MemorySize totalSize = 0;
        uint32 arraySize = IsArrayTexture(desc.dimension) ? desc.depthOrArraySize : 1;
        uint32 depth = Is3DTexture(desc.dimension) ? desc.depthOrArraySize : 1;

        // キューブマップは6面
        if (IsCubeTexture(desc.dimension))
        {
            arraySize = desc.depthOrArraySize; // 通常6の倍数
        }

        for (uint32 mip = 0; mip < desc.mipLevels; ++mip)
        {
            uint32 w = CalculateMipSize(desc.width, mip);
            uint32 h = CalculateMipSize(desc.height, mip);
            uint32 d = Is3DTexture(desc.dimension) ? CalculateMipSize(depth, mip) : 1;

            MemorySize mipSize = (static_cast<MemorySize>(w) * h * d * bpp + 7) / 8;
            totalSize += mipSize * arraySize;
        }

        // マルチサンプル
        totalSize *= GetSampleCountValue(desc.sampleCount);

        return totalSize;
    }

    /// 人間が読みやすい形式でサイズを取得
    inline const char* FormatMemorySize(MemorySize bytes, char* buffer, size_t bufferSize)
    {
        if (bytes >= kGigabyte)
        {
            snprintf(buffer, bufferSize, "%.2f GB", static_cast<double>(bytes) / kGigabyte);
        }
        else if (bytes >= kMegabyte)
        {
            snprintf(buffer, bufferSize, "%.2f MB", static_cast<double>(bytes) / kMegabyte);
        }
        else if (bytes >= kKilobyte)
        {
            snprintf(buffer, bufferSize, "%.2f KB", static_cast<double>(bytes) / kKilobyte);
        }
        else
        {
            snprintf(buffer, bufferSize, "%llu B", static_cast<unsigned long long>(bytes));
        }
        return buffer;
    }

    //=========================================================================
    // サブリソースインデックス (04-03)
    //=========================================================================

    /// サブリソースインデックス
    /// D3D12形式: mipSlice + arraySlice * mipLevels + planeSlice * mipLevels * arraySize
    using SubresourceIndex = uint32;

    /// 無効サブリソース
    constexpr SubresourceIndex kInvalidSubresource = ~0u;

    /// サブリソースインデックス計算
    inline constexpr SubresourceIndex CalculateSubresource(
        uint32 mipSlice, uint32 arraySlice, uint32 planeSlice, uint32 mipLevels, uint32 arraySize)
    {
        return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
    }

    /// サブリソースインデックスからMIPレベル取得
    inline constexpr uint32 GetMipFromSubresource(SubresourceIndex subresource, uint32 mipLevels)
    {
        return subresource % mipLevels;
    }

    /// サブリソースインデックスから配列スライス取得
    inline constexpr uint32 GetArraySliceFromSubresource(SubresourceIndex subresource,
                                                         uint32 mipLevels,
                                                         uint32 arraySize)
    {
        return (subresource / mipLevels) % arraySize;
    }

    /// サブリソースインデックスから平面スライス取得
    inline constexpr uint32 GetPlaneFromSubresource(SubresourceIndex subresource, uint32 mipLevels, uint32 arraySize)
    {
        return subresource / (mipLevels * arraySize);
    }

    //=========================================================================
    // RHISubresourceRange (04-03)
    //=========================================================================

    /// サブリソース範囲
    struct RHI_API RHISubresourceRange
    {
        /// 開始MIPレベル
        uint32 baseMipLevel = 0;

        /// MIPレベル数（0 = 残り全て）
        uint32 levelCount = 1;

        /// 開始配列スライス
        uint32 baseArrayLayer = 0;

        /// 配列レイヤー数（0 = 残り全て）
        uint32 layerCount = 1;

        /// 平面スライス（通常0）
        uint32 planeSlice = 0;

        /// 全サブリソースを含む範囲
        static RHISubresourceRange All()
        {
            RHISubresourceRange range;
            range.baseMipLevel = 0;
            range.levelCount = 0;
            range.baseArrayLayer = 0;
            range.layerCount = 0;
            return range;
        }

        /// 単一MIPレベル
        static RHISubresourceRange SingleMip(uint32 mipLevel, uint32 arrayLayer = 0)
        {
            RHISubresourceRange range;
            range.baseMipLevel = mipLevel;
            range.levelCount = 1;
            range.baseArrayLayer = arrayLayer;
            range.layerCount = 1;
            return range;
        }

        /// 単一配列スライス（全MIP）
        static RHISubresourceRange SingleArraySlice(uint32 arrayLayer, uint32 mipCount = 0)
        {
            RHISubresourceRange range;
            range.baseMipLevel = 0;
            range.levelCount = mipCount;
            range.baseArrayLayer = arrayLayer;
            range.layerCount = 1;
            return range;
        }

        /// MIP範囲
        static RHISubresourceRange MipRange(uint32 baseMip, uint32 count)
        {
            RHISubresourceRange range;
            range.baseMipLevel = baseMip;
            range.levelCount = count;
            range.baseArrayLayer = 0;
            range.layerCount = 0;
            return range;
        }
    };

    //=========================================================================
    // ERHICubeFace (04-03)
    //=========================================================================

    /// キューブマップ面
    enum class ERHICubeFace : uint8
    {
        PositiveX = 0, // +X (Right)
        NegativeX = 1, // -X (Left)
        PositiveY = 2, // +Y (Top)
        NegativeY = 3, // -Y (Bottom)
        PositiveZ = 4, // +Z (Front)
        NegativeZ = 5, // -Z (Back)
    };

    /// キューブマップ面数
    constexpr uint32 kCubeFaceCount = 6;

    /// キューブマップ面名取得
    inline const char* GetCubeFaceName(ERHICubeFace face)
    {
        switch (face)
        {
        case ERHICubeFace::PositiveX:
            return "+X";
        case ERHICubeFace::NegativeX:
            return "-X";
        case ERHICubeFace::PositiveY:
            return "+Y";
        case ERHICubeFace::NegativeY:
            return "-Y";
        case ERHICubeFace::PositiveZ:
            return "+Z";
        case ERHICubeFace::NegativeZ:
            return "-Z";
        default:
            return "Unknown";
        }
    }

    //=========================================================================
    // RHISubresourceLayout (04-03)
    //=========================================================================

    /// サブリソースデータレイアウト
    struct RHI_API RHISubresourceLayout
    {
        /// データ開始オフセット（バイト）
        MemoryOffset offset = 0;

        /// サブリソース全体サイズ（バイト）
        MemorySize size = 0;

        /// 行ピッチ（バイト）
        uint32 rowPitch = 0;

        /// スライスピッチ（3Dテクスチャ/配列の1要素分のサイズ、バイト）
        uint32 depthPitch = 0;

        /// 幅（ピクセルまたはブロック単位）
        uint32 width = 0;

        /// 高さ（ピクセルまたはブロック単位）
        uint32 height = 0;

        /// 深度
        uint32 depth = 1;
    };

    //=========================================================================
    // RHITextureMemoryInfo (04-02)
    //=========================================================================

    /// テクスチャメモリ情報
    struct RHI_API RHITextureMemoryInfo
    {
        /// 実際に割り当てられたサイズ
        MemorySize allocatedSize = 0;

        /// 使用可能サイズ
        MemorySize usableSize = 0;

        /// メモリタイプ
        ERHIHeapType heapType = ERHIHeapType::Default;

        /// 行ピッチ（バイト、リニアレイアウトの場合）
        uint32 rowPitch = 0;

        /// スライスピッチ（バイト）
        uint32 slicePitch = 0;

        /// アライメント
        uint32 alignment = 0;

        /// タイル配置（Virtual Textureの場合）
        bool isTiled = false;
    };

    //=========================================================================
    // RHITextureMapResult (04-04)
    //=========================================================================

    /// テクスチャマップ結果
    struct RHI_API RHITextureMapResult
    {
        /// マップされたデータポインタ
        void* data = nullptr;

        /// 行ピッチ（バイト）
        uint32 rowPitch = 0;

        /// 深度ピッチ（バイト）
        uint32 depthPitch = 0;

        /// マップサイズ
        MemorySize size = 0;

        /// 有効か
        bool IsValid() const { return data != nullptr; }

        /// キャスト取得
        template <typename T> T* As() const { return static_cast<T*>(data); }

        /// 指定行へのポインタ取得
        void* GetRowPointer(uint32 row) const { return static_cast<uint8*>(data) + row * rowPitch; }

        /// 指定スライスへのポインタ取得（3D/配列）
        void* GetSlicePointer(uint32 slice) const { return static_cast<uint8*>(data) + slice * depthPitch; }
    };

    //=========================================================================
    // IRHITexture (04-02 + 04-03 + 04-04)
    //=========================================================================

    /// テクスチャリソース
    /// GPU上のイメージデータ
    class RHI_API IRHITexture : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Texture)

        virtual ~IRHITexture() = default;

        //=====================================================================
        // ライフサイクル契約
        //=====================================================================
        //
        // - IRHITextureはTRefCountPtrで管理される
        // - 参照カウントが0になるとOnZeroRefCount()が呼ばれ、
        //   RHIDeferredDeleteQueue::Enqueue()によりGPU完了まで削除を遅延する
        // - GPU操作が完了するまでテクスチャメモリは解放されない
        // - ビュー（SRV/RTV/DSV/UAV）作成はスレッドセーフではない。
        //   同一テクスチャへの並行ビュー作成には外部同期が必要
        // - Placed resourceの場合、ヒープが全placed resourceより長生きすること
        //

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 次元取得
        virtual ERHITextureDimension GetDimension() const = 0;

        /// 幅取得（ピクセル）
        virtual uint32 GetWidth() const = 0;

        /// 高さ取得（ピクセル）
        virtual uint32 GetHeight() const = 0;

        /// 深度取得（3Dテクスチャの場合）
        virtual uint32 GetDepth() const = 0;

        /// 配列サイズ取得
        virtual uint32 GetArraySize() const = 0;

        /// 深度または配列サイズ取得
        uint32 GetDepthOrArraySize() const { return Is3DTexture(GetDimension()) ? GetDepth() : GetArraySize(); }

        /// 2Dサイズ取得
        Extent2D GetSize2D() const { return Extent2D{GetWidth(), GetHeight()}; }

        /// 3Dサイズ取得
        Extent3D GetSize3D() const { return Extent3D{GetWidth(), GetHeight(), GetDepth()}; }

        //=====================================================================
        // フォーマット
        //=====================================================================

        /// ピクセルフォーマット取得
        virtual ERHIPixelFormat GetFormat() const = 0;

        /// ピクセルあたりのビット数取得
        uint32 GetBitsPerPixel() const { return GetPixelFormatBitsPerPixel(GetFormat()); }

        /// 圧縮フォーマットか
        bool IsCompressed() const { return IsCompressedFormat(GetFormat()); }

        /// sRGBフォーマットか
        bool IsSRGB() const { return RHIPixelFormatSRGB::IsSRGB(GetFormat()); }

        //=====================================================================
        // MIPマップ
        //=====================================================================

        /// MIPレベル数取得
        virtual uint32 GetMipLevels() const = 0;

        /// 指定MIPレベルのサイズ取得
        Extent3D GetMipSize(uint32 mipLevel) const
        {
            return Extent3D{CalculateMipSize(GetWidth(), mipLevel),
                            CalculateMipSize(GetHeight(), mipLevel),
                            Is3DTexture(GetDimension()) ? CalculateMipSize(GetDepth(), mipLevel) : 1};
        }

        /// MIPレベルが有効か
        bool IsValidMipLevel(uint32 mipLevel) const { return mipLevel < GetMipLevels(); }

        //=====================================================================
        // マルチサンプル
        //=====================================================================

        /// サンプル数取得
        virtual ERHISampleCount GetSampleCount() const = 0;

        /// サンプル品質取得
        virtual uint32 GetSampleQuality() const = 0;

        /// マルチサンプルか
        bool IsMultisampled() const { return NS::RHI::IsMultisampled(GetSampleCount()); }

        //=====================================================================
        // 使用フラグ
        //=====================================================================

        /// 使用フラグ取得
        virtual ERHITextureUsage GetUsage() const = 0;

        /// シェーダーリソースとして使用可能か
        bool CanCreateSRV() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::ShaderResource); }

        /// UAVとして使用可能か
        bool CanCreateUAV() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::UnorderedAccess); }

        /// レンダーターゲットとして使用可能か
        bool CanCreateRTV() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::RenderTarget); }

        /// デプスステンシルとして使用可能か
        bool CanCreateDSV() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::DepthStencil); }

        /// CPUから読み取り可能か
        bool IsCPUReadable() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::CPUReadable); }

        /// CPUから書き込み可能か
        bool IsCPUWritable() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::CPUWritable); }

        /// プレゼント可能か
        bool IsPresentable() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::Present); }

        /// MIP自動生成対応か
        bool SupportsGenerateMips() const { return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::GenerateMips); }

        //=====================================================================
        // 次元判定
        //=====================================================================

        /// 1Dテクスチャか
        bool Is1D() const
        {
            auto dim = GetDimension();
            return dim == ERHITextureDimension::Texture1D || dim == ERHITextureDimension::Texture1DArray;
        }

        /// 2Dテクスチャか
        bool Is2D() const
        {
            auto dim = GetDimension();
            return dim == ERHITextureDimension::Texture2D || dim == ERHITextureDimension::Texture2DArray ||
                   dim == ERHITextureDimension::Texture2DMS || dim == ERHITextureDimension::Texture2DMSArray;
        }

        /// 3Dテクスチャか
        bool Is3D() const { return GetDimension() == ERHITextureDimension::Texture3D; }

        /// キューブマップか
        bool IsCube() const { return IsCubeTexture(GetDimension()); }

        /// 配列テクスチャか
        bool IsArray() const { return IsArrayTexture(GetDimension()); }

        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// メモリ情報を取得
        virtual RHITextureMemoryInfo GetMemoryInfo() const = 0;

        /// 割り当てサイズ取得
        MemorySize GetAllocatedSize() const { return GetMemoryInfo().allocatedSize; }

        /// ヒープタイプ取得
        ERHIHeapType GetHeapType() const { return GetMemoryInfo().heapType; }

        //=====================================================================
        // クリア値
        //=====================================================================

        /// 最適クリア値取得
        virtual RHIClearValue GetClearValue() const = 0;

        /// 最適クリア値でクリアすべきか
        bool HasOptimizedClearValue() const { return CanCreateRTV() || CanCreateDSV(); }

        //=====================================================================
        // スワップチェーン関連
        //=====================================================================

        /// スワップチェーン所属か
        virtual bool IsSwapChainTexture() const { return false; }

        /// 所属スワップチェーン取得
        virtual IRHISwapChain* GetSwapChain() const { return nullptr; }

        /// スワップチェーン内インデックス取得
        virtual uint32 GetSwapChainBufferIndex() const { return 0; }

        //=====================================================================
        // サブリソース (04-03)
        //=====================================================================

        /// 総サブリソース数取得
        uint32 GetTotalSubresourceCount() const { return GetMipLevels() * GetArraySize() * GetPlaneCount(); }

        /// 平面数取得（デプス/ステンシル分離フォーマットの場合2）
        uint32 GetPlaneCount() const { return GetPixelFormatPlaneCount(GetFormat()); }

        /// サブリソースインデックス計算
        SubresourceIndex GetSubresourceIndex(uint32 mipLevel, uint32 arraySlice = 0, uint32 planeSlice = 0) const
        {
            return CalculateSubresource(mipLevel, arraySlice, planeSlice, GetMipLevels(), GetArraySize());
        }

        /// 範囲が有効か検証
        bool IsValidSubresourceRange(const RHISubresourceRange& range) const
        {
            uint32 maxMip =
                range.baseMipLevel + (range.levelCount > 0 ? range.levelCount : GetMipLevels() - range.baseMipLevel);
            uint32 maxArray = range.baseArrayLayer +
                              (range.layerCount > 0 ? range.layerCount : GetArraySize() - range.baseArrayLayer);
            return maxMip <= GetMipLevels() && maxArray <= GetArraySize();
        }

        //=====================================================================
        // サブリソースレイアウト (04-03)
        //=====================================================================

        /// サブリソースレイアウト取得
        virtual RHISubresourceLayout GetSubresourceLayout(uint32 mipLevel, uint32 arraySlice = 0) const = 0;

        /// アップロード用に必要なサイズ計算
        MemorySize CalculateUploadSize(const RHISubresourceRange& range) const;

        /// 行数取得（圧縮フォーマット考慮）
        uint32 GetRowCount(uint32 mipLevel) const
        {
            uint32 h = CalculateMipSize(GetHeight(), mipLevel);
            uint32 blockHeight = GetPixelFormatBlockHeight(GetFormat());
            return (h + blockHeight - 1) / blockHeight;
        }

        //=====================================================================
        // キューブマップ面 (04-03)
        //=====================================================================

        /// キューブマップ面のサブリソースインデックス取得
        SubresourceIndex GetCubeFaceSubresource(ERHICubeFace face, uint32 mipLevel = 0, uint32 cubeIndex = 0) const
        {
            RHI_CHECK(IsCube());
            uint32 faceIndex = static_cast<uint32>(face);
            uint32 arraySlice = cubeIndex * kCubeFaceCount + faceIndex;
            return GetSubresourceIndex(mipLevel, arraySlice);
        }

        //=====================================================================
        // 記述情報 (04-02)
        //=====================================================================

        /// 作成時の記述情報を再構築
        /// @note debugNameはnullptr（リソース自体がデバッグ名を保持）
        RHITextureDesc GetDesc() const
        {
            RHITextureDesc desc;
            desc.dimension = GetDimension();
            desc.width = GetWidth();
            desc.height = GetHeight();
            desc.depthOrArraySize = GetDepthOrArraySize();
            desc.format = GetFormat();
            desc.mipLevels = GetMipLevels();
            desc.sampleCount = GetSampleCount();
            desc.sampleQuality = GetSampleQuality();
            desc.usage = GetUsage();
            desc.clearValue = GetClearValue();
            desc.debugName = nullptr;
            return desc;
        }

        /// 同じパラメータで新しいテクスチャを作成可能なDescを取得
        RHITextureDesc GetCloneDesc() const
        {
            RHITextureDesc desc = GetDesc();
            desc.debugName = nullptr;
            return desc;
        }

        //=====================================================================
        // Map/Unmap (04-04)
        //=====================================================================

        /// テクスチャサブリソースをマップ
        /// @note CPUWritable/CPUReadableフラグ、またはリニアレイアウトが必要
        virtual RHITextureMapResult Map(uint32 mipLevel, uint32 arraySlice, ERHIMapMode mode) = 0;

        /// テクスチャをアンマップ
        virtual void Unmap(uint32 mipLevel, uint32 arraySlice) = 0;

        /// マップ可能か
        bool CanMap() const { return IsCPUReadable() || IsCPUWritable(); }

    protected:
        IRHITexture() : IRHIResource(ERHIResourceType::Texture) {}
    };

    /// テクスチャ参照カウントポインタ
    using RHITextureRef = TRefCountPtr<IRHITexture>;

    //=========================================================================
    // RHISubresourceIterator (04-03)
    //=========================================================================

    /// サブリソース範囲イテレータ
    class RHI_API RHISubresourceIterator
    {
    public:
        RHISubresourceIterator(const IRHITexture* texture, const RHISubresourceRange& range)
            : m_texture(texture), m_range(range), m_currentMip(range.baseMipLevel),
              m_currentArray(range.baseArrayLayer), m_currentPlane(range.planeSlice)
        {
            m_maxMip = range.baseMipLevel +
                       (range.levelCount > 0 ? range.levelCount : texture->GetMipLevels() - range.baseMipLevel);
            m_maxArray = range.baseArrayLayer +
                         (range.layerCount > 0 ? range.layerCount : texture->GetArraySize() - range.baseArrayLayer);
        }

        /// 次のサブリソースへ進む
        bool Next()
        {
            ++m_currentMip;
            if (m_currentMip >= m_maxMip)
            {
                m_currentMip = m_range.baseMipLevel;
                ++m_currentArray;
                if (m_currentArray >= m_maxArray)
                {
                    return false;
                }
            }
            return true;
        }

        /// 現在のサブリソースインデックス取得
        SubresourceIndex GetCurrentIndex() const
        {
            return m_texture->GetSubresourceIndex(m_currentMip, m_currentArray, m_currentPlane);
        }

        /// 現在のMIPレベル取得
        uint32 GetCurrentMip() const { return m_currentMip; }

        /// 現在の配列スライス取得
        uint32 GetCurrentArraySlice() const { return m_currentArray; }

        /// 終了判定
        bool IsEnd() const { return m_currentArray >= m_maxArray; }

    private:
        const IRHITexture* m_texture;
        RHISubresourceRange m_range;
        uint32 m_currentMip;
        uint32 m_currentArray;
        uint32 m_currentPlane;
        uint32 m_maxMip;
        uint32 m_maxArray;
    };

    /// 範囲内のサブリソースを列挙（コールバック版）
    template <typename Func>
    void ForEachSubresource(const IRHITexture* texture, const RHISubresourceRange& range, Func&& func)
    {
        RHISubresourceIterator it(texture, range);
        do
        {
            func(it.GetCurrentIndex(), it.GetCurrentMip(), it.GetCurrentArraySlice());
        }
        while (it.Next());
    }

    //=========================================================================
    // テクスチャ初期データ (04-04)
    //=========================================================================

    /// テクスチャサブリソース初期データ
    struct RHI_API RHITextureSubresourceData
    {
        /// データポインタ
        const void* data = nullptr;

        /// 行ピッチ（バイト）
        uint32 rowPitch = 0;

        /// スライスピッチ（バイト）
        uint32 depthPitch = 0;
    };

    /// テクスチャ初期化データ
    struct RHI_API RHITextureInitData
    {
        /// サブリソースデータ配列
        /// インデックス = mip + arraySlice * mipLevels
        const RHITextureSubresourceData* subresources = nullptr;

        /// サブリソース数
        uint32 subresourceCount = 0;

        /// 単一サブリソースで初期化
        static RHITextureInitData Single(const RHITextureSubresourceData& data)
        {
            RHITextureInitData init;
            init.subresources = &data;
            init.subresourceCount = 1;
            return init;
        }
    };

    /// 作成と同時に初期化するテクスチャDesc
    struct RHI_API RHITextureCreateInfo
    {
        RHITextureDesc desc;
        RHITextureInitData initData;
    };

    //=========================================================================
    // RHITextureScopeLock (04-04)
    //=========================================================================

    /// テクスチャスコープロック（RAII）
    class RHI_API RHITextureScopeLock
    {
    public:
        RHITextureScopeLock() = default;

        RHITextureScopeLock(IRHITexture* texture, uint32 mipLevel, uint32 arraySlice, ERHIMapMode mode)
            : m_texture(texture), m_mipLevel(mipLevel), m_arraySlice(arraySlice)
        {
            if (m_texture)
            {
                m_mapResult = m_texture->Map(mipLevel, arraySlice, mode);
            }
        }

        ~RHITextureScopeLock() { Unlock(); }

        NS_DISALLOW_COPY(RHITextureScopeLock);

        RHITextureScopeLock(RHITextureScopeLock&& other) noexcept
            : m_texture(other.m_texture), m_mapResult(other.m_mapResult), m_mipLevel(other.m_mipLevel),
              m_arraySlice(other.m_arraySlice)
        {
            other.m_texture = nullptr;
            other.m_mapResult = {};
        }

        RHITextureScopeLock& operator=(RHITextureScopeLock&& other) noexcept
        {
            if (this != &other)
            {
                Unlock();
                m_texture = other.m_texture;
                m_mapResult = other.m_mapResult;
                m_mipLevel = other.m_mipLevel;
                m_arraySlice = other.m_arraySlice;
                other.m_texture = nullptr;
                other.m_mapResult = {};
            }
            return *this;
        }

        void Unlock()
        {
            if (m_texture && m_mapResult.IsValid())
            {
                m_texture->Unmap(m_mipLevel, m_arraySlice);
                m_mapResult = {};
            }
        }

        bool IsValid() const { return m_mapResult.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        void* GetData() const { return m_mapResult.data; }
        uint32 GetRowPitch() const { return m_mapResult.rowPitch; }
        uint32 GetDepthPitch() const { return m_mapResult.depthPitch; }

        void* GetRowPointer(uint32 row) const { return m_mapResult.GetRowPointer(row); }

    private:
        IRHITexture* m_texture = nullptr;
        RHITextureMapResult m_mapResult;
        uint32 m_mipLevel = 0;
        uint32 m_arraySlice = 0;
    };

    //=========================================================================
    // RHITextureUploader (04-04)
    //=========================================================================

    /// テクスチャアップロードヘルパー
    /// ステージングバッファ経由でGPUテクスチャに転送
    class RHI_API RHITextureUploader
    {
    public:
        RHITextureUploader() = default;

        /// コンストラクタ
        RHITextureUploader(IRHIDevice* device, IRHICommandContext* context);

        /// 2Dテクスチャへアップロード
        bool Upload2D(IRHITexture* dst, uint32 mipLevel, const void* srcData, uint32 srcRowPitch);

        /// 配列スライスへアップロード
        bool Upload2DArray(
            IRHITexture* dst, uint32 mipLevel, uint32 arraySlice, const void* srcData, uint32 srcRowPitch);

        /// 3Dテクスチャへアップロード
        bool Upload3D(IRHITexture* dst, uint32 mipLevel, const void* srcData, uint32 srcRowPitch, uint32 srcDepthPitch);

        /// キューブマップ面へアップロード
        bool UploadCubeFace(
            IRHITexture* dst, ERHICubeFace face, uint32 mipLevel, const void* srcData, uint32 srcRowPitch);

        /// 部分領域へアップロード
        bool UploadRegion(IRHITexture* dst,
                          uint32 mipLevel,
                          uint32 arraySlice,
                          const RHIBox& dstRegion,
                          const void* srcData,
                          uint32 srcRowPitch,
                          uint32 srcDepthPitch);

        /// 全MIPレベルを自動生成（GenerateMipsフラグ必要）
        void GenerateMips(IRHITexture* texture);

    private:
        IRHIDevice* m_device = nullptr;
        IRHICommandContext* m_context = nullptr;
        RHIBufferRef m_stagingBuffer;
    };

    //=========================================================================
    // RHITextureReadback (04-04)
    //=========================================================================

    /// テクスチャリードバックヘルパー
    /// GPUテクスチャからCPUへ読み取り
    class RHI_API RHITextureReadback
    {
    public:
        RHITextureReadback() = default;

        /// コンストラクタ
        RHITextureReadback(IRHIDevice* device, IRHICommandContext* context);

        /// 2Dテクスチャを読み取り（同期、低速）
        bool Read2D(IRHITexture* src, uint32 mipLevel, void* dstData, uint32 dstRowPitch);

        /// 非同期リードバック開始
        uint64 BeginAsyncRead(IRHITexture* src, uint32 mipLevel, uint32 arraySlice = 0);

        /// 非同期リードバック完了確認
        bool IsReadComplete(uint64 readbackId);

        /// 非同期リードバック結果取得
        bool GetReadResult(uint64 readbackId, void* dstData, uint32 dstRowPitch);

    private:
        IRHIDevice* m_device = nullptr;
        IRHICommandContext* m_context = nullptr;
    };

    //=========================================================================
    // RHITextureCopyDesc / RHITextureCopyHelper (04-04)
    //=========================================================================

    /// テクスチャコピー記述
    struct RHI_API RHITextureCopyDesc
    {
        /// ソーステクスチャ
        IRHITexture* srcTexture = nullptr;

        /// ソースサブリソース
        uint32 srcMipLevel = 0;
        uint32 srcArraySlice = 0;

        /// ソースオフセット
        Offset3D srcOffset = {0, 0, 0};

        /// デスティネーションテクスチャ
        IRHITexture* dstTexture = nullptr;

        /// デスティネーションサブリソース
        uint32 dstMipLevel = 0;
        uint32 dstArraySlice = 0;

        /// デスティネーションオフセット
        Offset3D dstOffset = {0, 0, 0};

        /// コピーサイズ（{0,0,0} = ソースMIPの全体）
        Extent3D extent = {0, 0, 0};
    };

    /// テクスチャコピーヘルパー
    class RHI_API RHITextureCopyHelper
    {
    public:
        /// コピーが有効か検証
        static bool Validate(const RHITextureCopyDesc& desc);

        /// コピー実行（バリア自動挿入）
        static void CopyWithBarriers(IRHICommandContext* context, const RHITextureCopyDesc& desc);

        /// フォーマット変換コピー（互換フォーマット間）
        static bool CopyWithFormatConversion(IRHICommandContext* context, const RHITextureCopyDesc& desc);
    };

}} // namespace NS::RHI
