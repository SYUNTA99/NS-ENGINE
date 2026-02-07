# 03-02: IRHIBufferインターフェース

## 目的

バッファリソースのコアインターフェースを定義する。

## 参照ドキュメント

- 03-01-buffer-desc.md (RHIBufferDesc)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIBuffer.h` (部分)

## TODO

### 1. IRHIBuffer: 基本インターフェース

```cpp
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHITypes.h"

namespace NS::RHI
{
    /// バッファリソース
    /// GPU上のリニアメモリ領域
    class RHI_API IRHIBuffer : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Buffer)

        virtual ~IRHIBuffer() = default;

        //=====================================================================
        // ライフサイクル契約
        //=====================================================================
        //
        // - IRHIBufferはTRefCountPtrで管理される
        // - 参照カウントが0になるとOnZeroRefCount()が呼ばれ、
        //   RHIDeferredDeleteQueue::Enqueue()によりGPU完了まで削除を遅延する
        // - GPU操作が完了するまでバッファメモリは解放されない
        // - Lock/Unlockはスレッドセーフではない。
        //   複数スレッドからの同時アクセスには外部同期が必要
        //

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// バッファサイズ取得（バイト）
        virtual MemorySize GetSize() const = 0;

        /// 使用フラグ取得
        virtual ERHIBufferUsage GetUsage() const = 0;

        /// 要素ストライド取得（構造化バッファ用）
        virtual uint32 GetStride() const = 0;

        /// 要素数取得（構造化バッファ用）
        /// GetSize() / GetStride()
        uint32 GetElementCount() const {
            uint32 stride = GetStride();
            return stride > 0 ? static_cast<uint32>(GetSize() / stride) : 0;
        }

        //=====================================================================
        // GPUアドレス
        //=====================================================================

        /// GPUバーチャルアドレス取得
        /// シェーダーバインディング用
        virtual uint64 GetGPUVirtualAddress() const = 0;

        /// GPU上の有効アドレス範囲取得
        /// @param outAddress 開始アドレス
        /// @param outSize 有効サイズ
        virtual void GetGPUAddressRange(uint64& outAddress, MemorySize& outSize) const {
            outAddress = GetGPUVirtualAddress();
            outSize = GetSize();
        }
    };

    /// バッファ参照カウントポインタ
    using RHIBufferRef = TRefCountPtr<IRHIBuffer>;
}
```

- [ ] IRHIBuffer 基本インターフェース
- [ ] サイズ・ストライド取得
- [ ] GPUアドレス取得

### 2. 使用フラグ判定メソッド

```cpp
namespace NS::RHI
{
    class IRHIBuffer
    {
    public:
        //=====================================================================
        // 使用フラグ判定
        //=====================================================================

        /// 頂点バッファとして使用可能か
        bool IsVertexBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::VertexBuffer);
        }

        /// インデックスバッファとして使用可能か
        bool IsIndexBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::IndexBuffer);
        }

        /// 定数バッファとして使用可能か
        bool IsConstantBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ConstantBuffer);
        }

        /// SRVとして使用可能か
        bool CanCreateSRV() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ShaderResource);
        }

        /// UAVとして使用可能か
        bool CanCreateUAV() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::UnorderedAccess);
        }

        /// 構造化バッファか
        bool IsStructuredBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::StructuredBuffer);
        }

        /// バイトアドレスバッファか
        bool IsByteAddressBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ByteAddressBuffer);
        }

        /// 間接引数バッファか
        bool IsIndirectArgsBuffer() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::IndirectArgs);
        }

        /// 動的バッファか
        bool IsDynamic() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::Dynamic);
        }

        /// CPUから書き込み可能か
        bool IsCPUWritable() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::CPUWritable);
        }

        /// CPUから読み取り可能か
        bool IsCPUReadable() const {
            return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::CPUReadable);
        }
    };
}
```

- [ ] 使用フラグ判定メソッド群

### 3. メモリ情報

```cpp
namespace NS::RHI
{
    /// バッファメモリ情報
    struct RHI_API RHIBufferMemoryInfo
    {
        /// 実際に割り当てられたサイズ（アライメント等で要求より大きい場合あり）
        MemorySize allocatedSize = 0;

        /// 使用可能サイズ（要求サイズ）
        MemorySize usableSize = 0;

        /// ヒープ内オフセット
        MemoryOffset heapOffset = 0;

        /// メモリタイプ
        ERHIHeapType heapType = ERHIHeapType::Default;

        /// アライメント
        uint32 alignment = 0;
    };

    class IRHIBuffer
    {
    public:
        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// メモリ情報取得
        virtual RHIBufferMemoryInfo GetMemoryInfo() const = 0;

        /// 実際に割り当てられたサイズ取得
        MemorySize GetAllocatedSize() const {
            return GetMemoryInfo().allocatedSize;
        }

        /// ヒープタイプ取得
        ERHIHeapType GetHeapType() const {
            return GetMemoryInfo().heapType;
        }
    };
}
```

- [ ] RHIBufferMemoryInfo 構造体
- [ ] GetMemoryInfo / GetAllocatedSize

### 4. ビュー作成情報取得

```cpp
namespace NS::RHI
{
    /// バッファビュー作成情報
    struct RHI_API RHIBufferViewInfo
    {
        /// バッファ
        IRHIBuffer* buffer = nullptr;

        /// 開始オフセット（バイト）
        MemoryOffset offset = 0;

        /// サイズ（バイト、0 = バッファ全体）
        MemorySize size = 0;

