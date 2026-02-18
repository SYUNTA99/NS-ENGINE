/// @file IRHIAdapter.h
/// @brief GPUアダプターインターフェース
/// @details 物理GPU（グラフィックスカード）を抽象化。機能クエリ、制限値、メモリ情報を提供。
/// @see 01-19-adapter-interface.md
#pragma once

#include "RHIAdapterDesc.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIPixelFormat.h"

namespace NS
{
    namespace RHI
    {
        /// 前方宣言
        class IRHIPipelineStateCache;
        class IRHIRootSignatureManager;

        /// GPUアダプターインターフェース
        /// 物理GPU（グラフィックスカード）を表す
        class RHI_API IRHIAdapter
        {
        public:
            virtual ~IRHIAdapter() = default;

            //=====================================================================
            // 基本アクセス
            //=====================================================================

            /// アダプター記述情報を取得
            virtual const RHIAdapterDesc& GetDesc() const = 0;

            /// アダプターインデックス取得
            uint32 GetAdapterIndex() const { return GetDesc().adapterIndex; }

            /// GPU名取得
            const char* GetDeviceName() const { return GetDesc().deviceName.c_str(); }

            /// ベンダーID取得
            uint32 GetVendorId() const { return GetDesc().vendorId; }

            //=====================================================================
            // デバイス管理
            //=====================================================================

            /// デバイスノード数取得
            virtual uint32 GetDeviceCount() const = 0;

            /// デバイス取得
            virtual IRHIDevice* GetDevice(uint32 index) const = 0;

            /// プライマリデバイス取得（インデックス0）
            IRHIDevice* GetPrimaryDevice() const { return GetDevice(0); }

            //=====================================================================
            // 機能クエリ
            //=====================================================================

            /// 機能サポート確認
            virtual ERHIFeatureSupport SupportsFeature(ERHIFeature feature) const = 0;

            /// 機能がサポートされているか（簡易版）
            bool IsFeatureSupported(ERHIFeature feature) const
            {
                return SupportsFeature(feature) != ERHIFeatureSupport::Unsupported;
            }

            /// 最大機能レベル取得
            ERHIFeatureLevel GetMaxFeatureLevel() const { return GetDesc().maxFeatureLevel; }

            /// 指定機能レベルをサポートするか
            bool SupportsFeatureLevel(ERHIFeatureLevel level) const { return GetMaxFeatureLevel() >= level; }

            //=====================================================================
            // 制限値クエリ
            //=====================================================================

            /// 最大テクスチャサイズ
            virtual uint32 GetMaxTextureSize() const = 0;

            /// 最大テクスチャ配列サイズ
            virtual uint32 GetMaxTextureArrayLayers() const = 0;

            /// 最大3Dテクスチャサイズ
            virtual uint32 GetMaxTexture3DSize() const = 0;

            /// 最大バッファサイズ
            virtual uint64 GetMaxBufferSize() const = 0;

            /// 最大コンスタントバッファサイズ
            virtual uint32 GetMaxConstantBufferSize() const = 0;

            /// 定数バッファアライメント
            virtual uint32 GetConstantBufferAlignment() const = 0;

            /// 構造化バッファストライドアライメント
            virtual uint32 GetStructuredBufferAlignment() const = 0;

            /// 最大サンプルカウント取得
            virtual ERHISampleCount GetMaxSampleCount(ERHIPixelFormat format) const = 0;

            //=====================================================================
            // メモリ情報
            //=====================================================================

            /// 専用ビデオメモリ取得
            uint64 GetDedicatedVideoMemory() const { return GetDesc().dedicatedVideoMemory; }

            /// 共有システムメモリ取得
            uint64 GetSharedSystemMemory() const { return GetDesc().sharedSystemMemory; }

            /// 統合メモリアーキテクチャか
            bool HasUnifiedMemory() const { return GetDesc().unifiedMemory; }

            //=====================================================================
            // 共有リソース管理
            //=====================================================================

            /// パイプラインステートキャッシュ取得
            virtual IRHIPipelineStateCache* GetPipelineStateCache() = 0;

            /// ルートシグネチャマネージャー取得
            virtual IRHIRootSignatureManager* GetRootSignatureManager() = 0;

            //=====================================================================
            // 出力（モニター）管理
            //=====================================================================

            /// 接続されている出力数
            virtual uint32 GetOutputCount() const = 0;

            /// 出力がHDRをサポートするか
            virtual bool OutputSupportsHDR(uint32 outputIndex) const = 0;

            //=====================================================================
            // 出力情報 (12-01)
            //=====================================================================

            /// 出力情報取得
            virtual bool GetOutputInfo(uint32 index, struct RHIOutputInfo& outInfo) const = 0;

            /// 出力のサポートされるディスプレイモード列挙
            virtual uint32 EnumerateDisplayModes(uint32 outputIndex,
                                                 ERHIPixelFormat format,
                                                 struct RHIDisplayMode* outModes,
                                                 uint32 maxModes) const = 0;

            /// 最適なディスプレイモード取得
            virtual bool FindClosestDisplayMode(uint32 outputIndex,
                                                const struct RHIDisplayMode& target,
                                                struct RHIDisplayMode& outClosest) const = 0;

            //=====================================================================
            // HDR出力能力 (12-04)
            //=====================================================================

            /// HDR出力能力取得
            virtual bool GetHDROutputCapabilities(uint32 outputIndex,
                                                  struct RHIHDROutputCapabilities& outCapabilities) const = 0;
        };

    } // namespace RHI
} // namespace NS
