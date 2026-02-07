# 08-03: ルートシグネチャ

## 目的

ルートシグネチャインターフェースを定義する。

## 参照ドキュメント

- 08-01-root-parameter.md (RHIRootParameter)
- 08-02-descriptor-range.md (RHIDescriptorRange)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIRootSignature.h` (部分

## TODO

### 1. 静的サンプラー記述

```cpp
namespace NS::RHI
{
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

    /// 静的サンプラー記述
    struct RHI_API RHIStaticSamplerDesc
    {
        /// シェーダーレジスタ（0, s1, ...）。
        uint32 shaderRegister = 0;

        /// レジスタスペース
        uint32 registerSpace = 0;

        /// シェーダービジビリティ
        ERHIShaderVisibility shaderVisibility = ERHIShaderVisibility::All;

        /// フィルターモード
        ERHIFilterMode filter = ERHIFilterMode::Linear;

        /// アドレスモード（U, V, W）。
        ERHIAddressMode addressU = ERHIAddressMode::Wrap;
        ERHIAddressMode addressV = ERHIAddressMode::Wrap;
        ERHIAddressMode addressW = ERHIAddressMode::Wrap;

        /// MIPバイアス
        float mipLODBias = 0.0f;

        /// 最大異方性（Anisotropicフィルター用）。
        uint32 maxAnisotropy = 16;

        /// 比較関数（シャドウサンプラー用）。
        ERHICompareFunc comparisonFunc = ERHICompareFunc::Never;

        /// ボーダーカラー
        enum class BorderColor : uint8 {
            TransparentBlack,
            OpaqueBlack,
            OpaqueWhite,
        };
        BorderColor borderColor = BorderColor::OpaqueBlack;

        /// 最小LOD
        float minLOD = 0.0f;

        /// 最大LOD
        float maxLOD = 1000.0f;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// ポイントサンプラー
        static RHIStaticSamplerDesc PointClamp(uint32 reg = 0, uint32 space = 0) {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Point;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Clamp;
            return desc;
        }

        /// ポイントラップ。
        static RHIStaticSamplerDesc PointWrap(uint32 reg = 0, uint32 space = 0) {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Point;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            return desc;
        }

        /// リニアクランプ
        static RHIStaticSamplerDesc LinearClamp(uint32 reg = 0, uint32 space = 0) {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Linear;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Clamp;
            return desc;
        }

        /// リニアラップ。
        static RHIStaticSamplerDesc LinearWrap(uint32 reg = 0, uint32 space = 0) {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Linear;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            return desc;
        }

        /// 異方性サンプラー
        static RHIStaticSamplerDesc Anisotropic(uint32 reg = 0, uint32 space = 0, uint32 maxAniso = 16) {
            RHIStaticSamplerDesc desc;
            desc.shaderRegister = reg;
            desc.registerSpace = space;
            desc.filter = ERHIFilterMode::Anisotropic;
            desc.addressU = desc.addressV = desc.addressW = ERHIAddressMode::Wrap;
            desc.maxAnisotropy = maxAniso;
            return desc;
        }

        /// シャドウ比較サンプラー
        static RHIStaticSamplerDesc ShadowPCF(uint32 reg = 0, uint32 space = 0) {
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
}
```

- [ ] ERHIFilterMode / ERHIAddressMode 列挙型
- [ ] RHIStaticSamplerDesc 構造体
- [ ] プリセット

### 2. ルートシグネチャ記述

```cpp
namespace NS::RHI
{
    /// ルートシグネチャフラグ
    enum class ERHIRootSignatureFlags : uint32
    {
        None = 0,

        /// 入力アセンブラーを使用しない
        DenyVertexShaderRootAccess = 1 << 0,
        DenyHullShaderRootAccess = 1 << 1,
        DenyDomainShaderRootAccess = 1 << 2,
        DenyGeometryShaderRootAccess = 1 << 3,
        DenyPixelShaderRootAccess = 1 << 4,
        DenyAmplificationShaderRootAccess = 1 << 5,
        DenyMeshShaderRootAccess = 1 << 6,

        /// 入力アセンブラー使用（頂点バッファ/インデックスバッファ）。
        AllowInputAssemblerInputLayout = 1 << 7,

        /// ストリームアウトプットを使用
        AllowStreamOutput = 1 << 8,

        /// ローカルルートシグネチャ（レイトレーシング用）。
        LocalRootSignature = 1 << 9,

        /// CBV/SRV/UAVヒープ直接インデックス可能
        CBVSRVUAVHeapDirectlyIndexed = 1 << 10,

        /// サンプラーヒープ直接インデックス可能
        SamplerHeapDirectlyIndexed = 1 << 11,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIRootSignatureFlags)

    /// ルートシグネチャ記述
    struct RHI_API RHIRootSignatureDesc
    {
        /// ルートパラメータ配列
        const RHIRootParameter* parameters = nullptr;

        /// パラメータ数
        uint32 parameterCount = 0;

        /// 静的サンプラー配列
        const RHIStaticSamplerDesc* staticSamplers = nullptr;

        /// 静的サンプラー数
        uint32 staticSamplerCount = 0;

        /// フラグ
        ERHIRootSignatureFlags flags = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout;

        /// 配列から作成
        template<uint32 ParamN, uint32 SamplerN>
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
        template<uint32 N>
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
}
```

- [ ] ERHIRootSignatureFlags 列挙型
- [ ] RHIRootSignatureDesc 構造体

### 3. IRHIRootSignature インターフェース

```cpp
namespace NS::RHI
{
    /// ルートシグネチャ
    class RHI_API IRHIRootSignature : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(RootSignature)