        /// フォーマット（型付きバッファ用）
        EPixelFormat format = EPixelFormat::Unknown;

        /// 要素数取得
        uint32 GetElementCount() const {
            if (!buffer) return 0;
            uint32 stride = buffer->GetStride();
            MemorySize effectiveSize = (size > 0) ? size : (buffer->GetSize() - offset);
            return stride > 0 ? static_cast<uint32>(effectiveSize / stride) : 0;
        }
    };

    class IRHIBuffer
    {
    public:
        //=====================================================================
        // ビュー作成用
        //=====================================================================

        /// 全体をカバーするビュー情報取得
        RHIBufferViewInfo GetFullViewInfo() const {
            RHIBufferViewInfo info;
            info.buffer = const_cast<IRHIBuffer*>(this);
            info.offset = 0;
            info.size = GetSize();
            return info;
        }

        /// 部分ビュー情報取得
        RHIBufferViewInfo GetSubViewInfo(MemoryOffset offset, MemorySize size) const {
            RHIBufferViewInfo info;
            info.buffer = const_cast<IRHIBuffer*>(this);
            info.offset = offset;
            info.size = size;
            return info;
        }
    };
}
```

- [ ] RHIBufferViewInfo 構造体
- [ ] GetFullViewInfo / GetSubViewInfo

### 5. 頂点/インデックスバッファビュー

```cpp
namespace NS::RHI
{
    class IRHIBuffer
    {
    public:
        //=====================================================================
        // 頂点/インデックスバッファ用
        //=====================================================================

        /// 頂点バッファビュー取得
        /// @param offset 開始オフセット（バイト）
        /// @param size サイズ（0 = バッファ全体）
        /// @param stride 頂点ストライド（0 = 構造化バッファのストライドを使用）
        RHIVertexBufferView GetVertexBufferView(
            MemoryOffset offset = 0,
            MemorySize size = 0,
            uint32 stride = 0) const
        {
            RHIVertexBufferView view;
            view.bufferAddress = GetGPUVirtualAddress() + offset;
            view.size = static_cast<uint32>((size > 0) ? size : (GetSize() - offset));
            view.stride = (stride > 0) ? stride : GetStride();
            return view;
        }

        /// インデックスバッファビュー取得
        /// @param format インデックスフォーマット
        /// @param offset 開始オフセット（バイト）
        /// @param size サイズ（0 = バッファ全体）
        RHIIndexBufferView GetIndexBufferView(
            ERHIIndexFormat format,
            MemoryOffset offset = 0,
            MemorySize size = 0) const
        {
            RHIIndexBufferView view;
            view.bufferAddress = GetGPUVirtualAddress() + offset;
            view.size = static_cast<uint32>((size > 0) ? size : (GetSize() - offset));
            view.format = format;
            return view;
        }
    };
}
```

- [ ] GetVertexBufferView
- [ ] GetIndexBufferView

## プラットフォーム実装ノート

| API | 内部型 | GPUアドレス取得 | 特記事項 |
|-----|--------|----------------|----------|
| D3D12 | ID3D12Resource | GetGPUVirtualAddress() | CBVは256バイトアライン必須 |
| Vulkan | VkBuffer + VkDeviceMemory | vkGetBufferDeviceAddress | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT必要 |
| Metal | MTLBuffer | gpuAddress (iOS 16+) | macOSは非サポート、SRVで代用 |

### D3D12実装例

```cpp
class D3D12Buffer : public IRHIBuffer
{
    uint64 GetGPUVirtualAddress() const override
    {
        return m_resource->GetGPUVirtualAddress();
    }

private:
    ComPtr<ID3D12Resource> m_resource;
    D3D12_HEAP_TYPE m_heapType;
};
```

### Vulkan実装例

```cpp
class VulkanBuffer : public IRHIBuffer
{
    uint64 GetGPUVirtualAddress() const override
    {
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = m_buffer;
        return vkGetBufferDeviceAddress(m_device, &info);
    }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};
```

## エラー処理

| 操作 | 想定エラー | 回復戦略 |
|------|-----------|---------:|
| CreateBuffer | OutOfVideoMemory | 低優先リソース解放後リトライ |
| CreateBuffer | InvalidSize | 0バイトは拒否、最大サイズチェック |
| Map | MapFailed | Upload/Readbackヒープ以外は拒否 |
| GetGPUVirtualAddress | DeviceLost | 0を返却、呼び出し元で検知 |

## テストケース

```cpp
TEST(BufferTest, CreateBuffer_ValidDesc_Succeeds)
{
    RHIBufferDesc desc{1024, ERHIBufferUsage::ConstantBuffer};
    auto buffer = device->CreateBuffer(desc, "TestCB");
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->GetSize(), 1024);
}

TEST(BufferTest, CreateBuffer_ZeroSize_ReturnsNull)
{
    RHIBufferDesc desc{0, ERHIBufferUsage::VertexBuffer};
    auto buffer = device->CreateBuffer(desc, "ZeroBuffer");
    EXPECT_EQ(buffer, nullptr);
}

TEST(BufferTest, GetGPUVirtualAddress_ValidBuffer_ReturnsNonZero)
{
    auto buffer = CreateTestBuffer(device, 1024);
    EXPECT_NE(buffer->GetGPUVirtualAddress(), 0);
}
```

## 検証方法

- [ ] インターフェースの完全性
- [ ] 使用フラグ判定の正確性
- [ ] ビュー作成情報の整合性
- [ ] 各プラットフォームでのGPUアドレス取得
- [ ] エラー処理の動作確認
