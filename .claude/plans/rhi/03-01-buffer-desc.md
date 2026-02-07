# 03-01: バッファ記述情報

## 目的

バッファ作成に必要な記述構造体を定義する。

## 参照ドキュメント

- 01-08-enums-buffer.md (ERHIBufferUsage, ERHIMapMode)
- 01-02-types-gpu.md (MemorySize, GPUMask)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIBuffer.h` (部分

## TODO

### 1. RHIBufferDesc: 基本記述構造体

```cpp
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHITypes.h"

namespace NS::RHI
{
    /// バッファ作成記述
    struct RHI_API RHIBufferDesc
    {
        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// バッファサイズ（バイト）
        MemorySize size = 0;

        /// 使用フラグ
        ERHIBufferUsage usage = ERHIBufferUsage::Default;

        //=====================================================================
        // 構造化バッファ用
        //=====================================================================

        /// 要素ストライド（バイト）
        /// StructuredBuffer使用時に必要。
        /// 0 = 非構造化
        uint32 stride = 0;

        //=====================================================================
        // メモリ配置
        //=====================================================================

        /// メモリアライメント要求（ = デフォルト）
        uint32 alignment = 0;

        //=====================================================================
        // マルチGPU
        //=====================================================================

        /// 対象GPU
        GPUMask gpuMask = GPUMask::GPU0();

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名
        const char* debugName = nullptr;

        //=====================================================================
        // ビルダーパターン
        //=====================================================================

        RHIBufferDesc& SetSize(MemorySize s) { size = s; return *this; }
        RHIBufferDesc& SetUsage(ERHIBufferUsage u) { usage = u; return *this; }
        RHIBufferDesc& SetStride(uint32 s) { stride = s; return *this; }
        RHIBufferDesc& SetAlignment(uint32 a) { alignment = a; return *this; }
        RHIBufferDesc& SetGPUMask(GPUMask m) { gpuMask = m; return *this; }
        RHIBufferDesc& SetDebugName(const char* name) { debugName = name; return *this; }
    };
}
```

- [ ] RHIBufferDesc 基本構造体
- [ ] ビルダーパターンメソッド

### 2. バッファ作成ヘルパー

```cpp
namespace NS::RHI
{
    /// 頂点バッファDesc作成
    inline RHIBufferDesc CreateVertexBufferDesc(
        MemorySize size,
        uint32 stride,
        bool dynamic = false)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage = dynamic
            ? ERHIBufferUsage::DynamicVertexBuffer
            : ERHIBufferUsage::VertexBuffer;
        return desc;
    }

    /// インデックスバッファDesc作成
    inline RHIBufferDesc CreateIndexBufferDesc(
        MemorySize size,
        ERHIIndexFormat format = ERHIIndexFormat::UInt16,
        bool dynamic = false)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = GetIndexFormatSize(format);
        desc.usage = dynamic
            ? ERHIBufferUsage::DynamicIndexBuffer
            : ERHIBufferUsage::IndexBuffer;
        return desc;
    }

    /// 定数バッファDesc作成
    inline RHIBufferDesc CreateConstantBufferDesc(
        MemorySize size,
        bool dynamic = true)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = dynamic
            ? ERHIBufferUsage::DynamicConstantBuffer
            : ERHIBufferUsage::ConstantBuffer;
        // 定数バッファは256バイトアライメント要求
        desc.alignment = 256;
        return desc;
    }

    /// 構造化バッファDesc作成
    inline RHIBufferDesc CreateStructuredBufferDesc(
        MemorySize size,
        uint32 stride,
        ERHIBufferUsage additionalUsage = ERHIBufferUsage::None)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage = ERHIBufferUsage::StructuredBuffer |
                     ERHIBufferUsage::ShaderResource |
                     additionalUsage;
        return desc;
    }

    /// UAV付き構造化バッファDesc作成
    inline RHIBufferDesc CreateRWStructuredBufferDesc(
        MemorySize size,
        uint32 stride)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage = ERHIBufferUsage::StructuredBuffer |
                     ERHIBufferUsage::ShaderResource |
                     ERHIBufferUsage::UnorderedAccess;
        return desc;
    }

    /// ステージングバッファDesc作成
    inline RHIBufferDesc CreateStagingBufferDesc(MemorySize size)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = ERHIBufferUsage::Staging;
        return desc;
    }

    /// 間接引数バッファDesc作成
    inline RHIBufferDesc CreateIndirectArgsBufferDesc(MemorySize size)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = ERHIBufferUsage::IndirectArgs |
                     ERHIBufferUsage::ShaderResource |
                     ERHIBufferUsage::UnorderedAccess;
        return desc;
    }
}
```

- [ ] CreateVertexBufferDesc
- [ ] CreateIndexBufferDesc
- [ ] CreateConstantBufferDesc
- [ ] CreateStructuredBufferDesc / CreateRWStructuredBufferDesc
- [ ] CreateStagingBufferDesc
- [ ] CreateIndirectArgsBufferDesc

### 3. バッファ初期データ

```cpp
namespace NS::RHI
{
    /// バッファ初期化データ
    struct RHI_API RHIBufferInitData
    {
        /// データポインタ
        const void* data = nullptr;

        /// データサイズ（バイト）
        /// 0 = バッファ全作
        MemorySize size = 0;

        /// バッファ内のオフセット
        MemoryOffset offset = 0;
    };

    /// 作成と同時に初期化するバッファDesc
    struct RHI_API RHIBufferCreateInfo
    {
        RHIBufferDesc desc;
        RHIBufferInitData initData;

        /// 初期データ設定
        RHIBufferCreateInfo& SetInitData(const void* data, MemorySize size = 0) {
            initData.data = data;
            initData.size = size;
            return *this;
        }

        /// テンプレート版: 配列から初期化
        template<typename T, size_t N>
        RHIBufferCreateInfo& SetInitData(const T (&array)[N]) {
            initData.data = array;
            initData.size = sizeof(T) * N;
            return *this;
        }

        /// テンプレート版: std::vector等から初期化
        template<typename Container>
        RHIBufferCreateInfo& SetInitDataFromContainer(const Container& container) {
            initData.data = container.data();
            initData.size = container.size() * sizeof(typename Container::value_type);
            return *this;
        }
    };
}
```

- [ ] RHIBufferInitData 構造体
- [ ] RHIBufferCreateInfo 構造体
- [ ] テンプレート初期化ヘルパー

### 4. 定数バッファアライメント

```cpp
namespace NS::RHI
{
    /// 定数バッファアライメント要件
    constexpr uint32 kConstantBufferAlignment = 256;

    /// 定数バッファサイズをアライメント
    inline constexpr MemorySize AlignConstantBufferSize(MemorySize size) {
        return AlignUp(size, kConstantBufferAlignment);
    }

    /// 構造体サイズから定数バッファサイズを計算
    template<typename T>
    inline constexpr MemorySize GetConstantBufferSize() {
        return AlignConstantBufferSize(sizeof(T));
    }

    /// 配列の定数バッファサイズを計算
    template<typename T>
    inline constexpr MemorySize GetConstantBufferArraySize(uint32 count) {
        // 各要素もアライメントが必要。
        return AlignConstantBufferSize(sizeof(T)) * count;
    }
}
```

- [ ] kConstantBufferAlignment 定数
- [ ] AlignConstantBufferSize
- [ ] GetConstantBufferSize / GetConstantBufferArraySize

## 検証方法

- [ ] Desc構造体のサイズ・アライメント確認
- [ ] ヘルパー関数の出力が正しいusageフラグを持つか
- [ ] アライメント計算の正確性
