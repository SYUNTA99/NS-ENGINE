/// @file IRHISampler.h
/// @brief サンプラー記述・インターフェース・キャッシュ・マネージャー
/// @details テクスチャサンプラーの記述構造、IRHISamplerインターフェース、
///          サンプラーキャッシュ、グローバルマネージャーを提供。
/// @see 18-01-sampler-desc.md, 18-02-sampler-interface.md
#pragma once

#include "IRHIResource.h"
#include "IRHIRootSignature.h"
#include "RHIMacros.h"
#include "RHIResourceType.h"
#include "RHITypes.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // サンプラー列挙型 (18-01)
        //=========================================================================

        /// フィルターモード
        enum class ERHIFilter : uint8
        {
            Point,
            Linear,
            Anisotropic,
        };

        /// テクスチャアドレスモード
        enum class ERHITextureAddressMode : uint8
        {
            Wrap,
            Mirror,
            Clamp,
            Border,
            MirrorOnce,
        };

        /// ボーダーカラー
        enum class ERHIBorderColor : uint8
        {
            TransparentBlack, // (0, 0, 0, 0)
            OpaqueBlack,      // (0, 0, 0, 1)
            OpaqueWhite,      // (1, 1, 1, 1)
        };

        //=========================================================================
        // RHISamplerDesc (18-01)
        //=========================================================================

        /// サンプラー記述
        struct RHI_API RHISamplerDesc
        {
            /// 縮小フィルター
            ERHIFilter minFilter = ERHIFilter::Linear;

            /// 拡大フィルター
            ERHIFilter magFilter = ERHIFilter::Linear;

            /// ミップマップフィルター
            ERHIFilter mipFilter = ERHIFilter::Linear;

            /// アドレスモード
            ERHITextureAddressMode addressU = ERHITextureAddressMode::Wrap;
            ERHITextureAddressMode addressV = ERHITextureAddressMode::Wrap;
            ERHITextureAddressMode addressW = ERHITextureAddressMode::Wrap;

            /// MIPバイアス
            float mipLODBias = 0.0f;

            /// 最大異方性（Anisotropic時）
            uint32 maxAnisotropy = 16;

            /// 比較関数（シャドウマップ等）
            ERHICompareFunc comparisonFunc = ERHICompareFunc::Never;

            /// 比較サンプラーとして使用
            bool enableComparison = false;

            /// ボーダーカラー
            ERHIBorderColor borderColor = ERHIBorderColor::OpaqueBlack;

            /// カスタムボーダーカラー（ERHIBorderColor未使用時）
            float customBorderColor[4] = {0, 0, 0, 1};
            bool useCustomBorderColor = false;

            /// 最小LOD
            float minLOD = 0.0f;

            /// 最大LOD
            float maxLOD = 1000.0f;

            //=====================================================================
            // プリセット
            //=====================================================================

            /// ポイントサンプラー
            static RHISamplerDesc Point()
            {
                RHISamplerDesc desc;
                desc.minFilter = ERHIFilter::Point;
                desc.magFilter = ERHIFilter::Point;
                desc.mipFilter = ERHIFilter::Point;
                return desc;
            }

            /// ポイントクランプ
            static RHISamplerDesc PointClamp()
            {
                RHISamplerDesc desc = Point();
                desc.addressU = ERHITextureAddressMode::Clamp;
                desc.addressV = ERHITextureAddressMode::Clamp;
                desc.addressW = ERHITextureAddressMode::Clamp;
                return desc;
            }

            /// リニアサンプラー
            static RHISamplerDesc Linear()
            {
                RHISamplerDesc desc;
                desc.minFilter = ERHIFilter::Linear;
                desc.magFilter = ERHIFilter::Linear;
                desc.mipFilter = ERHIFilter::Linear;
                return desc;
            }

            /// リニアクランプ
            static RHISamplerDesc LinearClamp()
            {
                RHISamplerDesc desc = Linear();
                desc.addressU = ERHITextureAddressMode::Clamp;
                desc.addressV = ERHITextureAddressMode::Clamp;
                desc.addressW = ERHITextureAddressMode::Clamp;
                return desc;
            }

            /// 異方性サンプラー
            static RHISamplerDesc Anisotropic(uint32 maxAniso = 16)
            {
                RHISamplerDesc desc;
                desc.minFilter = ERHIFilter::Anisotropic;
                desc.magFilter = ERHIFilter::Anisotropic;
                desc.mipFilter = ERHIFilter::Linear;
                desc.maxAnisotropy = maxAniso;
                return desc;
            }

            /// シャドウマップ（PCF）
            static RHISamplerDesc ShadowPCF()
            {
                RHISamplerDesc desc;
                desc.minFilter = ERHIFilter::Linear;
                desc.magFilter = ERHIFilter::Linear;
                desc.mipFilter = ERHIFilter::Point;
                desc.addressU = ERHITextureAddressMode::Border;
                desc.addressV = ERHITextureAddressMode::Border;
                desc.addressW = ERHITextureAddressMode::Border;
                desc.borderColor = ERHIBorderColor::OpaqueWhite;
                desc.enableComparison = true;
                desc.comparisonFunc = ERHICompareFunc::LessEqual;
                return desc;
            }

            /// シャドウマップ（ポイント）
            static RHISamplerDesc ShadowPoint()
            {
                RHISamplerDesc desc = ShadowPCF();
                desc.minFilter = ERHIFilter::Point;
                desc.magFilter = ERHIFilter::Point;
                return desc;
            }
        };

        //=========================================================================
        // サンプラーハッシュ・比較 (18-01)
        //=========================================================================

        /// サンプラー記述のハッシュ計算
        RHI_API uint64 CalculateSamplerDescHash(const RHISamplerDesc& desc);

        /// サンプラー記述の比較（フィールド完全比較）
        inline bool operator==(const RHISamplerDesc& a, const RHISamplerDesc& b)
        {
            return a.minFilter == b.minFilter && a.magFilter == b.magFilter && a.mipFilter == b.mipFilter &&
                   a.addressU == b.addressU && a.addressV == b.addressV && a.addressW == b.addressW &&
                   a.mipLODBias == b.mipLODBias && a.maxAnisotropy == b.maxAnisotropy &&
                   a.comparisonFunc == b.comparisonFunc && a.enableComparison == b.enableComparison &&
                   a.borderColor == b.borderColor && a.customBorderColor[0] == b.customBorderColor[0] &&
                   a.customBorderColor[1] == b.customBorderColor[1] &&
                   a.customBorderColor[2] == b.customBorderColor[2] &&
                   a.customBorderColor[3] == b.customBorderColor[3] &&
                   a.useCustomBorderColor == b.useCustomBorderColor && a.minLOD == b.minLOD && a.maxLOD == b.maxLOD;
        }

        inline bool operator!=(const RHISamplerDesc& a, const RHISamplerDesc& b)
        {
            return !(a == b);
        }

        //=========================================================================
        // RHISamplerBuilder (18-01)
        //=========================================================================

        /// サンプラービルダー
        class RHI_API RHISamplerBuilder
        {
        public:
            RHISamplerBuilder() = default;

            /// フィルター設定
            RHISamplerBuilder& SetFilter(ERHIFilter min, ERHIFilter mag, ERHIFilter mip)
            {
                m_desc.minFilter = min;
                m_desc.magFilter = mag;
                m_desc.mipFilter = mip;
                return *this;
            }

            /// 全フィルター同一設定
            RHISamplerBuilder& SetFilter(ERHIFilter filter) { return SetFilter(filter, filter, filter); }

            /// 異方性設定
            RHISamplerBuilder& SetAnisotropic(uint32 maxAniso = 16)
            {
                m_desc.minFilter = ERHIFilter::Anisotropic;
                m_desc.magFilter = ERHIFilter::Anisotropic;
                m_desc.maxAnisotropy = maxAniso;
                return *this;
            }

            /// アドレスモード設定
            RHISamplerBuilder& SetAddressMode(ERHITextureAddressMode u,
                                              ERHITextureAddressMode v,
                                              ERHITextureAddressMode w)
            {
                m_desc.addressU = u;
                m_desc.addressV = v;
                m_desc.addressW = w;
                return *this;
            }

            /// 全アドレスモード同一設定
            RHISamplerBuilder& SetAddressMode(ERHITextureAddressMode mode) { return SetAddressMode(mode, mode, mode); }

            /// MIPバイアス設定
            RHISamplerBuilder& SetMipLODBias(float bias)
            {
                m_desc.mipLODBias = bias;
                return *this;
            }

            /// LOD範囲設定
            RHISamplerBuilder& SetLODRange(float minLOD, float maxLOD)
            {
                m_desc.minLOD = minLOD;
                m_desc.maxLOD = maxLOD;
                return *this;
            }

            /// 比較関数設定
            RHISamplerBuilder& SetComparison(ERHICompareFunc func)
            {
                m_desc.enableComparison = true;
                m_desc.comparisonFunc = func;
                return *this;
            }

            /// ボーダーカラー設定
            RHISamplerBuilder& SetBorderColor(ERHIBorderColor color)
            {
                m_desc.borderColor = color;
                m_desc.useCustomBorderColor = false;
                return *this;
            }

            /// カスタムボーダーカラー設定
            RHISamplerBuilder& SetCustomBorderColor(float r, float g, float b, float a)
            {
                m_desc.customBorderColor[0] = r;
                m_desc.customBorderColor[1] = g;
                m_desc.customBorderColor[2] = b;
                m_desc.customBorderColor[3] = a;
                m_desc.useCustomBorderColor = true;
                return *this;
            }

            /// ビルド
            RHISamplerDesc Build() const { return m_desc; }

        private:
            RHISamplerDesc m_desc;
        };

        //=========================================================================
        // IRHISampler (18-02)
        //=========================================================================

        /// サンプラーインターフェース
        class RHI_API IRHISampler : public IRHIResource
        {
        protected:
            IRHISampler() : IRHIResource(ERHIResourceType::Sampler) {}

        public:
            DECLARE_RHI_RESOURCE_TYPE(Sampler)

            virtual ~IRHISampler() = default;

            //=====================================================================
            // 基本プロパティ
            //=====================================================================

            /// 所属デバイス取得
            virtual IRHIDevice* GetDevice() const = 0;

            /// 記述取得
            virtual const RHISamplerDesc& GetDesc() const = 0;

            //=====================================================================
            // フィルター情報
            //=====================================================================

            /// 縮小フィルター取得
            ERHIFilter GetMinFilter() const { return GetDesc().minFilter; }

            /// 拡大フィルター取得
            ERHIFilter GetMagFilter() const { return GetDesc().magFilter; }

            /// ミップフィルター取得
            ERHIFilter GetMipFilter() const { return GetDesc().mipFilter; }

            /// 異方性サンプラーか
            bool IsAnisotropic() const
            {
                return GetMinFilter() == ERHIFilter::Anisotropic || GetMagFilter() == ERHIFilter::Anisotropic;
            }

            /// 最大異方性取得
            uint32 GetMaxAnisotropy() const { return GetDesc().maxAnisotropy; }

            //=====================================================================
            // アドレスモード情報
            //=====================================================================

            /// Uアドレスモード取得
            ERHITextureAddressMode GetAddressU() const { return GetDesc().addressU; }

            /// Vアドレスモード取得
            ERHITextureAddressMode GetAddressV() const { return GetDesc().addressV; }

            /// Wアドレスモード取得
            ERHITextureAddressMode GetAddressW() const { return GetDesc().addressW; }

            //=====================================================================
            // 比較サンプラー情報
            //=====================================================================

            /// 比較サンプラーか
            bool IsComparisonSampler() const { return GetDesc().enableComparison; }

            /// 比較関数取得
            ERHICompareFunc GetComparisonFunc() const { return GetDesc().comparisonFunc; }

            //=====================================================================
            // ディスクリプタ
            //=====================================================================

            /// CPUディスクリプタハンドル取得
            virtual RHICPUDescriptorHandle GetCPUDescriptorHandle() const = 0;
        };

        using RHISamplerRef = TRefCountPtr<IRHISampler>;

        //=========================================================================
        // RHISamplerCache (18-02)
        //=========================================================================

        /// サンプラーキャッシュ
        /// 同一記述のサンプラーを再利用
        class RHI_API RHISamplerCache
        {
        public:
            RHISamplerCache() = default;

            /// 初期化
            bool Initialize(IRHIDevice* device, uint32 maxCachedSamplers = 256);

            /// シャットダウン
            void Shutdown();

            //=====================================================================
            // サンプラー取得
            //=====================================================================

            /// サンプラー取得（キャッシュヒット時は既存を返す）
            IRHISampler* GetOrCreate(const RHISamplerDesc& desc);

            /// プリセットサンプラー取得
            IRHISampler* GetPointSampler();
            IRHISampler* GetPointClampSampler();
            IRHISampler* GetLinearSampler();
            IRHISampler* GetLinearClampSampler();
            IRHISampler* GetAnisotropicSampler(uint32 maxAniso = 16);
            IRHISampler* GetShadowPCFSampler();

            //=====================================================================
            // キャッシュ管理
            //=====================================================================

            /// キャッシュクリア
            void Clear();

            /// キャッシュ統計
            struct CacheStats
            {
                uint32 cachedCount;
                uint32 hitCount;
                uint32 missCount;
            };
            CacheStats GetStats() const { return m_stats; }

            /// 統計リセット
            void ResetStats() { m_stats = {}; }

        private:
            IRHIDevice* m_device = nullptr;

            struct CacheEntry
            {
                uint64 hash;
                RHISamplerRef sampler;
            };
            CacheEntry* m_cache = nullptr;
            uint32 m_cacheCount = 0;
            uint32 m_cacheCapacity = 0;

            CacheStats m_stats = {};

            /// プリセットサンプラー
            RHISamplerRef m_pointSampler;
            RHISamplerRef m_pointClampSampler;
            RHISamplerRef m_linearSampler;
            RHISamplerRef m_linearClampSampler;
            RHISamplerRef m_shadowPCFSampler;
        };

        //=========================================================================
        // RHISamplerManager (18-02)
        //=========================================================================

        /// グローバルサンプラーマネージャー
        /// デバイス全体でのサンプラー管理
        class RHI_API RHISamplerManager
        {
        public:
            RHISamplerManager() = default;

            /// 初期化
            bool Initialize(IRHIDevice* device);

            /// シャットダウン
            void Shutdown();

            //=====================================================================
            // サンプラー取得
            //=====================================================================

            /// 記述からサンプラー取得
            IRHISampler* GetSampler(const RHISamplerDesc& desc);

            /// 名前付きサンプラー登録
            void RegisterSampler(const char* name, IRHISampler* sampler);

            /// 名前付きサンプラー取得
            IRHISampler* GetSampler(const char* name) const;

            //=====================================================================
            // プリセット
            //=====================================================================

            /// 共通サンプラーへのアクセス
            IRHISampler* GetPointSampler() { return m_cache.GetPointSampler(); }
            IRHISampler* GetLinearSampler() { return m_cache.GetLinearSampler(); }
            IRHISampler* GetAnisotropicSampler() { return m_cache.GetAnisotropicSampler(); }
            IRHISampler* GetShadowSampler() { return m_cache.GetShadowPCFSampler(); }

            //=====================================================================
            // Bindless
            //=====================================================================

            /// サンプラーをBindlessヒープに登録
            BindlessSamplerIndex RegisterBindless(IRHISampler* sampler);

            /// Bindless登録解除
            void UnregisterBindless(BindlessSamplerIndex index);

        private:
            IRHIDevice* m_device = nullptr;
            RHISamplerCache m_cache;

            /// 名前→サンプラーマッピング
            struct NamedSampler
            {
                char name[64];
                IRHISampler* sampler;
            };
            NamedSampler* m_namedSamplers = nullptr;
            uint32 m_namedCount = 0;
        };

        //=========================================================================
        // RHISamplerConversion (18-02)
        //=========================================================================

        /// 静的サンプラーとIRHISamplerの変換
        namespace RHISamplerConversion
        {
            /// ERHIFilter → ERHIFilterMode 変換
            inline ERHIFilterMode ToFilterMode(ERHIFilter filter)
            {
                switch (filter)
                {
                case ERHIFilter::Point:
                    return ERHIFilterMode::Point;
                case ERHIFilter::Linear:
                    return ERHIFilterMode::Linear;
                case ERHIFilter::Anisotropic:
                    return ERHIFilterMode::Anisotropic;
                default:
                    return ERHIFilterMode::Linear;
                }
            }

            /// ERHIFilterMode → ERHIFilter 変換
            inline ERHIFilter FromFilterMode(ERHIFilterMode mode)
            {
                switch (mode)
                {
                case ERHIFilterMode::Point:
                    return ERHIFilter::Point;
                case ERHIFilterMode::Linear:
                    return ERHIFilter::Linear;
                case ERHIFilterMode::Anisotropic:
                    return ERHIFilter::Anisotropic;
                default:
                    return ERHIFilter::Linear;
                }
            }

            /// ERHITextureAddressMode → ERHIAddressMode 変換
            inline ERHIAddressMode ToAddressMode(ERHITextureAddressMode mode)
            {
                return static_cast<ERHIAddressMode>(mode);
            }

            /// ERHIAddressMode → ERHITextureAddressMode 変換
            inline ERHITextureAddressMode FromAddressMode(ERHIAddressMode mode)
            {
                return static_cast<ERHITextureAddressMode>(mode);
            }

            /// RHISamplerDescからRHIStaticSamplerDescへ変換
            inline RHIStaticSamplerDesc ToStaticSampler(const RHISamplerDesc& desc,
                                                        uint32 shaderRegister,
                                                        uint32 registerSpace = 0,
                                                        EShaderVisibility visibility = EShaderVisibility::All)
            {
                RHIStaticSamplerDesc staticDesc;
                staticDesc.shaderRegister = shaderRegister;
                staticDesc.registerSpace = registerSpace;
                staticDesc.shaderVisibility = visibility;

                staticDesc.filter = ToFilterMode(desc.minFilter);
                staticDesc.addressU = ToAddressMode(desc.addressU);
                staticDesc.addressV = ToAddressMode(desc.addressV);
                staticDesc.addressW = ToAddressMode(desc.addressW);

                staticDesc.mipLODBias = desc.mipLODBias;
                staticDesc.maxAnisotropy = desc.maxAnisotropy;
                staticDesc.comparisonFunc = desc.comparisonFunc;
                staticDesc.minLOD = desc.minLOD;
                staticDesc.maxLOD = desc.maxLOD;

                // ボーダーカラー変換
                switch (desc.borderColor)
                {
                case ERHIBorderColor::TransparentBlack:
                    staticDesc.borderColor = RHIStaticSamplerDesc::BorderColor::TransparentBlack;
                    break;
                case ERHIBorderColor::OpaqueBlack:
                    staticDesc.borderColor = RHIStaticSamplerDesc::BorderColor::OpaqueBlack;
                    break;
                case ERHIBorderColor::OpaqueWhite:
                    staticDesc.borderColor = RHIStaticSamplerDesc::BorderColor::OpaqueWhite;
                    break;
                }

                return staticDesc;
            }

            /// RHIStaticSamplerDescからRHISamplerDescへ変換
            inline RHISamplerDesc FromStaticSampler(const RHIStaticSamplerDesc& staticDesc)
            {
                RHISamplerDesc desc;

                ERHIFilter filter = FromFilterMode(staticDesc.filter);
                desc.minFilter = filter;
                desc.magFilter = filter;
                desc.mipFilter = ERHIFilter::Linear;

                desc.addressU = FromAddressMode(staticDesc.addressU);
                desc.addressV = FromAddressMode(staticDesc.addressV);
                desc.addressW = FromAddressMode(staticDesc.addressW);

                desc.mipLODBias = staticDesc.mipLODBias;
                desc.maxAnisotropy = staticDesc.maxAnisotropy;
                desc.comparisonFunc = staticDesc.comparisonFunc;
                desc.minLOD = staticDesc.minLOD;
                desc.maxLOD = staticDesc.maxLOD;

                // ボーダーカラー変換
                switch (staticDesc.borderColor)
                {
                case RHIStaticSamplerDesc::BorderColor::TransparentBlack:
                    desc.borderColor = ERHIBorderColor::TransparentBlack;
                    break;
                case RHIStaticSamplerDesc::BorderColor::OpaqueBlack:
                    desc.borderColor = ERHIBorderColor::OpaqueBlack;
                    break;
                case RHIStaticSamplerDesc::BorderColor::OpaqueWhite:
                    desc.borderColor = ERHIBorderColor::OpaqueWhite;
                    break;
                }

                return desc;
            }
        } // namespace RHISamplerConversion

    } // namespace RHI
} // namespace NS
