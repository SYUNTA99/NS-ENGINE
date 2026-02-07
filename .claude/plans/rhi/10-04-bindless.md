# 10-04: Bindlessサポート

## 目的

Bindlessレンダリングのためのディスクリプタインデックス管理を定義する。

## 参照ドキュメント

- 10-01-descriptor-heap.md (ディスクリプタヒープ
- 10-02-online-descriptor.md (オンラインディスクリプタ)
- 08-02-descriptor-range.md (無制限デスクリプタレンジ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIBindless.h`

## TODO

### 1. Bindlessディスクリプタインデックス

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// Bindlessディスクリプタインデックス
    /// シェーダー内のディスクリプタを参照するためのインデックス
    struct RHI_API RHIBindlessIndex
    {
        /// 無効なインデックス
        static constexpr uint32 kInvalidIndex = ~0u;

        /// インデックス値
        uint32 index = kInvalidIndex;

        /// 有効か
        bool IsValid() const { return index != kInvalidIndex; }

        /// uint32への変換
        operator uint32() const { return index; }

        /// 比較演算子
        bool operator==(const RHIBindlessIndex& other) const { return index == other.index; }
        bool operator!=(const RHIBindlessIndex& other) const { return index != other.index; }

        /// 無効なインデックス作成
        static RHIBindlessIndex Invalid() { return {}; }

        /// インデックスから作成
        static RHIBindlessIndex FromIndex(uint32 i) {
            RHIBindlessIndex idx;
            idx.index = i;
            return idx;
        }
    };
}
```

- [ ] RHIBindlessIndex 構造体

### 2. Bindlessディスクリプタヒープ

```cpp
namespace NS::RHI
{
    /// Bindlessディスクリプタヒープ
    /// 大規模なGPU可視ヒープを永続的に管理
    /// @threadsafety Allocate/Freeは外部同期が必要。SetSRV等はコマンドコンテキストスレッドからのみ呼び出し可。
    class RHI_API RHIBindlessDescriptorHeap
    {
    public:
        /// 最大ディスクリプタ数
        static constexpr uint32 kMaxDescriptors = 1000000;  // 100万

        RHIBindlessDescriptorHeap() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 numDescriptors = kMaxDescriptors);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ディスクリプタ割り当て
        //=====================================================================

        /// ディスクリプタ割り当て
        /// @return Bindlessインデックス
        RHIBindlessIndex Allocate();

        /// 複数ディスクリプタ割り当て
        /// @param count 連続デスクリプタ数
        /// @return 先頭のBindlessインデックス
        RHIBindlessIndex AllocateRange(uint32 count);

        /// ディスクリプタ解放
        void Free(RHIBindlessIndex index);

        /// 範を解放
        void FreeRange(RHIBindlessIndex startIndex, uint32 count);

        //=====================================================================
        // ディスクリプタ設定
        //=====================================================================

        /// SRVを設定
        void SetSRV(RHIBindlessIndex index, IRHIShaderResourceView* srv);

        /// UAVを設定
        void SetUAV(RHIBindlessIndex index, IRHIUnorderedAccessView* uav);

        /// CBVを設定
        void SetCBV(RHIBindlessIndex index, IRHIConstantBufferView* cbv);

        /// ディスクリプタをコピー
        void CopyDescriptor(RHIBindlessIndex destIndex, RHICPUDescriptorHandle srcHandle);

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// インデックスからGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(RHIBindlessIndex index) const;

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const;

        /// 総デスクリプタ数
        uint32 GetTotalCount() const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
    };
}
```

- [ ] RHIBindlessDescriptorHeap クラス
- [ ] ディスクリプタ割り当て/解放

### 3. Bindlessサンプラーヒープ

```cpp
namespace NS::RHI
{
    /// Bindlessサンプラーヒープ
    /// サンプラー用の永続ヒープ（最大2048）。
    class RHI_API RHIBindlessSamplerHeap
    {
    public:
        /// 最大サンプラー数（D3D12の制限）
        static constexpr uint32 kMaxSamplers = 2048;

        RHIBindlessSamplerHeap() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 numSamplers = kMaxSamplers);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // サンプラー登録
        //=====================================================================

        /// サンプラー登録
        /// @return Bindlessインデックス
        RHIBindlessIndex RegisterSampler(IRHISampler* sampler);

        /// サンプラー登録解除
        void UnregisterSampler(RHIBindlessIndex index);

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// インデックスからGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(RHIBindlessIndex index) const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
    };
}
```

- [ ] RHIBindlessSamplerHeap クラス

### 4. Bindlessリソースマネージャー

```cpp
namespace NS::RHI
{
    /// Bindlessリソース登録情報
    struct RHI_API RHIBindlessResourceInfo
    {
        /// Bindlessインデックス
        RHIBindlessIndex index;

        /// リソースタイプ
        enum class Type : uint8 {
            SRV,
            UAV,
            CBV,
            Sampler,
        };
        Type type = Type::SRV;

        /// 元リソース
        IRHIResource* resource = nullptr;
    };

    /// Bindlessリソースマネージャー
    /// リソースとBindlessインデックスの対応を管理
    class RHI_API RHIBindlessResourceManager
    {
    public:
        RHIBindlessResourceManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // リソース登録
        //=====================================================================

        /// テクスチャSRV登録
        RHIBindlessIndex RegisterTextureSRV(
            IRHITexture* texture,
            const RHITextureSRVDesc& desc = {});

        /// バッファSRV登録
        RHIBindlessIndex RegisterBufferSRV(
            IRHIBuffer* buffer,
            const RHIBufferSRVDesc& desc = {});

        /// テクスチャUAV登録
        RHIBindlessIndex RegisterTextureUAV(
            IRHITexture* texture,
            const RHITextureUAVDesc& desc = {});

        /// バッファUAV登録
        RHIBindlessIndex RegisterBufferUAV(
            IRHIBuffer* buffer,
            const RHIBufferUAVDesc& desc = {});

        /// サンプラー登録
        RHIBindlessIndex RegisterSampler(IRHISampler* sampler);

        /// 登録解除
        void Unregister(RHIBindlessIndex index);

        /// リソースの全インデックスを解除
        void UnregisterResource(IRHIResource* resource);

        //=====================================================================
        // ヒープ取得
        //=====================================================================

        IRHIDescriptorHeap* GetCBVSRVUAVHeap() const { return m_descriptorHeap.GetHeap(); }
        IRHIDescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.GetHeap(); }

        //=====================================================================
        // コンテキストへのバインド
        //=====================================================================

        /// Bindlessヒープをコンテキストに設定
        void BindToContext(IRHICommandContext* context);

    private:
        IRHIDevice* m_device = nullptr;
        RHIBindlessDescriptorHeap m_descriptorHeap;
        RHIBindlessSamplerHeap m_samplerHeap;

        /// リソース→インデックスのマッピング
        // Hash map implementation
    };
}
```

- [ ] RHIBindlessResourceInfo 構造体
- [ ] RHIBindlessResourceManager クラス

### 5. Bindlessルートシグネチャ

```cpp
namespace NS::RHI
{
    /// Bindless用ルートシグネチャプリセット
    namespace RHIBindlessRootSignature
    {
        /// Bindless基本レイアウト
        /// - Root 0: CBV (per-frame constants)
        /// - Root 1: CBV (per-object constants)
        /// - Root 2: 32-bit constants (material index etc.)
        /// - Root 3: Bindless SRV table
        /// - Root 4: Bindless UAV table
        /// - Root 5: Bindless Sampler table
        inline RHIRootSignatureDesc CreateBasicBindless()
        {
            static RHIDescriptorRange srvRange = RHIDescriptorRange::UnboundedSRV(0, 0);
            static RHIDescriptorRange uavRange = RHIDescriptorRange::UnboundedUAV(0, 1);
            static RHIDescriptorRange samplerRange = RHIDescriptorRange::Sampler(0, 2048, 0);

            static RHIRootParameter params[] = {
                RHIRootParameter::CBV(0),                        // Per-frame
                RHIRootParameter::CBV(1),                        // Per-object
                RHIRootParameter::Constants(2, 4),               // 4x 32-bit constants
                RHIRootParameter::DescriptorTable(&srvRange, 1), // Bindless SRVs
                RHIRootParameter::DescriptorTable(&uavRange, 1), // Bindless UAVs
                RHIRootParameter::DescriptorTable(&samplerRange, 1), // Samplers
            };

            return RHIRootSignatureDesc::FromParameters(params,
                ERHIRootSignatureFlags::AllowInputAssemblerInputLayout |
                ERHIRootSignatureFlags::CBVSRVUAVHeapDirectlyIndexed |
                ERHIRootSignatureFlags::SamplerHeapDirectlyIndexed);
        }

        /// Bindlessコンピュート用
        inline RHIRootSignatureDesc CreateComputeBindless()
        {
            static RHIDescriptorRange srvRange = RHIDescriptorRange::UnboundedSRV(0, 0);
            static RHIDescriptorRange uavRange = RHIDescriptorRange::UnboundedUAV(0, 1);

            static RHIRootParameter params[] = {
                RHIRootParameter::CBV(0),                        // Constants
                RHIRootParameter::Constants(1, 8),               // Push constants
                RHIRootParameter::DescriptorTable(&srvRange, 1), // Bindless SRVs
                RHIRootParameter::DescriptorTable(&uavRange, 1), // Bindless UAVs
            };

            return RHIRootSignatureDesc::FromParameters(params,
                ERHIRootSignatureFlags::CBVSRVUAVHeapDirectlyIndexed);
        }
    }
}
```

- [ ] CreateBasicBindless
- [ ] CreateComputeBindless

### 6. シェーダー側Bindlessアクセス

```cpp
namespace NS::RHI
{
    /// Bindlessシェーダーマクロ定義
    /// HLSLシェーダーで使用する定義文字列。
    constexpr const char* kBindlessShaderDefines = R"(
// Bindless resource declarations
#define DECLARE_BINDLESS_TEXTURES \
    Texture2D g_BindlessTextures[] : register(t0, space0); \
    Texture3D g_BindlessTextures3D[] : register(t0, space1); \
    TextureCube g_BindlessTexturesCube[] : register(t0, space2);

#define DECLARE_BINDLESS_BUFFERS \
    ByteAddressBuffer g_BindlessBuffers[] : register(t0, space3); \
    StructuredBuffer<float4> g_BindlessStructuredBuffers[] : register(t0, space4);

#define DECLARE_BINDLESS_RWBUFFERS \
    RWByteAddressBuffer g_BindlessRWBuffers[] : register(u0, space0); \
    RWStructuredBuffer<float4> g_BindlessRWStructuredBuffers[] : register(u0, space1);

#define DECLARE_BINDLESS_SAMPLERS \
    SamplerState g_BindlessSamplers[] : register(s0, space0);

// Bindless access macros
#define SAMPLE_BINDLESS_TEXTURE(index, sampler, uv) \
    g_BindlessTextures[index].Sample(g_BindlessSamplers[sampler], uv)

#define LOAD_BINDLESS_BUFFER(index, offset) \
    g_BindlessBuffers[index].Load(offset)
)";
}
```

- [ ] Bindlessシェーダーマクロ定義

## 検証方法

- [ ] Bindlessインデックス割り当て/解放
- [ ] リソース登録/解除
- [ ] シェーダーからのBindlessアクセス
- [ ] 大量リソースの管理
