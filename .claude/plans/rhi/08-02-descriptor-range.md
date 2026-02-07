# 08-02: ディスクリプタレンジ

## 目的

ディスクリプタテーブル内のディスクリプタレンジ定義を定義する。

## 参照ドキュメント

- 08-01-root-parameter.md (RHIRootParameter)
- 01-07-enums-access.md (ERHIDescriptorType)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIRootSignature.h` (部分

## TODO

### 1. ディスクリプタレンジタイプ

```cpp
namespace NS::RHI
{
    /// ディスクリプタレンジタイプ
    enum class ERHIDescriptorRangeType : uint8
    {
        SRV,        // シェーダーリソースビュー
        UAV,        // アンオーダードアクセスビュー
        CBV,        // 定数バッファビュー
        Sampler,    // サンプラー
    };
}
```

- [ ] ERHIDescriptorRangeType 列挙型

### 2. ディスクリプタレンジ記述

```cpp
namespace NS::RHI
{
    /// 無制限デスクリプタ数
    constexpr uint32 kUnboundedDescriptorCount = ~0u;

    /// ディスクリプタレンジ記述
    struct RHI_API RHIDescriptorRange
    {
        /// レンジタイプ
        ERHIDescriptorRangeType rangeType = ERHIDescriptorRangeType::SRV;

        /// ディスクリプタ数（UnboundedDescriptorCountで無制限）
        uint32 numDescriptors = 1;

        /// ベースシェーダーレジスタ
        uint32 baseShaderRegister = 0;

        /// レジスタスペース
        uint32 registerSpace = 0;

        /// ディスクリプタテーブル内オフセットのAppendFromTableStartで自動）
        uint32 offsetInDescriptorsFromTableStart = 0;

        /// オフセット自動計算用定数
        static constexpr uint32 kAppendFromTableStart = ~0u;

