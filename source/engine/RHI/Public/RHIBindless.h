/// @file RHIBindless.h
/// @brief Bindlessディスクリプタ管理
/// @details Bindlessディスクリプタヒープ、サンプラーヒープ、リソースマネージャー、
///          ルートシグネチャプリセット、シェーダーマクロを提供。
/// @see 10-04-bindless.md
#pragma once

#include "IRHIRootSignature.h"
#include "RHIDescriptorHeap.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIBindlessDescriptorHeap (10-04)
    //=========================================================================

    /// Bindlessディスクリプタヒープ
    /// 大規模なGPU可視ヒープを永続的に管理
    /// @threadsafety Allocate/Freeは外部同期が必要。
    class RHI_API RHIBindlessDescriptorHeap
    {
    public:
        /// 最大ディスクリプタ数
        static constexpr uint32 kMaxDescriptors = 1000000;

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
        BindlessIndex Allocate();

        /// 複数ディスクリプタ割り当て
        /// @param count 連続ディスクリプタ数
        /// @return 先頭のBindlessインデックス
        BindlessIndex AllocateRange(uint32 count);

        /// ディスクリプタ解放
        void Free(BindlessIndex index);

        /// 範囲解放
        void FreeRange(BindlessIndex startIndex, uint32 count);

        //=====================================================================
        // ディスクリプタ設定
        //=====================================================================

        /// SRVを設定
        void SetSRV(BindlessIndex index, IRHIShaderResourceView* srv);

        /// UAVを設定
        void SetUAV(BindlessIndex index, IRHIUnorderedAccessView* uav);

        /// CBVを設定
        void SetCBV(BindlessIndex index, IRHIConstantBufferView* cbv);

        /// ディスクリプタをコピー
        void CopyDescriptor(BindlessIndex destIndex, RHICPUDescriptorHandle srcHandle);

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// インデックスからGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(BindlessIndex index) const;

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const;

        /// 総ディスクリプタ数
        uint32 GetTotalCount() const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
    };

    //=========================================================================
    // RHIBindlessSamplerHeap (10-04)
    //=========================================================================

    /// Bindlessサンプラーヒープ
    /// サンプラー用の永続ヒープ（最大2048）
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
        BindlessSamplerIndex RegisterSampler(IRHISampler* sampler);

        /// サンプラー登録解除
        void UnregisterSampler(BindlessSamplerIndex index);

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// インデックスからGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(BindlessSamplerIndex index) const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
    };

    //=========================================================================
    // RHIBindlessResourceInfo (10-04)
    //=========================================================================

    /// Bindlessリソース登録情報
    struct RHI_API RHIBindlessResourceInfo
    {
        /// Bindlessインデックス
        BindlessIndex index;

        /// リソースタイプ
        enum class Type : uint8
        {
            SRV,
            UAV,
            CBV,
            Sampler,
        };
        Type type = Type::SRV;

        /// 元リソース
        IRHIResource* resource = nullptr;
    };

    //=========================================================================
    // RHIBindlessResourceManager (10-04)
    //=========================================================================

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
        BindlessSRVIndex RegisterTextureSRV(IRHITexture* texture, const RHITextureSRVDesc& desc = {});

        /// バッファSRV登録
        BindlessSRVIndex RegisterBufferSRV(IRHIBuffer* buffer, const RHIBufferSRVDesc& desc = {});

        /// テクスチャUAV登録
        BindlessUAVIndex RegisterTextureUAV(IRHITexture* texture, const RHITextureUAVDesc& desc = {});

        /// バッファUAV登録
        BindlessUAVIndex RegisterBufferUAV(IRHIBuffer* buffer, const RHIBufferUAVDesc& desc = {});

        /// サンプラー登録
        BindlessSamplerIndex RegisterSampler(IRHISampler* sampler);

        /// 登録解除
        void Unregister(BindlessIndex index);

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
        /// ※ 非インライン（IRHICommandContextの定義に依存）
        void BindToContext(IRHICommandContext* context);

    private:
        IRHIDevice* m_device = nullptr;
        RHIBindlessDescriptorHeap m_descriptorHeap;
        RHIBindlessSamplerHeap m_samplerHeap;
    };

    //=========================================================================
    // RHIBindlessRootSignature (10-04)
    //=========================================================================

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
                RHIRootParameter::CBV(0),
                RHIRootParameter::CBV(1),
                RHIRootParameter::Constants(2, 4),
                RHIRootParameter::DescriptorTable(&srvRange, 1),
                RHIRootParameter::DescriptorTable(&uavRange, 1),
                RHIRootParameter::DescriptorTable(&samplerRange, 1),
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
                RHIRootParameter::CBV(0),
                RHIRootParameter::Constants(1, 8),
                RHIRootParameter::DescriptorTable(&srvRange, 1),
                RHIRootParameter::DescriptorTable(&uavRange, 1),
            };

            return RHIRootSignatureDesc::FromParameters(params, ERHIRootSignatureFlags::CBVSRVUAVHeapDirectlyIndexed);
        }
    } // namespace RHIBindlessRootSignature

    //=========================================================================
    // Bindlessシェーダーマクロ (10-04)
    //=========================================================================

    /// Bindlessシェーダーマクロ定義
    /// HLSLシェーダーで使用する定義文字列
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

} // namespace NS::RHI
