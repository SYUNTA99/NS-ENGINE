/// @file RHITypes.h
/// @brief GPU識別型・リソース識別型
/// @details マルチGPU識別、リソース追跡、ディスクリプタインデックス、メモリサイズ型を定義。
/// @see 01-02-types-gpu.md
#pragma once

#include "Common/Utility/Types.h"
#include "RHIMacros.h"

namespace NS { namespace RHI {
    //=========================================================================
    // GPUMask: マルチGPU識別
    //=========================================================================

    /// GPUビットマスク
    /// マルチGPU構成でどのGPUに影響するかを指定
    /// bit0 = GPU0, bit1 = GPU1, ...
    struct RHI_API GPUMask
    {
        uint32 mask;

        /// デフォルト: GPU0のみ
        constexpr GPUMask() : mask(1) {}

        /// 明示的マスク指定
        explicit constexpr GPUMask(uint32 m) : mask(m) {}

        /// 全GPU
        static constexpr GPUMask All() { return GPUMask(0xFFFFFFFF); }

        /// 指定GPU1つ
        static constexpr GPUMask FromIndex(uint32 index) { return GPUMask(1u << index); }

        /// GPU0のみ（デフォルト）
        static constexpr GPUMask GPU0() { return GPUMask(1); }

        /// 指定GPUを含むか
        constexpr bool Contains(uint32 index) const { return (mask & (1u << index)) != 0; }

        /// 最初のGPUインデックス取得
        uint32 GetFirstIndex() const;

        /// 有効GPU数
        uint32 CountBits() const;

        /// 空か
        constexpr bool IsEmpty() const { return mask == 0; }

        /// 単一GPUか
        bool IsSingleGPU() const { return CountBits() == 1; }

        /// 演算子
        constexpr GPUMask operator|(GPUMask other) const { return GPUMask(mask | other.mask); }
        constexpr GPUMask operator&(GPUMask other) const { return GPUMask(mask & other.mask); }
        GPUMask& operator|=(GPUMask other)
        {
            mask |= other.mask;
            return *this;
        }
        constexpr bool operator==(GPUMask other) const { return mask == other.mask; }
        constexpr bool operator!=(GPUMask other) const { return mask != other.mask; }
    };

    /// GPUインデックス型
    using GPUIndex = uint32;

    /// 無効GPUインデックス
    constexpr GPUIndex kInvalidGPUIndex = ~0u;

    //=========================================================================
    // ResourceId: リソース識別
    //=========================================================================

    /// リソース一意識別子（内部追跡・デバッグ用）
    using ResourceId = uint64;

    /// 無効リソースID
    constexpr ResourceId kInvalidResourceId = 0;

    /// リソースID生成（スレッドセーフなアトミックカウンター）
    RHI_API ResourceId GenerateResourceId();

    //=========================================================================
    // DescriptorIndex: ディスクリプタ識別
    //=========================================================================

    /// ディスクリプタヒープ内インデックス
    using DescriptorIndex = uint32;

    /// 無効ディスクリプタインデックス
    constexpr DescriptorIndex kInvalidDescriptorIndex = ~0u;

    /// ディスクリプタインデックスが有効か
    inline constexpr bool IsValidDescriptorIndex(DescriptorIndex index)
    {
        return index != kInvalidDescriptorIndex;
    }

    //=========================================================================
    // MemorySize: メモリサイズ型
    //=========================================================================

    /// メモリサイズ（バイト単位）
    using MemorySize = uint64;

    /// メモリオフセット
    using MemoryOffset = uint64;

    /// メモリサイズ定数
    constexpr MemorySize kKilobyte = 1024;
    constexpr MemorySize kMegabyte = 1024 * kKilobyte;
    constexpr MemorySize kGigabyte = 1024 * kMegabyte;