        /// フラグ
        enum class Flags : uint32 {
            None = 0,
            /// ディスクリプタが揮発性（バインド後に変更可能）。
            DescriptorsVolatile = 1 << 0,
            /// データが揮発性
            DataVolatile = 1 << 1,
            /// データが静的（初期化時に設定、以後変更なし）
            DataStatic = 1 << 2,
            /// 初期化時データ静的（コマンドリスト実行開始時点で有効）。
            DataStaticWhileSetAtExecute = 1 << 3,
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// SRVレンジ
        static RHIDescriptorRange SRV(
            uint32 baseReg,
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

        /// UAVレンジ
        static RHIDescriptorRange UAV(
            uint32 baseReg,
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

        /// CBVレンジ
        static RHIDescriptorRange CBV(
            uint32 baseReg,
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

        /// サンプラーレンジ
        static RHIDescriptorRange Sampler(
            uint32 baseReg,
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

        /// 無制限SRV（Bindless用）。
        static RHIDescriptorRange UnboundedSRV(
            uint32 baseReg,
            uint32 space = 0)
        {
            return SRV(baseReg, kUnboundedDescriptorCount, space);
        }

        /// 無制限UAV（Bindless用）。
        static RHIDescriptorRange UnboundedUAV(
            uint32 baseReg,
            uint32 space = 0)
        {
            return UAV(baseReg, kUnboundedDescriptorCount, space);
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIDescriptorRange::Flags)
}
```

- [ ] RHIDescriptorRange 構造体
- [ ] SRV / UAV / CBV / Sampler ビルダー
- [ ] UnboundedSRV / UnboundedUAV

### 3. ディスクリプタテーブル記述

```cpp
namespace NS::RHI
{
    /// ディスクリプタテーブル記述
    struct RHI_API RHIDescriptorTableDesc
    {
        /// レンジ配列
        const RHIDescriptorRange* ranges = nullptr;

        /// レンジ数
        uint32 rangeCount = 0;

        /// 配列から作成
        template<uint32 N>
        static RHIDescriptorTableDesc FromArray(const RHIDescriptorRange (&arr)[N]) {
            RHIDescriptorTableDesc desc;
            desc.ranges = arr;
            desc.rangeCount = N;
            return desc;
        }

        /// 総デスクリプタ数計算（無制限レンジ除く）
        uint32 CalculateTotalDescriptorCount() const {
            uint32 total = 0;
            for (uint32 i = 0; i < rangeCount; ++i) {
                if (ranges[i].numDescriptors != kUnboundedDescriptorCount) {
                    total += ranges[i].numDescriptors;
                }
            }
            return total;
        }

        /// 無制限レンジを含むか
        bool HasUnboundedRange() const {
            for (uint32 i = 0; i < rangeCount; ++i) {
                if (ranges[i].numDescriptors == kUnboundedDescriptorCount) {
                    return true;
                }
            }
            return false;
        }
    };
}
```

- [ ] RHIDescriptorTableDesc 構造体

### 4. ディスクリプタテーブルビルダー

```cpp
namespace NS::RHI
{
    /// 最大ディスクリプタレンジ数
    constexpr uint32 kMaxDescriptorRanges = 32;

    /// ディスクリプタテーブルビルダー
    class RHI_API RHIDescriptorTableBuilder
    {
    public:
        RHIDescriptorTableBuilder() = default;

        /// レンジ追加
        RHIDescriptorTableBuilder& Add(const RHIDescriptorRange& range) {
            if (m_count < kMaxDescriptorRanges) {
                m_ranges[m_count++] = range;
            }
            return *this;
        }

        /// SRVレンジ追加
        RHIDescriptorTableBuilder& AddSRV(uint32 baseReg, uint32 count = 1, uint32 space = 0) {
            return Add(RHIDescriptorRange::SRV(baseReg, count, space));
        }

        /// UAVレンジ追加
        RHIDescriptorTableBuilder& AddUAV(uint32 baseReg, uint32 count = 1, uint32 space = 0) {
            return Add(RHIDescriptorRange::UAV(baseReg, count, space));
        }

        /// CBVレンジ追加
        RHIDescriptorTableBuilder& AddCBV(uint32 baseReg, uint32 count = 1, uint32 space = 0) {
            return Add(RHIDescriptorRange::CBV(baseReg, count, space));
        }

        /// サンプラーレンジ追加
        RHIDescriptorTableBuilder& AddSampler(uint32 baseReg, uint32 count = 1, uint32 space = 0) {
            return Add(RHIDescriptorRange::Sampler(baseReg, count, space));
        }

        /// 無制限SRV追加
        RHIDescriptorTableBuilder& AddUnboundedSRV(uint32 baseReg, uint32 space = 0) {
            return Add(RHIDescriptorRange::UnboundedSRV(baseReg, space));
        }

        /// 記述取得
        RHIDescriptorTableDesc Build() const {
            RHIDescriptorTableDesc desc;
            desc.ranges = m_ranges;
            desc.rangeCount = m_count;
            return desc;
        }

        /// レンジ配列取得
        const RHIDescriptorRange* GetRanges() const { return m_ranges; }
        uint32 GetRangeCount() const { return m_count; }

        /// ルートパラメータとして取得
        RHIRootParameter AsRootParameter(
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All) const
        {
            return RHIRootParameter::DescriptorTable(m_ranges, m_count, visibility);
        }

    private:
        RHIDescriptorRange m_ranges[kMaxDescriptorRanges];
        uint32 m_count = 0;
    };
}
```

- [ ] RHIDescriptorTableBuilder クラス

### 5. 一般的なディスクリプタテーブルプリセット

```cpp
namespace NS::RHI
{
    /// ディスクリプタテーブルプリセット
    namespace RHIDescriptorTablePresets
    {
        /// 単一SRV
        inline RHIDescriptorTableBuilder SingleSRV(uint32 reg = 0) {
            return RHIDescriptorTableBuilder().AddSRV(reg);
        }

        /// 単一UAV
        inline RHIDescriptorTableBuilder SingleUAV(uint32 reg = 0) {
            return RHIDescriptorTableBuilder().AddUAV(reg);
        }

        /// 単一CBV
        inline RHIDescriptorTableBuilder SingleCBV(uint32 reg = 0) {
            return RHIDescriptorTableBuilder().AddCBV(reg);
        }

        /// テクスチャ＋サンプラー（2テーブル構成）
        /// @note D3D12ではCBV/SRV/UAVとSamplerは別ヒープのため、
        ///       1つのDescriptorTableに混在させることはできない
        struct TextureWithSamplerTables {
            RHIDescriptorTableBuilder srvTable;
            RHIDescriptorTableBuilder samplerTable;
        };

        inline TextureWithSamplerTables TextureWithSampler(
            uint32 texReg = 0, uint32 samplerReg = 0)
        {
            TextureWithSamplerTables result;
            result.srvTable = RHIDescriptorTableBuilder().AddSRV(texReg);
            result.samplerTable = RHIDescriptorTableBuilder().AddSampler(samplerReg);
            return result;
        }

        /// 複数テクスチャ
        inline RHIDescriptorTableBuilder MultipleTextures(
            uint32 baseReg, uint32 count)
        {
            return RHIDescriptorTableBuilder().AddSRV(baseReg, count);
        }

        /// Bindlessテクスチャ
        inline RHIDescriptorTableBuilder BindlessTextures(
            uint32 baseReg = 0, uint32 space = 0)
        {
            return RHIDescriptorTableBuilder().AddUnboundedSRV(baseReg, space);
        }

        /// フルGBuffer（複数SRV）。
        inline RHIDescriptorTableBuilder GBuffer(uint32 count = 4) {
            return RHIDescriptorTableBuilder().AddSRV(0, count);
        }
    }
}
```

- [ ] RHIDescriptorTablePresets

## 検証方法

- [ ] ディスクリプタレンジの整合性
- [ ] ビルダーの動作確認
- [ ] プリセットの検証
- [ ] 無制限レンジの処理
