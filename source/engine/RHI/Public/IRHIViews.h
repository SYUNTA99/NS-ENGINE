/// @file IRHIViews.h
/// @brief SRV・UAV・RTV・DSV・CBV ビューインターフェースと記述構造体
/// @details シェーダーリソースビュー、アンオーダードアクセスビュー、レンダーターゲットビュー、
///          デプスステンシルビュー、定数バッファビューの定義と関連ヘルパー。
/// @see 05-01-srv.md, 05-02-uav.md, 05-03-rtv.md, 05-04-dsv.md, 05-05-cbv.md
#pragma once

#include "IRHIResource.h"
#include "RHICheck.h"
#include "RHIEnums.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace NS
{
    namespace RHI
    {
        /// 前方宣言
        class IRHIDevice;
        class IRHIBuffer;
        class IRHITexture;
        class IRHICommandContext;
        class IRHIComputeContext;
        enum class ERHICubeFace : uint8;

        /// ピクセルフォーマットヘルパー前方宣言（15-xxで実装）
        RHI_API bool IsDepthFormat(ERHIPixelFormat format);
        RHI_API bool IsStencilFormat(ERHIPixelFormat format);

        //=========================================================================
        // バッファSRV記述 (05-01)
        //=========================================================================

        /// バッファSRV記述
        struct RHI_API RHIBufferSRVDesc
        {
            /// バッファ
            IRHIBuffer* buffer = nullptr;

            /// SRVフォーマット
            ERHIBufferSRVFormat srvFormat = ERHIBufferSRVFormat::Structured;

            /// 型付きバッファ用フォーマット
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// 開始要素インデックス
            uint32 firstElement = 0;

            /// 要素数（0 = 残り全て）
            uint32 numElements = 0;

            /// 構造体バイトストライド（StructuredBufferの場合。0 = バッファのストライドを使用）
            uint32 structureByteStride = 0;

            /// ビルダー
            static RHIBufferSRVDesc Structured(IRHIBuffer* buf, uint32 first = 0, uint32 count = 0)
            {
                RHIBufferSRVDesc desc;
                desc.buffer = buf;
                desc.srvFormat = ERHIBufferSRVFormat::Structured;
                desc.firstElement = first;
                desc.numElements = count;
                return desc;
            }

            static RHIBufferSRVDesc Raw(IRHIBuffer* buf, uint32 firstByte = 0, uint32 numBytes = 0)
            {
                RHIBufferSRVDesc desc;
                desc.buffer = buf;
                desc.srvFormat = ERHIBufferSRVFormat::Raw;
                desc.firstElement = firstByte / 4;
                desc.numElements = (numBytes > 0) ? numBytes / 4 : 0;
                return desc;
            }

            static RHIBufferSRVDesc Typed(IRHIBuffer* buf, ERHIPixelFormat fmt, uint32 first = 0, uint32 count = 0)
            {
                RHIBufferSRVDesc desc;
                desc.buffer = buf;
                desc.srvFormat = ERHIBufferSRVFormat::Typed;
                desc.format = fmt;
                desc.firstElement = first;
                desc.numElements = count;
                return desc;
            }
        };

        //=========================================================================
        // テクスチャSRV記述 (05-01)
        //=========================================================================

        /// テクスチャSRV記述
        struct RHI_API RHITextureSRVDesc
        {
            /// テクスチャ
            IRHITexture* texture = nullptr;

            /// ビューフォーマット（Unknown = テクスチャのフォーマットを使用）
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// 次元
            ERHITextureDimension dimension = ERHITextureDimension::Texture2D;

            /// MIPレベル範囲
            uint32 mostDetailedMip = 0;
            uint32 mipLevels = 0; // 0 = 残り全て

            /// 配列範囲
            uint32 firstArraySlice = 0;
            uint32 arraySize = 0; // 0 = 残り全て

            /// 平面スライス（デプス/ステンシル分離用）
            uint32 planeSlice = 0;

            /// 最小LODクランプ
            float minLODClamp = 0.0f;

            /// コンポーネントマッピング
            RHIComponentMapping componentMapping = RHIComponentMapping::Identity();

            /// ビルダー
            static RHITextureSRVDesc Default(IRHITexture* tex)
            {
                RHITextureSRVDesc desc;
                desc.texture = tex;
                return desc;
            }

            static RHITextureSRVDesc MipRange(IRHITexture* tex, uint32 mostDetailed, uint32 count)
            {
                RHITextureSRVDesc desc;
                desc.texture = tex;
                desc.mostDetailedMip = mostDetailed;
                desc.mipLevels = count;
                return desc;
            }

            static RHITextureSRVDesc ArraySlice(IRHITexture* tex, uint32 slice)
            {
                RHITextureSRVDesc desc;
                desc.texture = tex;
                desc.firstArraySlice = slice;
                desc.arraySize = 1;
                return desc;
            }

            static RHITextureSRVDesc CubeFace(IRHITexture* tex, uint32 faceIndex)
            {
                RHITextureSRVDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2D;
                desc.firstArraySlice = faceIndex;
                desc.arraySize = 1;
                return desc;
            }
        };

        //=========================================================================
        // IRHIShaderResourceView (05-01)
        //=========================================================================

        /// シェーダーリソースビュー
        class RHI_API IRHIShaderResourceView : public IRHIResource
        {
        protected:
            IRHIShaderResourceView() : IRHIResource(ERHIResourceType::ShaderResourceView) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(ShaderResourceView)

            virtual ~IRHIShaderResourceView() = default;

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// CPUディスクリプタハンドル取得
            RHICPUDescriptorHandle GetCPUHandle() const { return m_cpuHandle; }

            /// GPUディスクリプタハンドル取得（オンラインヒープの場合）
            RHIGPUDescriptorHandle GetGPUHandle() const { return m_gpuHandle; }

            /// Bindlessインデックス取得
            BindlessIndex GetBindlessIndex() const { return m_bindlessIndex; }

            /// ソースリソース取得
            virtual IRHIResource* GetResource() const = 0;

            /// バッファビューか
            virtual bool IsBufferView() const = 0;

            /// テクスチャビューか
            bool IsTextureView() const { return !IsBufferView(); }

            /// ソースバッファ取得（バッファビューの場合）
            IRHIBuffer* GetBuffer() const;

            /// ソーステクスチャ取得（テクスチャビューの場合）
            IRHITexture* GetTexture() const;

        protected:
            RHICPUDescriptorHandle m_cpuHandle{};
            RHIGPUDescriptorHandle m_gpuHandle{};
            BindlessIndex m_bindlessIndex{};
        };

        using RHIShaderResourceViewRef = TRefCountPtr<IRHIShaderResourceView>;

        //=========================================================================
        // RHISRVArray (05-01)
        //=========================================================================

        /// SRV配列ラッパー
        class RHI_API RHISRVArray
        {
        public:
            RHISRVArray() = default;

            explicit RHISRVArray(uint32 maxSize) : m_srvs(maxSize, nullptr) {}

            void Set(uint32 index, IRHIShaderResourceView* srv)
            {
                if (index < m_srvs.size())
                    m_srvs[index] = srv;
            }

            IRHIShaderResourceView* Get(uint32 index) const { return index < m_srvs.size() ? m_srvs[index] : nullptr; }

            uint32 GetSize() const { return static_cast<uint32>(m_srvs.size()); }
            IRHIShaderResourceView* const* GetData() const { return m_srvs.data(); }

            uint32 GetValidCount() const
            {
                uint32 count = 0;
                for (auto* srv : m_srvs)
                {
                    if (srv)
                        ++count;
                }
                return count;
            }

            void Clear() { std::fill(m_srvs.begin(), m_srvs.end(), nullptr); }

        private:
            std::vector<IRHIShaderResourceView*> m_srvs;
        };

        //=========================================================================
        // バッファUAV記述 (05-02)
        //=========================================================================

        /// バッファUAV記述
        struct RHI_API RHIBufferUAVDesc
        {
            /// バッファ
            IRHIBuffer* buffer = nullptr;

            /// UAVフォーマット
            ERHIBufferSRVFormat uavFormat = ERHIBufferSRVFormat::Structured;

            /// 型付きバッファ用フォーマット
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// 開始要素インデックス
            uint32 firstElement = 0;

            /// 要素数（0 = 残り全て）
            uint32 numElements = 0;

            /// 構造体バイトストライド
            uint32 structureByteStride = 0;

            /// カウンターバッファ（AppendStructuredBuffer用）
            IRHIBuffer* counterBuffer = nullptr;

            /// カウンターバッファオフセット
            uint64 counterOffset = 0;

            /// UAVフラグ
            enum class Flags : uint32
            {
                None = 0,
                Raw = 1 << 0,
                Append = 1 << 1,
                Counter = 1 << 2,
            };
            Flags flags = Flags::None;

            /// ビルダー
            static RHIBufferUAVDesc Structured(IRHIBuffer* buf, uint32 first = 0, uint32 count = 0)
            {
                RHIBufferUAVDesc desc;
                desc.buffer = buf;
                desc.uavFormat = ERHIBufferSRVFormat::Structured;
                desc.firstElement = first;
                desc.numElements = count;
                return desc;
            }

            static RHIBufferUAVDesc Raw(IRHIBuffer* buf, uint32 firstByte = 0, uint32 numBytes = 0)
            {
                RHIBufferUAVDesc desc;
                desc.buffer = buf;
                desc.uavFormat = ERHIBufferSRVFormat::Raw;
                desc.firstElement = firstByte / 4;
                desc.numElements = (numBytes > 0) ? numBytes / 4 : 0;
                desc.flags = Flags::Raw;
                return desc;
            }

            static RHIBufferUAVDesc Typed(IRHIBuffer* buf, ERHIPixelFormat fmt, uint32 first = 0, uint32 count = 0)
            {
                RHIBufferUAVDesc desc;
                desc.buffer = buf;
                desc.uavFormat = ERHIBufferSRVFormat::Typed;
                desc.format = fmt;
                desc.firstElement = first;
                desc.numElements = count;
                return desc;
            }

            static RHIBufferUAVDesc WithCounter(IRHIBuffer* buf, IRHIBuffer* counter, uint64 counterOff = 0)
            {
                RHIBufferUAVDesc desc = Structured(buf);
                desc.counterBuffer = counter;
                desc.counterOffset = counterOff;
                desc.flags = Flags::Counter;
                return desc;
            }
        };
        RHI_ENUM_CLASS_FLAGS(RHIBufferUAVDesc::Flags)

        //=========================================================================
        // テクスチャUAV記述 (05-02)
        //=========================================================================

        /// テクスチャUAV記述
        struct RHI_API RHITextureUAVDesc
        {
            /// テクスチャ
            IRHITexture* texture = nullptr;

            /// ビューフォーマット（Unknown = テクスチャのフォーマットを使用）
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// MIPレベル（単一MIPのみ指定可能）
            uint32 mipSlice = 0;

            /// 配列範囲（2D配列/3Dの場合）
            uint32 firstArraySlice = 0;
            uint32 arraySize = 0; // 0 = 残り全て

            /// 平面スライス
            uint32 planeSlice = 0;

            /// ビルダー
            static RHITextureUAVDesc Default(IRHITexture* tex, uint32 mip = 0)
            {
                RHITextureUAVDesc desc;
                desc.texture = tex;
                desc.mipSlice = mip;
                return desc;
            }

            static RHITextureUAVDesc ArraySlice(IRHITexture* tex, uint32 mip, uint32 slice)
            {
                RHITextureUAVDesc desc;
                desc.texture = tex;
                desc.mipSlice = mip;
                desc.firstArraySlice = slice;
                desc.arraySize = 1;
                return desc;
            }

            static RHITextureUAVDesc Slice3D(IRHITexture* tex, uint32 mip, uint32 firstW, uint32 wSize)
            {
                RHITextureUAVDesc desc;
                desc.texture = tex;
                desc.mipSlice = mip;
                desc.firstArraySlice = firstW;
                desc.arraySize = wSize;
                return desc;
            }
        };

        //=========================================================================
        // IRHIUnorderedAccessView (05-02)
        //=========================================================================

        /// アンオーダードアクセスビュー
        class RHI_API IRHIUnorderedAccessView : public IRHIResource
        {
        protected:
            IRHIUnorderedAccessView() : IRHIResource(ERHIResourceType::UnorderedAccessView) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(UnorderedAccessView)

            virtual ~IRHIUnorderedAccessView() = default;

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// CPUディスクリプタハンドル取得
            RHICPUDescriptorHandle GetCPUHandle() const { return m_cpuHandle; }

            /// GPUディスクリプタハンドル取得
            RHIGPUDescriptorHandle GetGPUHandle() const { return m_gpuHandle; }

            /// Bindlessインデックス取得
            BindlessIndex GetBindlessIndex() const { return m_bindlessIndex; }

            /// ソースリソース取得
            virtual IRHIResource* GetResource() const = 0;

            /// バッファビューか
            virtual bool IsBufferView() const = 0;

            /// テクスチャビューか
            bool IsTextureView() const { return !IsBufferView(); }

            /// ソースバッファ取得
            IRHIBuffer* GetBuffer() const;

            /// ソーステクスチャ取得
            IRHITexture* GetTexture() const;

            /// カウンターを持つか
            virtual bool HasCounter() const = 0;

            /// カウンターリソース取得
            virtual IRHIBuffer* GetCounterResource() const = 0;

            /// カウンターオフセット取得
            virtual uint64 GetCounterOffset() const = 0;

        protected:
            RHICPUDescriptorHandle m_cpuHandle{};
            RHIGPUDescriptorHandle m_gpuHandle{};
            BindlessIndex m_bindlessIndex{};
        };

        using RHIUnorderedAccessViewRef = TRefCountPtr<IRHIUnorderedAccessView>;

        //=========================================================================
        // RHIUAVClearValue (05-02)
        //=========================================================================

        /// UAVクリア値
        union RHI_API RHIUAVClearValue
        {
            float floatValue[4];
            uint32 uintValue[4];

            RHIUAVClearValue() { uintValue[0] = uintValue[1] = uintValue[2] = uintValue[3] = 0; }

            static RHIUAVClearValue Float(float r, float g = 0, float b = 0, float a = 0)
            {
                RHIUAVClearValue v;
                v.floatValue[0] = r;
                v.floatValue[1] = g;
                v.floatValue[2] = b;
                v.floatValue[3] = a;
                return v;
            }

            static RHIUAVClearValue Uint(uint32 r, uint32 g = 0, uint32 b = 0, uint32 a = 0)
            {
                RHIUAVClearValue v;
                v.uintValue[0] = r;
                v.uintValue[1] = g;
                v.uintValue[2] = b;
                v.uintValue[3] = a;
                return v;
            }

            static RHIUAVClearValue Zero() { return RHIUAVClearValue(); }
        };

        //=========================================================================
        // RHIUAVCounterHelper (05-02)
        //=========================================================================

        /// UAVカウンター操作ヘルパー
        class RHI_API RHIUAVCounterHelper
        {
        public:
            /// カウンターをリセット
            static void ResetCounter(IRHICommandContext* context, IRHIUnorderedAccessView* uav, uint32 value = 0);

            /// カウンター値をバッファにコピー
            static void CopyCounterToBuffer(IRHICommandContext* context,
                                            IRHIUnorderedAccessView* uav,
                                            IRHIBuffer* destBuffer,
                                            uint64 destOffset = 0);

            /// カウンター値をバッファからセット
            static void SetCounterFromBuffer(IRHICommandContext* context,
                                             IRHIUnorderedAccessView* uav,
                                             IRHIBuffer* srcBuffer,
                                             uint64 srcOffset = 0);
        };

        //=========================================================================
        // レンダーターゲットビュー記述 (05-03)
        //=========================================================================

        /// レンダーターゲットビュー記述
        struct RHI_API RHIRenderTargetViewDesc
        {
            /// テクスチャ
            IRHITexture* texture = nullptr;

            /// ビューフォーマット（Unknown = テクスチャのフォーマットを使用）
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// 次元
            ERHITextureDimension dimension = ERHITextureDimension::Texture2D;

            /// MIPレベル
            uint32 mipSlice = 0;

            /// 平面スライス
            uint32 planeSlice = 0;

            /// 配列範囲
            uint32 firstArraySlice = 0;
            uint32 arraySize = 0; // 0 = 残り全て

            /// 3Dテクスチャ用Wスライス
            uint32 firstWSlice = 0;
            uint32 wSize = 0; // 0 = 残り全て

            /// ビルダー
            static RHIRenderTargetViewDesc Texture2D(IRHITexture* tex, uint32 mip = 0)
            {
                RHIRenderTargetViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2D;
                desc.mipSlice = mip;
                return desc;
            }

            static RHIRenderTargetViewDesc Texture2DArray(IRHITexture* tex,
                                                          uint32 mip,
                                                          uint32 firstSlice,
                                                          uint32 count = 1)
            {
                RHIRenderTargetViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2DArray;
                desc.mipSlice = mip;
                desc.firstArraySlice = firstSlice;
                desc.arraySize = count;
                return desc;
            }

            static RHIRenderTargetViewDesc Texture2DMS(IRHITexture* tex)
            {
                RHIRenderTargetViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2DMS;
                return desc;
            }

            static RHIRenderTargetViewDesc Texture3D(IRHITexture* tex, uint32 mip, uint32 firstW, uint32 wSz = 1)
            {
                RHIRenderTargetViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture3D;
                desc.mipSlice = mip;
                desc.firstWSlice = firstW;
                desc.wSize = wSz;
                return desc;
            }

            static RHIRenderTargetViewDesc CubeFace(IRHITexture* tex, uint32 faceIndex, uint32 mip = 0)
            {
                RHIRenderTargetViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2DArray;
                desc.mipSlice = mip;
                desc.firstArraySlice = faceIndex;
                desc.arraySize = 1;
                return desc;
            }
        };

        //=========================================================================
        // IRHIRenderTargetView (05-03)
        //=========================================================================

        /// レンダーターゲットビュー
        class RHI_API IRHIRenderTargetView : public IRHIResource
        {
        protected:
            IRHIRenderTargetView() : IRHIResource(ERHIResourceType::RenderTargetView) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(RenderTargetView)

            virtual ~IRHIRenderTargetView() = default;

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// CPUディスクリプタハンドル取得
            RHICPUDescriptorHandle GetCPUHandle() const { return m_cpuHandle; }

            /// ソーステクスチャ取得
            virtual IRHITexture* GetTexture() const = 0;

            /// MIPレベル取得
            virtual uint32 GetMipSlice() const = 0;

            /// 配列スライス取得
            virtual uint32 GetFirstArraySlice() const = 0;

            /// 配列サイズ取得
            virtual uint32 GetArraySize() const = 0;

            /// ビューの幅取得（MIPレベル考慮）
            uint32 GetWidth() const;

            /// ビューの高さ取得（MIPレベル考慮）
            uint32 GetHeight() const;

            /// サイズ取得
            Extent2D GetSize() const { return Extent2D{GetWidth(), GetHeight()}; }

            /// ビューフォーマット取得
            virtual ERHIPixelFormat GetFormat() const = 0;

            /// サンプル数取得
            ERHISampleCount GetSampleCount() const;

            /// マルチサンプルか
            bool IsMultisampled() const { return NS::RHI::IsMultisampled(GetSampleCount()); }

        protected:
            RHICPUDescriptorHandle m_cpuHandle{};
        };

        using RHIRenderTargetViewRef = TRefCountPtr<IRHIRenderTargetView>;

        //=========================================================================
        // RHIRenderTargetArray (05-03)
        //=========================================================================

        /// レンダーターゲット配列（MRT用）
        struct RHI_API RHIRenderTargetArray
        {
            /// RTV配列
            IRHIRenderTargetView* rtvs[kMaxRenderTargets] = {};

            /// 有効なRTV数
            uint32 count = 0;

            void Clear()
            {
                for (uint32 i = 0; i < kMaxRenderTargets; ++i)
                    rtvs[i] = nullptr;
                count = 0;
            }

            bool Add(IRHIRenderTargetView* rtv)
            {
                if (count >= kMaxRenderTargets)
                    return false;
                rtvs[count++] = rtv;
                return true;
            }

            void Set(uint32 slot, IRHIRenderTargetView* rtv)
            {
                if (slot < kMaxRenderTargets)
                {
                    rtvs[slot] = rtv;
                    if (slot >= count)
                        count = slot + 1;
                }
            }

            IRHIRenderTargetView* Get(uint32 slot) const { return slot < kMaxRenderTargets ? rtvs[slot] : nullptr; }

            bool IsEmpty() const { return count == 0; }

            bool ValidateSizeConsistency() const;

            Extent2D GetCommonSize() const { return (count > 0 && rtvs[0]) ? rtvs[0]->GetSize() : Extent2D{0, 0}; }
        };

        //=========================================================================
        // RHIRTVClearValue (05-03)
        //=========================================================================

        /// レンダーターゲットクリア値
        struct RHI_API RHIRTVClearValue
        {
            float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            RHIRTVClearValue() = default;

            RHIRTVClearValue(float r, float g, float b, float a = 1.0f)
            {
                color[0] = r;
                color[1] = g;
                color[2] = b;
                color[3] = a;
            }

            static RHIRTVClearValue Black() { return RHIRTVClearValue(0, 0, 0, 1); }
            static RHIRTVClearValue White() { return RHIRTVClearValue(1, 1, 1, 1); }
            static RHIRTVClearValue Transparent() { return RHIRTVClearValue(0, 0, 0, 0); }
            static RHIRTVClearValue Red() { return RHIRTVClearValue(1, 0, 0, 1); }
            static RHIRTVClearValue Green() { return RHIRTVClearValue(0, 1, 0, 1); }
            static RHIRTVClearValue Blue() { return RHIRTVClearValue(0, 0, 1, 1); }
        };

        //=========================================================================
        // ERHIDSVFlags (05-04)
        //=========================================================================

        /// DSVフラグ
        enum class ERHIDSVFlags : uint8
        {
            None = 0,
            ReadOnlyDepth = 1 << 0,
            ReadOnlyStencil = 1 << 1,
            ReadOnly = ReadOnlyDepth | ReadOnlyStencil,
        };
        RHI_ENUM_CLASS_FLAGS(ERHIDSVFlags)

        //=========================================================================
        // デプスステンシルビュー記述 (05-04)
        //=========================================================================

        /// デプスステンシルビュー記述
        struct RHI_API RHIDepthStencilViewDesc
        {
            /// テクスチャ
            IRHITexture* texture = nullptr;

            /// ビューフォーマット（Unknown = テクスチャのフォーマットを使用）
            ERHIPixelFormat format = ERHIPixelFormat::Unknown;

            /// 次元
            ERHITextureDimension dimension = ERHITextureDimension::Texture2D;

            /// MIPレベル
            uint32 mipSlice = 0;

            /// 配列範囲
            uint32 firstArraySlice = 0;
            uint32 arraySize = 0; // 0 = 残り全て

            /// DSVフラグ
            ERHIDSVFlags flags = ERHIDSVFlags::None;

            /// ビルダー
            static RHIDepthStencilViewDesc Texture2D(IRHITexture* tex,
                                                     uint32 mip = 0,
                                                     ERHIDSVFlags dsvFlags = ERHIDSVFlags::None)
            {
                RHIDepthStencilViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2D;
                desc.mipSlice = mip;
                desc.flags = dsvFlags;
                return desc;
            }

            static RHIDepthStencilViewDesc Texture2DReadOnly(IRHITexture* tex, uint32 mip = 0)
            {
                return Texture2D(tex, mip, ERHIDSVFlags::ReadOnly);
            }

            static RHIDepthStencilViewDesc Texture2DReadOnlyDepth(IRHITexture* tex, uint32 mip = 0)
            {
                return Texture2D(tex, mip, ERHIDSVFlags::ReadOnlyDepth);
            }

            static RHIDepthStencilViewDesc Texture2DMS(IRHITexture* tex, ERHIDSVFlags dsvFlags = ERHIDSVFlags::None)
            {
                RHIDepthStencilViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2DMS;
                desc.flags = dsvFlags;
                return desc;
            }

            static RHIDepthStencilViewDesc Texture2DArray(IRHITexture* tex,
                                                          uint32 mip,
                                                          uint32 firstSlice,
                                                          uint32 count = 1,
                                                          ERHIDSVFlags dsvFlags = ERHIDSVFlags::None)
            {
                RHIDepthStencilViewDesc desc;
                desc.texture = tex;
                desc.dimension = ERHITextureDimension::Texture2DArray;
                desc.mipSlice = mip;
                desc.firstArraySlice = firstSlice;
                desc.arraySize = count;
                desc.flags = dsvFlags;
                return desc;
            }

            static RHIDepthStencilViewDesc CubeFace(IRHITexture* tex, uint32 faceIndex, uint32 mip = 0)
            {
                return Texture2DArray(tex, mip, faceIndex, 1);
            }
        };

        //=========================================================================
        // IRHIDepthStencilView (05-04)
        //=========================================================================

        /// デプスステンシルビュー
        class RHI_API IRHIDepthStencilView : public IRHIResource
        {
        protected:
            IRHIDepthStencilView() : IRHIResource(ERHIResourceType::DepthStencilView) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(DepthStencilView)

            virtual ~IRHIDepthStencilView() = default;

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// CPUディスクリプタハンドル取得
            RHICPUDescriptorHandle GetCPUHandle() const { return m_cpuHandle; }

            /// ソーステクスチャ取得
            virtual IRHITexture* GetTexture() const = 0;

            /// MIPレベル取得
            virtual uint32 GetMipSlice() const = 0;

            /// 配列スライス取得
            virtual uint32 GetFirstArraySlice() const = 0;

            /// 配列サイズ取得
            virtual uint32 GetArraySize() const = 0;

            /// ビューの幅取得
            uint32 GetWidth() const;

            /// ビューの高さ取得
            uint32 GetHeight() const;

            Extent2D GetSize() const { return Extent2D{GetWidth(), GetHeight()}; }

            /// ビューフォーマット取得
            virtual ERHIPixelFormat GetFormat() const = 0;

            /// サンプル数取得
            ERHISampleCount GetSampleCount() const;

            /// DSVフラグ取得
            virtual ERHIDSVFlags GetFlags() const = 0;

            /// デプス読み取り専用か
            bool IsDepthReadOnly() const { return EnumHasAnyFlags(GetFlags(), ERHIDSVFlags::ReadOnlyDepth); }

            /// ステンシル読み取り専用か
            bool IsStencilReadOnly() const { return EnumHasAnyFlags(GetFlags(), ERHIDSVFlags::ReadOnlyStencil); }

            /// 完全に読み取り専用か
            bool IsReadOnly() const { return IsDepthReadOnly() && IsStencilReadOnly(); }

            /// デプスフォーマットを持つか
            bool HasDepth() const { return IsDepthFormat(GetFormat()); }

            /// ステンシルフォーマットを持つか
            bool HasStencil() const { return IsStencilFormat(GetFormat()); }

        protected:
            RHICPUDescriptorHandle m_cpuHandle{};
        };

        using RHIDepthStencilViewRef = TRefCountPtr<IRHIDepthStencilView>;

        //=========================================================================
        // ERHIClearDSFlags / RHIDSVClearValue (05-04)
        //=========================================================================

        /// デプスステンシルクリアフラグ
        enum class ERHIClearDSFlags : uint8
        {
            None = 0,
            Depth = 1 << 0,
            Stencil = 1 << 1,
            Both = Depth | Stencil,
        };
        RHI_ENUM_CLASS_FLAGS(ERHIClearDSFlags)

        /// デプスステンシルクリア値
        struct RHI_API RHIDSVClearValue
        {
            float depth = 1.0f;
            uint8 stencil = 0;
            ERHIClearDSFlags flags = ERHIClearDSFlags::Both;

            RHIDSVClearValue() = default;

            RHIDSVClearValue(float d, uint8 s = 0, ERHIClearDSFlags f = ERHIClearDSFlags::Both)
                : depth(d), stencil(s), flags(f)
            {}

            static RHIDSVClearValue DepthOnly(float d = 1.0f)
            {
                return RHIDSVClearValue(d, 0, ERHIClearDSFlags::Depth);
            }
            static RHIDSVClearValue StencilOnly(uint8 s = 0)
            {
                return RHIDSVClearValue(1.0f, s, ERHIClearDSFlags::Stencil);
            }
            static RHIDSVClearValue ReversedDepth(uint8 s = 0) { return RHIDSVClearValue(0.0f, s); }
        };

        //=========================================================================
        // RHIDepthBounds (05-04)
        //=========================================================================

        /// デプスバウンド（深度範囲テスト）
        struct RHI_API RHIDepthBounds
        {
            float minDepth = 0.0f;
            float maxDepth = 1.0f;

            bool IsEnabled() const { return minDepth > 0.0f || maxDepth < 1.0f; }
            static RHIDepthBounds Disabled() { return RHIDepthBounds{0.0f, 1.0f}; }
            static RHIDepthBounds Range(float min, float max) { return RHIDepthBounds{min, max}; }
        };

        //=========================================================================
        // 定数バッファビュー記述 (05-05)
        //=========================================================================

        /// 定数バッファビュー記述
        struct RHI_API RHIConstantBufferViewDesc
        {
            /// バッファ（nullの場合はGPUアドレスを直接使用）
            IRHIBuffer* buffer = nullptr;

            /// バッファ内オフセット（256バイトアライメント必要）
            MemoryOffset offset = 0;

            /// サイズ（256バイトアライメント必要、0 = バッファ全体）
            MemorySize size = 0;

            /// GPUアドレス直接指定（bufferがnullの場合に使用）
            uint64 gpuAddress = 0;

            /// バッファから作成
            static RHIConstantBufferViewDesc FromBuffer(IRHIBuffer* buf, MemoryOffset off = 0, MemorySize sz = 0)
            {
                RHIConstantBufferViewDesc desc;
                desc.buffer = buf;
                desc.offset = off;
                desc.size = sz;
                return desc;
            }

            /// GPUアドレスから作成
            static RHIConstantBufferViewDesc FromGPUAddress(uint64 address, MemorySize sz)
            {
                RHIConstantBufferViewDesc desc;
                desc.gpuAddress = address;
                desc.size = sz;
                return desc;
            }

            /// 有効なGPUアドレス取得（非インライン - IRHIBuffer未定義のため）
            uint64 GetEffectiveGPUAddress() const;

            /// 有効なサイズ取得（非インライン - IRHIBuffer未定義のため）
            MemorySize GetEffectiveSize() const;

            /// アライメント検証
            bool ValidateAlignment() const { return IsAligned(offset, kConstantBufferAlignment); }
        };

        //=========================================================================
        // IRHIConstantBufferView (05-05)
        //=========================================================================

        /// 定数バッファビュー
        class RHI_API IRHIConstantBufferView : public IRHIResource
        {
        protected:
            IRHIConstantBufferView() : IRHIResource(ERHIResourceType::ConstantBufferView) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(ConstantBufferView)

            virtual ~IRHIConstantBufferView() = default;

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// CPUディスクリプタハンドル取得
            RHICPUDescriptorHandle GetCPUHandle() const { return m_cpuHandle; }

            /// GPUディスクリプタハンドル取得
            RHIGPUDescriptorHandle GetGPUHandle() const { return m_gpuHandle; }

            /// Bindlessインデックス取得
            BindlessIndex GetBindlessIndex() const { return m_bindlessIndex; }

            /// ソースバッファ取得（nullの場合あり）
            virtual IRHIBuffer* GetBuffer() const = 0;

            /// GPUアドレス取得
            uint64 GetGPUVirtualAddress() const { return m_gpuVirtualAddress; }

            /// オフセット取得
            virtual MemoryOffset GetOffset() const = 0;

            /// サイズ取得
            virtual MemorySize GetSize() const = 0;

            /// データを更新（バッファがCPU書き込み可能な場合）
            bool UpdateData(const void* data, MemorySize dataSize, MemoryOffset localOffset = 0);

            /// 型付きデータを更新
            template <typename T> bool Update(const T& value) { return UpdateData(&value, sizeof(T)); }

        protected:
            RHICPUDescriptorHandle m_cpuHandle{};
            RHIGPUDescriptorHandle m_gpuHandle{};
            BindlessIndex m_bindlessIndex{};
            uint64 m_gpuVirtualAddress = 0;
        };

        using RHIConstantBufferViewRef = TRefCountPtr<IRHIConstantBufferView>;

        //=========================================================================
        // RHIRootConstants (05-05)
        //=========================================================================

        /// ルート定数データ
        struct RHI_API RHIRootConstants
        {
            /// データ（最大256バイト推奨）
            uint8 data[256];

            /// 使用サイズ（バイト）
            uint32 size = 0;

            /// DWORDカウント
            uint32 GetDWORDCount() const { return (size + 3) / 4; }

            /// データ設定
            template <typename T> void Set(const T& value)
            {
                static_assert(sizeof(T) <= sizeof(data), "Root constants too large");
                std::memcpy(data, &value, sizeof(T));
                size = sizeof(T);
            }

            /// 配列データ設定
            void SetRaw(const void* srcData, uint32 srcSize)
            {
                RHI_CHECK(srcSize <= sizeof(data));
                std::memcpy(data, srcData, srcSize);
                size = srcSize;
            }
        };

        //=========================================================================
        // RHIConstantBufferRing (05-05)
        //=========================================================================

        /// 定数バッファリングヘルパー
        /// フレームごとに異なるバッファを使用してCPU-GPU同期を回避
        template <typename T, uint32 BufferCount = 3> class RHIConstantBufferRing
        {
        public:
            RHIConstantBufferRing() = default;

            /// 初期化（実装はIRHIDevice/IRHIBufferが利用可能な.cppで行う）
            bool Initialize(IRHIDevice* device, const char* debugName = nullptr);

            /// フレーム開始時に呼び出す
            void BeginFrame() { m_currentIndex = (m_currentIndex + 1) % BufferCount; }

            /// 現在のバッファにデータを更新
            bool Update(const T& data);

            /// 現在のCBV取得
            IRHIConstantBufferView* GetCurrentCBV() const { return m_cbvs[m_currentIndex].Get(); }

            /// 現在のバッファ取得
            IRHIBuffer* GetCurrentBuffer() const { return m_buffers[m_currentIndex].Get(); }

            /// GPUアドレス取得
            uint64 GetCurrentGPUAddress() const;

        private:
            TRefCountPtr<IRHIBuffer> m_buffers[BufferCount];
            RHIConstantBufferViewRef m_cbvs[BufferCount];
            uint32 m_currentIndex = 0;
        };

    } // namespace RHI
} // namespace NS