    /// アライメントユーティリティ
    inline constexpr MemorySize AlignUp(MemorySize size, MemorySize alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    inline constexpr MemorySize AlignDown(MemorySize size, MemorySize alignment)
    {
        return size & ~(alignment - 1);
    }

    inline constexpr bool IsAligned(MemorySize size, MemorySize alignment)
    {
        return (size & (alignment - 1)) == 0;
    }
    //=========================================================================
    // Extent2D / Extent3D: サイズ型
    //=========================================================================

    /// 2D範囲（幅・高さ）
    struct Extent2D
    {
        uint32 width = 0;
        uint32 height = 0;

        constexpr Extent2D() = default;
        constexpr Extent2D(uint32 w, uint32 h) : width(w), height(h) {}

        constexpr bool IsEmpty() const { return width == 0 || height == 0; }
        constexpr uint64 Area() const { return static_cast<uint64>(width) * height; }

        constexpr bool operator==(const Extent2D& other) const
        {
            return width == other.width && height == other.height;
        }
        constexpr bool operator!=(const Extent2D& other) const { return !(*this == other); }
    };

    /// 3D範囲（幅・高さ・深度）
    struct Extent3D
    {
        uint32 width = 0;
        uint32 height = 0;
        uint32 depth = 1;

        constexpr Extent3D() = default;
        constexpr Extent3D(uint32 w, uint32 h, uint32 d = 1) : width(w), height(h), depth(d) {}

        /// 2Dから変換
        explicit constexpr Extent3D(const Extent2D& e2d)
            : width(e2d.width), height(e2d.height), depth(1)
        {
        }

        constexpr bool IsEmpty() const { return width == 0 || height == 0 || depth == 0; }
        constexpr uint64 Volume() const { return static_cast<uint64>(width) * height * depth; }

        /// 2Dとして取得
        constexpr Extent2D ToExtent2D() const { return Extent2D(width, height); }

        constexpr bool operator==(const Extent3D& other) const
        {
            return width == other.width && height == other.height && depth == other.depth;
        }
        constexpr bool operator!=(const Extent3D& other) const { return !(*this == other); }
    };

    //=========================================================================
    // Offset2D / Offset3D: オフセット型
    //=========================================================================

    /// 2Dオフセット
    struct Offset2D
    {
        int32 x = 0;
        int32 y = 0;

        constexpr Offset2D() = default;
        constexpr Offset2D(int32 inX, int32 inY) : x(inX), y(inY) {}
    };

    /// 3Dオフセット
    struct Offset3D
    {
        int32 x = 0;
        int32 y = 0;
        int32 z = 0;

        constexpr Offset3D() = default;
        constexpr Offset3D(int32 inX, int32 inY, int32 inZ = 0) : x(inX), y(inY), z(inZ) {}
    };

    //=========================================================================
    // RHIViewport: ビューポート
    //=========================================================================

    /// ビューポート定義
    /// 正規化されたデプス範囲 [minDepth, maxDepth]
    struct RHIViewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        constexpr RHIViewport() = default;

        constexpr RHIViewport(float inX, float inY, float inW, float inH,
                              float inMinD = 0.0f, float inMaxD = 1.0f)
            : x(inX), y(inY), width(inW), height(inH), minDepth(inMinD), maxDepth(inMaxD)
        {
        }

        /// Extent2Dから作成
        explicit constexpr RHIViewport(const Extent2D& extent)
            : x(0.0f)
            , y(0.0f)
            , width(static_cast<float>(extent.width))
            , height(static_cast<float>(extent.height))
            , minDepth(0.0f)
            , maxDepth(1.0f)
        {
        }

        constexpr bool IsEmpty() const { return width <= 0.0f || height <= 0.0f; }

        /// アスペクト比
        constexpr float GetAspectRatio() const { return (height > 0.0f) ? (width / height) : 0.0f; }

        constexpr bool operator==(const RHIViewport& other) const
        {
            return x == other.x && y == other.y && width == other.width && height == other.height
                   && minDepth == other.minDepth && maxDepth == other.maxDepth;
        }
        constexpr bool operator!=(const RHIViewport& other) const { return !(*this == other); }
    };

    //=========================================================================
    // RHIRect: 整数矩形
    //=========================================================================

    /// 整数矩形（シザー矩形等）
    struct RHIRect
    {
        int32 left = 0;
        int32 top = 0;
        int32 right = 0;
        int32 bottom = 0;

        constexpr RHIRect() = default;

        constexpr RHIRect(int32 l, int32 t, int32 r, int32 b) : left(l), top(t), right(r), bottom(b)
        {
        }

        /// オフセット + サイズから作成
        static constexpr RHIRect FromExtent(int32 x, int32 y, uint32 w, uint32 h)
        {
            return RHIRect(x, y, x + static_cast<int32>(w), y + static_cast<int32>(h));
        }