        virtual ~IRHIRootSignature() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// パラメータ数取得
        virtual uint32 GetParameterCount() const = 0;

        /// 静的サンプラー数取得
        virtual uint32 GetStaticSamplerCount() const = 0;

        /// フラグ取得
        virtual ERHIRootSignatureFlags GetFlags() const = 0;

        //=====================================================================
        // パラメータ情報
        //=====================================================================

        /// パラメータタイプ取得
        virtual ERHIRootParameterType GetParameterType(uint32 index) const = 0;

        /// パラメータビジビリティを取得
        virtual ERHIShaderVisibility GetParameterVisibility(uint32 index) const = 0;

        /// ディスクリプタテーブルサイズ取得
        virtual uint32 GetDescriptorTableSize(uint32 paramIndex) const = 0;

        //=====================================================================
        // シリアライズ
        //=====================================================================

        /// シリアライズ済みBlob取得
        virtual RHIShaderBytecode GetSerializedBlob() const = 0;
    };

    using RHIRootSignatureRef = TRefCountPtr<IRHIRootSignature>;
}
```

- [ ] IRHIRootSignature インターフェース

### 4. ルートシグネチャ作成

```cpp
namespace NS::RHI
{
    /// ルートシグネチャ作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// ルートシグネチャ作成
        virtual IRHIRootSignature* CreateRootSignature(
            const RHIRootSignatureDesc& desc,
            const char* debugName = nullptr) = 0;

        /// シリアライズ済みBlobからルートシグネチャ作成
        virtual IRHIRootSignature* CreateRootSignatureFromBlob(
            const RHIShaderBytecode& blob,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] CreateRootSignature
- [ ] CreateRootSignatureFromBlob

### 5. ルートシグネチャビルダー

```cpp
namespace NS::RHI
{
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

        /// ルート定数追加
        RHIRootSignatureBuilder& AddConstants(
            uint32 shaderRegister,
            uint32 num32BitValues,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::Constants(
                shaderRegister, num32BitValues, registerSpace, visibility));
        }

        /// ルートCBV追加
        RHIRootSignatureBuilder& AddCBV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::CBV(
                shaderRegister, registerSpace, visibility));
        }

        /// ルートSRV追加
        RHIRootSignatureBuilder& AddSRV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::SRV(
                shaderRegister, registerSpace, visibility));
        }

        /// ルートUAV追加
        RHIRootSignatureBuilder& AddUAV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            return AddParameter(RHIRootParameter::UAV(
                shaderRegister, registerSpace, visibility));
        }

        /// ディスクリプタテーブル追加
        RHIRootSignatureBuilder& AddDescriptorTable(
            const RHIDescriptorTableBuilder& table,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            if (m_paramCount < kMaxRootParameters && m_rangeCount + table.GetRangeCount() <= m_maxRanges) {
                // レンジをコピー
                uint32 rangeStart = m_rangeCount;
                for (uint32 i = 0; i < table.GetRangeCount(); ++i) {
                    m_ranges[m_rangeCount++] = table.GetRanges()[i];
                }
                // パラメータ追加
                m_parameters[m_paramCount++] = RHIRootParameter::DescriptorTable(
                    &m_ranges[rangeStart], table.GetRangeCount(), visibility);
            }
            return *this;
        }

        //=====================================================================
        // 静的サンプラー追加
        //=====================================================================

        RHIRootSignatureBuilder& AddStaticSampler(const RHIStaticSamplerDesc& sampler) {
            if (m_samplerCount < kMaxStaticSamplers) {
                m_staticSamplers[m_samplerCount++] = sampler;
            }
            return *this;
        }

        //=====================================================================
        // フラグ設定
        //=====================================================================

        RHIRootSignatureBuilder& SetFlags(ERHIRootSignatureFlags f) {
            m_flags = f;
            return *this;
        }

        RHIRootSignatureBuilder& AddFlags(ERHIRootSignatureFlags f) {
            m_flags = m_flags | f;
            return *this;
        }

        //=====================================================================
        // ビルド
        //=====================================================================

        RHIRootSignatureDesc Build() const {
            RHIRootSignatureDesc desc;
            desc.parameters = m_parameters;
            desc.parameterCount = m_paramCount;
            desc.staticSamplers = m_staticSamplers;
            desc.staticSamplerCount = m_samplerCount;
            desc.flags = m_flags;
            return desc;
        }

        /// 検証
        bool Validate() const {
            return ValidateRootSignatureCost(m_parameters, m_paramCount);
        }

    private:
        RHIRootSignatureBuilder& AddParameter(const RHIRootParameter& param) {
            if (m_paramCount < kMaxRootParameters) {
                m_parameters[m_paramCount++] = param;
            }
            return *this;
        }

        RHIRootParameter m_parameters[kMaxRootParameters];
        uint32 m_paramCount = 0;

        static constexpr uint32 m_maxRanges = kMaxRootParameters * kMaxDescriptorRanges;
        RHIDescriptorRange m_ranges[m_maxRanges];
        uint32 m_rangeCount = 0;

        RHIStaticSamplerDesc m_staticSamplers[kMaxStaticSamplers];
        uint32 m_samplerCount = 0;

        ERHIRootSignatureFlags m_flags = ERHIRootSignatureFlags::AllowInputAssemblerInputLayout;
    };
}
```

- [ ] RHIRootSignatureBuilder クラス

## 検証方法

- [ ] ルートシグネチャ作成の正常動作
- [ ] ビルダーの動作確認
- [ ] コスト検証
- [ ] シリアライズ/デシリアライズ
