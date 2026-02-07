# 01-04: ディスクリプタハンドル・定数

## 目的

ディスクリプタハンドル型とRHI制限定数を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part3.md (ディスクリプタ管理)
- docs/RHI/RHI_Implementation_Guide_Part10.md (Bindless)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHITypes.h` (部分)

## TODO

### 1. CPUディスクリプタハンドル

```cpp
namespace NS::RHI
{
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
        RHICPUDescriptorHandle Offset(uint32 count, uint32 incrementSize) const {
            return RHICPUDescriptorHandle(ptr + count * incrementSize);
        }

        constexpr bool operator==(const RHICPUDescriptorHandle& other) const {
            return ptr == other.ptr;
        }
        constexpr bool operator!=(const RHICPUDescriptorHandle& other) const {
            return ptr != other.ptr;
        }
    };
}
```

- [ ] RHICPUDescriptorHandle 構造体
- [ ] Offset計算メソッド

### 2. GPUディスクリプタハンドル

```cpp
namespace NS::RHI
{
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
        RHIGPUDescriptorHandle Offset(uint32 count, uint32 incrementSize) const {
            return RHIGPUDescriptorHandle(ptr + count * incrementSize);
        }

        constexpr bool operator==(const RHIGPUDescriptorHandle& other) const {
            return ptr == other.ptr;
        }
    };
}
```

- [ ] RHIGPUDescriptorHandle 構造体
- [ ] Offset計算メソッド

### 3. 統合デスクリプタハンドル

```cpp
namespace NS::RHI
{
    /// 統合デスクリプタハンドル
    /// CPU/GPUハンドルのペアとメタデータ
    struct RHIDescriptorHandle
    {
        RHICPUDescriptorHandle cpu;
        RHIGPUDescriptorHandle gpu;  // GPUVisible時のみ有効
        uint32 heapIndex = 0;        // 所属ヒープインデックス
        uint32 offsetInHeap = 0;     // ヒープ内オフセット

        constexpr RHIDescriptorHandle() = default;

        /// CPUのみのハンドル
        static RHIDescriptorHandle CPUOnly(RHICPUDescriptorHandle cpuHandle) {
            RHIDescriptorHandle h;
            h.cpu = cpuHandle;
            return h;
        }

        /// CPU+GPUハンドル
        static RHIDescriptorHandle CPUAndGPU(
            RHICPUDescriptorHandle cpuHandle,
            RHIGPUDescriptorHandle gpuHandle) {
            RHIDescriptorHandle h;
            h.cpu = cpuHandle;
            h.gpu = gpuHandle;
            return h;
        }

        bool IsValid() const { return cpu.IsValid(); }
        bool IsGPUVisible() const { return gpu.IsValid(); }
    };
}
```

- [ ] RHIDescriptorHandle 統合構造体
- [ ] ファクトリメソッド

### 4. Bindlessインデックス

```cpp
namespace NS::RHI
{
    /// BindlessリソースインデックスA
    /// シェーダー内部のリソース参照用
    struct BindlessIndex
    {
        uint32 index = kInvalidDescriptorIndex;

        constexpr BindlessIndex() = default;
        explicit constexpr BindlessIndex(uint32 i) : index(i) {}

        constexpr bool IsValid() const {
            return index != kInvalidDescriptorIndex;
        }

        /// シェーダーに渡す値として取得
        constexpr uint32 GetShaderIndex() const { return index; }
    };

    /// Bindless SRVインデックス
    struct BindlessSRVIndex : BindlessIndex {
        using BindlessIndex::BindlessIndex;
    };

    /// Bindless UAVインデックス
    struct BindlessUAVIndex : BindlessIndex {
        using BindlessIndex::BindlessIndex;
    };

    /// Bindless Samplerインデックス
    struct BindlessSamplerIndex : BindlessIndex {
        using BindlessIndex::BindlessIndex;
    };
}
```

- [ ] BindlessIndex 基底型
- [ ] 型安全なBindlessXxxIndex派生型

### 5. RHI制限定数

```cpp
namespace NS::RHI
{
    //=========================================================================
    // レンダリング制限
    //=========================================================================

    /// 最大同時レンダーターゲット数
    constexpr uint32 kMaxRenderTargets = 8;

    /// 最大頂点ストリーム数
    constexpr uint32 kMaxVertexStreams = 16;

    /// 最大頂点要素数
    constexpr uint32 kMaxVertexElements = 16;

    /// 最大ビューポート・シザー数
    constexpr uint32 kMaxViewports = 16;

    //=========================================================================
    // テクスチャ制限
    //=========================================================================

    /// 最大MIPレベル数
    constexpr uint32 kMaxTextureMipCount = 15;

    /// 最大配列サイズ
    constexpr uint32 kMaxTextureArraySize = 2048;

    /// 最大キューブマップ配列サイズ
    constexpr uint32 kMaxCubeArraySize = 2048;

    //=========================================================================
    // ディスクリプタ制限
    //=========================================================================

    /// 最大サンプラー数
    constexpr uint32 kMaxSamplerCount = 2048;

    /// 最大CBV/SRV/UAV数（Bindless用）
    constexpr uint32 kMaxCBVSRVUAVCount = 1000000;

    /// オフラインディスクリプタヒープサイズ
    constexpr uint32 kOfflineDescriptorHeapSize = 4096;

    //=========================================================================
    // アライメント要件
    //=========================================================================

    /// 定数バッファアライメント（D3D12: 256バイト）
    constexpr uint32 kConstantBufferAlignment = 256;

    /// テクスチャデータアライメント（D3D12: 512バイト）
    constexpr uint32 kTextureDataAlignment = 512;

    /// バッファアライメント
    constexpr uint32 kBufferAlignment = 16;

    //=========================================================================
    // フレーム制限
    //=========================================================================

    /// 最大フレームインフライト数
    constexpr uint32 kMaxFramesInFlight = 3;

    /// デフォルトバックバッファ数
    constexpr uint32 kDefaultBackBufferCount = 2;
}
```

- [ ] レンダリング制限定数
- [ ] テクスチャ制限定数
- [ ] ディスクリプタ制限定数
- [ ] アライメント定数
- [ ] フレーム制限定数

## 検証方法

- [ ] ハンドル演算の正確性
- [ ] 定数値の妥当性（D3D12/Vulkan仕様適合）
- [ ] static_assertでサイズ検証