        /// Extent2Dから作成（原点0,0）
        explicit constexpr RHIRect(const Extent2D& extent)
            : left(0)
            , top(0)
            , right(static_cast<int32>(extent.width))
            , bottom(static_cast<int32>(extent.height))
        {
        }

        constexpr int32 Width() const { return right - left; }
        constexpr int32 Height() const { return bottom - top; }
        constexpr bool IsEmpty() const { return Width() <= 0 || Height() <= 0; }

        /// Extent2Dとして取得
        constexpr Extent2D ToExtent2D() const
        {
            return Extent2D(static_cast<uint32>(Width() > 0 ? Width() : 0),
                            static_cast<uint32>(Height() > 0 ? Height() : 0));
        }

        constexpr bool operator==(const RHIRect& other) const
        {
            return left == other.left && top == other.top && right == other.right
                   && bottom == other.bottom;
        }
        constexpr bool operator!=(const RHIRect& other) const { return !(*this == other); }
    };

    //=========================================================================
    // RHIBox: 3Dボックス
    //=========================================================================

    /// 3Dボックス（テクスチャコピー領域等）
    struct RHIBox
    {
        uint32 left = 0;
        uint32 top = 0;
        uint32 front = 0;
        uint32 right = 0;
        uint32 bottom = 0;
        uint32 back = 0;

        constexpr RHIBox() = default;

        constexpr RHIBox(uint32 l, uint32 t, uint32 f, uint32 r, uint32 b, uint32 bk)
            : left(l), top(t), front(f), right(r), bottom(b), back(bk)
        {
        }

        /// Extent3Dから作成（原点0,0,0）
        explicit constexpr RHIBox(const Extent3D& extent)
            : left(0)
            , top(0)
            , front(0)
            , right(extent.width)
            , bottom(extent.height)
            , back(extent.depth)
        {
        }

        constexpr uint32 Width() const { return right - left; }
        constexpr uint32 Height() const { return bottom - top; }
        constexpr uint32 Depth() const { return back - front; }

        constexpr bool IsEmpty() const { return Width() == 0 || Height() == 0 || Depth() == 0; }

        constexpr Extent3D ToExtent3D() const { return Extent3D(Width(), Height(), Depth()); }

        constexpr bool operator==(const RHIBox& other) const
        {
            return left == other.left && top == other.top && front == other.front
                   && right == other.right && bottom == other.bottom && back == other.back;
        }
        constexpr bool operator!=(const RHIBox& other) const { return !(*this == other); }
    };

    //=========================================================================
    // CPUディスクリプタハンドル
    //=========================================================================

    /// CPUディスクリプタハンドル
    /// CPU側でのディスクリプタ参照（ステージング用）
    struct RHICPUDescriptorHandle
    {
        size_t ptr = 0;

        constexpr RHICPUDescriptorHandle() = default;
        explicit constexpr RHICPUDescriptorHandle(size_t p) : ptr(p) {}

        constexpr bool IsValid() const { return ptr != 0; }
        constexpr bool IsNull() const { return ptr == 0; }

        /// オフセット追加
        RHICPUDescriptorHandle Offset(uint32 count, uint32 incrementSize) const
        {
            return RHICPUDescriptorHandle(ptr + count * incrementSize);
        }

        constexpr bool operator==(const RHICPUDescriptorHandle& other) const
        {
            return ptr == other.ptr;
        }
        constexpr bool operator!=(const RHICPUDescriptorHandle& other) const
        {
            return ptr != other.ptr;
        }
    };

    //=========================================================================
    // GPUディスクリプタハンドル
    //=========================================================================

    /// GPUディスクリプタハンドル
    /// シェーダーからアクセス可能なディスクリプタ参照
    struct RHIGPUDescriptorHandle
    {
        uint64 ptr = 0;

        constexpr RHIGPUDescriptorHandle() = default;
        explicit constexpr RHIGPUDescriptorHandle(uint64 p) : ptr(p) {}

        constexpr bool IsValid() const { return ptr != 0; }
        constexpr bool IsNull() const { return ptr == 0; }

        /// オフセット追加
        RHIGPUDescriptorHandle Offset(uint32 count, uint32 incrementSize) const
        {
            return RHIGPUDescriptorHandle(ptr + count * incrementSize);
        }

        constexpr bool operator==(const RHIGPUDescriptorHandle& other) const
        {
            return ptr == other.ptr;
        }
        constexpr bool operator!=(const RHIGPUDescriptorHandle& other) const
        {
            return ptr != other.ptr;
        }
    };

