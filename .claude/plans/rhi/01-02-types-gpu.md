# 01-02: GPU識別型・リソース識別型

## 目的

GPU識別とリソース識別に使用する基本型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (Device, Adapter)
- docs/RHI/RHI_Implementation_Guide_Part4.md (マルチGPU)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHITypes.h` (部分)

## TODO

### 1. GPUMask: マルチGPU識別

```cpp
#pragma once

#include "Common/Utility/Types.h"
#include "RHIMacros.h"

namespace NS::RHI
{
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
        static constexpr GPUMask FromIndex(uint32 index) {
            return GPUMask(1u << index);
        }

        /// GPU0のみ（デフォルト）
        static constexpr GPUMask GPU0() { return GPUMask(1); }

        /// 指定GPUを含むか
        constexpr bool Contains(uint32 index) const {
            return (mask & (1u << index)) != 0;
        }

        /// 最初のGPUインデックス取得
        uint32 GetFirstIndex() const;

        /// 有効GPU数
        uint32 CountBits() const;

        /// 空か
        constexpr bool IsEmpty() const { return mask == 0; }

        /// 単一GPUか
        bool IsSingleGPU() const { return CountBits() == 1; }

        /// 演算子
        constexpr GPUMask operator|(GPUMask other) const {
            return GPUMask(mask | other.mask);
        }
        constexpr GPUMask operator&(GPUMask other) const {
            return GPUMask(mask & other.mask);
        }
        GPUMask& operator|=(GPUMask other) {
            mask |= other.mask;
            return *this;
        }
        constexpr bool operator==(GPUMask other) const {
            return mask == other.mask;
        }
        constexpr bool operator!=(GPUMask other) const {
            return mask != other.mask;
        }
    };

    /// GPUインデックス型
    using GPUIndex = uint32;

    /// 無効GPUインデックス
    constexpr GPUIndex kInvalidGPUIndex = ~0u;
}
```

- [ ] GPUMask 構造体
- [ ] GPUIndex 型エイリアス
- [ ] GPUMask演算子

### 2. ResourceId: リソース識別

```cpp
namespace NS::RHI
{
    /// リソース一意識別子
    /// 内部でのリソース追跡・デバッグ用
    using ResourceId = uint64;

    /// 無効リソースID
    constexpr ResourceId kInvalidResourceId = 0;

    /// リソースID生成
    /// スレッドセーフなアトミックカウンター
    ResourceId GenerateResourceId();
}
```

- [ ] ResourceId 型
- [ ] kInvalidResourceId 定数
- [ ] GenerateResourceId() 関数宣言

### 3. DescriptorIndex: ディスクリプタ識別

```cpp
namespace NS::RHI
{
    /// ディスクリプタヒープ内インデックス
    using DescriptorIndex = uint32;

    /// 無効ディスクリプタインデックス
    constexpr DescriptorIndex kInvalidDescriptorIndex = ~0u;

    /// ディスクリプタインデックスが有効か
    inline constexpr bool IsValidDescriptorIndex(DescriptorIndex index) {
        return index != kInvalidDescriptorIndex;
    }
}
```

- [ ] DescriptorIndex 型
- [ ] kInvalidDescriptorIndex 定数

### 4. MemorySize: メモリサイズ型

```cpp
namespace NS::RHI
{
    /// メモリサイズ（バイト単位）
    using MemorySize = uint64;

    /// メモリオフセット
    using MemoryOffset = uint64;

    /// メモリサイズ定数
    constexpr MemorySize kKilobyte = 1024;
    constexpr MemorySize kMegabyte = 1024 * kKilobyte;
    constexpr MemorySize kGigabyte = 1024 * kMegabyte;

    /// アライメントユーティリティ
    inline constexpr MemorySize AlignUp(MemorySize size, MemorySize alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    inline constexpr MemorySize AlignDown(MemorySize size, MemorySize alignment) {
        return size & ~(alignment - 1);
    }

    inline constexpr bool IsAligned(MemorySize size, MemorySize alignment) {
        return (size & (alignment - 1)) == 0;
    }
}
```

- [ ] MemorySize / MemoryOffset 型
- [ ] サイズ定数
- [ ] アライメントユーティリティ

## 検証方法

- [ ] GPUMask演算の正確性
- [ ] アライメント計算の正確性
- [ ] 型サイズの検証（static_assert）
