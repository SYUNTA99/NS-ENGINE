# 11-01: ヒープタイプ

## 目的

GPUメモリヒープのタイプと特性を定義する。

## 参照ドキュメント

- 01-02-types-gpu.md (MemorySize)
- 03-01-buffer-desc.md (ERHIBufferMemoryType)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIMemoryTypes.h`

## TODO

### 1. メモリヒープタイプ

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// メモリヒープタイプ
    enum class ERHIHeapType : uint8
    {
        /// デフォルト（VRAM）。
        /// GPU専用、最高帯域幅
        Default,

        /// アップロード（CPU→GPU）。
        /// CPU書き込み可能、Write-Combined
        Upload,

        /// リードバック（GPU→CPU）。
        /// CPU読み取り可能
        Readback,

        /// カスタム
        /// 細かい制御用
        Custom,
    };

    /// メモリプールタイプ
    enum class ERHIMemoryPool : uint8
    {
        /// Unknown/デフォルト
        Unknown,

        /// L0（システムメモリ）。
        /// CPU側のシステムRAM
        L0,

        /// L1（RAM）。
        /// GPU専用ビデオメモリ
        L1,
    };

    /// CPUページプロパティ
    enum class ERHICPUPageProperty : uint8
    {
        Unknown,
        NotAvailable,   // CPUアクセス不可
        WriteCombine,   // 書き込み結合
        WriteBack,      // ライトバック
    };
}
```

- [ ] ERHIHeapType 列挙型
- [ ] ERHIMemoryPool 列挙型
- [ ] ERHICPUPageProperty 列挙型

### 2. ヒープ記述

```cpp
namespace NS::RHI
{
    /// ヒープフラグ
    enum class ERHIHeapFlags : uint32
    {
        None = 0,

        /// シェーダー可視
        ShaderVisible = 1 << 0,

        /// バッファ用
        AllowOnlyBuffers = 1 << 1,

        /// 非RTテクスチャ用
        AllowOnlyNonRTDSTextures = 1 << 2,

        /// RTテクスチャ用
        AllowOnlyRTDSTextures = 1 << 3,

        /// MSAA用
        DenyMSAATextures = 1 << 4,

        /// レイトレーシング加速構造用
        AllowRaytracingAccelerationStructure = 1 << 5,

        /// 常駐管理対象
        CreateNotResident = 1 << 6,

        /// マルチアダプター共有
        SharedCrossAdapter = 1 << 7,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIHeapFlags)

    /// ヒープ記述
    struct RHI_API RHIHeapDesc
    {
        /// サイズ（バイト）
        uint64 sizeInBytes = 0;

        /// ヒープタイプ
        ERHIHeapType type = ERHIHeapType::Default;

        /// メモリプール（Customタイプ時）。
        ERHIMemoryPool memoryPool = ERHIMemoryPool::Unknown;

        /// CPUページプロパティ（Customタイプ時）。
        ERHICPUPageProperty cpuPageProperty = ERHICPUPageProperty::Unknown;

        /// フラグ
        ERHIHeapFlags flags = ERHIHeapFlags::None;

        /// アライメント（0でデフォルト）
        uint64 alignment = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// デフォルトヒープ
        static RHIHeapDesc Default(uint64 size, ERHIHeapFlags f = ERHIHeapFlags::None) {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Default;
            desc.flags = f;
            return desc;
        }

        /// アップロードヒープ
        static RHIHeapDesc Upload(uint64 size) {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Upload;
            return desc;
        }

        /// リードバックヒープ
        static RHIHeapDesc Readback(uint64 size) {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Readback;
            return desc;
        }

        /// バッファ専用ヒープ
        static RHIHeapDesc BufferHeap(uint64 size, ERHIHeapType type = ERHIHeapType::Default) {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = type;
            desc.flags = ERHIHeapFlags::AllowOnlyBuffers;
            return desc;
        }

        /// テクスチャ専用ヒープ
        static RHIHeapDesc TextureHeap(uint64 size, bool allowRT = false) {
            RHIHeapDesc desc;
            desc.sizeInBytes = size;
            desc.type = ERHIHeapType::Default;
            desc.flags = allowRT ? ERHIHeapFlags::AllowOnlyRTDSTextures
                                 : ERHIHeapFlags::AllowOnlyNonRTDSTextures;
            return desc;
        }
    };
}
```

- [ ] ERHIHeapFlags 列挙型
- [ ] RHIHeapDesc 構造体

### 3. IRHIHeap インターフェース