    //=========================================================================
    // 統合デスクリプタハンドル
    //=========================================================================

    /// 統合デスクリプタハンドル
    /// CPU/GPUハンドルのペアとメタデータ
    struct RHIDescriptorHandle
    {
        RHICPUDescriptorHandle cpu;
        RHIGPUDescriptorHandle gpu; // GPUVisible時のみ有効
        uint32 heapIndex = 0;       // 所属ヒープインデックス
        uint32 offsetInHeap = 0;    // ヒープ内オフセット

        constexpr RHIDescriptorHandle() = default;

        /// CPUのみのハンドル
        static RHIDescriptorHandle CPUOnly(RHICPUDescriptorHandle cpuHandle)
        {
            RHIDescriptorHandle h;
            h.cpu = cpuHandle;
            return h;
        }

        /// CPU+GPUハンドル
        static RHIDescriptorHandle CPUAndGPU(RHICPUDescriptorHandle cpuHandle,
                                             RHIGPUDescriptorHandle gpuHandle)
        {
            RHIDescriptorHandle h;
            h.cpu = cpuHandle;
            h.gpu = gpuHandle;
            return h;
        }

        bool IsValid() const { return cpu.IsValid(); }
        bool IsGPUVisible() const { return gpu.IsValid(); }
    };

    //=========================================================================
    // Bindlessインデックス
    //=========================================================================

    /// Bindlessリソースインデックス
    /// シェーダー内部のリソース参照用
    struct BindlessIndex
    {
        uint32 index = kInvalidDescriptorIndex;

        constexpr BindlessIndex() = default;
        explicit constexpr BindlessIndex(uint32 i) : index(i) {}

        constexpr bool IsValid() const { return index != kInvalidDescriptorIndex; }

        /// シェーダーに渡す値として取得
        constexpr uint32 GetShaderIndex() const { return index; }
    };

    /// Bindless SRVインデックス
    struct BindlessSRVIndex : BindlessIndex
    {
        using BindlessIndex::BindlessIndex;
    };

    /// Bindless UAVインデックス
    struct BindlessUAVIndex : BindlessIndex
    {
        using BindlessIndex::BindlessIndex;
    };

    /// Bindless Samplerインデックス
    struct BindlessSamplerIndex : BindlessIndex
    {
        using BindlessIndex::BindlessIndex;
    };

    //=========================================================================
    // RHI制限定数
    //=========================================================================

    // --- レンダリング制限 ---

    /// 最大同時レンダーターゲット数
    constexpr uint32 kMaxRenderTargets = 8;

    /// 最大頂点ストリーム数
    constexpr uint32 kMaxVertexStreams = 16;

    /// 最大頂点要素数
    constexpr uint32 kMaxVertexElements = 16;

    /// 最大ビューポート・シザー数
    constexpr uint32 kMaxViewports = 16;

    // --- テクスチャ制限 ---

    /// 最大MIPレベル数
    constexpr uint32 kMaxTextureMipCount = 15;

    /// 最大配列サイズ
    constexpr uint32 kMaxTextureArraySize = 2048;

    /// 最大キューブマップ配列サイズ
    constexpr uint32 kMaxCubeArraySize = 2048;

    // --- ディスクリプタ制限 ---

    /// 最大サンプラー数
    constexpr uint32 kMaxSamplerCount = 2048;

    /// 最大CBV/SRV/UAV数（Bindless用）
    constexpr uint32 kMaxCBVSRVUAVCount = 1000000;

    /// オフラインディスクリプタヒープサイズ
    constexpr uint32 kOfflineDescriptorHeapSize = 4096;

    // --- アライメント要件 ---

    /// 定数バッファアライメント（D3D12: 256バイト）
    constexpr uint32 kConstantBufferAlignment = 256;

    /// テクスチャデータアライメント（D3D12: 512バイト）
    constexpr uint32 kTextureDataAlignment = 512;

    /// バッファアライメント
    constexpr uint32 kBufferAlignment = 16;

    // --- フレーム制限 ---

    /// 最大フレームインフライト数
    constexpr uint32 kMaxFramesInFlight = 3;

    /// デフォルトバックバッファ数
    constexpr uint32 kDefaultBackBufferCount = 2;

}} // namespace NS::RHI
