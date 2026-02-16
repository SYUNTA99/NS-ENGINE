/// @file IRHIRootSignature.h
/// @brief ルートシグネチャインターフェース
/// @details ルートパラメータ、ディスクリプタレンジ、静的サンプラー、ルートシグネチャを定義。
/// @see 08-01-root-parameter.md, 08-02-descriptor-range.md, 08-03-root-signature.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIRootParameterType (08-01)
    //=========================================================================

    /// ルートパラメータタイプ
    enum class ERHIRootParameterType : uint8
    {
        DescriptorTable, ///< ディスクリプタテーブル（複数ディスクリプタ）
        Constants,       ///< ルート定数（32bit値、直接埋め込み）
        CBV,             ///< ルートCBV（アドレス直接）
        SRV,             ///< ルートSRV（アドレス直接）
        UAV,             ///< ルートUAV（アドレス直接）
    };

    //=========================================================================
    // RHIRootConstantsDesc (08-01)
    //=========================================================================

    /// ルート定数パラメータ記述
    /// @note RHIRootConstants（IRHIViews.h）はデータ格納用。こちらはパラメータ定義用。
    struct RHI_API RHIRootConstantsDesc
    {
        uint32 shaderRegister = 0; ///< バインドするシェーダーレジスタ（b0, b1, ...）
        uint32 registerSpace = 0;  ///< レジスタスペース
        uint32 num32BitValues = 0; ///< 32bit定数の数

        /// ビルダー
        static RHIRootConstantsDesc Create(uint32 reg, uint32 numValues, uint32 space = 0)
        {
            RHIRootConstantsDesc rc;
            rc.shaderRegister = reg;
            rc.registerSpace = space;
            rc.num32BitValues = numValues;
            return rc;
        }

        /// テンプレート版（構造体サイズから自動計算）
        template <typename T> static RHIRootConstantsDesc CreateForType(uint32 reg, uint32 space = 0)
        {
            static_assert(sizeof(T) % 4 == 0, "Size must be multiple of 4 bytes");
            return Create(reg, sizeof(T) / 4, space);
        }
    };

    //=========================================================================
    // RHIRootDescriptor (08-01)
    //=========================================================================

    /// ルートデスクリプタパラメータ記述
    /// ルートCBV/SRV/UAV用
    struct RHI_API RHIRootDescriptor
    {
        uint32 shaderRegister = 0; ///< バインドするシェーダーレジスタ
        uint32 registerSpace = 0;  ///< レジスタスペース

        /// フラグ
        enum class Flags : uint32
        {
            None = 0,
            DataStatic = 1 << 0,         ///< データが静的（コマンドリスト実行中に変更されない）
            DataVolatile = 1 << 1,       ///< データが揮発性（毎描画変更）
            DescriptorsVolatile = 1 << 2 ///< ディスクリプタが揮発性
        };
        Flags flags = Flags::None;

        static RHIRootDescriptor CBV(uint32 reg, uint32 space = 0, Flags f = Flags::None)
        {
            RHIRootDescriptor rd;
            rd.shaderRegister = reg;
            rd.registerSpace = space;
            rd.flags = f;
            return rd;
        }

        static RHIRootDescriptor SRV(uint32 reg, uint32 space = 0, Flags f = Flags::None) { return CBV(reg, space, f); }

        static RHIRootDescriptor UAV(uint32 reg, uint32 space = 0, Flags f = Flags::None) { return CBV(reg, space, f); }
    };
    RHI_ENUM_CLASS_FLAGS(RHIRootDescriptor::Flags)

    //=========================================================================
    // RHIRootParameter (08-01)
    //=========================================================================

    /// 前方宣言
    struct RHIDescriptorRange;

    /// ルートパラメータ記述
    struct RHI_API RHIRootParameter
    {
        ERHIRootParameterType parameterType = ERHIRootParameterType::DescriptorTable;
        EShaderVisibility shaderVisibility = EShaderVisibility::All;

        /// パラメータデータ
        union
        {
            struct
            {
                const RHIDescriptorRange* ranges; ///< ディスクリプタレンジ配列
                uint32 rangeCount;                ///< レンジ数
            } descriptorTable;

            RHIRootConstantsDesc constants; ///< ルート定数用

            RHIRootDescriptor descriptor; ///< ルートデスクリプタ用
        };

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ディスクリプタテーブルパラメータ
        static RHIRootParameter DescriptorTable(const RHIDescriptorRange* ranges,
                                                uint32 rangeCount,
                                                EShaderVisibility visibility = EShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::DescriptorTable;
            param.shaderVisibility = visibility;
            param.descriptorTable.ranges = ranges;
            param.descriptorTable.rangeCount = rangeCount;
            return param;
        }

        /// ルート定数パラメータ
        static RHIRootParameter Constants(uint32 shaderRegister,
                                          uint32 num32BitValues,
                                          uint32 registerSpace = 0,
                                          EShaderVisibility visibility = EShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::Constants;
            param.shaderVisibility = visibility;
            param.constants = RHIRootConstantsDesc::Create(shaderRegister, num32BitValues, registerSpace);
            return param;
        }

        /// ルートCBVパラメータ
        static RHIRootParameter CBV(uint32 shaderRegister,
                                    uint32 registerSpace = 0,
                                    EShaderVisibility visibility = EShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::CBV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::CBV(shaderRegister, registerSpace);
            return param;
        }

        /// ルートSRVパラメータ
        static RHIRootParameter SRV(uint32 shaderRegister,
                                    uint32 registerSpace = 0,
                                    EShaderVisibility visibility = EShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::SRV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::SRV(shaderRegister, registerSpace);
            return param;
        }

        /// ルートUAVパラメータ
        static RHIRootParameter UAV(uint32 shaderRegister,
                                    uint32 registerSpace = 0,
                                    EShaderVisibility visibility = EShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::UAV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::UAV(shaderRegister, registerSpace);
            return param;
        }
    };

    //=========================================================================
    // ルートパラメータコスト計算 (08-01)
    //=========================================================================

    /// ルートシグネチャコスト制限
    constexpr uint32 kMaxRootSignatureCost = 64; // DWORDs

    /// ルートパラメータコスト取得
    /// D3D12: ルートシグネチャは最大64 DWORD
    inline uint32 GetRootParameterCost(const RHIRootParameter& param)
    {
        switch (param.parameterType)
        {
        case ERHIRootParameterType::DescriptorTable:
            return 1; // GPUハンドル = 1 DWORD
        case ERHIRootParameterType::Constants:
            return param.constants.num32BitValues; // N DWORDs
        case ERHIRootParameterType::CBV:
        case ERHIRootParameterType::SRV:
        case ERHIRootParameterType::UAV:
            return 2; // 64bitアドレス = 2 DWORDs
        default:
            return 0;
        }
    }

    /// ルートパラメータ配列のコスト合計計算
    inline uint32 CalculateTotalRootParameterCost(const RHIRootParameter* params, uint32 paramCount)
    {
        uint32 totalCost = 0;
        for (uint32 i = 0; i < paramCount; ++i)
        {
            totalCost += GetRootParameterCost(params[i]);
        }
        return totalCost;
    }

    /// コスト制限の検証
    inline bool ValidateRootSignatureCost(const RHIRootParameter* params, uint32 paramCount)
    {
        return CalculateTotalRootParameterCost(params, paramCount) <= kMaxRootSignatureCost;
    }

    //=========================================================================
    // RHIDescriptorRange (08-02)
    //=========================================================================

    /// 無制限ディスクリプタ数
    constexpr uint32 kUnboundedDescriptorCount = ~0u;

    /// ディスクリプタレンジ記述
    struct RHI_API RHIDescriptorRange
    {
        ERHIDescriptorRangeType rangeType = ERHIDescriptorRangeType::SRV;
        uint32 numDescriptors = 1;                    ///< ディスクリプタ数（kUnboundedDescriptorCountで無制限）
        uint32 baseShaderRegister = 0;                ///< ベースシェーダーレジスタ
        uint32 registerSpace = 0;                     ///< レジスタスペース
        uint32 offsetInDescriptorsFromTableStart = 0; ///< テーブル内オフセット

        /// オフセット自動計算用定数
        static constexpr uint32 kAppendFromTableStart = ~0u;

        /// フラグ
        enum class Flags : uint32
        {
            None = 0,
            DescriptorsVolatile = 1 << 0,         ///< ディスクリプタが揮発性
            DataVolatile = 1 << 1,                ///< データが揮発性
            DataStatic = 1 << 2,                  ///< データが静的
            DataStaticWhileSetAtExecute = 1 << 3, ///< 実行開始時点で有効
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIDescriptorRange SRV(uint32 baseReg,
                                      uint32 count = 1,
                                      uint32 space = 0,
                                      uint32 offset = kAppendFromTableStart)
        {
            RHIDescriptorRange range;
            range.rangeType = ERHIDescriptorRangeType::SRV;
            range.numDescriptors = count;
            range.baseShaderRegister = baseReg;
            range.registerSpace = space;
            range.offsetInDescriptorsFromTableStart = offset;
            return range;
        }

        static RHIDescriptorRange UAV(uint32 baseReg,
                                      uint32 count = 1,
                                      uint32 space = 0,
                                      uint32 offset = kAppendFromTableStart)
        {
            RHIDescriptorRange range;
            range.rangeType = ERHIDescriptorRangeType::UAV;
            range.numDescriptors = count;
            range.baseShaderRegister = baseReg;
            range.registerSpace = space;
            range.offsetInDescriptorsFromTableStart = offset;
            return range;
        }

        static RHIDescriptorRange CBV(uint32 baseReg,
                                      uint32 count = 1,
                                      uint32 space = 0,
                                      uint32 offset = kAppendFromTableStart)
        {
            RHIDescriptorRange range;
            range.rangeType = ERHIDescriptorRangeType::CBV;
            range.numDescriptors = count;
            range.baseShaderRegister = baseReg;
            range.registerSpace = space;
            range.offsetInDescriptorsFromTableStart = offset;
            return range;
        }

        static RHIDescriptorRange Sampler(uint32 baseReg,
                                          uint32 count = 1,
                                          uint32 space = 0,
                                          uint32 offset = kAppendFromTableStart)
        {
            RHIDescriptorRange range;
            range.rangeType = ERHIDescriptorRangeType::Sampler;
            range.numDescriptors = count;
            range.baseShaderRegister = baseReg;
            range.registerSpace = space;
            range.offsetInDescriptorsFromTableStart = offset;
            return range;
        }

        /// 無制限SRV（Bindless用）
        static RHIDescriptorRange UnboundedSRV(uint32 baseReg, uint32 space = 0)
        {
            return SRV(baseReg, kUnboundedDescriptorCount, space);
        }

        /// 無制限UAV（Bindless用）
        static RHIDescriptorRange UnboundedUAV(uint32 baseReg, uint32 space = 0)
        {
            return UAV(baseReg, kUnboundedDescriptorCount, space);
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIDescriptorRange::Flags)

    //=========================================================================
    // RHIDescriptorTableDesc (08-02)
    //=========================================================================

    /// ディスクリプタテーブル記述
    struct RHI_API RHIDescriptorTableDesc
    {
        const RHIDescriptorRange* ranges = nullptr;
        uint32 rangeCount = 0;

        /// 配列から作成
        template <uint32 N> static RHIDescriptorTableDesc FromArray(const RHIDescriptorRange (&arr)[N])
        {
            RHIDescriptorTableDesc desc;
            desc.ranges = arr;
            desc.rangeCount = N;
            return desc;
        }

        /// 総ディスクリプタ数計算（無制限レンジ除く）
        uint32 CalculateTotalDescriptorCount() const
        {
            uint32 total = 0;
            for (uint32 i = 0; i < rangeCount; ++i)
            {
                if (ranges[i].numDescriptors != kUnboundedDescriptorCount)
                {
                    total += ranges[i].numDescriptors;
                }
            }
            return total;
        }

        /// 無制限レンジを含むか
        bool HasUnboundedRange() const
        {
            for (uint32 i = 0; i < rangeCount; ++i)
            {
                if (ranges[i].numDescriptors == kUnboundedDescriptorCount)
                {
                    return true;
                }
            }
            return false;
        }
    };

    //=========================================================================
    // RHIDescriptorTableBuilder (08-02)
    //=========================================================================

    /// 最大ディスクリプタレンジ数
    constexpr uint32 kMaxDescriptorRanges = 32;

    /// ディスクリプタテーブルビルダー
    class RHI_API RHIDescriptorTableBuilder
    {
    public:
        RHIDescriptorTableBuilder() = default;

        /// レンジ追加
        RHIDescriptorTableBuilder& Add(const RHIDescriptorRange& range)
        {
            if (m_count < kMaxDescriptorRanges)
            {
                m_ranges[m_count++] = range;
            }
            return *this;
        }

        RHIDescriptorTableBuilder& AddSRV(uint32 baseReg, uint32 count = 1, uint32 space = 0)
        {
            return Add(RHIDescriptorRange::SRV(baseReg, count, space));
        }

        RHIDescriptorTableBuilder& AddUAV(uint32 baseReg, uint32 count = 1, uint32 space = 0)
        {
            return Add(RHIDescriptorRange::UAV(baseReg, count, space));
        }

        RHIDescriptorTableBuilder& AddCBV(uint32 baseReg, uint32 count = 1, uint32 space = 0)
        {
            return Add(RHIDescriptorRange::CBV(baseReg, count, space));
        }

        RHIDescriptorTableBuilder& AddSampler(uint32 baseReg, uint32 count = 1, uint32 space = 0)
        {
            return Add(RHIDescriptorRange::Sampler(baseReg, count, space));
        }

        RHIDescriptorTableBuilder& AddUnboundedSRV(uint32 baseReg, uint32 space = 0)
        {
            return Add(RHIDescriptorRange::UnboundedSRV(baseReg, space));
        }

        /// 記述取得
        RHIDescriptorTableDesc Build() const
        {
            RHIDescriptorTableDesc desc;
            desc.ranges = m_ranges;
            desc.rangeCount = m_count;
            return desc;
        }

        const RHIDescriptorRange* GetRanges() const { return m_ranges; }
        uint32 GetRangeCount() const { return m_count; }

        /// ルートパラメータとして取得
        RHIRootParameter AsRootParameter(EShaderVisibility visibility = EShaderVisibility::All) const
        {
            return RHIRootParameter::DescriptorTable(m_ranges, m_count, visibility);
        }

    private:
        RHIDescriptorRange m_ranges[kMaxDescriptorRanges];
        uint32 m_count = 0;
    };

    //=========================================================================
    // RHIDescriptorTablePresets (08-02)
    //=========================================================================

    /// ディスクリプタテーブルプリセット
    namespace RHIDescriptorTablePresets
    {
        /// 単一SRV
        inline RHIDescriptorTableBuilder SingleSRV(uint32 reg = 0)
        {
            return RHIDescriptorTableBuilder().AddSRV(reg);
        }

        /// 単一UAV
        inline RHIDescriptorTableBuilder SingleUAV(uint32 reg = 0)
        {
            return RHIDescriptorTableBuilder().AddUAV(reg);
        }

        /// 単一CBV
        inline RHIDescriptorTableBuilder SingleCBV(uint32 reg = 0)
        {
            return RHIDescriptorTableBuilder().AddCBV(reg);
        }

        /// テクスチャ＋サンプラー（2テーブル構成）
        /// @note D3D12ではCBV/SRV/UAVとSamplerは別ヒープのため、
        ///       1つのDescriptorTableに混在させることはできない
        struct TextureWithSamplerTables
        {
            RHIDescriptorTableBuilder srvTable;
            RHIDescriptorTableBuilder samplerTable;
        };

        inline TextureWithSamplerTables TextureWithSampler(uint32 texReg = 0, uint32 samplerReg = 0)
        {
            TextureWithSamplerTables result;
            result.srvTable = RHIDescriptorTableBuilder().AddSRV(texReg);
            result.samplerTable = RHIDescriptorTableBuilder().AddSampler(samplerReg);
            return result;
        }

        /// 複数テクスチャ
        inline RHIDescriptorTableBuilder MultipleTextures(uint32 baseReg, uint32 count)
        {
            return RHIDescriptorTableBuilder().AddSRV(baseReg, count);
        }

        /// Bindlessテクスチャ
        inline RHIDescriptorTableBuilder BindlessTextures(uint32 baseReg = 0, uint32 space = 0)
        {
            return RHIDescriptorTableBuilder().AddUnboundedSRV(baseReg, space);
        }

        /// フルGBuffer（複数SRV）
        inline RHIDescriptorTableBuilder GBuffer(uint32 count = 4)
        {
            return RHIDescriptorTableBuilder().AddSRV(0, count);
        }
    } // namespace RHIDescriptorTablePresets

    //=========================================================================
    // ERHIFilterMode / ERHIAddressMode (08-03)
    //=========================================================================

    /// フィルターモード
    enum class ERHIFilterMode : uint8
    {
        Point,
        Linear,
        Anisotropic,
    };

    /// アドレスモード
    enum class ERHIAddressMode : uint8
    {
        Wrap,
        Mirror,
        Clamp,
        Border,
        MirrorOnce,
    };

    //=========================================================================
    // RHIStaticSamplerDesc (08-03)
    //=========================================================================

    /// 静的サンプラー記述
    struct RHI_API RHIStaticSamplerDesc
    {
        uint32 shaderRegister = 0;                                   ///< シェーダーレジスタ（s0, s1, ...）
        uint32 registerSpace = 0;                                    ///< レジスタスペース
        EShaderVisibility shaderVisibility = EShaderVisibility::All; ///< シェーダービジビリティ
        ERHIFilterMode filter = ERHIFilterMode::Linear;              ///< フィルターモード
        ERHIAddressMode addressU = ERHIAddressMode::Wrap;            ///< アドレスモード U
        ERHIAddressMode addressV = ERHIAddressMode::Wrap;            ///< アドレスモード V
        ERHIAddressMode addressW = ERHIAddressMode::Wrap;            ///< アドレスモード W
        float mipLODBias = 0.0f;                                     ///< MIPバイアス
        uint32 maxAnisotropy = 16;                                   ///< 最大異方性
        ERHICompareFunc comparisonFunc = ERHICompareFunc::Never;     ///< 比較関数

        /// ボーダーカラー
        enum class BorderColor : uint8
        {
            TransparentBlack,
            OpaqueBlack,
            OpaqueWhite,
        };
        BorderColor borderColor = BorderColor::OpaqueBlack;

        float minLOD = 0.0f;    ///< 最小LOD
        float maxLOD = 1000.0f; ///< 最大LOD

        //=====================================================================
        // プリセット
        //=====================================================================

        static RHIStaticSamplerDesc PointClamp(uint32 reg = 0, uint32 space = 0)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Point;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Clamp;
            return desc;
        }

        static RHIStaticSamplerDesc PointWrap(uint32 reg = 0, uint32 space = 0)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Point;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            return desc;
        }

        static RHIStaticSamplerDesc LinearClamp(uint32 reg = 0, uint32 space = 0)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Linear;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Clamp;
            return desc;
        }

        static RHIStaticSamplerDesc LinearWrap(uint32 reg = 0, uint32 space = 0)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Linear;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            return desc;
        }

        static RHIStaticSamplerDesc Anisotropic(uint32 reg = 0, uint32 space = 0, uint32 maxAniso = 16)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Anisotropic;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            desc.maxAnisotropy = maxAniso;
            return desc;
        }

        static RHIStaticSamplerDesc ShadowPCF(uint32 reg = 0, uint32 space = 0)
        {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Linear;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Border;
            desc.borderColor = BorderColor::OpaqueWhite;
            desc.comparisonFunc = ERHICompareFunc::LessEqual;
            return desc;
        }
    };

    //=========================================================================
    // ERHIRootSignatureFlags (08-03)
    //=========================================================================

    /// ルートシグネチャフラグ
    enum class ERHIRootSignatureFlags : uint32
    {
        None = 0,
        DenyVertexShaderRootAccess = 1 << 0,
        DenyHullShaderRootAccess = 1 << 1,
        DenyDomainShaderRootAccess = 1 << 2,
        DenyGeometryShaderRootAccess = 1 << 3,
        DenyPixelShaderRootAccess = 1 << 4,
        DenyAmplificationShaderRootAccess = 1 << 5,
        DenyMeshShaderRootAccess = 1 << 6,
        AllowInputAssemblerInputLayout = 1 << 7, ///< 入力アセンブラー使用
        AllowStreamOutput = 1 << 8,              ///< ストリームアウトプット
        LocalRootSignature = 1 << 9,             ///< ローカルルートシグネチャ（レイトレーシング用）
        CBVSRVUAVHeapDirectlyIndexed = 1 << 10,  ///< ヒープ直接インデックス可能
        SamplerHeapDirectlyIndexed = 1 << 11,    ///< サンプラーヒープ直接インデックス可能
    };
    RHI_ENUM_CLASS_FLAGS(ERHIRootSignatureFlags)

    //=========================================================================
    // RHIRootSignatureDesc (08-03)
    //=========================================================================

    /// ルートシグネチャ記述
    struct RHI_API RHIRootSignatureDesc
    {
        const RHIRootParameter* parameters = nullptr;         ///< ルートパラメータ配列
        uint32 parameterCount = 0;                            ///< パラメータ数
        const RHIStaticSamplerDesc* staticSamplers = nullptr; ///< 静的サンプラー配列
        uint32 staticSamplerCount = 0;                        ///< 静的サンプラー数
        ERHIRootSignatureFlags flags = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout;

        /// 配列から作成
        template <uint32 ParamN, uint32 SamplerN>
        static RHIRootSignatureDesc FromArrays(
            const RHIRootParameter (&params)[ParamN],
            const RHIStaticSamplerDesc (&samplers)[SamplerN],
            ERHIRootSignatureFlags f = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout)
        {
            RHIRootSignatureDesc desc;
            desc.parameters = params;
            desc.parameterCount = ParamN;
            desc.staticSamplers = samplers;
            desc.staticSamplerCount = SamplerN;
            desc.flags = f;
            return desc;
        }

        /// パラメータのみ
        template <uint32 N>
        static RHIRootSignatureDesc FromParameters(
            const RHIRootParameter (&params)[N],
            ERHIRootSignatureFlags f = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout)
        {
            RHIRootSignatureDesc desc;
            desc.parameters = params;
            desc.parameterCount = N;
            desc.flags = f;
            return desc;
        }
    };

    //=========================================================================
    // IRHIRootSignature (08-03)
    //=========================================================================

    /// ルートシグネチャ
    class RHI_API IRHIRootSignature : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(RootSignature)

        virtual ~IRHIRootSignature() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        virtual IRHIDevice* GetDevice() const = 0;
        virtual uint32 GetParameterCount() const = 0;
        virtual uint32 GetStaticSamplerCount() const = 0;
        virtual ERHIRootSignatureFlags GetFlags() const = 0;

        //=====================================================================
        // パラメータ情報
        //=====================================================================

        virtual ERHIRootParameterType GetParameterType(uint32 index) const = 0;
        virtual EShaderVisibility GetParameterVisibility(uint32 index) const = 0;
        virtual uint32 GetDescriptorTableSize(uint32 paramIndex) const = 0;

        //=====================================================================
        // シリアライズ
        //=====================================================================

        virtual RHIShaderBytecode GetSerializedBlob() const = 0;
    };

    using RHIRootSignatureRef = TRefCountPtr<IRHIRootSignature>;

    //=========================================================================
    // RHIRootSignatureBuilder (08-03)
    //=========================================================================

    /// 最大ルートパラメータ数
    constexpr uint32 kMaxRootParameters = 64;

    /// 最大静的サンプラー数
    constexpr uint32 kMaxStaticSamplers = 16;

    /// ルートシグネチャビルダー
    class RHI_API RHIRootSignatureBuilder
    {
    public:
        RHIRootSignatureBuilder() = default;

        //=====================================================================
        // パラメータ追加
        //=====================================================================

        RHIRootSignatureBuilder& AddConstants(uint32 shaderRegister,
                                              uint32 num32BitValues,
                                              uint32 registerSpace = 0,
                                              EShaderVisibility visibility = EShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::Constants(shaderRegister, num32BitValues, registerSpace, visibility));
        }

        RHIRootSignatureBuilder& AddCBV(uint32 shaderRegister,
                                        uint32 registerSpace = 0,
                                        EShaderVisibility visibility = EShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::CBV(shaderRegister, registerSpace, visibility));
        }

        RHIRootSignatureBuilder& AddSRV(uint32 shaderRegister,
                                        uint32 registerSpace = 0,
                                        EShaderVisibility visibility = EShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::SRV(shaderRegister, registerSpace, visibility));
        }

        RHIRootSignatureBuilder& AddUAV(uint32 shaderRegister,
                                        uint32 registerSpace = 0,
                                        EShaderVisibility visibility = EShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::UAV(shaderRegister, registerSpace, visibility));
        }

        RHIRootSignatureBuilder& AddDescriptorTable(const RHIDescriptorTableBuilder& table,
                                                    EShaderVisibility visibility = EShaderVisibility::All)
        {
            if (m_paramCount < kMaxRootParameters && m_rangeCount + table.GetRangeCount() <= kMaxTotalRanges)
            {
                uint32 rangeStart = m_rangeCount;
                for (uint32 i = 0; i < table.GetRangeCount(); ++i)
                {
                    m_ranges[m_rangeCount++] = table.GetRanges()[i];
                }
                m_parameters[m_paramCount++] =
                    RHIRootParameter::DescriptorTable(&m_ranges[rangeStart], table.GetRangeCount(), visibility);
            }
            return *this;
        }

        //=====================================================================
        // 静的サンプラー追加
        //=====================================================================

        RHIRootSignatureBuilder& AddStaticSampler(const RHIStaticSamplerDesc& sampler)
        {
            if (m_samplerCount < kMaxStaticSamplers)
            {
                m_staticSamplers[m_samplerCount++] = sampler;
            }
            return *this;
        }

        //=====================================================================
        // フラグ設定
        //=====================================================================

        RHIRootSignatureBuilder& SetFlags(ERHIRootSignatureFlags f)
        {
            m_flags = f;
            return *this;
        }

        RHIRootSignatureBuilder& AddFlags(ERHIRootSignatureFlags f)
        {
            m_flags = m_flags | f;
            return *this;
        }

        //=====================================================================
        // ビルド
        //=====================================================================

        RHIRootSignatureDesc Build() const
        {
            RHIRootSignatureDesc desc;
            desc.parameters = m_parameters;
            desc.parameterCount = m_paramCount;
            desc.staticSamplers = m_staticSamplers;
            desc.staticSamplerCount = m_samplerCount;
            desc.flags = m_flags;
            return desc;
        }

        /// 検証
        bool Validate() const { return ValidateRootSignatureCost(m_parameters, m_paramCount); }

    private:
        RHIRootSignatureBuilder& AddParameter(const RHIRootParameter& param)
        {
            if (m_paramCount < kMaxRootParameters)
            {
                m_parameters[m_paramCount++] = param;
            }
            return *this;
        }

        RHIRootParameter m_parameters[kMaxRootParameters];
        uint32 m_paramCount = 0;

        /// 全ディスクリプタテーブルのレンジを格納する共有バッファ
        static constexpr uint32 kMaxTotalRanges = 256;
        RHIDescriptorRange m_ranges[kMaxTotalRanges];
        uint32 m_rangeCount = 0;

        RHIStaticSamplerDesc m_staticSamplers[kMaxStaticSamplers];
        uint32 m_samplerCount = 0;

        ERHIRootSignatureFlags m_flags = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout;
    };

}} // namespace NS::RHI