```cpp
namespace NS::RHI
{
    /// GPUメモリヒープ
    class RHI_API IRHIHeap : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Heap)

        virtual ~IRHIHeap() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// サイズ取得
        virtual uint64 GetSize() const = 0;

        /// ヒープタイプ取得
        virtual ERHIHeapType GetType() const = 0;

        /// フラグ取得
        virtual ERHIHeapFlags GetFlags() const = 0;

        /// アライメント取得
        virtual uint64 GetAlignment() const = 0;

        //=====================================================================
        // 状態
        //=====================================================================

        /// 常駐していか
        virtual bool IsResident() const = 0;

        /// GPUアドレス取得（バッファヒープ用）。
        virtual uint64 GetGPUVirtualAddress() const = 0;
    };

    using RHIHeapRef = TRefCountPtr<IRHIHeap>;
}
```

- [ ] IRHIHeap インターフェース

### 4. ヒープ作成

```cpp
namespace NS::RHI
{
    /// ヒープ作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// ヒープ作成
        virtual IRHIHeap* CreateHeap(
            const RHIHeapDesc& desc,
            const char* debugName = nullptr) = 0;

        //=====================================================================
        // メモリ情報クエリ
        //=====================================================================

        /// 利用可能VRAM取得
        virtual uint64 GetAvailableVideoMemory() const = 0;

        /// 合計RAM取得
        virtual uint64 GetTotalVideoMemory() const = 0;

        /// 利用可能システムメモリ取得
        virtual uint64 GetAvailableSystemMemory() const = 0;

        /// メモリ予算情報
        struct MemoryBudgetInfo {
            uint64 budget;              // 予算
            uint64 currentUsage;        // 現在使用量
            uint64 availableForReservation;  // 予約可能量
            uint64 currentReservation;  // 現在予約量
        };

        /// メモリ予算取得
        virtual MemoryBudgetInfo GetMemoryBudget(ERHIMemoryPool pool) const = 0;
    };
}
```

- [ ] CreateHeap
- [ ] メモリ情報クエリ

### 5. リソースのヒープ配置

```cpp
namespace NS::RHI
{
    /// リソース配置情報
    struct RHI_API RHIResourceAllocationInfo
    {
        /// 必要サイズ
        uint64 sizeInBytes = 0;

        /// アライメント
        uint64 alignment = 0;
    };

    /// 配置リソース作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// リソース配置情報を取得
        virtual RHIResourceAllocationInfo GetResourceAllocationInfo(
            const RHIBufferDesc& desc) const = 0;

        virtual RHIResourceAllocationInfo GetResourceAllocationInfo(
            const RHITextureDesc& desc) const = 0;

        /// 配置バッファ作成
        /// @param heap 配置先ヒープ
        /// @param heapOffset ヒープのオフセット
        /// @param desc バッファ記述
        virtual IRHIBuffer* CreatePlacedBuffer(
            IRHIHeap* heap,
            uint64 heapOffset,
            const RHIBufferDesc& desc,
            const char* debugName = nullptr) = 0;

        /// 配置テクスチャ作成
        virtual IRHITexture* CreatePlacedTexture(
            IRHIHeap* heap,
            uint64 heapOffset,
            const RHITextureDesc& desc,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] RHIResourceAllocationInfo 構造体
- [ ] CreatePlacedBuffer / CreatePlacedTexture

### 6. エイリアシングバリア

```cpp
namespace NS::RHI
{
    /// エイリアシングバリア
    /// 同一メモリ領域を使用する異なるリソース間の遷移
    struct RHI_API RHIAliasingBarrier
    {
        /// 前リソース（nullで未使用）。
        IRHIResource* resourceBefore = nullptr;

        /// 後リソース（nullで未使用）。
        IRHIResource* resourceAfter = nullptr;
    };

    /// エイリアシングバリア（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// エイリアシングバリア挿入
        virtual void AliasingBarrier(
            IRHIResource* resourceBefore,
            IRHIResource* resourceAfter) = 0;

        /// 複数エイリアシングバリア挿入
        virtual void AliasingBarriers(
            const RHIAliasingBarrier* barriers,
            uint32 count) = 0;
    };
}
```

- [ ] RHIAliasingBarrier 構造体
- [ ] AliasingBarrier / AliasingBarriers

## 検証方法

- [ ] ヒープ作成/破棄
- [ ] 配置リソース作成
- [ ] メモリ情報を取得
- [ ] エイリアシングバリア
